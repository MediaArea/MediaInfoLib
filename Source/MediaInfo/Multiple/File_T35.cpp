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
#if defined(MEDIAINFO_SMPTEST209450_YES)
    #include "MediaInfo/Video/File_SmpteSt209450.h"
#endif
#if defined(MEDIAINFO_AFDBARDATA_YES)
    #include "MediaInfo/Video/File_AfdBarData.h"
#endif //defined(MEDIAINFO_AFDBARDATA_YES)
#include <string>
#include <vector>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_T35::File_T35(File__Analyze* Container_, hdr* HDR_, Ztring* EtsiTS103433_, DV_RPU* DV_RPU_data_)
:File__Analyze(),
 Container(Container_),
 HDR(HDR_?*HDR_:HDR_Own),
 EtsiTS103433(EtsiTS103433_?*EtsiTS103433_:EtsiTS103433_Own),
 DV_RPU_data(DV_RPU_data_?*DV_RPU_data_:DV_RPU_data_Own)
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_InfosToParent=true; //The message is a part of an element of the container
    #endif //MEDIAINFO_TRACE
    FrameIsAlwaysComplete=true; //The container provides whole messages
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_T35::Streams_Fill()
{
    if (!IsMetadataTrackOrItem)
        return;

    //The track has the format of the messages, the video stream or image it describes has their content
    if (Message_Format)
    {
        Stream_Prepare(Stream_Other);
        Fill(Stream_Other, 0, Other_Format, Message_Format);
    }
    if (!HDR.empty())
    {
        Stream_Prepare(Stream_Video);
        Fill_HDR(HDR);
        SmpteSt209450_Fill(this, Stream_Video, 0);
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_T35::Read_Buffer_Init()
{
    Accept("T35");
}

//---------------------------------------------------------------------------
void File_T35::Read_Buffer_Continue()
{
    //Parsing
    GA94_03_Offset=0;
    Identifier_Offset=0;
    itu_t_t35();
    while (Element_Level)
        Element_End0(); //Elements not ended after a parsing problem, the next message starts from the top level
    Element_Offset=Element_Size; //Remaining data is not parsed

    if (IsMetadataTrackOrItem)
        Finish();
}

//---------------------------------------------------------------------------
void File_T35::Get_Code(int32u& Code, int8u Size, const char* Name)
{
    if (Identifier_Offset>=Identifier.size())
    {
        switch (Size)
        {
            case 1 : {int8u  Temp; Get_B1(Temp, Name); Code=Temp; break;}
            case 2 : {int16u Temp; Get_B2(Temp, Name); Code=Temp; break;}
            default: Get_B4(Code, Name);
        }
        return;
    }

    //The code is read from the identifier provided by the container, then from the message
    Peek_Code(Code, Size);
    auto FromIdentifier=Identifier.size()-Identifier_Offset;
    if (FromIdentifier>Size)
        FromIdentifier=Size;
    Identifier_Offset+=FromIdentifier;
    Element_Offset+=Size-FromIdentifier;
    if (Element_Offset>Element_Size)
        Element_Offset=Element_Size;
    #if MEDIAINFO_TRACE
        switch (Size)
        {
            case 1 : Param(Name, (int8u)Code); break;
            case 2 : Param(Name, (int16u)Code); break;
            default: Param(Name, Code);
        }
    #else //MEDIAINFO_TRACE
        (void)Name;
    #endif //MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_T35::Get_Code1(int8u& Code, const char* Name)
{
    int32u Temp;
    Get_Code(Temp, 1, Name);
    Code=(int8u)Temp;
}

//---------------------------------------------------------------------------
void File_T35::Get_Code2(int16u& Code, const char* Name)
{
    int32u Temp;
    Get_Code(Temp, 2, Name);
    Code=(int16u)Temp;
}

//---------------------------------------------------------------------------
void File_T35::Get_Code4(int32u& Code, const char* Name)
{
    Get_Code(Code, 4, Name);
}

//---------------------------------------------------------------------------
void File_T35::Peek_Code(int32u& Code, int8u Size)
{
    Code=0;
    auto Offset=Element_Offset;
    for (size_t i=0; i<Size; i++)
    {
        Code<<=8;
        if (Identifier_Offset+i<Identifier.size())
            Code|=(int8u)Identifier[Identifier_Offset+i];
        else if (Offset<Element_Size)
            Code|=Buffer[Buffer_Offset+(size_t)Offset++];
    }
}

//---------------------------------------------------------------------------
int64u File_T35::Code_Remain()
{
    return Identifier.size()-Identifier_Offset+Element_Size-Element_Offset;
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_T35::itu_t_t35()
{
    //Parsing
    int8u itu_t_t35_country_code;
    Get_Code1(itu_t_t35_country_code,                           "itu_t_t35_country_code");
    if (itu_t_t35_country_code==0xFF)
    {
        int8u itu_t_t35_country_code_extension_byte;
        Get_Code1(itu_t_t35_country_code_extension_byte,        "itu_t_t35_country_code_extension_byte");
    }

    switch (itu_t_t35_country_code)
    {
        case 0xB5:  Param_Info1("United States"); itu_t_t35_B5(); break;
        case 0x26:  itu_t_t35_26(); break; // China
        default  :  if (Element_Size-Element_Offset)
                        Skip_XX(Element_Size-Element_Offset,    "Unknown");
    }
}

//---------------------------------------------------------------------------
// USA
void File_T35::itu_t_t35_B5()
{
    if (Code_Remain()<=2)
    {
        if (Element_Size-Element_Offset)
            Skip_XX(Element_Size-Element_Offset,                "Unknown");
        return;
    }

    int16u itu_t_t35_terminal_provider_code;
    Get_Code2(itu_t_t35_terminal_provider_code,                 "itu_t_t35_terminal_provider_code");

    switch (itu_t_t35_terminal_provider_code)
    {
        case 0x0031: itu_t_t35_B5_0031(); break;
        case 0x003A: itu_t_t35_B5_003A(); break;
        case 0x003B: Param_Info1("Dolby Laboratories, Inc."); itu_t_t35_B5_003B(); break;
        case 0x003C: Param_Info1("Samsung Electronics America"); itu_t_t35_B5_003C(); break;
        case 0x0090: Param_Info1("SMPTE"); itu_t_t35_B5_0090(); break;
        case 0x5890: Param_Info1("AOMedia"); itu_t_t35_B5_5890(); break;
        default    : if (Element_Size-Element_Offset)
                         Skip_XX(Element_Size-Element_Offset,   "Unknown");
    }
}

//---------------------------------------------------------------------------
// USA - 0031
void File_T35::itu_t_t35_B5_0031()
{
    if (Code_Remain()<=4)
    {
        if (Element_Size-Element_Offset)
            Skip_XX(Element_Size-Element_Offset,                "Unknown");
        return;
    }

    int32u user_identifier;
    Peek_Code(user_identifier, 4);
    switch (user_identifier)
    {
        case 0x44544731 :   itu_t_t35_B5_0031_DTG1(); return;
        case 0x47413934 :   itu_t_t35_B5_0031_GA94(); return;
        default         :   if (Element_Size-Element_Offset)
                                Skip_XX(Element_Size-Element_Offset, "Unknown");
    }
}

//---------------------------------------------------------------------------
// USA - 0031 - DTG1
void File_T35::itu_t_t35_B5_0031_DTG1()
{
    Element_Info1("Active Format Description");

    //Parsing
    if (Identifier_Offset<Identifier.size())
    {
        int32u afd_identifier;
        Get_Code4(afd_identifier,                               "afd_identifier");
    }
    else
        Skip_C4(                                                "afd_identifier");
    #if defined(MEDIAINFO_AFDBARDATA_YES)
    if (Element_Offset<Element_Size)
    {
        File_AfdBarData DTG1_Parser;
        DTG1_Parser.aspect_ratio_FromContainer=aspect_ratio_FromContainer;
        Open_Buffer_Init(&DTG1_Parser);
        DTG1_Parser.Format=File_AfdBarData::Format_A53_4_DTG1;
        Open_Buffer_Continue(&DTG1_Parser, Buffer+Buffer_Offset+(size_t)Element_Offset, Element_Size-Element_Offset);
        if (Container)
            Container->Merge(DTG1_Parser, Stream_Video, 0, 0);
        Element_Offset=Element_Size;
    }
    #endif //defined(MEDIAINFO_AFDBARDATA_YES)
}

//---------------------------------------------------------------------------
// USA - 0031 - GA94
void File_T35::itu_t_t35_B5_0031_GA94()
{
    //Parsing
    int8u user_data_type_code;
    int32u GA94_identifier;
    Get_Code4(GA94_identifier,                                  "GA94_identifier");
    Get_B1 (user_data_type_code,                                "user_data_type_code");
    switch (user_data_type_code)
    {
        case 0x03 : GA94_03_Offset=Element_Offset; break; //DTVCC transport, parsed by the container
        case 0x06 : itu_t_t35_B5_0031_GA94_06(); break;
        case 0x09 : itu_t_t35_B5_0031_GA94_09(); break;
        default   : Skip_XX(Element_Size-Element_Offset,        "GA94_reserved_user_data");
    }
}

//---------------------------------------------------------------------------
// USA - 0031 - GA94 - 06 - Bar data
void File_T35::itu_t_t35_B5_0031_GA94_06()
{
    Element_Info1("Bar data");

    //Parsing
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
    BS_End();

    if (Element_Size-Element_Offset)
        Skip_XX(Element_Size-Element_Offset,                    "additional_bar_data");
}

//---------------------------------------------------------------------------
// USA - 0031 - GA94 - 09 - SMPTE ST 2094-10
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
    Element_Info1("SMPTE ST 2094-10");
    int32u app_identifier, app_version;
    bool metadata_refresh_flag;
    vector<int32u> ext_block_level_List;
    BS_Begin();
    Get_UE(app_identifier, "app_identifier");
    if (app_identifier!=1)
    {
        BS_End();
        return;
    }
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
                    switch (ext_block_level)
                    {
                        case 1:
                            Skip_S2(12,                         "min_PQ");
                            Skip_S2(12,                         "max_PQ");
                            Skip_S2(12,                         "avg_PQ");
                            break;
                        case 2:
                            Skip_S2(12,                         "target_max_PQ");
                            Skip_S2(12,                         "trim_slope");
                            Skip_S2(12,                         "trim_offset");
                            Skip_S2(12,                         "trim_power");
                            Skip_S2(12,                         "trim_chroma_weight");
                            Skip_S2(12,                         "trim_saturation_gain");
                            Skip_S1( 3,                         "ms_weight");
                            break;
                        case 3:
                            Skip_S2(12,                         "min_PQ_offset");
                            Skip_S2(12,                         "max_PQ_offset");
                            Skip_S2(12,                         "avg_PQ_offset");
                            break;
                        case 4:
                            Skip_S2(12,                         "TF_PQ_mean");
                            Skip_S2(12,                         "TF_PQ_stdev");
                            break;
                        case 5:
                            Skip_S2(13,                         "active_area_left_offset");
                            Skip_S2(13,                         "active_area_right_offset");
                            Skip_S2(13,                         "active_area_top_offset");
                            Skip_S2(13,                         "active_area_bottom_offset");
                            break;
                    }
                    if (Data_BS_Remain()>End)
                    {
                        auto Align=Data_BS_Remain()-End;
                        if (Align)
                            Skip_BS(Align,                      Align>=8?"(Unknown)":"dm_alignment_zero_bits");
                    }
                    Element_End0();
                }
            }
        }
        auto Align=Data_BS_Remain()%8;
        if (Align)
            Skip_BS(Align,                                      Align>=8?"(Unknown)":"dm_alignment_zero_bits");
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
    if (!Trusted_Get() && Container)
    {
        Container->Fill(Stream_Video, 0, "ConformanceErrors", 1, 10, true);
        Container->Fill(Stream_Video, 0, "ConformanceErrors SMPTE_ST_2049_CVT", 1, 10, true);
        Container->Fill(Stream_Video, 0, "ConformanceErrors SMPTE_ST_2049_CVT Coherency", "Bitstream parsing ran out of data to read before the end of the syntax was reached, most probably the bitstream is malformed", Unlimited, true, true);
    }
}

