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
#if defined(MEDIAINFO_T35_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_T35.h"
#if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
#include "MediaInfo/Text/File_DtvccTransport.h"
#endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
#if defined(MEDIAINFO_AFDBARDATA_YES)
#include "MediaInfo/Video/File_AfdBarData.h"
#endif //defined(MEDIAINFO_AFDBARDATA_YES)
#include <string>
#define FMT_UNICODE 0
#include "ThirdParty/fmt/format.h"
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_T35::File_T35(source Source)
: Source(Source)
{
    //Config
    StreamSource=IsStream;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_T35::Streams_Fill()
{
    //MasteringDisplay
    if (MasteringMetadata) {
        Ztring& MasteringDisplay_ColorPrimaries = HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_SmpteSt2086];
        Ztring& MasteringDisplay_Luminance = HDR[Video_MasteringDisplay_Luminance][HdrFormat_SmpteSt2086];
        Get_MasteringDisplayColorVolume(MasteringDisplay_ColorPrimaries, MasteringDisplay_Luminance, *MasteringMetadata);
        if (!MasteringDisplay_ColorPrimaries.empty() || !MasteringDisplay_Luminance.empty()) {
            auto& HDR_Format = HDR[Video_HDR_Format][HdrFormat_SmpteSt2086];
            if (HDR_Format.empty()) {
                HDR_Format = __T("SMPTE ST 2086");
                HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt2086] = "HDR10";
            }
        }
    }

    auto HDR_Format = HDR.find(Video_HDR_Format);
    if (HDR_Format != HDR.end()) {
        if (!Count_Get(Stream_Video)) {
            Stream_Prepare(Stream_Video);
        }
        bitset<HdrFormat_Max> HDR_Present;
        size_t HDR_FirstFormatPos = (size_t)-1;
        for (size_t i = 0; i < HdrFormat_Max; i++) {
            if (!HDR_Format->second[i].empty()) {
                if (HDR_FirstFormatPos == (size_t)-1)
                    HDR_FirstFormatPos = i;
                HDR_Present[i] = true;
            }
        }
        bool LegacyStreamDisplay = MediaInfoLib::Config.LegacyStreamDisplay_Get();
        for (const auto& HDR_Item : HDR)
        {
            size_t i = HDR_FirstFormatPos;
            size_t HDR_FirstFieldNonEmpty = (size_t)-1;
            if (HDR_Item.first > Video_HDR_Format_Compatibility) {
                for (; i < HdrFormat_Max; i++) {
                    if (!HDR_Present[i]) {
                        continue;
                    }
                    if (HDR_FirstFieldNonEmpty == (size_t)-1 && !HDR_Item.second[i].empty()) {
                        HDR_FirstFieldNonEmpty = i;
                    }
                    if (!HDR_Item.second[i].empty() && HDR_Item.second[i] != HDR_Item.second[HDR_FirstFieldNonEmpty]) {
                        break;
                    }
                }
            }
            if (i == HdrFormat_Max && HDR_FirstFieldNonEmpty != (size_t)-1) {
                Fill(Stream_Video, 0, HDR_Item.first, HDR_Item.second[HDR_FirstFieldNonEmpty]);
            }
            else {
                ZtringList Value;
                Value.Separator_Set(0, __T(" / "));
                if (i != HdrFormat_Max) {
                    for (i = HDR_FirstFormatPos; i < HdrFormat_Max; i++) {
                        if (!LegacyStreamDisplay && HDR_FirstFormatPos != HdrFormat_SmpteSt2086 && i >= HdrFormat_SmpteSt2086) {
                            break;
                        }
                        if (!HDR_Present[i]) {
                            continue;
                        }
                        Value.push_back(HDR_Item.second[i]);
                    }
                }
                auto Value_Flat = Value.Read();
                if (!Value.empty() && Value_Flat.size() > (Value.size() - 1) * 3) {
                    Fill(Stream_Video, 0, HDR_Item.first, Value.Read());
                }
            }
        }
    }
    if (!EtsiTS103433.empty()) {
        Fill(Stream_Video, 0, "EtsiTS103433", EtsiTS103433);
        Fill_SetOptions(Stream_Video, 0, "EtsiTS103433", "N NTN");
    }
    if (!maximum_content_light_level.empty() || !maximum_frame_average_light_level.empty()) {
        if (!Count_Get(Stream_Video)) {
            Stream_Prepare(Stream_Video);
        }
        if (!maximum_content_light_level.empty()) {
            Fill(Stream_Video, 0, Video_MaxCLL, maximum_content_light_level);
        }
        if (!maximum_frame_average_light_level.empty()) {
            Fill(Stream_Video, 0, Video_MaxFALL, maximum_frame_average_light_level);
        }
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_T35::Read_Buffer_Init()
{
    Accept();
}

//---------------------------------------------------------------------------
void File_T35::Read_Buffer_Continue()
{
    #define CASE(x) case style::x: ##x(); break;
    switch (Style) {
    CASE(itu_t_t35)
    CASE(mastering_display_colour_volume)
    CASE(light_level)
    }
    #undef CASE

    Skip_XX(Element_Size - Element_Offset,                      "(Unknown)");
}

//---------------------------------------------------------------------------
void File_T35::itu_t_t35()
{
    int8u itu_t_t35_country_code;
    Get_B1 (itu_t_t35_country_code,                             "itu_t_t35_country_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_##x(); break;
    switch (itu_t_t35_country_code) {
        CASE(26, "China")
        CASE(B5, "USA")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// T35 - China
void File_T35::itu_t_t35_26()
{
    int16u itu_t_t35_terminal_provider_code;
    Get_B2 (itu_t_t35_terminal_provider_code,                   "itu_t_t35_terminal_provider_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_26_##x(); break;
    switch (itu_t_t35_terminal_provider_code) {
        CASE(0004, "UWA")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// T35 - China - UWA
void File_T35::itu_t_t35_26_0004()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_B2 (itu_t_t35_terminal_provider_oriented_code,          "itu_t_t35_terminal_provider_oriented_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_26_0004_##x(); break;
    switch (itu_t_t35_terminal_provider_oriented_code) {
        CASE(0005, "TUWA 005 (HDR Vivid)")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// T35 - China - UWA - TUWA 005 (HDR Vivid)
void File_T35::itu_t_t35_26_0004_0005()
{
    int16u targeted_system_display_maximum_luminance_Max=0;
    int8u system_start_code;
    bool color_saturation_mapping_enable_flag;
    Get_B1 (system_start_code,                                  "system_start_code");
    if (system_start_code!=0x01) {
        return;
    }
    BS_Begin();
    //int8u num_windows=1;
    //for (int8u w=0; w<num_windows; w++)
    {
        Skip_S2(12,                                             "minimum_maxrgb_pq");
        Skip_S2(12,                                             "average_maxrgb_pq");
        Skip_S2(12,                                             "variance_maxrgb_pq");
        Skip_S2(12,                                             "maximum_maxrgb_pq");
    }

    //for (int8u w=0; w<num_windows; w++)
    {
        bool tone_mapping_enable_mode_flag;
        Get_SB (tone_mapping_enable_mode_flag,                  "tone_mapping_enable_mode_flag");
        if (tone_mapping_enable_mode_flag)
        {
            bool tone_mapping_param_enable_num;
            Get_SB (   tone_mapping_param_enable_num,           "tone_mapping_param_enable_num");
            for (auto i = 0; i <= static_cast<decltype(i)>(tone_mapping_param_enable_num); i++)
            {
                Element_Begin1("tone_mapping_param");
                int16u targeted_system_display_maximum_luminance_pq;
                bool base_enable_flag, ThreeSpline_enable_flag;
                Get_S2 (12, targeted_system_display_maximum_luminance_pq, "targeted_system_display_maximum_luminance_pq");
                if (targeted_system_display_maximum_luminance_Max < targeted_system_display_maximum_luminance_pq) {
                    targeted_system_display_maximum_luminance_Max = targeted_system_display_maximum_luminance_pq;
                }
                Get_SB (   base_enable_flag,                    "base_enable_flag");
                if (base_enable_flag)
                {
                    Skip_S2(14,                                 "base_param_m_p");
                    Skip_S1( 6,                                 "base_param_m_m");
                    Skip_S2(10,                                 "base_param_m_a");
                    Skip_S2(10,                                 "base_param_m_b");
                    Skip_S1( 6,                                 "base_param_m_n");
                    Skip_S1( 2,                                 "base_param_K1");
                    Skip_S1( 2,                                 "base_param_K2");
                    Skip_S1( 4,                                 "base_param_K3");
                    Skip_S1( 3,                                 "base_param_Delta_enable_mode");
                    Skip_S1( 7,                                 "base_param_enable_Delta");
                }
                Get_SB (ThreeSpline_enable_flag,                "3Spline_enable_flag");
                if (ThreeSpline_enable_flag)
                {
                    bool ThreeSpline_enable_num;
                    Get_SB (   ThreeSpline_enable_num,          "3Spline_enable_num");
                    for (auto j = 0; j <= static_cast<decltype(j)>(ThreeSpline_enable_num); j++)
                    {
                        Element_Begin1("3Spline");
                        int8u ThreeSpline_TH_enable_mode;
                        Get_S1 (2, ThreeSpline_TH_enable_mode,  "3Spline_TH_enable_mode");
                        switch (ThreeSpline_TH_enable_mode) {
                            case 0:
                            case 2:
                                Skip_S1(8,                      "3Spline_TH_enable_MB");
                                break;
                        }
                        Skip_S2(12,                             "3Spline_TH_enable");
                        Skip_S2(10,                             "3Spline_TH_enable_Delta1");
                        Skip_S2(10,                             "3Spline_TH_enable_Delta2");
                        Skip_S1( 8,                             "3Spline_enable_Strength");
                        Element_End0();
                    }
                }
                Element_End0();
            }
        }
        Get_SB (color_saturation_mapping_enable_flag,           "color_saturation_mapping_enable_flag");
        if (color_saturation_mapping_enable_flag)
        {
            int8u color_saturation_enable_num;
            Get_S1 (3, color_saturation_enable_num,             "color_saturation_enable_num");
            for (int i = 0; i < color_saturation_enable_num; i++) {
                Skip_S1(8,                                      "color_saturation_enable_gain");
            }
        }
    }
    BS_End();

    FILLING_BEGIN();
        auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_HdrVivid];
        if (HDR_Format.empty())
        {
            HDR_Format=__T("HDR Vivid");
            HDR[Video_HDR_Format_Version][HdrFormat_HdrVivid] = Ztring().From_UTF8(fmt::to_string(system_start_code));
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
// USA
void File_T35::itu_t_t35_B5()
{
    int16u itu_t_t35_terminal_provider_code;
    Get_B2 (itu_t_t35_terminal_provider_code,                   "itu_t_t35_terminal_provider_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_B5_##x(); break;
    switch (itu_t_t35_terminal_provider_code) {
        CASE(0031, "ATSC")
        CASE(003A, "ETSI")
        CASE(003B, "Dolby")
        CASE(003C, "Samsung Electronics America")
        CASE(5890, "AOMedia")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// USA - ATSC
void File_T35::itu_t_t35_B5_0031()
{
    int32u identifier;
    Get_B4 (identifier,                                         "identifier");
    #define CASE(x,z,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_B5_0031_##z(); break;
    switch (identifier) {
        CASE(44544731, DTG1, "Digital TV Group Active Format Description")
        CASE(47413934, GA94, "General Instrument 94")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// USA - ATSC - DTG1
void File_T35::itu_t_t35_B5_0031_DTG1()
{
    File_AfdBarData DTG1_Parser;
    DTG1_Parser.aspect_ratio_FromContainer = aspect_ratio_FromContainer;
    Open_Buffer_Init(&DTG1_Parser);
    DTG1_Parser.Format=File_AfdBarData::Format_A53_4_DTG1;
    Open_Buffer_Continue(&DTG1_Parser);
    Merge(DTG1_Parser, Stream_Video, 0, 0);
}

//---------------------------------------------------------------------------
// USA - ATSC - GA94
void File_T35::itu_t_t35_B5_0031_GA94()
{
    int8u user_data_type_code;
    Get_B1(user_data_type_code,                                 "user_data_type_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_B5_0031_GA94_##x(); break;
    switch (user_data_type_code) {
        //CASE(03, "EIA 708");
        CASE(06, "Bar data");
        CASE(09, "SMPTE ST 2094-10 (SMPTE ST 2094 App 1)");
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// USA - ATSC - GA94 - Bar data
void File_T35::itu_t_t35_B5_0031_GA94_06()
{
    bool   top_bar_flag, bottom_bar_flag, left_bar_flag, right_bar_flag;
    BS_Begin();
    Get_SB (top_bar_flag,                                       "top_bar_flag");
    Get_SB (bottom_bar_flag,                                    "bottom_bar_flag");
    Get_SB (left_bar_flag,                                      "left_bar_flag");
    Get_SB (right_bar_flag,                                     "right_bar_flag");
    Mark_1_NoTrustError();
    Mark_1_NoTrustError();
    Mark_1_NoTrustError();
    Mark_1_NoTrustError();
    BS_End();
    if (top_bar_flag)
    {
        Mark_1();
        Mark_1();
        Skip_S2(14,                                             "line_number_end_of_top_bar");
    }
    if (bottom_bar_flag)
    {
        Mark_1();
        Mark_1();
        Skip_S2(14,                                             "line_number_start_of_bottom_bar");
    }
    if (left_bar_flag)
    {
        Mark_1();
        Mark_1();
        Skip_S2(14,                                             "pixel_number_end_of_left_bar");
    }
    if (right_bar_flag)
    {
        Mark_1();
        Mark_1();
        Skip_S2(14,                                             "pixel_number_start_of_right_bar");
    }
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    BS_End();

    if (Element_Size-Element_Offset)
        Skip_XX(Element_Size-Element_Offset,                    "additional_bar_data");
}

//---------------------------------------------------------------------------
// USA - ATSC - GA94 - SMPTE ST 2094-10
static const char* Smpte209410_BlockNames[]=
{
    nullptr,
    "Content Range",
    "Trim Pass",
    nullptr,
    nullptr,
    "Active Area",
};
static const auto Smpte209410_BlockNames_Size=sizeof(Smpte209410_BlockNames)/sizeof(decltype(*Smpte209410_BlockNames));
void File_T35::itu_t_t35_B5_0031_GA94_09()
{
    int32u app_identifier, app_version;
    bool metadata_refresh_flag;
    vector<int32u> ext_block_level_List;
    BS_Begin();
    Get_UE(app_identifier,                                      "app_identifier");
    if (app_identifier!=1)
        return;
    Get_UE (app_version,                                        "app_version");
    if (!app_version)
    {
        Get_SB(metadata_refresh_flag,                           "metadata_refresh_flag");
        if (metadata_refresh_flag)
        {
            int32u num_ext_blocks;
            Get_UE (num_ext_blocks,                             "num_ext_blocks");
            if (num_ext_blocks)
            {
                auto Align=Data_BS_Remain()%8;
                if (Align)
                    Skip_BS(Align,                              "dm_alignment_zero_bits");
                for (int32u i=0; i<num_ext_blocks; i++)
                {
                    Element_Begin1("block");
                    Element_Begin1("Header");
                    int32u ext_block_length;
                    int8u ext_block_level;
                    Get_UE (ext_block_length,                   "ext_block_length");
                    Get_S1 (8, ext_block_level,                 "ext_block_level");
                    Element_End0();
                    Element_Info1((ext_block_level<Smpte209410_BlockNames_Size && Smpte209410_BlockNames[ext_block_level])?Smpte209410_BlockNames[ext_block_level]:to_string(ext_block_level).c_str());
                    if (ext_block_length>Data_BS_Remain())
                    {
                        Element_End0();
                        Trusted_IsNot("Coherency");
                        break;
                    }
                    ext_block_length*=8;
                    if (ext_block_length>Data_BS_Remain())
                    {
                        Element_End0();
                        Trusted_IsNot("Coherency");
                        break;
                    }
                    auto End=Data_BS_Remain()-ext_block_length;
                    ext_block_level_List.push_back(ext_block_level);
                    switch (ext_block_level) {
                    case 1:
                        Skip_S2(12,                             "min_PQ");
                        Skip_S2(12,                             "max_PQ");
                        Skip_S2(12,                             "avg_PQ");
                        break;
                    case 2:
                        Skip_S2(12,                             "target_max_PQ");
                        Skip_S2(12,                             "trim_slope");
                        Skip_S2(12,                             "trim_offset");
                        Skip_S2(12,                             "trim_power");
                        Skip_S2(12,                             "trim_chroma_weight");
                        Skip_S2(12,                             "trim_saturation_gain");
                        Skip_S1( 3,                             "ms_weight");
                        break;
                    case 3:
                        Skip_S2(12,                             "min_PQ_offset");
                        Skip_S2(12,                             "max_PQ_offset");
                        Skip_S2(12,                             "avg_PQ_offset");
                        break;
                    case 4:
                        Skip_S2(12,                             "TF_PQ_mean");
                        Skip_S2(12,                             "TF_PQ_stdev");
                        break;
                    case 5:
                        Skip_S2(13,                             "active_area_left_offset");
                        Skip_S2(13,                             "active_area_right_offset");
                        Skip_S2(13,                             "active_area_top_offset");
                        Skip_S2(13,                             "active_area_bottom_offset");
                        break;
                    }
                    if (Data_BS_Remain() > End) {
                        auto Align = Data_BS_Remain() - End;
                        if (Align)
                            Skip_BS(Align,                      Align >= 8 ? "(Unknown)" : "dm_alignment_zero_bits");
                    }
                    Element_End0();
                }
            }
        }
        auto Align = Data_BS_Remain() % 8;
        if (Align)
            Skip_BS(Align,                                      Align >= 8 ? "(Unknown)" : "dm_alignment_zero_bits");
        BS_End();
    }

    auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_SmpteSt209410];
    if (HDR_Format.empty())
    {
        HDR_Format=__T("SMPTE ST 2094-10");
        FILLING_BEGIN();
            HDR[Video_HDR_Format_Version][HdrFormat_SmpteSt209410].From_Number(app_version);
        FILLING_END();
    }
    if (!Trusted_Get())
    {
        Fill(Stream_Video, 0, "ConformanceErrors", "Yes", Unlimited, true, true);
        Fill(Stream_Video, 0, "ConformanceErrors SMPTE_ST_2049_CVT", "Yes", Unlimited, true, true);
        Fill(Stream_Video, 0, "ConformanceErrors SMPTE_ST_2049_CVT Coherency", "Bitstream parsing ran out of data to read before the end of the syntax was reached, most probably the bitstream is malformed", Unlimited, true, true);
    }
}

//---------------------------------------------------------------------------
// USA - ETSI
void File_T35::itu_t_t35_B5_003A()
{
    int8u itu_t_t35_terminal_provider_oriented_code;
    Get_B1 (itu_t_t35_terminal_provider_oriented_code,          "itu_t_t35_terminal_provider_oriented_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_B5_003A_##x(); break;
    switch (itu_t_t35_terminal_provider_oriented_code) {
        CASE(00, "ETSI 103-433-1 (SL-HDR) message")
        CASE(02, "ETSI 103-433-1 (SL-HDR) information")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// USA - ETSI - ETSI 103-433-1 (SL-HDR) message
void File_T35::itu_t_t35_B5_003A_00()
{
    Element_Begin1("sl_hdr_info");
    BS_Begin();
    int8u sl_hdr_mode_value_minus1, sl_hdr_spec_major_version_idc, sl_hdr_spec_minor_version_idc;
    bool sl_hdr_cancel_flag;
    Get_S1 (4, sl_hdr_mode_value_minus1,                        "sl_hdr_mode_value_minus1");
    Get_S1 (4, sl_hdr_spec_major_version_idc,                   "sl_hdr_spec_major_version_idc");
    Get_S1 (7, sl_hdr_spec_minor_version_idc,                   "sl_hdr_spec_minor_version_idc");
    Get_SB (sl_hdr_cancel_flag,                                 "sl_hdr_cancel_flag");
    if (sl_hdr_cancel_flag) {
        BS_End();

        FILLING_BEGIN()
            auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_EtsiTs103433];
            HDR_Format.clear();
            EtsiTS103433.clear();
        FILLING_END();
    }
    else {
        int8u sl_hdr_payload_mode;
        int8u k_coefficient_value[3];
        mastering_metadata_2086 Meta;
        bool coded_picture_info_present_flag, target_picture_info_present_flag, src_mdcv_info_present_flag;
        Skip_SB(                                                "sl_hdr_persistence_flag");
        Get_SB (coded_picture_info_present_flag,                "coded_picture_info_present_flag");
        Get_SB (target_picture_info_present_flag,               "target_picture_info_present_flag");
        Get_SB (src_mdcv_info_present_flag,                     "src_mdcv_info_present_flag");
        Skip_SB(                                                "sl_hdr_extension_present_flag");
        Get_S1 (3, sl_hdr_payload_mode,                         "sl_hdr_payload_mode");
        BS_End();
        if (coded_picture_info_present_flag) {
            Skip_B1(                                            "coded_picture_primaries");
            Skip_B2(                                            "coded_picture_max_luminance");
            Skip_B2(                                            "coded_picture_min_luminance");
        }
        if (target_picture_info_present_flag) {
            Skip_B1(                                            "target_picture_primaries");
            Skip_B2(                                            "target_picture_max_luminance");
            Skip_B2(                                            "target_picture_min_luminance");
        }
        if (src_mdcv_info_present_flag) {
            int16u max, min;
            for (auto i = 0; i < 3; i++) {
                Get_B2 (Meta.Primaries[i*2  ],                  "src_mdcv_primaries_x");
                Get_B2 (Meta.Primaries[i*2+1],                  "src_mdcv_primaries_y");
            }
            Get_B2 (Meta.Primaries[3*2  ],                      "src_mdcv_ref_white_x");
            Get_B2 (Meta.Primaries[3*2+1],                      "src_mdcv_ref_white_y");
            Get_B2 (max,                                        "src_mdcv_max_mastering_luminance");
            Get_B2 (min,                                        "src_mdcv_min_mastering_luminance");
            Meta.Luminance[0]=min;
            Meta.Luminance[1]=((int32u)max)*10000;
        }
        for (auto i = 0; i < 4; i++) {
            Skip_B2(                                            "matrix_coefficient_value");
        }
        for (auto i = 0; i < 2; i++) {
            Skip_B2(                                            "chroma_to_luma_injection");
        }
        for (auto i = 0; i < 3; i++) {
            Get_B1 (k_coefficient_value[i],                     "k_coefficient_value");
        }
        Skip_BS(Data_BS_Remain(),                               "(Not parsed)");

        FILLING_BEGIN()
            auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_EtsiTs103433];
            if (HDR_Format.empty()) {
                HDR_Format=__T("SL-HDR")+Ztring().From_Number(sl_hdr_mode_value_minus1+1);
                HDR[Video_HDR_Format_Version][HdrFormat_EtsiTs103433]=Ztring().From_Number(sl_hdr_spec_major_version_idc)+__T('.')+Ztring().From_Number(sl_hdr_spec_minor_version_idc);
                Get_MasteringDisplayColorVolume(HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_EtsiTs103433], HDR[Video_MasteringDisplay_Luminance][HdrFormat_EtsiTs103433], Meta);
                auto& HDR_Format_Settings=HDR[Video_HDR_Format_Settings][HdrFormat_EtsiTs103433];
                if (sl_hdr_payload_mode<2)
                    HDR_Format_Settings=sl_hdr_payload_mode?__T("Table-based"):__T("Parameter-based");
                else
                    HDR_Format_Settings=__T("Payload Mode ") + Ztring().From_Number(sl_hdr_payload_mode);
                if (!sl_hdr_mode_value_minus1)
                    HDR_Format_Settings+=k_coefficient_value[0]==0 && k_coefficient_value[1]==0 && k_coefficient_value[2]==0?__T(", non-constant"):__T(", constant");

                EtsiTS103433 = __T("SL-HDR") + Ztring().From_Number(sl_hdr_mode_value_minus1 + 1);
                if (!sl_hdr_mode_value_minus1)
                    EtsiTS103433 += k_coefficient_value[0] == 0 && k_coefficient_value[1] == 0 && k_coefficient_value[2] == 0 ? __T(" NCL") : __T(" CL");
                EtsiTS103433 += __T(" specVersion=") + Ztring().From_Number(sl_hdr_spec_major_version_idc) + __T(".") + Ztring().From_Number(sl_hdr_spec_minor_version_idc);
                EtsiTS103433 += __T(" payloadMode=") + Ztring().From_Number(sl_hdr_payload_mode);
            }
        FILLING_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
// USA - ETSI - ETSI 103-433-1 (SL-HDR) information
void File_T35::itu_t_t35_B5_003A_02()
{
    int8u ts_103_433_spec_version;
    BS_Begin();
    Get_S1 (4, ts_103_433_spec_version,                         "ts_103_433_spec_version");
    switch (ts_103_433_spec_version) {
    case 0:
        Skip_S1(4,                                              "ts_103_433_payload_mode");
        break;
    case 1:
        Skip_S1(3,                                              "sl_hdr_mode_support");
        break;
    default:
        Skip_BS(Data_BS_Remain(),                               "(Unknown)");
    }
    BS_End();
}

//---------------------------------------------------------------------------
// USA - Dolby
void File_T35::itu_t_t35_B5_003B()
{
    int32u itu_t_t35_terminal_provider_oriented_code;
    Get_B4 (itu_t_t35_terminal_provider_oriented_code,           "itu_t_t35_terminal_provider_oriented_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_B5_003B_##x(); break;
    switch (itu_t_t35_terminal_provider_oriented_code) {
        CASE(00000800, "EMDF")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// USA - Dolby - EMDF
void File_T35::itu_t_t35_B5_003B_00000800()
{
}

//---------------------------------------------------------------------------
// USA - Samsung Electronics America
void File_T35::itu_t_t35_B5_003C()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_B2 (itu_t_t35_terminal_provider_oriented_code,          "itu_t_t35_terminal_provider_oriented_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_B5_003C_##x(); break;
    switch (itu_t_t35_terminal_provider_oriented_code) {
        CASE(0001, "SMPTE ST 2094")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// USA - Samsung Electronics America - SMPTE ST 2094
void File_T35::itu_t_t35_B5_003C_0001()
{
    int8u application_identifier;
    Get_B1 (application_identifier,                             "application_identifier");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_B5_003C_0001_##x(); break;
    switch (application_identifier) {
        CASE(04, "SMPTE ST 2094-40 (SMPTE ST 2094 App 4)")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// USA - Samsung Electronics America - SMPTE ST 2094 - SMPTE ST 2094-40 (SMPTE ST 2094 App 4)
void File_T35::itu_t_t35_B5_003C_0001_04()
{
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
// USA - AOMedia
void File_T35::itu_t_t35_B5_5890()
{
    int8u itu_t_t35_terminal_provider_oriented_code;
    Get_B1 (itu_t_t35_terminal_provider_oriented_code,          "itu_t_t35_terminal_provider_oriented_code");

    #define CASE(x,y) case 0x##x: Param_Info1(y); Element_Info1(y); itu_t_t35_B5_5890_##x(); break;
    switch (itu_t_t35_terminal_provider_oriented_code) {
        CASE(01, "AFGS1 (AOMedia Film Grain Synthesis 1)")
    }
    #undef CASE
}

//---------------------------------------------------------------------------
// USA - AOMedia - AFGS1 (AOMedia Film Grain Synthesis 1)
void File_T35::itu_t_t35_B5_5890_01()
{
    Element_Begin1("av1_film_grain_param_sets");
    BS_Begin();
    bool afgs1_enable_flag;
    Get_SB(afgs1_enable_flag,                                   "afgs1_enable_flag");
    if (!afgs1_enable_flag) {
        BS_End();
        return;
    }
    Skip_S1(4,                                                  "reserved_4bits");
    Skip_S1(3,                                                  "num_film_grain_sets_minus1");
    Skip_BS(Data_BS_Remain(),                                   "(Not parsed)");
    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_T35::mastering_display_colour_volume()
{
    Element_Info1("mastering_display_colour_volume");

    auto& HDR_Format = HDR[Video_HDR_Format][HdrFormat_SmpteSt2086];
    if (HDR_Format.empty())
    {
        HDR_Format = __T("SMPTE ST 2086");
        HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt2086] = "HDR10";
    }
    Get_MasteringDisplayColorVolume(HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_SmpteSt2086], HDR[Video_MasteringDisplay_Luminance][HdrFormat_SmpteSt2086], Source == source::aomedia);
}

//---------------------------------------------------------------------------
void File_T35::light_level()
{
    Element_Info1("light_level");

    Get_LightLevel(maximum_content_light_level, maximum_frame_average_light_level);
}

} //NameSpace

#endif
