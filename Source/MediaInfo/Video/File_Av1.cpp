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
#if defined(MEDIAINFO_AV1_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Av1.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
extern const char* Mpegv_colour_primaries(int8u colour_primaries);
extern const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
extern const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
extern const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);
extern const char* Avc_video_full_range[];

//---------------------------------------------------------------------------
const char* Av1_obu_type(int8u obu_type)
{
    switch (obu_type)
    {
        case  0x1 : return "sequence_header";
        case  0x2 : return "temporal_delimiter";
        case  0x3 : return "frame_header";
        case  0x4 : return "tile_group";
        case  0x5 : return "metadata";
        case  0x6 : return "frame";
        case  0x7 : return "redundant_frame_header";
        case  0x8 : return "tile_list";
        case  0xF : return "padding";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
const char* Av1_seq_profile(int8u seq_profile)
{
    switch (seq_profile)
    {
        case  0x0 : return "Main";
        case  0x1 : return "High";
        case  0x2 : return "Professional";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
const char* Av1_metadata_type(int8u metadata_type)
{
    switch (metadata_type)
    {
    case  0x1: return "METADATA_TYPE_HDR_CLL";
    case  0x2: return "METADATA_TYPE_HDR_MDCV";
    case  0x3: return "METADATA_TYPE_SCALABILITY";
    case  0x4: return "METADATA_TYPE_ITUT_T35";
    case  0x5: return "METADATA_TYPE_TIMECODE";
    default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Av1_frame_type[4] =
{
    "Key",
    "Inter",
    "Intra Only",
    "Switch",
};

//---------------------------------------------------------------------------
static const char* Av1_chroma_sample_position[3] =
{
    "Type 0",
    "Type 2",
    "3",
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Av1::File_Av1()
{
    //Config
    #if MEDIAINFO_EVENTS
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;

    //In
    Frame_Count_Valid=0;
    FrameIsAlwaysComplete=false;

    //Temp
    sequence_header_Parsed=false;
    SeenFrameHeader=false;
}

//---------------------------------------------------------------------------
File_Av1::~File_Av1()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av1::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "AV1");

    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "AV1");

    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?8:(IsSub?1:2);
}

//---------------------------------------------------------------------------
void File_Av1::Streams_Fill()
{
}

//---------------------------------------------------------------------------
void File_Av1::Streams_Finish()
{
    Fill(Stream_Video, 0, Video_Format_Settings_GOP, GOP_Detect(GOP));

    //Merge info about different HDR formats
    auto HDR_Format = HDR.find(Video_HDR_Format);
    if (HDR_Format != HDR.end())
    {
        std::bitset<HdrFormat_Max> HDR_Present;
        size_t HDR_FirstFormatPos = (size_t)-1;
        for (size_t i = 0; i < HdrFormat_Max; ++i)
            if (!HDR_Format->second[i].empty())
            {
                if (HDR_FirstFormatPos == (size_t)-1)
                    HDR_FirstFormatPos = i;
                HDR_Present[i] = true;
            }
        bool LegacyStreamDisplay = MediaInfoLib::Config.LegacyStreamDisplay_Get();
        for (const auto& HDR_Item : HDR)
        {
            size_t i = HDR_FirstFormatPos;
            size_t HDR_FirstFieldNonEmpty = (size_t)-1;
            if (HDR_Item.first > Video_HDR_Format_Compatibility)
            {
                for (; i < HdrFormat_Max; ++i)
                {
                    if (!HDR_Present[i])
                        continue;
                    if (HDR_FirstFieldNonEmpty == (size_t)-1 && !HDR_Item.second[i].empty())
                        HDR_FirstFieldNonEmpty = i;
                    if (!HDR_Item.second[i].empty() && HDR_FirstFieldNonEmpty < HdrFormat_Max && HDR_Item.second[i] != HDR_Item.second[HDR_FirstFieldNonEmpty])
                        break;
                }
            }
            if (i == HdrFormat_Max && HDR_FirstFieldNonEmpty != (size_t)-1)
                Fill(Stream_Video, 0, HDR_Item.first, HDR_Item.second[HDR_FirstFieldNonEmpty]);
            else
            {
                ZtringList Value;
                Value.Separator_Set(0, __T(" / "));
                if (i != HdrFormat_Max)
                    for (i = HDR_FirstFormatPos; i < HdrFormat_Max; ++i)
                    {
                        if (!LegacyStreamDisplay && HDR_FirstFormatPos != HdrFormat_SmpteSt2086 && i >= HdrFormat_SmpteSt2086)
                            break;
                        if (!HDR_Present[i])
                            continue;
                        Value.push_back(HDR_Item.second[i]);
                    }
                auto Value_Flat = Value.Read();
                if (!Value.empty() && Value_Flat.size() > (Value.size() - 1) * 3)
                    Fill(Stream_Video, 0, HDR_Item.first, Value.Read());
            }
        }
    }

    if (!maximum_content_light_level.empty())
        Fill(Stream_Video, 0, Video_MaxCLL, maximum_content_light_level);
    if (!maximum_frame_average_light_level.empty())
        Fill(Stream_Video, 0, Video_MaxFALL, maximum_frame_average_light_level);
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av1::Read_Buffer_OutOfBand()
{
    //Parsing
    bool initial_presentation_delay_present;
    BS_Begin();
    Mark_1 ();
    Skip_S1(7,                                                  "version");
    Skip_S1(3,                                                  "seq_profile");
    Skip_S1(5,                                                  "seq_level_idx_0");
    Skip_SB(                                                    "seq_tier_0");
    Skip_SB(                                                    "high_bitdepth");
    Skip_SB(                                                    "twelve_bit");
    Skip_SB(                                                    "monochrome");
    Skip_SB(                                                    "chroma_subsampling_x");
    Skip_SB(                                                    "chroma_subsampling_y");
    Skip_S1(2,                                                  "chroma_sample_position");
    Skip_S1(3,                                                  "reserved");
    Get_SB (   initial_presentation_delay_present,              "initial_presentation_delay_present");
    Skip_S1(4,                                                  initial_presentation_delay_present?"initial_presentation_delay_minus_one":"reserved");
    BS_End();

    Open_Buffer_Continue(Buffer, Buffer_Size);
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av1::Header_Parse()
{
    //Parsing
    int8u obu_type;
    bool obu_extension_flag;
    BS_Begin();
    Mark_0 ();
    Get_S1 ( 4, obu_type,                                       "obu_type");
    Get_SB (    obu_extension_flag,                             "obu_extension_flag");
    Skip_SB(                                                    "obu_has_size_field");
    Skip_SB(                                                    "obu_reserved_1bit");
    if (obu_extension_flag)
    {
        Skip_S1(3,                                              "temporal_id");
        Skip_S1(2,                                              "spatial_id");
        Skip_S1(3,                                              "extension_header_reserved_3bits");
    }
    BS_End();

    int64u obu_size;
    Get_leb128 (obu_size,                                       "obu_size");

    FILLING_BEGIN();
    Header_Fill_Size(Element_Offset+obu_size);
    FILLING_END();

    if (FrameIsAlwaysComplete && (Element_IsWaitingForMoreData() || Element_Offset+obu_size>Element_Size))
    {
        // Trashing the remaining bytes, as the the frame is always complete so remaining bytes should not be prepending the next frame
        Buffer_Offset=Buffer_Size;
        Element_Offset=0;
        return;
    }

    FILLING_BEGIN();
        Header_Fill_Code(obu_type, Av1_obu_type(obu_type));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::Data_Parse()
{
    //Probing mode in case of raw stream //TODO: better reject of bad files
    if (!IsSub && !Status[IsAccepted] && (!Element_Code || Element_Code>6))
    {
        Reject();
        return;
    }

    //Parsing
    switch (Element_Code)
    {
        case  0x1 : sequence_header(); break;
        case  0x2 : temporal_delimiter(); break;
        case  0x3 : frame_header(); break;
        case  0x4 : tile_group(); break;
        case  0x5 : metadata(); break;
        case  0x6 : frame(); break;
        case  0xF : padding(); break;
        default   : Skip_XX(Element_Size-Element_Offset,        "Data");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Av1::sequence_header()
{
    //Parsing
    int32u max_frame_width_minus_1, max_frame_height_minus_1;
    int8u seq_profile, seq_level_idx[33]{}, operating_points_cnt_minus_1, buffer_delay_length_minus_1, frame_width_bits_minus_1, frame_height_bits_minus_1, seq_force_screen_content_tools, BitDepth, color_primaries, transfer_characteristics, matrix_coefficients, chroma_sample_position;
    bool reduced_still_picture_header, seq_tier[33], timing_info_present_flag, decoder_model_info_present_flag, seq_choose_screen_content_tools, mono_chrome, color_range, color_description_present_flag, subsampling_x, subsampling_y;
    BS_Begin();
    Get_S1 ( 3, seq_profile,                                    "seq_profile"); Param_Info1(Av1_seq_profile(seq_profile));
    Skip_SB(                                                    "still_picture");
    Get_SB (    reduced_still_picture_header,                   "reduced_still_picture_header");
    if (reduced_still_picture_header)
    {
        Get_S1 ( 5, seq_level_idx[0],                           "seq_level_idx[0]");
        decoder_model_info_present_flag=false;
        seq_tier[0]=false;
    }
    else
    {
        TEST_SB_GET(timing_info_present_flag,                   "timing_info_present_flag");
            bool equal_picture_interval;
            Skip_S4(32,                                         "num_units_in_tick");
            Skip_S4(32,                                         "time_scale");
            Get_SB (equal_picture_interval,                     "equal_picture_interval");
            if (equal_picture_interval)
                Skip_UE(                                        "num_ticks_per_picture_minus1");
            TEST_SB_GET (decoder_model_info_present_flag,       "decoder_model_info_present_flag");
                Get_S1 ( 5, buffer_delay_length_minus_1,        "buffer_delay_length_minus_1");
                Skip_S4(32,                                     "num_units_in_decoding_tick");
                Skip_S1( 5,                                     "buffer_removal_time_length_minus_1");
                Skip_S1( 5,                                     "frame_presentation_time_length_minus_1");
            TEST_SB_END();
        TEST_SB_END();
        Skip_SB(                                                "initial_display_delay_present_flag");
        Get_S1 ( 5, operating_points_cnt_minus_1,               "operating_points_cnt_minus_1");
        for (int8u i=0; i<=operating_points_cnt_minus_1; i++)
        {
            Element_Begin1("operating_point");
            Skip_S2(12,                                         "operating_point_idc[i]");
            Get_S1(5, seq_level_idx[i],                         "seq_level_idx[i]");
            if (seq_level_idx[i]>7)
                Get_SB(seq_tier[i],                             "seq_tier[i]");
            if (timing_info_present_flag && decoder_model_info_present_flag)
            {
                TEST_SB_SKIP(                                   "decoder_model_present_for_this_op[i]");
                    Skip_S5(buffer_delay_length_minus_1+1,      "decoder_buffer_delay[op]");
                    Skip_S5(buffer_delay_length_minus_1+1,      "encoder_buffer_delay[op]");
                    Skip_SB(                                    "low_delay_mode_flag[op]");
                TEST_SB_END();

            }
            Element_End0();
        }
    }
    Get_S1 ( 4, frame_width_bits_minus_1,                       "frame_width_bits_minus_1");
    Get_S1 ( 4, frame_height_bits_minus_1,                      "frame_height_bits_minus_1");
    Get_S4 (frame_width_bits_minus_1+1, max_frame_width_minus_1, "max_frame_width_minus_1");
    Get_S4 (frame_height_bits_minus_1+1, max_frame_height_minus_1, "max_frame_height_minus_1");
    if (!reduced_still_picture_header)
    {
        TEST_SB_SKIP(                                           "frame_id_numbers_present_flag");
            Skip_S1(4,                                          "delta_frame_id_length_minus2");
            Skip_S1(3,                                          "frame_id_length_minus1");
        TEST_SB_END();
    }
    Skip_SB(                                                    "use_128x128_superblock");
    Skip_SB(                                                    "enable_dual_filter");
    Skip_SB(                                                    "enable_intra_edge_filter");
    if (!reduced_still_picture_header)
    {
        bool enable_order_hint;
        Skip_SB(                                                "enable_interintra_compound");
        Skip_SB(                                                "enable_masked_compound");
        Skip_SB(                                                "enable_warped_motion");
        Skip_SB(                                                "enable_dual_filter");
        TEST_SB_GET (enable_order_hint,                         "enable_order_hint");
            Skip_SB(                                            "enable_jnt_comp");
            Skip_SB(                                            "enable_ref_frame_mvs");
        TEST_SB_END();
        Get_SB (seq_choose_screen_content_tools,                "seq_choose_screen_content_tools");
        if (seq_choose_screen_content_tools)
            seq_force_screen_content_tools=2;
        else
            Get_S1 (1, seq_force_screen_content_tools,          "seq_force_screen_content_tools");
        if (seq_force_screen_content_tools)
        {
            bool seq_choose_integer_mv;
            Get_SB(seq_choose_integer_mv,                       "seq_choose_integer_mv");
            if (!seq_choose_integer_mv)
                Skip_S1(1,                                      "seq_force_integer_mv");
        }
        if (enable_order_hint)
            Skip_S1(3,                                          "order_hint_bits_minus_1");
    }
    Skip_SB(                                                    "enable_superres");
    Skip_SB(                                                    "enable_cdef");
    Skip_SB(                                                    "enable_restoration");
    Element_Begin1("color_config");
        bool high_bitdepth;
        Get_SB (high_bitdepth,                                  "high_bitdepth");
        BitDepth=high_bitdepth?10:8;
        if (seq_profile>=2 && high_bitdepth)
        {
            bool twelve_bit;
            Get_SB (twelve_bit,                                 "twelve_bit");
            if (twelve_bit)
                BitDepth+=2;
        }
        if (seq_profile==1)
            mono_chrome=false;
        else
            Get_SB (mono_chrome,                                "mono_chrome");
        TESTELSE_SB_GET (color_description_present_flag,        "color_description_present_flag");
            Get_S1 (8, color_primaries,                         "color_primaries"); Param_Info1(Mpegv_colour_primaries(color_primaries));
            Get_S1 (8, transfer_characteristics,                "transfer_characteristics"); Param_Info1(Mpegv_transfer_characteristics(transfer_characteristics));
            Get_S1 (8, matrix_coefficients,                     "matrix_coefficients"); Param_Info1(Mpegv_matrix_coefficients(matrix_coefficients));
        TESTELSE_SB_ELSE(                                       "color_description_present_flag");
            color_primaries=2;
            transfer_characteristics=2;
            matrix_coefficients=2;
        TESTELSE_SB_END();
        if (mono_chrome)
        {
            color_range=true;
            subsampling_x=true;
            subsampling_y=true;
        }
        else if (color_primaries==1 && transfer_characteristics==13 && matrix_coefficients==0)
        {
            subsampling_x=false;
            subsampling_y=false;
        }
        else
        {
            Get_SB(color_range,                                 "color_range"); Param_Info1(Avc_video_full_range[color_range]);
            if (seq_profile==0)
            {
                subsampling_x=true;
                subsampling_y=true;
            }
            else if (seq_profile==1)
            {
                subsampling_x=false;
                subsampling_y=false;
            }
            else
            {
                if ( BitDepth == 12 )
                {
                    Get_SB(subsampling_x,                       "subsampling_x");
                    if (subsampling_x)
                        Get_SB(subsampling_y,                   "subsampling_y");
                    else
                        subsampling_y=false;
                }
                else
                {
                    subsampling_x=true;
                    subsampling_y=false;
                }
            } 
            if (subsampling_x && subsampling_y)
                Get_S1 ( 2, chroma_sample_position,             "chroma_sample_position");
        }
        Skip_SB(                                                "separate_uv_delta_q");
    Element_End0();
    Skip_SB(                                                    "film_grain_params_present");
    Mark_1();
    if (Data_BS_Remain()<8)
        while (Data_BS_Remain())
            Mark_0();
    BS_End();

    FILLING_BEGIN_PRECISE();
        if (!sequence_header_Parsed)
        {
            if (IsSub)
                Accept();
            Fill(Stream_Video, 0, Video_Format_Profile, Ztring().From_UTF8(Av1_seq_profile(seq_profile))+(seq_level_idx[0]==31?Ztring():(__T("@L")+Ztring().From_Number(2+(seq_level_idx[0]>>2))+__T(".")+Ztring().From_Number(seq_level_idx[0]&3))));
            Fill(Stream_Video, 0, Video_Width, max_frame_width_minus_1+1);
            Fill(Stream_Video, 0, Video_Height, max_frame_height_minus_1+1);
            Fill(Stream_Video, 0, Video_BitDepth, BitDepth);
            Fill(Stream_Video, 0, Video_ColorSpace, mono_chrome?"Y":((color_primaries==1 && transfer_characteristics==13 && matrix_coefficients==0)?"RGB":"YUV"));
            if (Retrieve(Stream_Video, 0, Video_ColorSpace)==__T("YUV"))
            {
                Fill(Stream_Video, 0, Video_ChromaSubsampling, subsampling_x?(subsampling_y?"4:2:0":"4:2:2"):"4:4:4"); // "!subsampling_x && subsampling_y" (4:4:0) not possible
                if (subsampling_x && subsampling_y && chroma_sample_position)
                    Fill(Stream_Video, 0, Video_ChromaSubsampling_Position, Av1_chroma_sample_position[chroma_sample_position-1]);
            }
            if (color_description_present_flag)
            {
                Fill(Stream_Video, 0, Video_colour_description_present, "Yes");
                Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(color_primaries));
                Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(transfer_characteristics));
                Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(matrix_coefficients));
            }
            if (mono_chrome ||  !(color_primaries==1 && transfer_characteristics==13 && matrix_coefficients==0))
                Fill(Stream_Video, 0, Video_colour_range, Avc_video_full_range[color_range]);

            sequence_header_Parsed=true;
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::temporal_delimiter()
{
    SeenFrameHeader=false;

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::frame_header()
{
    /* bitstream conformance requires SeenFrameHeader is only 1 when OBU type is OBU_REDUNDANT_FRAME_HEADER
     * since tile_group is not parsed, SeenFrameHeader cannot be relied upon as it is supposed to be reset by tile_group
    if (SeenFrameHeader)
    {
        Skip_XX(Element_Size,                                   "Duplicated data");
        return;
    }
    */
    SeenFrameHeader=1;

    if (!sequence_header_Parsed)
    {
        Skip_XX(Element_Size,                                   "Data");
        return;
    }

    //Parsing
    BS_Begin();
    Element_Begin1("uncompressed_header");
        int8u frame_type;
        TEST_SB_SKIP(                                           "show_existing_frame");
            BS_End();
            SeenFrameHeader = 0;
            Skip_XX(Element_Size-Element_Offset,                "Data");
            return;
        TEST_SB_END();
        Get_S1 (2, frame_type,                                  "frame_type"); Param_Info1(Av1_frame_type[frame_type]);

        FILLING_BEGIN();
            GOP.push_back((frame_type&1)?'P':'I');
        FILLING_ELSE();
            GOP.push_back(' ');
        FILLING_END();
        if (GOP.size()>=512)
            GOP.resize(384);
    Element_End0();
    BS_End();

    FILLING_BEGIN();
        if (!Status[IsAccepted])
            Accept();
        Frame_Count++;
        if (Frame_Count>=Frame_Count_Valid)
            Finish();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::tile_group()
{
    Skip_XX(Element_Size - Element_Offset,                      "Data");
}

//---------------------------------------------------------------------------
void File_Av1::metadata()
{
    //Parsing
    int64u metadata_type;
    Get_leb128 (metadata_type,                                  "metadata_type");
    Param_Info1(Av1_metadata_type(metadata_type));

    switch (metadata_type)
    {
        case    1 : metadata_hdr_cll(); break;
        case    2 : metadata_hdr_mdcv(); break;
        case    4 : metadata_itu_t_t35(); break;
        default   : Skip_XX(Element_Size-Element_Offset,        "Data");
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_hdr_cll()
{
    //Parsing
    Get_LightLevel(maximum_content_light_level, maximum_frame_average_light_level);
}

//---------------------------------------------------------------------------
void File_Av1::metadata_hdr_mdcv()
{
    //Parsing
    auto& HDR_Format = HDR[Video_HDR_Format][HdrFormat_SmpteSt2086];
    if (HDR_Format.empty())
    {
        HDR_Format = __T("SMPTE ST 2086");
        HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt2086] = "HDR10";
    }
    Get_MasteringDisplayColorVolume(HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_SmpteSt2086], HDR[Video_MasteringDisplay_Luminance][HdrFormat_SmpteSt2086], true);
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35()
{
    //Parsing
    int8u itu_t_t35_country_code;
    Get_B1(itu_t_t35_country_code,                              "itu_t_t35_country_code");
    if (itu_t_t35_country_code == 0xFF)
        Skip_B1(                                                "itu_t_t35_country_code_extension_byte");

    switch (itu_t_t35_country_code)
    {
    case 0xB5: Param_Info1("United States"); metadata_itu_t_t35_B5(); break;
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5()
{
    int16u itu_t_t35_terminal_provider_code;
    Get_B2(itu_t_t35_terminal_provider_code,                    "itu_t_t35_terminal_provider_code");

    switch (itu_t_t35_terminal_provider_code)
    {
    case 0x003B: Param_Info1("Dolby Laboratories, Inc."); metadata_itu_t_t35_B5_003B(); break;
    case 0x003C: Param_Info1("Samsung Electronics America"); metadata_itu_t_t35_B5_003C(); break;
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5_003B()
{
    int32u itu_t_t35_terminal_provider_oriented_code;
    Get_B4(itu_t_t35_terminal_provider_oriented_code,           "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
    case 0x00000800: metadata_itu_t_t35_B5_003B_00000800(); break;
    }

}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5_003B_00000800()
{
    Element_Info1("Extensible Metadata Delivery Format (EMDF)");

    auto Get_V4{ [this](int8u Bits, int32u& Info, const char* Name)
            {
                Info = 0;

            #if MEDIAINFO_TRACE
                int8u Count = 0;
            #endif //MEDIAINFO_TRACE
                for (;;)
                {
                    Info += BS->Get4(Bits);
            #if MEDIAINFO_TRACE
                    Count += Bits;
            #endif //MEDIAINFO_TRACE
                    if (!BS->GetB())
                        break;
                    Info <<= Bits;
                    Info += (1 << Bits);
                }
            #if MEDIAINFO_TRACE
                if (Trace_Activated)
                {
                    Param(Name, Info, Count);
                    Param_Info(__T("(") + Ztring::ToZtring(Count) + __T(" bits)"));
                }
            #endif //MEDIAINFO_TRACE
            }
        };

    auto Skip_V4{ [&](int8u Bits, const char* Name) {
                int32u Info;
                Get_V4(Bits, Info, Name);
            }
        };

    BS_Begin();
    size_t Start = Data_BS_Remain();
    int32u version, key_id;
    Element_Begin1("emdf_container");
    Get_S4 (2, version,                                         "emdf_version");
    if (version == 3)
    {
        int32u add;
        Get_V4(2, add,                                          "emdf_version addition");
        version += add;
    }
    if (version)
    {
        Skip_BS(Data_BS_Remain(),                               "(Unparsed emdf_container data)");
        Element_End0(); 
        return;
    }

    Get_S4 (3, key_id,                                          "key_id");
    if (key_id == 7)
    {
        int32u add;
        Get_V4 (3, add,                                         "key_id addition");
        key_id += add;
    }
    Param_Info1C(key_id == 0x6, "Ignore protection bits");

    int32u emdf_payload_id = 0;
        
    for(;;)
    {
        Element_Begin1("emdf_payload");
        Get_S4 (5, emdf_payload_id,                             "emdf_payload_id");
        if (emdf_payload_id==0x1F)
        {
            int32u add;
            Get_V4 (5, add,                                     "emdf_payload_id addition");
            emdf_payload_id += add;
        }

        if (emdf_payload_id == 256)
            Element_Info1("Dolby Vision Reference Processing Unit (RPU)");
        if (emdf_payload_id == 0x00)
        {
            Element_End0();
            break;
        }

        Element_Begin1("emdf_payload_config");

        bool smploffste = false;
        Get_SB (smploffste,                                     "smploffste");
        if (smploffste)
        {
            Skip_S2(11,                                         "smploffst");
            Skip_SB(                                            "reserved");
        }

        TEST_SB_SKIP(                                           "duratione");
            Skip_V4(11,                                         "duration");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "groupide");
            Skip_V4(2,                                          "groupid");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "codecdatae");
            Skip_S1(8,                                          "reserved");
        TEST_SB_END();

        bool discard_unknown_payload = false;
        Get_SB(discard_unknown_payload,                         "discard_unknown_payload");
        if (!discard_unknown_payload)
        {
            bool payload_frame_aligned = false;
            if (!smploffste)
            {
                Get_SB (payload_frame_aligned,                  "payload_frame_aligned");
                if (payload_frame_aligned)
                {
                    Skip_SB(                                    "create_duplicate");
                    Skip_SB(                                    "remove_duplicate");
                }
            }

            if (smploffste || payload_frame_aligned)
            {
                Skip_S1(5,                                      "priority");
                Skip_S1(2,                                      "proc_allowed");
            }
        }

        Element_End0(); // emdf_payload_config

        int32u emdf_payload_size = 0;
        Get_V4 (8, emdf_payload_size,                           "emdf_payload_size");
        size_t emdf_payload_End=Data_BS_Remain()-emdf_payload_size*8;

        Element_Begin1("emdf_payload_bytes");
            switch (emdf_payload_id)
            {
                case 256: Dolby_Vision_reference_processing_unit(); break;
                default : Skip_BS(emdf_payload_size * 8, "(Unknown)"); break;
            }
            size_t RemainginBits=Data_BS_Remain();
            if (RemainginBits>=emdf_payload_End)
            {
                if (RemainginBits>emdf_payload_End)
                    Skip_BS(RemainginBits-emdf_payload_End,     "(Unparsed bits)");
            }
            else
            {
                //There is a problem, too many bits were consumed by the parser. //TODO: prevent the parser to consume more bits than count of bits in this element
                if (Data_BS_Remain())
                    Skip_BS(Data_BS_Remain(),                   "(Problem during emdf_payload parsing)");
                else
                    Skip_BS(Data_BS_Remain(),                   "(Problem during emdf_payload parsing, going to end directly)");
                Element_End0();
                Element_End0();
                break;
            }
        Element_End0(); // emdf_payload_bytes

        Element_End0(); // emdf_payload
    }

    Element_Begin1("emdf_protection");

    int8u len_primary = 0, len_second = 0;
    Get_S1(2, len_primary,                                      "protection_length_primary");
    Get_S1(2, len_second,                                       "protection_length_secondary");

    switch (len_primary)
    {
        //case 0: break; //protection_length_primary coherency was already tested in sync layer
        case 1: len_primary = 8; break;
        case 2: len_primary = 32; break;
        case 3: len_primary = 128; break;
        default:; //Cannot append, read only 2 bits
    };
    switch (len_second)
    {
        case 0: len_second = 0; break;
        case 1: len_second = 8; break;
        case 2: len_second = 32; break;
        case 3: len_second = 128; break;
        default:; //Cannot append, read only 2 bits
    };
    Skip_BS(len_primary,                                        "protection_bits_primary");
    if (len_second)
        Skip_BS(len_primary,                                    "protection_bits_secondary");

    Element_End0(); // emdf_protection

    Element_End0(); // emdf_container
    BS_End();
}

//---------------------------------------------------------------------------
void File_Av1::Dolby_Vision_reference_processing_unit()
{
    using std::to_string;

    #define UNSUPPORTED() \
        BS_End(); \
        Skip_XX(Element_Size - Element_Offset, "Data"); \
        Trusted_IsNot("Unsupported"); \
        return;

    auto DV_content_type{ [](int8u content_type) -> const char* {
                switch (content_type) {
                case 0: return "Default";
                case 1: return "Movies";
                case 2: return "Game";
                case 3: return "Sport";
                case 4: return "User Generated Content";
                default: return "";
                }
            }
        };

    auto DV_white_point{ [](int8u white_point) -> const char* {
                switch (white_point) {
                case 0: return "D65";
                case 8: return "D93";
                default: return "";
                }
            }
        };

    auto DV_intended_setting{ [](int8u setting, bool off) -> const char* {
                switch (setting) {
                case 0: return "Default";
                case 1: return off ? "Off" : "Low";
                case 2: return "Medium";
                case 3: return "High";
                default: return "";
                }
            }
        };

    //Parsing
    Element_Begin1("rpu_data");
    auto RemainingBitsBegin = Data_BS_Remain();

    // EDR RPU header
    Element_Begin1("rpu_data_header");
    int8u rpu_type;
    int16u rpu_format;
    Get_S1 ( 6, rpu_type,                                       "rpu_type");
    Get_S2 (11, rpu_format,                                     "rpu_format");
    if (rpu_type != 2 || (rpu_format & 0x700) != 0) {
        UNSUPPORTED();
    }
    // if (rpu_type == 2) {
    // EDR RPU frame header
    int32u coefficient_log2_denom, bl_bit_depth_minus8, el_bit_depth_minus8;
    int8u dm_compression, coefficient_data_type;
    bool disable_residual_flag;
    Skip_S1( 4,                                                 "vdr_rpu_profile");
    Skip_S1( 4,                                                 "vdr_rpu_level");
    TESTELSE_SB_SKIP(                                           "vdr_seq_info_present_flag");
        // EDR RPU sequence header
        Skip_SB(                                                "chroma_resampling_explicit_filter_flag");
        Get_S1 (2, coefficient_data_type,                       "coefficient_data_type");
        switch (coefficient_data_type) {
        case 0: //COEFF_FIXED
            Get_UE(coefficient_log2_denom,                      "coefficient_log2_denom");
            break;
        case 1: //COEFF_FLOAT
            break;
        }
        Skip_S1(2,                                              "vdr_rpu_normalized_idc");
        Skip_SB(                                                "BL_video_full_range_flag");
        if ((rpu_format & 0x700) == 0) {
            // sequence header
            Get_UE (bl_bit_depth_minus8,                        "BL_bit_depth_minus8");
            Get_UE (el_bit_depth_minus8,                        "EL_bit_depth_minus8");
            Skip_UE(                                            "vdr_bit_depth_minus8");
            Skip_SB(                                            "spatial_resampling_filter_flag");
            Get_S1 (3, dm_compression,                          "dm_compression");
            Skip_SB(                                            "el_spatial_resampling_filter_flag");
            Get_SB (disable_residual_flag,                      "disable_residual_flag");
        }
    TESTELSE_SB_ELSE("vdr_seq_info_present");
        UNSUPPORTED();
    TESTELSE_SB_END();
    bool use_prev_vdr_rpu_flag, vdr_dm_metadata_present_flag;
    Get_SB (vdr_dm_metadata_present_flag,                       "vdr_dm_metadata_present_flag");
    Get_SB (use_prev_vdr_rpu_flag,                              "use_prev_vdr_rpu_flag");
    if (use_prev_vdr_rpu_flag) {
        Skip_UE(                                                "prev_vdr_rpu_id");
    }
    else {
        int32u num_pivots_minus_2[3]{};
        Skip_UE(                                                "vdr_rpu_id");
        Skip_UE(                                                "mapping_color_space");
        Skip_UE(                                                "mapping_chroma_format_idc");
        // pivot points for BL three components
        for (int8u cmp = 0; cmp < 3; ++cmp) {
            Get_UE(num_pivots_minus_2[cmp],                     ("num_pivots_minus_2[" + to_string(cmp) + "]").c_str());
            for (int32u pivot_idx = 0; pivot_idx < num_pivots_minus_2[cmp] + 2; ++pivot_idx) {
                Skip_BS(static_cast<size_t>(bl_bit_depth_minus8) + 8, ("pred_pivot_value[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
            }
        }
        auto use_nlq{ (rpu_format & 0x700) == 0 && !disable_residual_flag };
        int8u nlq_method_idc;
        if (use_nlq) {
            // vl.x architecture EL specific
            Get_S1(3, nlq_method_idc,                           "nlq_method_idc");
            for (int8u i = 0; i < 2; ++i) {
                Skip_BS(static_cast<size_t>(bl_bit_depth_minus8) + 8, "nlq_pred_pivot_value");
            }
        }
        Skip_UE(                                                "num_x_partitions_minus1");
        Skip_UE(                                                "num_y_partitions_minus1");
        // }
        Element_End0(); // rpu_data_header / EDR RPU header

        // if (rpu_type == 2) {
        // EDR RPU data
        Element_Begin1("vdr_rpu_data_payload");
        Element_Begin1("rpu_data_mapping");
        for (int8u cmp = 0; cmp < 3; ++cmp) {
            for (int32u pivot_idx = 0; pivot_idx < num_pivots_minus_2[cmp] + 1; ++pivot_idx) {
                Element_Begin1("rpu_data_mapping_param");
                int32u mapping_idc;
                Get_UE(mapping_idc,                             ("mapping_idc[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                switch (mapping_idc) {
                case 0: //MAPPING_POLYNOMIAL
                {
                    // Polynomial coefficients
                    int32u poly_order_minus1;
                    bool linear_interp_flag{};
                    Get_UE(poly_order_minus1,                   ("poly_order_minus1[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    if (poly_order_minus1 == 0)
                        Get_SB(linear_interp_flag,              ("linear_interp_flag[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    if (poly_order_minus1 == 0 && linear_interp_flag) {
                        // Linear interpolation
                        if (coefficient_data_type == 0) {
                            Skip_UE(                            ("pred_linear_interp_value_int[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                            Skip_BS(coefficient_log2_denom,     ("pred_linear_interp_value[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                        }
                        else
                            Skip_S4(32,                         ("pred_linear_interp_value[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                        if (pivot_idx == num_pivots_minus_2[cmp]) {
                            if (coefficient_data_type == 0) {
                                Skip_UE(                        ("pred_linear_interp_value_int[" + to_string(cmp) + "][" + to_string(pivot_idx + 1) + "]").c_str());
                                Skip_BS(coefficient_log2_denom, ("pred_linear_interp_value[" + to_string(cmp) + "][" + to_string(pivot_idx + 1) + "]").c_str());
                            }
                            else
                                Skip_S4(32,                     ("pred_linear_interp_value[" + to_string(cmp) + "][" + to_string(pivot_idx + 1) + "]").c_str());
                        }
                    }
                    else {
                        // Non-linear
                        // the i-th order
                        for (int32u i = 0; i <= poly_order_minus1 + 1; ++i) {
                            if (coefficient_data_type == 0) {
                                Skip_SE(                        ("poly_coef_int[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "]").c_str());
                                Skip_BS(coefficient_log2_denom, ("poly_coef[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "]").c_str());
                            }
                            else
                                Skip_S4(32,                     ("poly_coef[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "]").c_str());
                        }
                    }
                    break;
                }
                case 1: //MAPPING_MMR
                {
                    // MR coefficients
                    int8u mmr_order_minus1;
                    Get_S1(2, mmr_order_minus1,                 ("mmr_order_minus1[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    if (coefficient_data_type == 0) {
                        Skip_SE(                                ("mmr_constant_int[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                        Skip_BS(coefficient_log2_denom,         ("mmr_constant[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    }
                    else
                        Skip_S4(32,                             ("mmr_constant[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    // the i-th order
                    for (int8u i = 1; i <= mmr_order_minus1 + 1; ++i) {
                        // the j-th coefficients
                        for (int8u j = 0; j < 7; ++j) {
                            if (coefficient_data_type == 0) {
                                Skip_SE(                        ("mmr_coef_int[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "][" + to_string(j) + "]").c_str());
                                Skip_BS(coefficient_log2_denom, ("mmr_coef[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "][" + to_string(j) + "]").c_str());
                            }
                            else
                                Skip_S4(32,                     ("mmr_coef[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "][" + to_string(j) + "]").c_str());
                        }
                    }
                    break;
                }
                }
                Element_End0(); // rpu_data_mapping_param
            }
        }
        Element_End0(); // rpu_data_mapping
        if (use_nlq) {
            Element_Begin1("rpu_data_nlq");
            bool isMEL{ true };
            // nlq_num_pivots_minus2 == 0 so no pivot_idx loop
            for (int8u cmp = 0; cmp < 3; ++cmp) {
                // Nonlinear Quantization Parameters
                Element_Begin1("rpu_data_nlq_param");
                int16u nlq_offset;
                int32u vdr_in_max_int, vdr_in_max, linear_deadzone_slope_int, linear_deadzone_slope, linear_deadzone_threshold_int, linear_deadzone_threshold;
                Get_S2(static_cast<size_t>(el_bit_depth_minus8) + 8, nlq_offset, ("nlq_offset[0][" + to_string(cmp) + "]").c_str());
                if (coefficient_data_type == 0) {
                    Get_UE(vdr_in_max_int,                      ("vdr_in_max_int[0][" + to_string(cmp) + "]").c_str());
                    Get_S4(coefficient_log2_denom, vdr_in_max,  ("vdr_in_max[0][" + to_string(cmp) + "]").c_str());
                }
                else
                    Get_S4(32, vdr_in_max,                      ("vdr_in_max[0][" + to_string(cmp) + "]").c_str());
                switch (nlq_method_idc) {
                case 0: //NLQ_LINEAR_DZ
                    //  Linear dead zone coefficients
                    if (coefficient_data_type == 0) {
                        Get_UE(linear_deadzone_slope_int,       ("linear_deadzone_slope_int[0][" + to_string(cmp) + "]").c_str());
                        Get_S4(coefficient_log2_denom, linear_deadzone_slope, ("linear_deadzone_slope[0][" + to_string(cmp) + "]").c_str());
                    }
                    else
                        Get_S4(32, linear_deadzone_slope,       ("linear_deadzone_slope[0][" + to_string(cmp) + "]").c_str());
                    if (coefficient_data_type == 0) {
                        Get_UE(linear_deadzone_threshold_int,   ("linear_deadzone_threshold_int[0][" + to_string(cmp) + "]").c_str());
                        Get_S4(coefficient_log2_denom, linear_deadzone_threshold, ("linear_deadzone_threshold[0][" + to_string(cmp) + "]").c_str());
                    }
                    else
                        Get_S4(32, vdr_in_max,                  ("linear_deadzone_threshold[0][" + to_string(cmp) + "]").c_str());
                    break;
                }
                if ((nlq_offset | vdr_in_max | linear_deadzone_slope_int | linear_deadzone_slope | linear_deadzone_threshold_int | linear_deadzone_threshold) != 0 || vdr_in_max_int != 1)
                    isMEL = false;
                Element_End0(); // rpu_data_nlq_param
            }

            // TEMPORARY ==========================================================================
            if (isMEL)
                Param2("EL Type", "Minimum Enhancement Layer (MEL)");
            else
                Param2("EL Type", "Full Enhancement Layer (FEL / non-MEL)");
            // end TEMPORARY ======================================================================

            Element_End0(); // rpu_data_nlq
        }
        
    }
    // }
    Element_End0(); // vdr_rpu_data_payload

    // Display Management
    if (vdr_dm_metadata_present_flag) {
        Element_Begin1("vdr_dm_data_payload");
        Skip_UE(                                                "affected_dm_metadata_id");
        Skip_UE(                                                "current_dm_metadata_id");
        Skip_UE(                                                "scene_refresh_flag");
        if (dm_compression) {
            UNSUPPORTED();
        }
        for (int8u i = 0; i < 9; ++i) {
            Skip_S2(16,                                         ("YCCtoRGB_coef" + to_string(i)).c_str());
        }
        for (int8u i = 0; i < 3; ++i) {
            Skip_S4(32,                                         ("YCCtoRGB_offset" + to_string(i)).c_str());
        }
        for (int8u i = 0; i < 9; ++i) {
            Skip_S2(16,                                         ("RGBtoLMS_coef" + to_string(i)).c_str());
        }
        Skip_S2(16,                                             "signal_eotf");
        Skip_S2(16,                                             "signal_eotf_param0");
        Skip_S2(16,                                             "signal_eotf_param1");
        Skip_S4(32,                                             "signal_eotf_param2");
        Skip_S1( 5,                                             "signal_bit_depth");
        Skip_S1( 2,                                             "signal_color_space");
        Skip_S1( 2,                                             "signal_chroma_format");
        Skip_S1( 2,                                             "signal_full_range_flag");
        Skip_S2(12,                                             "source_min_PQ");
        Skip_S2(12,                                             "source_max_PQ");
        Skip_S2(10,                                             "source_diagonal");

        // Extension blocks
        // ----------------------------------------------------------------------------------------
        
        // Content Mapping v2.9 (CMv2.9)
        int32u num_ext_blocks;
        Get_UE(num_ext_blocks,                                  "num_ext_blocks");
        if (num_ext_blocks) {
            auto RemainingBitsEnd = Data_BS_Remain();
            auto size = RemainingBitsBegin - RemainingBitsEnd;
            Skip_BS((8 - (size & 7)) & 7,                       "dm_alignment_zero_bit");
            for (int32u i = 0; i < num_ext_blocks; ++i) {
                Element_Begin1("ext_metadata_block");
                int32u ext_block_length;
                int8u ext_block_level;
                Get_UE(ext_block_length,                        "ext_block_length");
                Get_S1(8, ext_block_level,                      "ext_block_level");
                Element_Begin1("ext_block_payload");
                size_t ext_block_len_bits{ static_cast<size_t>(ext_block_length) * 8 };
                auto ext_block_use_bits{ 0 };
                switch (ext_block_level) {
                case 1: // ANALYSIS METADATA (DYNAMIC)
                    Skip_S2(12,                                 "min_PQ");
                    Skip_S2(12,                                 "max_PQ");
                    Skip_S2(12,                                 "avg_PQ");
                    ext_block_use_bits += 36;
                    break;
                case 2: // PER-TARGET TRIM METADATA (DYNAMIC)
                    Skip_S2(12,                                 "target_max_PQ");
                    Skip_S2(12,                                 "trim_slope");
                    Skip_S2(12,                                 "trim_offset");
                    Skip_S2(12,                                 "trim_power");
                    Skip_S2(12,                                 "trim_chroma_weight");
                    Skip_S2(12,                                 "trim_saturation_gain");
                    Skip_S2(13,                                 "ms_weight");
                    ext_block_use_bits += 85;
                    break;
                case 4:
                    Skip_S2(12,                                 "anchor_pq");
                    Skip_S2(12,                                 "anchor_power");
                    ext_block_use_bits += 24;
                    break;
                case 5: // PER-SHOT ASPECT RATIO (DYNAMIC)
                    Skip_S2(13,                                 "active_area_left_offset");
                    Skip_S2(13,                                 "active_area_right_offset");
                    Skip_S2(13,                                 "active_area_top_offset");
                    Skip_S2(13,                                 "active_area_bottom_offset");
                    ext_block_use_bits += 52;
                    break;
                case 6: // OPTIONAL HDR10 METADATA (STATIC)
                    Skip_S2(16,                                 "max_display_mastering_luminance");
                    Skip_S2(16,                                 "min_display_mastering_luminance");
                    Skip_S2(16,                                 "max_content_light_level");
                    Skip_S2(16,                                 "max_frame_average_light_level");
                    ext_block_use_bits += 64;
                    break;
                case 255:
                    Skip_S1(8,                                  "dm_run_mode");
                    Skip_S1(8,                                  "dm_run_version");
                    Skip_S1(8,                                  "dm_debug0");
                    Skip_S1(8,                                  "dm_debug1");
                    Skip_S1(8,                                  "dm_debug2");
                    Skip_S1(8,                                  "dm_debug3");
                    ext_block_use_bits += 48;
                    break;
                default:
                    Skip_BS(ext_block_len_bits - ext_block_use_bits, "(Not parsed)");
                    break;
                }
                Skip_BS(ext_block_len_bits - ext_block_use_bits, "ext_dm_alignment_zero_bit");
                Element_End0(); // ext_block_payload
                Element_End0(); // ext_metadata_block
            }
        }

        // Content Mapping v4.0 (CMv4.0)
        if (Data_BS_Remain() >= 56) {
            int32u num_ext_blocks2;
            Get_UE(num_ext_blocks2,                             "num_ext_blocks");
            if (num_ext_blocks2) {
                auto RemainingBitsEnd = Data_BS_Remain();
                auto size = RemainingBitsBegin - RemainingBitsEnd;
                Skip_BS((8 - (size & 7)) & 7,                   "dm_alignment_zero_bit");
                for (int32u i = 0; i < num_ext_blocks2; ++i) {
                    Element_Begin1("ext_metadata_block");
                    int32u ext_block_length;
                    int8u ext_block_level;
                    Get_UE(ext_block_length,                    "ext_block_length");
                    Get_S1(8, ext_block_level,                  "ext_block_level");
                    Element_Begin1("ext_block_payload");
                    size_t ext_block_len_bits{ static_cast<size_t>(ext_block_length) * 8 };
                    auto ext_block_use_bits{ 0 };
                    switch (ext_block_level) {
                    case 3: // OFFSETS TO L1 (DYNAMIC)
                        Skip_S2(12,                             "min_pq_offset");
                        Skip_S2(12,                             "max_pq_offset");
                        Skip_S2(12,                             "avg_pq_offset");
                        ext_block_use_bits += 36;
                        break;
                    case 8: // PER-TARGET TRIM METADATA (DYNAMIC)
                        Skip_S1( 8,                             "target_display_index");
                        Skip_S2(12,                             "trim_slope");
                        Skip_S2(12,                             "trim_offset");
                        Skip_S2(12,                             "trim_power");
                        Skip_S2(12,                             "trim_chroma_weight");
                        Skip_S2(12,                             "trim_saturation_gain");
                        Skip_S2(12,                             "ms_weight");
                        ext_block_use_bits += 80;
                        if (ext_block_length > 10) {
                            Skip_S2(12,                         "target_mid_contrast");
                            ext_block_use_bits += 12;
                        }
                        if (ext_block_length > 12) {
                            Skip_S2(12,                         "clip_trim");
                            ext_block_use_bits += 12;
                        }
                        if (ext_block_length > 13) {
                            for (int8u i = 0; i < 6; ++i) {
                                Skip_S1(8,                      ("saturation_vector_field" + to_string(i)).c_str());
                            }
                            ext_block_use_bits += 48;
                        }
                        if (ext_block_length > 19) {
                            for (int8u i = 0; i < 6; ++i) {
                                Skip_S1(8,                      ("hue_vector_field" + to_string(i)).c_str());
                            }
                            ext_block_use_bits += 48;
                        }
                        break;
                    case 9: // PER-SHOT SOURCE CONTENT PRIMARIES (DYNAMIC)
                        Skip_S1(8,                              "source_primary_index");
                        ext_block_use_bits += 8;
                        if (ext_block_length > 1) {
                            Skip_S2(16,                         "source_primary_red_x");
                            Skip_S2(16,                         "source_primary_red_y");
                            Skip_S2(16,                         "source_primary_green_x");
                            Skip_S2(16,                         "source_primary_green_y");
                            Skip_S2(16,                         "source_primary_blue_x");
                            Skip_S2(16,                         "source_primary_blue_y");
                            Skip_S2(16,                         "source_primary_white_x");
                            Skip_S2(16,                         "source_primary_white_y");
                            ext_block_use_bits += 128;
                        }
                        break;
                    case 10:
                        Skip_S1( 8,                             "target_display_index");
                        Skip_S2(12,                             "target_max_pq");
                        Skip_S2(12,                             "target_min_pq");
                        Skip_S1( 8,                             "target_primary_index");
                        ext_block_use_bits += 40;
                        if (ext_block_length > 5) {
                            Skip_S2(16,                         "target_primary_red_x");
                            Skip_S2(16,                         "target_primary_red_y");
                            Skip_S2(16,                         "target_primary_green_x");
                            Skip_S2(16,                         "target_primary_green_y");
                            Skip_S2(16,                         "target_primary_blue_x");
                            Skip_S2(16,                         "target_primary_blue_y");
                            Skip_S2(16,                         "target_primary_white_x");
                            Skip_S2(16,                         "target_primary_white_y");
                            ext_block_use_bits += 128;
                        }
                        break;
                    case 11:
                    {   // Automatic Picture/Playback Optimization (APO) / Dolby Vision IQ - Content Type Metadata (L11)
                        int8u content_type, white_point, sharpness, noise_reduction, mpeg_noise_reduction, frame_rate_conversion, brightness, color;
                        Get_S1 (8, content_type,                "content_type");            Param_Info1(DV_content_type(content_type));
                        Get_S1 (4, white_point,                 "white_point");             Param_Info1(DV_white_point(white_point));
                        Skip_SB(                                "reference_mode_flag");
                        Skip_S1(3,                              "reserved");
                        Get_S1 (2, sharpness,                   "sharpness");               Param_Info1(DV_intended_setting(sharpness, true));
                        Get_S1 (2, noise_reduction,             "noise_reduction");         Param_Info1(DV_intended_setting(noise_reduction, true));
                        Get_S1 (2, mpeg_noise_reduction,        "mpeg_noise_reduction");    Param_Info1(DV_intended_setting(mpeg_noise_reduction, true));
                        Get_S1 (2, frame_rate_conversion,       "frame_rate_conversion");   Param_Info1(DV_intended_setting(frame_rate_conversion, true));
                        Get_S1 (2, brightness,                  "brightness");              Param_Info1(DV_intended_setting(brightness, false));
                        Get_S1 (2, color,                       "color");                   Param_Info1(DV_intended_setting(color, false));
                        Skip_S1(2,                              "reserved1");
                        Skip_S1(2,                              "reserved2");
                        ext_block_use_bits += 32;
                        break;
                    }
                    case 254: // CM version
                        Skip_S1(8,                              "dm_mode");
                        Skip_S1(8,                              "dm_version_index");
                        ext_block_use_bits += 16;
                        break;
                    default:
                        Skip_BS(ext_block_len_bits,             "(Not parsed)");
                        ext_block_use_bits += ext_block_len_bits;
                        break;
                    }
                    Skip_BS(ext_block_len_bits - ext_block_use_bits, "ext_dm_alignment_zero_bit");
                    Element_End0(); // ext_block_payload
                    Element_End0(); // ext_metadata_block
                }
            }
        }
        Element_End0(); // vdr_dm_data_payload
    }

    auto RemainingBitsEnd = Data_BS_Remain();
    auto size = RemainingBitsBegin - RemainingBitsEnd;
    Skip_BS((8 - (size & 7)) & 7,                               "rpu_alignment_zero_bit");
    Skip_S4(32,                                                 "rpu_data_crc32");
    Element_End0(); // rpu_data

    Mark_1(); // rbsp_stop_one_bit
    Skip_S1(7,                                                  "rbsp_alignment_zero_bit");

    #undef UNSUPPORTED
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5_003C()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_B2(itu_t_t35_terminal_provider_oriented_code,           "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
    case 0x0001: metadata_itu_t_t35_B5_003C_0001(); break;
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5_003C_0001()
{
    int8u application_identifier;
    Get_B1(application_identifier,                              "application_identifier");

    switch (application_identifier)
    {
    case 0x04: metadata_itu_t_t35_B5_003C_0001_04(); break;
    }
}

//---------------------------------------------------------------------------
void File_Av1::metadata_itu_t_t35_B5_003C_0001_04()
{
    Element_Info1("SMPTE ST 2094 App 4");

    int8u application_version{};
    bool IsHDRplus{ false }, tone_mapping_flag{};
    Get_SMPTE_ST_2094_40(application_version, IsHDRplus, tone_mapping_flag);

    FILLING_BEGIN();
        auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_SmpteSt209440];
        if (HDR_Format.empty())
        {
            HDR_Format=__T("SMPTE ST 2094 App 4");
            HDR[Video_HDR_Format_Version][HdrFormat_SmpteSt209440].From_Number(application_version);
            if (IsHDRplus)
                HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt209440]=tone_mapping_flag?__T("HDR10+ Profile B"):__T("HDR10+ Profile A");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Av1::frame()
{
    //Parsing
    Element_Begin1("frame_header");
    frame_header();
    Element_End0();

    Element_Begin1("tile_group");
    tile_group();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Av1::padding()
{
    Skip_XX(Element_Size,                                       "Padding");
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
//TODO generic code
std::string File_Av1::GOP_Detect (std::string PictureTypes)
{
    //Finding a string without blanks
    size_t PictureTypes_Limit=PictureTypes.find(' ');
    if (PictureTypes_Limit!=string::npos)
    {
        if (PictureTypes_Limit>PictureTypes.size()/2)
            PictureTypes.resize(PictureTypes_Limit);
        else
        {
            //Trim
            size_t TrimPos;
            TrimPos=PictureTypes.find_first_not_of(' ');
            if (TrimPos!=string::npos)
                PictureTypes.erase(0, TrimPos);
            TrimPos=PictureTypes.find_last_not_of(' ');
            if (TrimPos!=string::npos)
                PictureTypes.erase(TrimPos+1);

            //Finding the longest string
            ZtringList List; List.Separator_Set(0, __T(" "));
            List.Write(Ztring().From_UTF8(PictureTypes));
            size_t MaxLength=0;
            size_t MaxLength_Pos=0;
            for (size_t Pos=0; Pos<List.size(); Pos++)
                if (List[Pos].size()>MaxLength)
                {
                    MaxLength=List[Pos].size();
                    MaxLength_Pos=Pos;
                }
            PictureTypes=List[MaxLength_Pos].To_Local();

        }
    }

    //Creating all GOP values
    std::vector<Ztring> GOPs;
    size_t GOP_Frame_Count=0;
    size_t GOP_BFrames_Max=0;
    size_t I_Pos1=PictureTypes.find('I');
    while (I_Pos1!=std::string::npos)
    {
        size_t I_Pos2=PictureTypes.find('I', I_Pos1+1);
        if (I_Pos2!=std::string::npos)
        {
            std::vector<size_t> P_Positions;
            size_t P_Position=I_Pos1;
            do
            {
                P_Position=PictureTypes.find('P', P_Position+1);
                if (P_Position<I_Pos2)
                    P_Positions.push_back(P_Position);
            }
            while (P_Position<I_Pos2);
            if (P_Positions.size()>1 && P_Positions[0]>I_Pos1+1 && P_Positions[P_Positions.size()-1]==I_Pos2-1)
                P_Positions.resize(P_Positions.size()-1); //Removing last P-Frame for next test, this is often a terminating P-Frame replacing a B-Frame
            Ztring GOP;
            bool IsOK=true;
            if (!P_Positions.empty())
            {
                size_t Delta=P_Positions[0]-I_Pos1;
                for (size_t Pos=1; Pos<P_Positions.size(); Pos++)
                    if (P_Positions[Pos]-P_Positions[Pos-1]!=Delta)
                    {
                        IsOK=false;
                        break;
                    }
                if (IsOK)
                {
                    GOP+=__T("M=")+Ztring::ToZtring(P_Positions[0]-I_Pos1)+__T(", ");
                    if (P_Positions[0]-I_Pos1>GOP_BFrames_Max)
                        GOP_BFrames_Max=P_Positions[0]-I_Pos1;
                }
            }
            if (IsOK)
            {
                GOP+=__T("N=")+Ztring::ToZtring(I_Pos2-I_Pos1);
                GOPs.push_back(GOP);
            }
            else
                GOPs.push_back(Ztring()); //There is a problem, blank
            GOP_Frame_Count+=I_Pos2-I_Pos1;
        }
        I_Pos1=I_Pos2;
    }

    //Some clean up
    if (GOP_Frame_Count+GOP_BFrames_Max>Frame_Count && !GOPs.empty())
        GOPs.resize(GOPs.size()-1); //Removing the last one, there may have uncomplete B-frame filling
    if (GOPs.size()>4)
        GOPs.erase(GOPs.begin()); //Removing the first one, it is sometime different and we have enough to deal with

    //Filling
    if (GOPs.size()>=4)
    {
        bool IsOK=true;
        for (size_t Pos=1; Pos<GOPs.size(); Pos++)
            if (GOPs[Pos]!=GOPs[0])
            {
                IsOK=false;
                break;
            }
        if (IsOK)
            return GOPs[0].To_Local();
    }

    return string();
}

void File_Av1::Get_leb128(int64u& Info, const char* Name)
{
    Info=0;
    for (int8u i=0; i<8; i++)
    {
        if (Element_Offset>=Element_Size)
            break; // End of stream reached, not normal
        int8u leb128_byte=BigEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset);
        Element_Offset++;
        Info|=(static_cast<int64u>(leb128_byte&0x7f)<<(i*7));
        if (!(leb128_byte&0x80))
        {
            #if MEDIAINFO_TRACE
                if (Trace_Activated)
                {
                    Element_Offset-=(1LL+i);
                    Param(Name, Info, (i+1)*8);
                    Param_Info(__T("(")+Ztring::ToZtring(i+1)+__T(" bytes)"));
                    Element_Offset+=(1LL+i);
                }
            #endif //MEDIAINFO_TRACE
            return;
        }
    }
    Trusted_IsNot("Size is wrong");
    Info=0;
}

//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_AV1_YES