//---------------------------------------------------------------------------
// USA - 003A
void File_T35::itu_t_t35_B5_003A()
{
    int8u itu_t_t35_terminal_provider_oriented_code;
    Get_Code1(itu_t_t35_terminal_provider_oriented_code,        "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
        case 0x00: itu_t_t35_B5_003A_00(); break;
        case 0x02: itu_t_t35_B5_003A_02(); break;
    }
}

//---------------------------------------------------------------------------
// USA - 003A - ETSI 103-433-1
void File_T35::itu_t_t35_B5_003A_00()
{
    Element_Info1("SL-HDR message");
    BS_Begin();
    int8u sl_hdr_mode_value_minus1, sl_hdr_spec_major_version_idc, sl_hdr_spec_minor_version_idc;
    bool sl_hdr_cancel_flag;
    Get_S1 (4, sl_hdr_mode_value_minus1,                        "sl_hdr_mode_value_minus1");
    Get_S1 (4, sl_hdr_spec_major_version_idc,                   "sl_hdr_spec_major_version_idc");
    Get_S1 (7, sl_hdr_spec_minor_version_idc,                   "sl_hdr_spec_minor_version_idc");
    Get_SB (sl_hdr_cancel_flag,                                 "sl_hdr_cancel_flag");
    int8u sl_hdr_payload_mode;
    int8u k_coefficient_value[3];
    if (!sl_hdr_cancel_flag)
    {
        mastering_metadata_2086 Meta;
        bool coded_picture_info_present_flag, target_picture_info_present_flag, src_mdcv_info_present_flag;
        Skip_SB(                                                "sl_hdr_persistence_flag");
        Get_SB (coded_picture_info_present_flag,                "coded_picture_info_present_flag");
        Get_SB (target_picture_info_present_flag,               "target_picture_info_present_flag");
        Get_SB (src_mdcv_info_present_flag,                     "src_mdcv_info_present_flag");
        Skip_SB(                                                "sl_hdr_extension_present_flag");
        Get_S1 (3, sl_hdr_payload_mode,                         "sl_hdr_payload_mode");
        BS_End();
        if (coded_picture_info_present_flag)
        {
            Skip_B1(                                            "coded_picture_primaries");
            Skip_B2(                                            "coded_picture_max_luminance");
            Skip_B2(                                            "coded_picture_min_luminance");
        }
        if (target_picture_info_present_flag)
        {
            Skip_B1(                                            "target_picture_primaries");
            Skip_B2(                                            "target_picture_max_luminance");
            Skip_B2(                                            "target_picture_min_luminance");
        }
        if (src_mdcv_info_present_flag)
        {
            int16u max, min;
            for (int8u i = 0; i < 3; i++)
            {
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
        for (int8u i = 0; i < 4; i++)
            Skip_B2(                                            "matrix_coefficient_value");
        for (int8u i = 0; i < 2; i++)
            Skip_B2(                                            "chroma_to_luma_injection");
        for (int8u i = 0; i < 3; i++)
            Get_B1 (k_coefficient_value[i],                     "k_coefficient_value");

        FILLING_BEGIN()
            auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_EtsiTs103433];
            if (HDR_Format.empty())
            {
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
    else
        BS_End();
}

//---------------------------------------------------------------------------
// USA - 003A - ETSI 103-433
void File_T35::itu_t_t35_B5_003A_02()
{
    Element_Info1("SL-HDR information");
    int8u ts_103_433_spec_version;
    BS_Begin();
    Get_S1 (4, ts_103_433_spec_version,                         "ts_103_433_spec_version");
    if (ts_103_433_spec_version==0)
    {
        Skip_S1 (4,                                             "ts_103_433_payload_mode");
    }
    else if (ts_103_433_spec_version==1)
    {
        Skip_S1 (3,                                             "sl_hdr_mode_support");
    }
    else
        Skip_BS (Data_BS_Remain(),                              "Unknown");
    BS_End();
}

//---------------------------------------------------------------------------
// USA - 003B
void File_T35::itu_t_t35_B5_003B()
{
    int32u itu_t_t35_terminal_provider_oriented_code;
    Get_Code4(itu_t_t35_terminal_provider_oriented_code,        "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
    case 0x00000800: itu_t_t35_B5_003B_00000800(); break;
    }

}

//---------------------------------------------------------------------------
// USA - 003B - 00000800 - Extensible Metadata Delivery Format (EMDF)
void File_T35::itu_t_t35_B5_003B_00000800()
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
                    if (BS->Remain() < Bits + 1u)
                    {
                        Trusted_IsNot("Variable length value not valid");
                        break;
                    }
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
        BS_End();
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

        Element_Info1C(emdf_payload_id == 256, "Dolby Vision Reference Processing Unit (RPU)");
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
        if (emdf_payload_size > Data_BS_Remain() / 8)
        {
            Trusted_IsNot("emdf_payload_size is too large");
            Skip_BS(Data_BS_Remain(),                           "(Problem)");
            Element_End0(); // emdf_payload
            break;
        }
        size_t emdf_payload_End = Data_BS_Remain() - static_cast<size_t>(emdf_payload_size) * 8;

        Element_Begin1("emdf_payload_bytes");
            switch (emdf_payload_id)
            {
                case 256: Get_DolbyVision_ReferenceProcessingUnit(DV_RPU_data, emdf_payload_End); break;
                default : Skip_BS(static_cast<size_t>(emdf_payload_size) * 8, "(Unknown)"); break;
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
        Skip_BS(len_second,                                     "protection_bits_secondary");

    Element_End0(); // emdf_protection

    Element_End0(); // emdf_container
    BS_End();
}

//---------------------------------------------------------------------------
// USA - 003C
void File_T35::itu_t_t35_B5_003C()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_Code2(itu_t_t35_terminal_provider_oriented_code,        "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
        case 0x0001: case 0x0003: itu_t_t35_B5_003C_0001(); break;
    }
}

//---------------------------------------------------------------------------
// USA - 003C - 0001
void File_T35::itu_t_t35_B5_003C_0001()
{
    int8u application_identifier;
    Get_B1 (application_identifier,                             "application_identifier");

    switch (application_identifier)
    {
        case 0x04: itu_t_t35_B5_003C_0001_04(); break;
    }
}

//---------------------------------------------------------------------------
// USA - 003C - 0001 - SMPTE ST 2094-40 (HDR10+)
void File_T35::itu_t_t35_B5_003C_0001_04()
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
// USA - SMPTE
void File_T35::itu_t_t35_B5_0090()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_Code2(itu_t_t35_terminal_provider_oriented_code,        "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
        case 0x0001: itu_t_t35_B5_0090_0001(); break;
    }
}

//---------------------------------------------------------------------------
// USA - SMPTE - SMPTE ST 2094-50
void File_T35::itu_t_t35_B5_0090_0001()
{
    Element_Info1("SMPTE ST 2094 App 5");
    if (!Message_Format)
        Message_Format="SMPTE ST 2094-50";

    #if defined(MEDIAINFO_SMPTEST209450_YES)
        //Parsing
        File_SmpteSt209450 MI;
        Open_Buffer_Init(&MI);
        Open_Buffer_Continue(&MI);

        FILLING_BEGIN();
            auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_SmpteSt209450];
            if (HDR_Format.empty())
            {
                for (const auto& Item : MI.HDR)
                {
                    if (Item.first=="HDR_Format")
                        HDR_Format=Item.second;
                    else if (Item.first=="HDR_Format_Version")
                        HDR[Video_HDR_Format_Version][HdrFormat_SmpteSt209450]=Item.second;
                    else if (Item.first=="HDR_Format_Settings")
                        HDR[Video_HDR_Format_Settings][HdrFormat_SmpteSt209450]=Item.second;
                }
                SmpteSt209450_Infos=MI.Infos;
            }
        FILLING_END();
    #endif //defined(MEDIAINFO_SMPTEST209450_YES)
}

