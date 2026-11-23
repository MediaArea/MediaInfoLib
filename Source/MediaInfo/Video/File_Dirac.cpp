/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

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
#if defined(MEDIAINFO_DIRAC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Dirac.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
const char* Dirac_base_video_format(intu base_video_format)
{
    switch (base_video_format)
    {
        case   1 : return "QSIF525";
        case   2 : return "QCIF";
        case   3 : return "SIF525";
        case   4 : return "CIF";
        case   5 : return "4SIF525";
        case   6 : return "4CIF";
        case   7 : return "480i60";
        case   8 : return "576i50";
        case   9 : return "720p50";
        case  10 : return "720p60";
        case  11 : return "1080i60";
        case  12 : return "1080i50";
        case  13 : return "1080p60";
        case  14 : return "1080p60";
        case  15 : return "2K-24";
        case  16 : return "4K-24";
        case  17 : return "4K-60";
        case  18 : return "4K-50";
        case  19 : return "8K-60";
        case  20 : return "8K-50";
        case  21 : return "1080p24";
        case  22 : return "Pro486";
        default  : return "";
    }
}

//---------------------------------------------------------------------------
float32 Dirac_frame_rate(int32u frame_rate_index)
{
    switch (frame_rate_index)
    {
        case  0 : return (float32)0; //Reserved
        case  1 : return (float32)24000/(float32)1001;
        case  2 : return (float32)24;
        case  3 : return (float32)25;
        case  4 : return (float32)30000/(float32)1001;
        case  5 : return (float32)30;
        case  6 : return (float32)50;
        case  7 : return (float32)60000/(float32)1001;
        case  8 : return (float32)60;
        case  9 : return (float32)15000/(float32)1001;
        case 10 : return (float32)12.5;
        case 11 : return (float32)48;
        case 12 : return (float32)48000/(float32)1001;
        case 13 : return (float32)96;
        case 14 : return (float32)100;
        case 15 : return (float32)120000/(float32)1001;
        case 16 : return (float32)120;
        default : return (float32)0; //Unknown
    }
}

//---------------------------------------------------------------------------
float32 Dirac_pixel_aspect_ratio(int32u pixel_aspect_ratio_index)
{
    switch (pixel_aspect_ratio_index)
    {
        case  0 : return (float32)0; //Reserved
        case  1 : return (float32)1; //Reserved
        case  2 : return (float32)10/(float32)11;
        case  3 : return (float32)12/(float32)11;
        case  4 : return (float32)40/(float32)33;
        case  5 : return (float32)16/(float32)11;
        case  6 : return (float32) 4/(float32) 3;
        default : return (float32)0; //Unknown
    }
}

