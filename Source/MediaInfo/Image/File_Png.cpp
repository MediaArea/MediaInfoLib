/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about PNG files
//
// From http://www.fileformat.info/format/png/
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_PNG_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Png.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Png_Colour_type(int8u Colour_type)
{
    switch (Colour_type)
    {
        case 0 : return "Greyscale";
        case 2 : return "Truecolour";
        case 3 : return "Indexed-colour";
        case 4 : return "Greyscale with alpha";
        case 6 : return "Truecolour with alpha";
    default: return "";
    }
}

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
namespace Elements
{
    const int32u IDAT=0x49444154;
    const int32u IEND=0x49454E44;
    const int32u IHDR=0x49484452;
    const int32u PLTE=0x506C5445;
    const int32u gAMA=0x67414D41;
    const int32u pHYs=0x70485973;
    const int32u sBIT=0x73424954;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Png::File_Png()
{
    //Config
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;

    //In
    StreamKind=Stream_Max;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::Streams_Accept()
{
    if (!IsSub)
    {
        TestContinuousFileNames();

        Stream_Prepare((Config->File_Names.size()>1 || Config->File_IsReferenced_Get())?Stream_Video:Stream_Image);
        if (File_Size!=(int64u)-1)
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize), File_Size);
        if (StreamKind_Last==Stream_Video)
            Fill(Stream_Video, StreamPos_Last, Video_FrameCount, Config->File_Names.size());
    }
    else
        Stream_Prepare(StreamKind==Stream_Max?StreamKind_Last:StreamKind);
}

//***************************************************************************
// Header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Png::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<8)
        return false; //Must wait for more data

    if (CC4(Buffer+4)!=0x0D0A1A0A) //Byte order
    {
        Reject("PNG");
        return false;
    }

    switch (CC4(Buffer)) //Signature
    {
        case 0x89504E47 :
            Accept("PNG");

            Fill(Stream_General, 0, General_Format, "PNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "PNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), "PNG");

            break;

        case 0x8A4E4E47 :
            Accept("PNG");

            Fill(Stream_General, 0, General_Format, "MNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "MNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), "MNG");

            Finish("PNG");
            break;

        case 0x8B4A4E47 :
            Accept("PNG");

            Fill(Stream_General, 0, General_Format, "JNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "JNG");
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), "JNG");

            Finish("PNG");
            break;

        default:
            Reject("PNG");
    }

    //All should be OK...
    return true;
}

//---------------------------------------------------------------------------
void File_Png::FileHeader_Parse()
{
    Skip_B4(                                                    "Signature");
    Skip_B4(                                                    "ByteOrder");
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::Read_Buffer_Unsynched()
{
    Read_Buffer_Unsynched_OneFramePerFile();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::Header_Parse()
{
    //Parsing
    int32u Length, Chunk_Type;
    Get_B4 (Length,                                             "Length");
    Get_C4 (Chunk_Type,                                         "Chunk Type");

    //Filling
    if (Chunk_Type==Elements::IDAT)
        Element_ThisIsAList(); // No need of the content
    Header_Fill_Size(12+Length); //+4 for CRC
    Header_Fill_Code(Chunk_Type, Ztring().From_CC4(Chunk_Type));
}

//---------------------------------------------------------------------------
void File_Png::Data_Parse()
{
    Element_Size-=4; //For CRC

    #define CASE_INFO(_NAME, _DETAIL) \
        case Elements::_NAME : Element_Info1(_DETAIL); _NAME(); break;

    //Parsing
    switch (Element_Code)
    {
        CASE_INFO(IDAT,                                         "Image data");
        CASE_INFO(IEND,                                         "Image trailer");
        CASE_INFO(IHDR,                                         "Image header");
        CASE_INFO(PLTE,                                         "Palette table");
        CASE_INFO(gAMA,                                         "Palette table");
        CASE_INFO(pHYs,                                         "Palette table");
        CASE_INFO(sBIT,                                         "Significant bits");
        default : Skip_XX(Element_Size,                         "Unknown");
    }

    Element_Size+=4; //For CRC
    if (Element_Code!=Elements::IDAT)
        Skip_B4(                                                "CRC"); 
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Png::IDAT()
{
    //Parsing
    Skip_XX(Element_TotalSize_Get()-4,                          "Data");
    Param2("CRC",                                               "(Skipped) (4 bytes)");
    if (Config->ParseSpeed<1.0)
        Finish();
}

//---------------------------------------------------------------------------
void File_Png::IHDR()
{
    //Parsing
    int32u Width, Height;
    int8u  Bit_depth, Colour_type, Compression_method, Interlace_method;
    Get_B4 (Width,                                              "Width");
    Get_B4 (Height,                                             "Height");
    Get_B1 (Bit_depth,                                          "Bit depth");
    Get_B1 (Colour_type,                                        "Colour type"); Param_Info1(Png_Colour_type(Colour_type));
    Get_B1 (Compression_method,                                 "Compression method");
    Skip_B1(                                                    "Filter method");
    Get_B1 (Interlace_method,                                   "Interlace method");

    FILLING_BEGIN_PRECISE();
        if (!Status[IsFilled])
        {
            Fill(StreamKind_Last, 0, "Width", Width);
            Fill(StreamKind_Last, 0, "Height", Height);
            string ColorSpace=(Colour_type&(1<<1))?"RGB":"Y";
            if (Colour_type&(1<<2))
                ColorSpace+='A';
            Fill(StreamKind_Last, 0, "ColorSpace", ColorSpace);
            Fill(StreamKind_Last, 0, "BitDepth", Bit_depth);
            if (Retrieve_Const(StreamKind_Last, 0, "PixelAspectRatio").empty())
                Fill(StreamKind_Last, 0, "PixelAspectRatio", 1.0, 3);
            switch (Compression_method)
            {
                case 0 :
                    Fill(StreamKind_Last, 0, "Format_Compression", "Deflate");
                    break;
                default: ;
            }
            switch (Interlace_method)
            {
                case 0 :
                    break;
                case 1 :
                    break;
                default: ;
            }

            Fill();
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Png::gAMA()
{
    //Parsing
    int32u Gamma;
    Get_B4 (Gamma,                                              "Gamma");

    FILLING_BEGIN()
        Fill(StreamKind_Last, 0, "Gamma", Gamma/100000.0);
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Png::pHYs()
{
    //Parsing
    int32u X, Y;
    Get_B4 (X,                                                  "Pixels per unit, X axis");
    Get_B4 (Y,                                                  "Pixels per unit, Y axis");
    Skip_B1(                                                    "Unit specifier");

    FILLING_BEGIN()
        if (X && Y)
        {
            Clear(StreamKind_Last, 0, "DisplayAspectRatio");
            Fill(StreamKind_Last, 0, "PixelAspectRatio", ((float32)Y) / X, 3, true);
        }
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Png::sBIT()
{
    //Parsing
    map<int8u, int64u> Bits;
    for (int64u i=0; i<Element_Size; i++)
    {
        int8u Bit;
        Get_B1 (Bit,                                            "Significant bits");
        Bits[Bit]++;
    }

    FILLING_BEGIN()
        if (Bits.size()==1)
            Fill(StreamKind_Last, 0, "BitDepth", Bits.begin()->first, 10, true);
    FILLING_END()
}

} //NameSpace

#endif
