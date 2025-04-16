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

File_WebP::File_WebP()
{
    HasAlpha = false;
    IsLossless = false;
    IsAnimated = false;
}

bool File_WebP::FileHeader_Begin()
{
    return Buffer_Size >= 12
        && CC4(Buffer) == 0x52494646
        && CC4(Buffer + 8) == 0x57454250;
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

    int64u totalSize = chunkSize + (chunkSize % 2); // padding
    Header_Fill_Size(8 + totalSize);
    Header_Fill_Code(chunkId, Ztring().From_CC4(chunkId));
}

void File_WebP::Data_Parse()
{
    if (Element_Size < 8)
    {
        Skip_XX(Element_Size, "Corrupt chunk");
        return;
    }

    switch (Element_Code)
    {
        case WebP_Chunk::VP8X: Parse_VP8X(); break;
        case WebP_Chunk::VP8_: Parse_VP8();  break;
        case WebP_Chunk::VP8L: Parse_VP8L(); break;
        case WebP_Chunk::ALPH: HasAlpha = true; Skip_XX(Element_Size, "Alpha"); break;
        case WebP_Chunk::ANIM: Parse_ANIM(); break;
        case WebP_Chunk::ANMF: IsAnimated = true; Skip_XX(Element_Size, "Frame"); break;
        case WebP_Chunk::ICCP: Fill(Stream_Image, 0, "Color_Profile", "ICC"); Skip_XX(Element_Size, "ICC"); break;
        case WebP_Chunk::EXIF: Fill(Stream_Image, 0, "Metadata_Exif", "Yes"); Skip_XX(Element_Size, "EXIF"); break;
        case WebP_Chunk::XMP:
        {
            Ztring xmp;
            if (Element_Size)
                Get_UTF8(Element_Size, xmp, "XMP");
            Fill(Stream_Image, 0, "Metadata_XMP", xmp);
            break;
        }
        default:
            Skip_XX(Element_Size, "Unknown chunk");
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
        Skip_XX(Element_Size, "Short VP8X");
        return;
    }

    int8u flags;
    Get_B1(flags, "Flags");
    Skip_B3("Reserved");

    HasAlpha |= (flags & 0x10);
    IsAnimated |= (flags & 0x02);

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
        Skip_XX(Element_Size, "Short VP8");
        return;
    }

    Skip_B3("Frame tag");
    int8u s0, s1, s2;
    Get_B1(s0, "Start[0]");
    Get_B1(s1, "Start[1]");
    Get_B1(s2, "Start[2]");
    if (!(s0 == 0x9D && s1 == 0x01 && s2 == 0x2A))
        Skip_XX(Element_Size - 6, "Invalid start code");

    int16u wCode, hCode;
    Get_L2(wCode, "Width");
    Get_L2(hCode, "Height");

    int32u w = ((wCode & 0x3FFF) + 1);
    int32u h = ((hCode & 0x3FFF) + 1);

    if (Retrieve(Stream_Image, 0, Image_Width).empty())
        Fill(Stream_Image, 0, Image_Width, w);
    if (Retrieve(Stream_Image, 0, Image_Height).empty())
        Fill(Stream_Image, 0, Image_Height, h);

    Fill(Stream_Image, 0, Image_Format_Profile, "Lossy");
    Fill(Stream_Image, 0, Image_Compression_Mode, "Lossy");
    Fill(Stream_Image, 0, Image_ColorSpace, "YUV");
}

void File_WebP::Parse_VP8L()
{
    if (Element_Size < 5) {
        Skip_XX(Element_Size, "Short VP8L");
        return;
    }

    int8u sig;
    Get_B1(sig, "Signature");
    if (sig != 0x2F)
    {
        Skip_XX(Element_Size - 1, "Invalid VP8L signature");
        return;
    }

    int32u bits;
    Get_L4(bits, "Image code");

    int32u w = ((bits & 0x3FFF) + 1);
    int32u h = (((bits >> 14) & 0x3FFF) + 1);
    int8u version = (bits >> 29) & 0x07;

    if (version != 0)
        Skip_XX(Element_Size - 5, "Unsupported VP8L version");

    HasAlpha |= ((bits >> 28) & 0x1) != 0;
    IsLossless = true;

    if (Retrieve(Stream_Image, 0, Image_Width).empty())
        Fill(Stream_Image, 0, Image_Width, w);
    if (Retrieve(Stream_Image, 0, Image_Height).empty())
        Fill(Stream_Image, 0, Image_Height, h);

    Fill(Stream_Image, 0, Image_Format_Profile, "Lossless");
    Fill(Stream_Image, 0, Image_Compression_Mode, "Lossless");
    Fill(Stream_Image, 0, Image_ColorSpace, "RGB");
}

void File_WebP::Parse_ANIM()
{
    if (Element_Size < 6) {
        Skip_XX(Element_Size, "Short ANIM");
        return;
    }

    int32u bgColor;
    int16u loopCount;
    Get_L4(bgColor, "BGRA");
    Get_L2(loopCount, "Loops");

    IsAnimated = true;
    Fill(Stream_Image, 0, "Animation_Loops", loopCount);
}

} // namespace MediaInfoLib

#endif