//---------------------------------------------------------------------------
const char* Dirac_picture_coding_mode(int32u picture_coding_mode)
{
    switch (picture_coding_mode)
    {
        case 0 : return "PPF";
        case 1 : return "Interlaced";
        default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Dirac_source_sampling(int32u source_sampling)
{
    switch (source_sampling)
    {
        case 0 : return "Progressive";
        case 1 : return "Interlaced";
        default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Dirac_source_sampling_Codec(int32u source_sampling)
{
    switch (source_sampling)
    {
        case 0 : return "PPF";
        case 1 : return "Interlaced";
        default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Dirac_chroma_format(int32u chroma_format)
{
    switch (chroma_format)
    {
        case 0 : return "4:4:4";
        case 1 : return "4:2:2";
        case 2 : return "4:2:0";
        default: return "";
    }
}

//---------------------------------------------------------------------------
void Dirac_base_video_format(int32u   base_video_format,
                             int32u  &frame_width,
                             int32u  &frame_height,
                             int32u  &chroma_format,
                             int32u  &source_sampling,
                             int32u  &clean_width,
                             int32u  &clean_height,
                             int32u  &clean_left_offset,
                             int32u  &clean_top_offset,
                             float32 &frame_rate,
                             float32 &pixel_aspect_ratio)
{
    switch (base_video_format)
    {
        case   0 :  frame_width=640;
                    frame_height=480;
                    chroma_format=2;
                    source_sampling=0;
                    clean_width=640;
                    clean_height=480;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(1);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case   1 :  frame_width=176;
                    frame_height=120;
                    chroma_format=2;
                    source_sampling=0;
                    clean_width=176;
                    clean_height=120;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(9);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(2);
                    return;
        case   2 :  frame_width=176;
                    frame_height=144;
                    chroma_format=2;
                    source_sampling=0;
                    clean_width=176;
                    clean_height=144;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(10);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(3);
                    return;
        case   3 :  frame_width=352;
                    frame_height=240;
                    chroma_format=2;
                    source_sampling=0;
                    clean_width=352;
                    clean_height=240;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(9);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(2);
                    return;
        case   4 :  frame_width=352;
                    frame_height=288;
                    chroma_format=2;
                    source_sampling=0;
                    clean_width=352;
                    clean_height=288;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(10);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(3);
                    return;
        case   5 :  frame_width=704;
                    frame_height=480;
                    chroma_format=2;
                    source_sampling=0;
                    clean_width=704;
                    clean_height=480;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(9);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(2);
                    return;
        case   6 :  frame_width=704;
                    frame_height=576;
                    chroma_format=2;
                    source_sampling=0;
                    clean_width=704;
                    clean_height=576;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(10);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(3);
                    return;
        case   7 :  frame_width=720;
                    frame_height=480;
                    chroma_format=1;
                    source_sampling=1;
                    clean_width=704;
                    clean_height=480;
                    clean_left_offset=8;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(4);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(2);
                    return;
        case   8 :  frame_width=720;
                    frame_height=576;
                    chroma_format=1;
                    source_sampling=1;
                    clean_width=704;
                    clean_height=576;
                    clean_left_offset=8;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(3);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(3);
                    return;
        case   9 :  frame_width=1280;
                    frame_height=720;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=1280;
                    clean_height=720;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(7);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  10 :  frame_width=1280;
                    frame_height=720;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=1280;
                    clean_height=720;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(6);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  11 :  frame_width=1920;
                    frame_height=1080;
                    chroma_format=1;
                    source_sampling=1;
                    clean_width=1920;
                    clean_height=1080;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(4);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  12 :  frame_width=1920;
                    frame_height=1080;
                    chroma_format=1;
                    source_sampling=1;
                    clean_width=1920;
                    clean_height=1080;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(3);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  13 :  frame_width=1920;
                    frame_height=1080;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=1920;
                    clean_height=1080;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(7);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  14 :  frame_width=1920;
                    frame_height=1080;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=1920;
                    clean_height=1080;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(6);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  15 :  frame_width=2048;
                    frame_height=1080;
                    chroma_format=0;
                    source_sampling=0;
                    clean_width=2048;
                    clean_height=1080;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(2);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  16 :  frame_width=4096;
                    frame_height=2160;
                    chroma_format=0;
                    source_sampling=0;
                    clean_width=4096;
                    clean_height=2160;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(2);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  17 :  frame_width=3840;
                    frame_height=2160;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=3840;
                    clean_height=2160;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(7);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  18 :  frame_width=3840;
                    frame_height=2160;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=3840;
                    clean_height=2160;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(6);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  19 :  frame_width=7680;
                    frame_height=4320;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=7680;
                    clean_height=4320;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(7);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  20 :  frame_width=7680;
                    frame_height=4320;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=7680;
                    clean_height=4320;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(6);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  21 :  frame_width=1920;
                    frame_height=1080;
                    chroma_format=1;
                    source_sampling=0;
                    clean_width=1920;
                    clean_height=1080;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(1);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(1);
                    return;
        case  22 :  frame_width=720;
                    frame_height=486;
                    chroma_format=1;
                    source_sampling=1;
                    clean_width=720;
                    clean_height=486;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate(4);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio(2);
                    return;
        default  :  frame_width=0;
                    frame_height=0;
                    chroma_format=(int32u)-1;
                    source_sampling=(int32u)-1;
                    clean_width=0;
                    clean_height=0;
                    clean_left_offset=0;
                    clean_top_offset=0;
                    frame_rate=Dirac_frame_rate((int32u)-1);
                    pixel_aspect_ratio=Dirac_pixel_aspect_ratio((int32u)-1);
                    return;
    }
}

//---------------------------------------------------------------------------
// Compression mode of a picture, the highest one of the parsed pictures is kept
enum dirac_compression_mode : int8u
{
    Dirac_CompressionMode_None,
    Dirac_CompressionMode_Lossless, //Quantisers of 0
    Dirac_CompressionMode_Unknown,  //Quantisation indexes not read
    Dirac_CompressionMode_Lossy,
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Dirac::File_Dirac()
:File__Analyze()
{
    //Configuration
    MustSynchronize=true;

    //In
    Frame_Count_Valid=1;
    Ignore_End_of_Sequence=false;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dirac::Streams_Fill()
{
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, (IsVc2 && !HasDiracOnlyPicture)?"VC-2":"Dirac");
    Fill(Stream_Video, 0, Video_Codec, (IsVc2 && !HasDiracOnlyPicture)?"VC-2":"Dirac");

    if (clean_width)
        Fill(Stream_Video, StreamPos_Last, Video_Width, clean_width);
    if (clean_height)
        Fill(Stream_Video, StreamPos_Last, Video_Height, clean_height);
    if (pixel_aspect_ratio)
    {
        Fill(Stream_Video, 0, Video_PixelAspectRatio, pixel_aspect_ratio, 3, true);
        if (clean_height!=0)
            Fill(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio, ((float)clean_width)/clean_height*pixel_aspect_ratio, 3, true);
    }
    if (frame_rate)
        Fill(Stream_Video, StreamPos_Last, Video_FrameRate, frame_rate);
    Fill(Stream_Video, 0, Video_ChromaSubsampling, Dirac_chroma_format(chroma_format));
    Fill(Stream_Video, 0, Video_ScanType, Dirac_source_sampling(source_sampling));
    Fill(Stream_Video, 0, Video_Interlacement, Dirac_source_sampling_Codec(source_sampling));
}

//---------------------------------------------------------------------------
void File_Dirac::Streams_Finish()
{
    //Lossless only if every picture is parsed, an encoder with a rate control codes simple pictures (e.g. black) without quantisation in a lossy stream
    if (Compression_Mode==Dirac_CompressionMode_Lossy || (Compression_Mode==Dirac_CompressionMode_Lossless && Config->ParseSpeed>=1))
        Fill(Stream_Video, 0, Video_Compression_Mode, Compression_Mode==Dirac_CompressionMode_Lossless?"Lossless":"Lossy", Unlimited, true, true);

    //Purge what is not needed anymore
    if (!File_Name.empty()) //Only if this is not a buffer, with buffer we can have more data
        Streams.clear();
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Dirac::Synchronize()
{
    static const int8u Sync_Pattern[5] = { 'B', 'B', 'C', 'D', 0x00 };

    //Synchronizing
    while (Buffer_Size - Buffer_Offset >= 5)
    {
        const size_t Candidate_End = Buffer_Size - 4;
        const int8u* Sync = (const int8u*)memchr(Buffer + Buffer_Offset, Sync_Pattern[0], Candidate_End - Buffer_Offset);
        if (!Sync)
        {
            Buffer_Offset = Candidate_End;
            break;
        }
        Buffer_Offset = (size_t)(Sync - Buffer);

        if (!memcmp(Sync + 1, Sync_Pattern + 1, 4))
        {
            // Checking the next frame start
            if (Buffer_Size - Buffer_Offset <= 9)
                return false; //Need more data
            const auto Size = BigEndian2int32u(Buffer + Buffer_Offset + 5);
            if (Size >= 13 && Size <= 1024)
            {
                if (Buffer_Size - Buffer_Offset <= Size)
                    return false; //Need more data
                const size_t Next_Available = Buffer_Size - Buffer_Offset - Size;
                char Previous_Size[4];
                int32u2BigEndian(Previous_Size, Size);
                if (!memcmp(Sync + Size, Sync_Pattern, Next_Available < 4 ? Next_Available : 4)
                 && (Next_Available <= 9 || !memcmp(Sync + Size + 9, Previous_Size, Next_Available < 13 ? Next_Available - 9 : 4)))
                {
                    if (Next_Available < 13)
                        return false; //Need more data
                    return true; //Synched is OK
                }
            }
        }

        Buffer_Offset++;
    }

    //Parsing last bytes if needed
    while (Buffer_Offset < Buffer_Size
        && memcmp(Buffer + Buffer_Offset, Sync_Pattern, Buffer_Size - Buffer_Offset))
    {
        Buffer_Offset++;
    }

    return false;
}

//---------------------------------------------------------------------------
bool File_Dirac::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+4>Buffer_Size)
        return false;

    //Quick test of synchro
    if (CC4(Buffer+Buffer_Offset)!=0x42424344) //"BBCD"
        Synched=false;

    //Quick search
    if (Synched && !Header_Parser_QuickSearch())
        return false;

    //We continue
    return true;
}

//---------------------------------------------------------------------------
void File_Dirac::Synched_Init()
{
    //Temp
    Next_Parse_Offset=0;
    version_major=0;
    IsVc2=false;
    HasDiracOnlyPicture=false;
    Compression_Mode=Dirac_CompressionMode_None;
    Dirac_base_video_format((int32u)-1, frame_width, frame_height, chroma_format, source_sampling,
                            clean_width, clean_height, clean_left_offset, clean_top_offset,
                            frame_rate, pixel_aspect_ratio);

    //Default stream values
    Streams.resize(0x100);
    Streams[0x00].Searching_Payload=true; //Sequence header
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dirac::Header_Parse()
{
    //Parsing
    int32u Previous_Parse_Offset;
    int8u  Parse_Code;
    Skip_C4(                                                    "Parse Info Prefix");
    Get_B1 (Parse_Code,                                         "Parse Code");
    Get_B4 (Next_Parse_Offset,                                  "Next Parse Offset");
    Get_B4 (Previous_Parse_Offset,                              "Previous Parse Offset");

    //Filling
    Header_Fill_Code(Parse_Code, Ztring().From_CC1(Parse_Code));
    if (!Next_Parse_Offset && (Parse_Code&0x08) && !Element_IsWaitingForMoreData()) //Picture from a real-time encoder which does not know its size
    {
        if (!Header_Parser_Fill_Size())
            Element_WaitForMoreData();
        return;
    }
    Header_Fill_Size((Parse_Code==0x10 && Next_Parse_Offset==0)?13:Next_Parse_Offset); //Speacial case if this is the End Of Sequence
}

//---------------------------------------------------------------------------
bool File_Dirac::Header_Parser_Fill_Size()
{
    //Look for the next parse info prefix
    if (Buffer_Offset_Temp==0) //Buffer_Offset_Temp is not 0 if the previous bytes were already searched
        Buffer_Offset_Temp=Buffer_Offset+13;
    while (Buffer_Offset_Temp+4<=Buffer_Size)
    {
        const int8u* Prefix=(const int8u*)memchr(Buffer+Buffer_Offset_Temp, 'B', Buffer_Size-3-Buffer_Offset_Temp);
        if (!Prefix)
        {
            Buffer_Offset_Temp=Buffer_Size-3;
            break;
        }
        Buffer_Offset_Temp=(size_t)(Prefix-Buffer);
        if (CC4(Prefix)==0x42424344) //"BBCD"
            break;
        Buffer_Offset_Temp++;
    }

    //Must wait more data?
    if (Buffer_Offset_Temp+4>Buffer_Size)
    {
        if (FrameIsAlwaysComplete || Config->IsFinishing || File_Offset+Buffer_Size==File_Size)
            Buffer_Offset_Temp=Buffer_Size; //No other parse info header
        else
            return false;
    }

    //OK, we continue
    Header_Fill_Size(Buffer_Offset_Temp-Buffer_Offset);
    Buffer_Offset_Temp=0;
    return true;
}

//---------------------------------------------------------------------------
bool File_Dirac::Header_Parser_QuickSearch()
{
    for (;;) 
    {
        if (Buffer_Size - Buffer_Offset <= 8)
            return false;

        //Getting start_code
        auto prefix_and_start_code = BigEndian2int40u(Buffer + Buffer_Offset);
        auto prefix = prefix_and_start_code >> 8;
        if (prefix != 0x42424344) break; //"BBCD"
        auto start_code = prefix_and_start_code & 0xFF;

        //Searching start
        if (Streams[start_code].Searching_Payload)
            return true;

        //Getting size
        auto Size = BigEndian2int32u(Buffer + Buffer_Offset + 5);
        if (Size <= 8) break;
        if (Size > Buffer_Size - Buffer_Offset) return false;
        Buffer_Offset += Size;
    }

    Synched=false;
    return Synchronize();
}

//---------------------------------------------------------------------------
void File_Dirac::Data_Parse()
{
    //Parsing
    switch (Element_Code)
    {
        case 0x00 : Sequence_header(); break;
        case 0x10 : End_of_Sequence(); break;
        case 0x20 : Auxiliary_data(); break;
        case 0x30 : Padding_data(); break;
        case 0x0C : Intra_Reference_Picture(); break;
        case 0x08 : Intra_Non_Reference_Picture(); break;
        case 0x4C : Intra_Reference_Picture_No(); break;
        case 0x48 : Intra_Non_Reference_Picture_No(); break;
        case 0x0D : Inter_Reference_Picture_1(); break;
        case 0x0E : Inter_Reference_Picture_2(); break;
        case 0x09 : Inter_Non_Reference_Picture_1(); break;
        case 0x0A : Inter_Non_Reference_Picture_2(); break;
        case 0xCC : if (version_major<3) Reference_Picture_Low(); else Low_Delay_Picture_Fragment(); break;
        case 0xC8 : Intra_Non_Reference_Picture_Low(); break;
        case 0xE8 : High_Quality_Picture(); break;
        case 0xEC : High_Quality_Picture_Fragment(); break;
        default   : Reserved();
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
// Packet "00"
void File_Dirac::Sequence_header()
{
    Element_Name("Sequence header");

    //Parsing
    int32u version_minor, profile, level, base_video_format;
    BS_Begin();
    Get_UI(version_major,                                       "version major");
    Get_UI(version_minor,                                       "version minor");
    Get_UI(profile,                                             "profile");
    Get_UI(level,                                               "level");

    if (version_major<=3)
    {
        Get_UI(base_video_format,                               "base video format"); //Param_Info1(Dirac_base_video_format(base_video_format));
        Dirac_base_video_format(base_video_format, frame_width, frame_height, chroma_format, source_sampling,
                                clean_width, clean_height, clean_left_offset, clean_top_offset,
                                frame_rate, pixel_aspect_ratio);
        bool custom_dimensions_flag;
        TEST_SB_GET (custom_dimensions_flag,                    "custom dimensions flag");
            Get_UI (frame_width,                                "frame width");
            Get_UI (frame_height,                               "frame height");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "custom chroma format flag");
            Get_UI (chroma_format,                              "chroma format"); Param_Info1(Dirac_chroma_format(chroma_format));
        TEST_SB_END();
        TEST_SB_SKIP(                                           "custom scan format flag");
            Get_UI (source_sampling,                            "source sampling"); Param_Info1(Dirac_source_sampling(source_sampling));
        TEST_SB_END();
        TEST_SB_SKIP(                                           "frame rate flag");
            int32u frame_rate_index;
            Get_UI (frame_rate_index,                           "index"); Param_Info1(Dirac_frame_rate(frame_rate_index));
            if (frame_rate_index==0)
            {
                int32u frame_rate_numer, frame_rate_denom;
                Get_UI (frame_rate_numer,                       "frame rate numer");
                Get_UI (frame_rate_denom,                       "frame rate denom");
                if (Element_IsOK() && !frame_rate_numer)
                    Fill_Conformance("sequence_header frame_rate_numer", "frame_rate_numer is 0");
                if (Element_IsOK() && !frame_rate_denom)
                    Fill_Conformance("sequence_header frame_rate_denom", "frame_rate_denom is 0");
                frame_rate=(frame_rate_numer && frame_rate_denom)?((float32)frame_rate_numer)/((float32)frame_rate_denom):0;
            }
            else
                frame_rate=Dirac_frame_rate(frame_rate_index);
        TEST_SB_END();
        TEST_SB_SKIP(                                           "pixel aspect ratio flag");
            int32u pixel_aspect_ratio_index;
            Get_UI (pixel_aspect_ratio_index,                   "index"); Param_Info1(Dirac_pixel_aspect_ratio(pixel_aspect_ratio_index));
            if (pixel_aspect_ratio_index==0)
            {
                int32u pixel_aspect_ratio_numer, pixel_aspect_ratio_denom;
                Get_UI (pixel_aspect_ratio_numer,               "pixel aspect ratio numer");
                Get_UI (pixel_aspect_ratio_denom,               "pixel aspect ratio denom");
                if (Element_IsOK() && !pixel_aspect_ratio_numer)
                    Fill_Conformance("sequence_header pixel_aspect_ratio_numer", "pixel_aspect_ratio_numer is 0");
                if (Element_IsOK() && !pixel_aspect_ratio_denom)
                    Fill_Conformance("sequence_header pixel_aspect_ratio_denom", "pixel_aspect_ratio_denom is 0");
                pixel_aspect_ratio=(pixel_aspect_ratio_numer && pixel_aspect_ratio_denom)?((float32)pixel_aspect_ratio_numer)/((float32)pixel_aspect_ratio_denom):0;
            }
            else
                pixel_aspect_ratio=Dirac_pixel_aspect_ratio(pixel_aspect_ratio_index);
        TEST_SB_END();
        TESTELSE_SB_SKIP(                                       "custom clean area flag");
            Get_UI (clean_width,                                "clean width");
            Get_UI (clean_height,                               "clean height");
            Get_UI (clean_left_offset,                          "clean left offset");
            Get_UI (clean_top_offset,                           "clean top offset");
        TESTELSE_SB_ELSE(                                       "custom clean area flag");
            //The clean area of the base video format is kept, except with custom dimensions which it does not fit (some encoders write custom dimensions only)
            if (custom_dimensions_flag)
            {
                clean_width=frame_width;
                clean_height=frame_height;
            }
        TESTELSE_SB_END();
        TEST_SB_SKIP(                                           "custom signal range flag");
            int32u custom_signal_range_index;
            Get_UI(custom_signal_range_index,                   "index");
            if (custom_signal_range_index==0)
            {
                Skip_UI(                                        "luma offset");
                Skip_UI(                                        "luma excursion");
                Skip_UI(                                        "chroma offset");
                Skip_UI(                                        "chroma excursion");
            }
        TEST_SB_END();
        TEST_SB_SKIP(                                           "custom colour spec flag");
            int32u custom_colour_spec_index;
            Get_UI(custom_colour_spec_index,                    "index");
            if (custom_colour_spec_index==0)
            {
                TEST_SB_SKIP(                                   "custom colour primaries flag");
                    Skip_UI(                                    "custom colour primaries index");
                TEST_SB_END();
                TEST_SB_SKIP(                                   "colour matrix flag");
                    Skip_UI(                                    "colour matrix index");
                TEST_SB_END();
                TEST_SB_SKIP(                                   "custom transfer function flag");
                    Skip_UI(                                    "custom transfer function index");
                TEST_SB_END();
            }
        TEST_SB_END();
        Info_UI(picture_coding_mode,                            "picture coding mode"); Param_Info1(Dirac_picture_coding_mode(picture_coding_mode));
        BS_End();
    }
    else
    {
        BS_End();
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");
    }

    FILLING_BEGIN();
        //Version 1 of Dirac is VC-2, version 3 and the high quality profile exist only in VC-2
        if (version_major==1 || version_major>=3 || profile==3)
            IsVc2=true;

        //Autorisation of other streams
        Streams[0x10].Searching_Payload=true; //End_of_Sequence
        Streams[0x20].Searching_Payload=true; //Auxiliary_data
        Streams[0x30].Searching_Payload=true; //Padding_data
        Streams[0x0C].Searching_Payload=true; //Intra_Reference_Picture
        Streams[0x08].Searching_Payload=true; //Intra_Non_Reference_Picture
        Streams[0x4C].Searching_Payload=true; //Intra_Reference_Picture_No
        Streams[0x48].Searching_Payload=true; //Intra_Non_Reference_Picture_No
        Streams[0x0D].Searching_Payload=true; //Inter_Reference_Picture_1
        Streams[0x0E].Searching_Payload=true; //Inter_Reference_Picture_2
        Streams[0x09].Searching_Payload=true; //Inter_Non_Reference_Picture_1
        Streams[0x0A].Searching_Payload=true; //Inter_Non_Reference_Picture_2
        Streams[0xCC].Searching_Payload=true; //Reference_Picture_Low
        Streams[0xC8].Searching_Payload=true; //Intra_Non_Reference_Picture_Low
        Streams[0xE8].Searching_Payload=true; //High_Quality_Picture
        Streams[0xEC].Searching_Payload=true; //High_Quality_Picture_Fragment
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "10"
void File_Dirac::End_of_Sequence()
{
    Element_Name("End of Sequence");

    //Parsing
    if (!Ignore_End_of_Sequence)
    {
        NextCode_Clear();
        Accept("Dirac");
        Finish("Dirac");
    }
}

//---------------------------------------------------------------------------
// Packet "20"
void File_Dirac::Auxiliary_data()
{
    Element_Name("Auxiliary data");

    //Parsing
    Skip_XX(Element_Size,                                       "Auxiliary data");
}

//---------------------------------------------------------------------------
// Packet "30"
void File_Dirac::Padding_data()
{
    Element_Name("Padding data");

    //Parsing
    Skip_XX(Element_Size,                                       "Padding data");
}

//---------------------------------------------------------------------------
// Packet "0C"
void File_Dirac::Intra_Reference_Picture()
{
    Element_Name("Intra Reference Picture");
    HasDiracOnlyPicture=true;

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "08"
void File_Dirac::Intra_Non_Reference_Picture()
{
    Element_Name("Intra Non Reference Picture");

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "4C"
void File_Dirac::Intra_Reference_Picture_No()
{
    Element_Name("Intra Reference Picture (no arithmetic coding)");
    HasDiracOnlyPicture=true;

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "48"
void File_Dirac::Intra_Non_Reference_Picture_No()
{
    Element_Name("Intra Non Reference Picture (no arithmetic coding)");

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "0D"
void File_Dirac::Inter_Reference_Picture_1()
{
    Element_Name("Inter Reference Picture (1 picture)");
    HasDiracOnlyPicture=true;

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "0E"
void File_Dirac::Inter_Reference_Picture_2()
{
    Element_Name("Inter Reference Picture (2 pictures)");
    HasDiracOnlyPicture=true;

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "09"
void File_Dirac::Inter_Non_Reference_Picture_1()
{
    Element_Name("Inter Non Reference Picture (1 picture)");
    HasDiracOnlyPicture=true;

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "0A"
void File_Dirac::Inter_Non_Reference_Picture_2()
{
    Element_Name("Inter Non Reference Picture (2 pictures)");
    HasDiracOnlyPicture=true;

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "CC"
void File_Dirac::Reference_Picture_Low()
{
    Element_Name("Reference Picture (low-delay)");
    HasDiracOnlyPicture=true;

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "CC", from version 3
void File_Dirac::Low_Delay_Picture_Fragment()
{
    Element_Name("Low Delay Picture Fragment");

    //Parsing
    picture_fragment();
}

//---------------------------------------------------------------------------
// Packet "C8"
void File_Dirac::Intra_Non_Reference_Picture_Low()
{
    Element_Name("Intra Non Reference Picture (low-delay)");

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "E8"
void File_Dirac::High_Quality_Picture()
{
    Element_Name("High Quality Picture");
    IsVc2=true;

    //Parsing
    picture();
}

//---------------------------------------------------------------------------
// Packet "EC"
void File_Dirac::High_Quality_Picture_Fragment()
{
    Element_Name("High Quality Picture Fragment");
    IsVc2=true;

    //Parsing
    picture_fragment();
}

//---------------------------------------------------------------------------
void File_Dirac::Reserved()
{
    Element_Name("Reserved");

    Skip_XX(Element_Size,                                       "Unknown");
}

//---------------------------------------------------------------------------
void File_Dirac::picture(bool IsFragment)
{
    //Parsing
    int8u Mode=Dirac_CompressionMode_Unknown;
    if (!IsFragment && !(Element_Code&0x03) && version_major && version_major<=3) //Quantisation indexes of inter pictures are after the motion data, not read; version 0 is from software before the specification, with another picture syntax
        Mode=picture_Compression_Mode();
    Skip_XX(Element_Size-Element_Offset,                        "Data");

    FILLING_BEGIN();
        if (Compression_Mode<Mode)
            Compression_Mode=Mode;

        //Counting
        if (File_Offset+Buffer_Offset+Element_Size==File_Size)
            Frame_Count_Valid=Frame_Count; //Finalize frames in case of there are less than Frame_Count_Valid frames

        //Name
        Element_Info1(Ztring::ToZtring(Frame_Count));

        //Filling only if not already done
        Frame_Count++;
        Frame_Count_InThisBlock++;
        if (Frame_Count>=Frame_Count_Valid && Count_Get(Stream_Video)==0)
        {
            NextCode_Clear();
            Accept("Dirac");
            Finish("Dirac");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dirac::picture_fragment()
{
    //Parsing
    int16u fragment_slice_count;
    Skip_B4(                                                    "picture number");
    Skip_B2(                                                    "fragment data length");
    Get_B2 (fragment_slice_count,                               "fragment slice count");
    if (fragment_slice_count)
    {
        Skip_B2(                                                "fragment x offset");
        Skip_B2(                                                "fragment y offset");
        Skip_XX(Element_Size-Element_Offset,                    "Data");
        return;
    }

    //First fragment of a picture, with its transform parameters
    picture(true);
}

//---------------------------------------------------------------------------
// Compression mode from the quantisation indexes, read without decoding the coefficients
// Read without the element functions: a picture not as expected is accepted as before, the parser synchronizing again only on a sequence header
int8u File_Dirac::picture_Compression_Mode()
{
    const int8u* Data=Buffer+Buffer_Offset+(size_t)Element_Offset;
    int64u Data_Size=Element_Size-Element_Offset;
    int64u BitPos=4*8, BitSize=Data_Size*8; //After the picture number
    bool IsComplete=Next_Parse_Offset==Header_Size+Element_Size; //Else the data unit is cut or its size is unknown, and a count bigger than its data is not malformed
    auto GetBit=[&]() -> bool {
        if (BitPos>=BitSize)
        {
            BitPos=BitSize+1; //Out of data, ending the codes
            return true;
        }
        bool Value=(Data[(size_t)(BitPos>>3)]>>(7-(BitPos&7)))&1;
        BitPos++;
        return Value;
    };
    auto GetUInt=[&]() -> int32u {
        int32u Value=1;
        while (!GetBit())
        {
            Value<<=1;
            if (GetBit())
                Value++;
        }
        return Value-1;
    };
    auto Align=[&]() {
        BitPos=(BitPos+7)&~(int64u)7;
    };
    auto Remain=[&]() -> int64u {
        return BitPos<BitSize?BitSize-BitPos:0;
    };
    auto Loop_IsOK=[&](int64u Count, int64u RemainingSize, int64u MinBlockSize, const char* Name) -> bool {
        if (BitPos>BitSize) //Out of data, the count is not from the stream
            return false;
        if (!IsComplete)
            return Count<=RemainingSize/MinBlockSize;
        int64u Count_Max=Count;
        Loop_CheckValue(Count_Max, RemainingSize, MinBlockSize, Name);
        return Count_Max==Count;
    };

    //Picture header
    if ((Element_Code&0x0C)==0x0C) //Retired picture of a reference picture
    {
        if (GetUInt())
            GetBit();
        Align();
    }

    //Transform parameters
    GetUInt(); //Wavelet index
    int32u dwt_depth=GetUInt(), dwt_depth_ho=0, slices_x=0, slices_y=0, slice_bytes_numerator=0, slice_bytes_denominator=0, slice_prefix_bytes=0, slice_size_scaler=0, quant_matrix_min=0;
    if (version_major>=3)
    {
        if (GetBit())
            GetUInt(); //Horizontal only wavelet index
        if (GetBit())
            dwt_depth_ho=GetUInt();
    }
    bool IsSliced=(Element_Code&0x88)==0x88; //Low delay or high quality
    if (!IsSliced)
    {
        if (GetBit()) //Spatial partition
        {
            int64u Levels=(int64u)dwt_depth+1;
            if (!Loop_IsOK(Levels, Remain(), 2, "codeblocks"))
                return Dirac_CompressionMode_Unknown;
            for (int64u Level=0; Level<Levels; Level++)
            {
                GetUInt();
                GetUInt();
            }
            if (GetUInt()) //Codeblock mode 1, quantisation offsets of the codeblocks are in the coefficient data
                return Dirac_CompressionMode_Unknown;
        }
    }
    else
    {
        slices_x=GetUInt();
        slices_y=GetUInt();
        if (Element_Code&0x20) //High quality
        {
            slice_prefix_bytes=GetUInt();
            slice_size_scaler=GetUInt();
        }
        else
        {
            slice_bytes_numerator=GetUInt();
            slice_bytes_denominator=GetUInt();
        }
        if (GetBit()) //Custom quantisation matrix
        {
            int64u Values=1+(int64u)dwt_depth_ho+3*(int64u)dwt_depth;
            if (!Loop_IsOK(Values, Remain(), 1, "quant_matrix"))
                return Dirac_CompressionMode_Unknown;
            quant_matrix_min=(int32u)-1;
            for (int64u Pos=0; Pos<Values; Pos++)
            {
                int32u Value=GetUInt();
                if (quant_matrix_min>Value)
                    quant_matrix_min=Value;
            }
        }
    }
    Align();
    if (BitPos>BitSize)
        return Dirac_CompressionMode_Unknown;

    //Transform data, the quantiser of a subband or a slice is its index minus the quantisation matrix value
    //A picture whose indexes are not all read is unknown, also with a lossy index read before, which may come from data shifted by a lost packet
    int8u Mode=Dirac_CompressionMode_None;
    auto QuantIndex=[&](int32u Value) {
        if (Value>quant_matrix_min)
            Mode=Dirac_CompressionMode_Lossy;
        else if (Mode==Dirac_CompressionMode_None)
            Mode=Dirac_CompressionMode_Lossless;
    };
    if (!IsSliced)
    {
        int64u Subbands=3*(1+3*(int64u)dwt_depth);
        if (!Loop_IsOK(Subbands, Remain()/8, 1, "subbands"))
            return Dirac_CompressionMode_Unknown;
        for (int64u Pos=0; Pos<Subbands; Pos++)
        {
            int32u length=GetUInt();
            int32u quant_index=length?GetUInt():0;
            Align();
            if ((int64u)length*8>Remain() || BitPos>BitSize)
                return Dirac_CompressionMode_Unknown;
            if (length)
                QuantIndex(quant_index);
            BitPos+=(int64u)length*8;
        }
    }
    else
    {
        Data+=(size_t)(BitPos/8);
        Data_Size-=BitPos/8;
        int64u Slices=(int64u)slices_x*slices_y;
        if (Element_Code&0x20) //High quality, prefix bytes, a quantisation index, then for each component a length of 1 byte and the data
        {
            if (!Loop_IsOK(Slices, Data_Size, (int64u)slice_prefix_bytes+4, "slices"))
                return Dirac_CompressionMode_Unknown;
            int64u Offset=0;
            for (int64u Pos=0; Pos<Slices; Pos++)
            {
                int64u QuantIndex_Offset=Offset+slice_prefix_bytes;
                Offset=QuantIndex_Offset+1;
                int8u Component=0;
                for (; Component<3 && Offset<Data_Size; Component++)
                    Offset+=1+(int64u)slice_size_scaler*Data[(size_t)Offset];
                if (Component<3 || Offset>Data_Size)
                    return Dirac_CompressionMode_Unknown;
                QuantIndex(Data[(size_t)QuantIndex_Offset]);
            }
        }
        else if (slice_bytes_denominator && slice_bytes_numerator>=slice_bytes_denominator) //Low delay, slices of 1 byte or more with a quantisation index of 7 bits at their start
        {
            if (!Loop_IsOK(Slices, Data_Size, slice_bytes_numerator/slice_bytes_denominator, "slices"))
                return Dirac_CompressionMode_Unknown;
            for (int64u Pos=0; Pos<Slices; Pos++)
            {
                if ((Pos+1)*slice_bytes_numerator/slice_bytes_denominator>Data_Size)
                    return Dirac_CompressionMode_Unknown;
                QuantIndex(Data[(size_t)(Pos*slice_bytes_numerator/slice_bytes_denominator)]>>1);
            }
        }
    }

    if (Mode==Dirac_CompressionMode_None)
        Mode=Dirac_CompressionMode_Unknown;
    return Mode;
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_DIRAC_YES
