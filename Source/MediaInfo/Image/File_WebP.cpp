// File_WebP.cpp - Implementation for WebP parser (MediaInfoLib)

#include "MediaInfo/Image/File_WebP.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/MediaInfo_Internal.h"

#if defined(MEDIAINFO_WEBP_YES)

#include <cstring>
#include <vector>

namespace MediaInfoLib
{

// WebP chunk identifiers
namespace WebP_Chunk {
    const int32u VP8_ = 0x56503820;  // 'VP8 '
    const int32u VP8L = 0x5650384C;  // 'VP8L'
    const int32u VP8X = 0x56503858;  // 'VP8X'
    const int32u ALPH = 0x414C5048;  // 'ALPH'
    const int32u ANIM = 0x414E494D;  // 'ANIM'
    const int32u ANMF = 0x414E4D46;  // 'ANMF'
    const int32u ICCP = 0x49434350;  // 'ICCP'
    const int32u EXIF = 0x45584946;  // 'EXIF'
    const int32u XMP  = 0x584D5020;  // 'XMP '
}

// WebP format-specific constants
namespace WebP_Constants {
    const int32u RIFF_SIGNATURE = 0x52494646;  // 'RIFF'
    const int32u WEBP_SIGNATURE = 0x57454250;  // 'WEBP'
    const int8u FLAG_ALPHA    = 0x10;
    const int8u FLAG_ANIMATION = 0x02;
    const int8u VP8_START_0 = 0x9D;
    const int8u VP8_START_1 = 0x01;
    const int8u VP8_START_2 = 0x2A;
    const int8u VP8L_SIGNATURE = 0x2F;
    const int32u DIMENSION_BITS_MASK = 0x3FFF;
    const int32u VP8L_VERSION_SHIFT = 29;
    const int32u VP8L_VERSION_MASK = 0x07;
    const int32u VP8L_ALPHA_BIT_SHIFT = 28;
    const int32u VP8L_ALPHA_BIT_MASK = 0x01;
    const int32u VP8L_HEIGHT_SHIFT = 14;
}

File_WebP::File_WebP()
{
    HasAlpha = false;
    IsLossless = false;
    IsAnimated = false;
}

bool File_WebP::FileHeader_Begin()
{
    return Buffer_Size >= 12
        && CC4(Buffer) == WebP_Constants::RIFF_SIGNATURE
        && CC4(Buffer + 8) == WebP_Constants::WEBP_SIGNATURE;
}

void File_WebP::FileHeader_Parse()
{
    int32u riffId, fileSize, webpId;
    Get_C4(riffId, "RIFF");
    Get_L4(fileSize, "File size");
    Get_C4(webpId, "WEBP");

    FILLING_BEGIN();
        Accept("WebP");
        Stream_Prepare(Stream_Image);
        Fill(Stream_Image, 0, Image_Format, "WebP");
    FILLING_END();
}

void File_WebP::Header_Parse()
{
    int32u chunkId, chunkSize;
    Get_C4(chunkId, "Chunk FourCC");
    Get_L4(chunkSize, "Chunk Size");

    int64u totalSize = chunkSize + (chunkSize % 2); 
    Header_Fill_Size(8 + totalSize);
    Header_Fill_Code(chunkId, Ztring().From_CC4(chunkId));
}

void File_WebP::Data_Parse()
{
    if (Element_Size < 8)
    {
        Skip_XX(Element_Size, "Corrupt chunk (size < 8)");
        return;
    }

    switch (Element_Code)
    {
        case WebP_Chunk::VP8X: Parse_VP8X(); break;
        case WebP_Chunk::VP8_: Parse_VP8();  break;
        case WebP_Chunk::VP8L: Parse_VP8L(); break;
        case WebP_Chunk::ALPH: 
            HasAlpha = true; 
            Skip_XX(Element_Size, "Alpha data"); 
            break;
        case WebP_Chunk::ANIM: Parse_ANIM(); break;
        case WebP_Chunk::ANMF: 
            IsAnimated = true; 
            Skip_XX(Element_Size, "Animation frame data"); 
            break;
        case WebP_Chunk::ICCP: 
            Fill(Stream_Image, 0, "Color_Profile", "ICC"); 
            Skip_XX(Element_Size, "ICC profile data"); 
            break;
        case WebP_Chunk::EXIF: 
            Fill(Stream_Image, 0, "Metadata_Exif", "Yes"); 
            Skip_XX(Element_Size, "EXIF metadata"); 
            break;
        case WebP_Chunk::XMP:
        {
            Ztring xmp;
            if (Element_Size)
                Get_UTF8(Element_Size, xmp, "XMP metadata");
            Fill(Stream_Image, 0, "Metadata_XMP", xmp);
            break;
        }
        default:
            Skip_XX(Element_Size, "Unknown chunk type");
            break;
    }

    if (!Status[IsAccepted])
        return;

    if (!Status[IsFilled] && Count_Get(Stream_Image)) {
        if (HasAlpha && IsAnimated)
            Fill(Stream_Image, 0, Image_Format_Settings, "Alpha / Animation");
        else if (HasAlpha)
            Fill(Stream_Image, 0, Image_Format_Settings, "Alpha");
        else if (IsAnimated)
            Fill(Stream_Image, 0, Image_Format_Settings, "Animation");

        Fill(Stream_Image, 0, Image_BitDepth, 8);
        Finish("WebP");
    }
}

void File_WebP::Parse_VP8X()
{
    if (Element_Size < 10) {
        Skip_XX(Element_Size, "Invalid VP8X chunk (too short)");
        return;
    }

    int8u flags;
    Get_B1(flags, "Feature flags");
    Skip_B3("Reserved");

    HasAlpha |= (flags & WebP_Constants::FLAG_ALPHA) != 0;
    IsAnimated |= (flags & WebP_Constants::FLAG_ANIMATION) != 0;

    int8u w1, w2, w3, h1, h2, h3;
    Get_B1(w1, "Canvas Width Byte 0");
    Get_B1(w2, "Canvas Width Byte 1");
    Get_B1(w3, "Canvas Width Byte 2");
    Get_B1(h1, "Canvas Height Byte 0");
    Get_B1(h2, "Canvas Height Byte 1");
    Get_B1(h3, "Canvas Height Byte 2");

    int32u w = 1 + ((w1) | (w2 << 8) | (w3 << 16));
    int32u h = 1 + ((h1) | (h2 << 8) | (h3 << 16));

    Fill(Stream_Image, 0, Image_Width, w);
    Fill(Stream_Image, 0, Image_Height, h);
}

void File_WebP::Parse_VP8()
{
    if (Element_Size < 10) {
        Skip_XX(Element_Size, "Invalid VP8 chunk (too short)");
        return;
    }

    Skip_B3("Frame tag");

    int8u s0, s1, s2;
    Get_B1(s0, "Start[0]");
    Get_B1(s1, "Start[1]");
    Get_B1(s2, "Start[2]");

    if (s0 != WebP_Constants::VP8_START_0 || 
        s1 != WebP_Constants::VP8_START_1 || 
        s2 != WebP_Constants::VP8_START_2) {
        Skip_XX(Element_Size - 6, "Invalid VP8 start code");
        return;
    }

    int16u wCode, hCode;
    Get_L2(wCode, "Width code");
    Get_L2(hCode, "Height code");

    int32u w = (wCode & WebP_Constants::DIMENSION_BITS_MASK);
    int32u h = (hCode & WebP_Constants::DIMENSION_BITS_MASK);

    FillDimensionsIfEmpty(w, h);

    Fill(Stream_Image, 0, Image_Format_Profile, "Lossy");
    Fill(Stream_Image, 0, Image_Compression_Mode, "Lossy");
    Fill(Stream_Image, 0, Image_ColorSpace, "YUV");
}

void File_WebP::Parse_VP8L()
{
    if (Element_Size < 5) {
        Skip_XX(Element_Size, "Invalid VP8L chunk (too short)");
        return;
    }

    int8u sig;
    Get_B1(sig, "VP8L signature");
    if (sig != WebP_Constants::VP8L_SIGNATURE)
    {
        Skip_XX(Element_Size - 1, "Invalid VP8L signature");
        return;
    }

    int32u bits;
    Get_L4(bits, "Image code");

    int32u w = ((bits & WebP_Constants::DIMENSION_BITS_MASK) + 1);
    int32u h = (((bits >> WebP_Constants::VP8L_HEIGHT_SHIFT) & WebP_Constants::DIMENSION_BITS_MASK) + 1);

    int8u version = (bits >> WebP_Constants::VP8L_VERSION_SHIFT) & WebP_Constants::VP8L_VERSION_MASK;

    if (version != 0) {
        Skip_XX(Element_Size - 5, "Unsupported VP8L version");
        return;
    }

    HasAlpha |= ((bits >> WebP_Constants::VP8L_ALPHA_BIT_SHIFT) & WebP_Constants::VP8L_ALPHA_BIT_MASK) != 0;
    IsLossless = true;

    FillDimensionsIfEmpty(w, h);

    Fill(Stream_Image, 0, Image_Format_Profile, "Lossless");
    Fill(Stream_Image, 0, Image_Compression_Mode, "Lossless");
    Fill(Stream_Image, 0, Image_ColorSpace, "RGB");
    Fill(Stream_Image, 0, "WebP_LosslessVersion", Ztring::ToZtring(version));
}

void File_WebP::Parse_ANIM()
{
    if (Element_Size < 6) {
        Skip_XX(Element_Size, "Invalid ANIM chunk (too short)");
        return;
    }

    int32u bgColor;
    int16u loopCount;
    Get_L4(bgColor, "Background color (BGRA)");
    Get_L2(loopCount, "Loop count");

    IsAnimated = true;
    Fill(Stream_Image, 0, "Animation_Loops", loopCount);

    Ztring bg;
    bg.From_Number(bgColor, 16);
    bg.MakeUpperCase();
    bg.insert(0, __T("0x"));
    Fill(Stream_Image, 0, "Animation_BackgroundColor", bg);
}

void File_WebP::FillDimensionsIfEmpty(int32u width, int32u height)
{
    if (Retrieve(Stream_Image, 0, Image_Width).empty())
        Fill(Stream_Image, 0, Image_Width, width);

    if (Retrieve(Stream_Image, 0, Image_Height).empty())
        Fill(Stream_Image, 0, Image_Height, height);
}

} // namespace MediaInfoLib

#endif