//---------------------------------------------------------------------------
void File_T35::SmpteSt209450_Fill(File__Analyze* Parent, stream_t StreamKind, size_t StreamPos)
{
    for (const auto& Info : SmpteSt209450_Infos)
        Parent->Fill(StreamKind, StreamPos, Info.first.c_str(), Info.second, true);
}

//---------------------------------------------------------------------------
// USA - 5890
void File_T35::itu_t_t35_B5_5890()
{
    int8u itu_t_t35_terminal_provider_oriented_code;
    Get_Code1(itu_t_t35_terminal_provider_oriented_code,        "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
    case 0x01: itu_t_t35_B5_5890_01(); break;
    }
}

//---------------------------------------------------------------------------
// USA - 5890 - 01 - AOMedia Film Grain Synthesis 1 (AFGS1)
void File_T35::itu_t_t35_B5_5890_01()
{
    Element_Info1("AOMedia Film Grain Synthesis 1 (AFGS1)");

    Element_Begin1("av1_film_grain_param_sets");
    BS_Begin();
    bool afgs1_enable_flag;
    Get_SB(afgs1_enable_flag,                                   "afgs1_enable_flag");
    if (!afgs1_enable_flag) {
        BS_End();
        Element_End0();
        return;
    }
    Skip_S1(4,                                                  "reserved_4bits");
    Skip_S1(3,                                                  "num_film_grain_sets_minus1");
    BS_End();
    Skip_XX(Element_Size - Element_Offset, "(Not parsed)");
    Element_End0();
}

//---------------------------------------------------------------------------
// China
void File_T35::itu_t_t35_26()
{
    int16u itu_t_t35_terminal_provider_code;
    Get_Code2(itu_t_t35_terminal_provider_code,                 "itu_t_t35_terminal_provider_code");

    //HDR Vivid terminal provide code 0x0004, terminal provide oriented code 0x0005 , 0x0005 indicates HDR Vivid version 1
    switch (itu_t_t35_terminal_provider_code)
    {
        case 0x0004: itu_t_t35_26_0004(); break;
    }
}

//---------------------------------------------------------------------------
// China    similar to itu_t_t35_B5_003C()
void File_T35::itu_t_t35_26_0004()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_Code2(itu_t_t35_terminal_provider_oriented_code,        "itu_t_t35_terminal_provider_oriented_code");
    //HDR Vivid terminal provide code 0x0004, terminal provide oriented code 0x0005
    switch (itu_t_t35_terminal_provider_oriented_code)
    {
        case 0x0005: itu_t_t35_26_0004_0005(); break;
    }
}

//---------------------------------------------------------------------------
// China - 0004
void File_T35::itu_t_t35_26_0004_0005()
{
    //Parsing
    Element_Info1("T/UWA 005 (HDR Vivid)");
    int16u targeted_system_display_maximum_luminance_Max=0;
    int8u system_start_code;
    bool color_saturation_mapping_enable_flag;
    Get_B1(system_start_code,                                   "system_start_code");
    if (system_start_code!=0x01)
    {
        Skip_XX(Element_Size,                                   "Unknown");
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
        Get_SB(tone_mapping_enable_mode_flag,                   "tone_mapping_enable_mode_flag");
        if (tone_mapping_enable_mode_flag)
        {
            int8u tone_mapping_param_enable_num;
            Get_S1(1, tone_mapping_param_enable_num,            "tone_mapping_param_enable_num");
            for (int i=0; i<(tone_mapping_param_enable_num+1); i++)
            {
                Element_Begin1("tone_mapping_param");
                int16u targeted_system_display_maximum_luminance_pq;
                bool base_enable_flag, ThreeSpline_enable_flag;
                Get_S2(12, targeted_system_display_maximum_luminance_pq,
                                                                "targeted_system_display_maximum_luminance_pq");
                if (targeted_system_display_maximum_luminance_Max<targeted_system_display_maximum_luminance_pq)
                    targeted_system_display_maximum_luminance_Max=targeted_system_display_maximum_luminance_pq;
                Get_SB(    base_enable_flag,                    "base_enable_flag");
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
                Get_SB(ThreeSpline_enable_flag,                 "3Spline_enable_flag");
                if (ThreeSpline_enable_flag)
                {
                    int8u ThreeSpline_enable_num;
                    Get_S1(1, ThreeSpline_enable_num,           "3Spline_enable_num");
                    for (int j=0; j<(ThreeSpline_enable_num+1); j++)
                    {
                        Element_Begin1("3Spline");
                        int8u ThreeSpline_TH_enable_mode;
                        Get_S1(2, ThreeSpline_TH_enable_mode,   "3Spline_TH_enable_mode");
                        switch (ThreeSpline_TH_enable_mode)
                        {
                            case 0:
                            case 2:
                                Skip_S1(8,                      "3Spline_TH_enable_MB");
                                break;
                            default:;
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
        Get_SB(color_saturation_mapping_enable_flag,            "color_saturation_mapping_enable_flag");
        if (color_saturation_mapping_enable_flag)
        {
            int8u color_saturation_enable_num;
            Get_S1(3, color_saturation_enable_num,              "color_saturation_enable_num");
            for (int i=0; i<color_saturation_enable_num; i++)
            {
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
            HDR[Video_HDR_Format_Version][HdrFormat_HdrVivid]=Ztring::ToZtring(system_start_code);
        }
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_T35_YES
