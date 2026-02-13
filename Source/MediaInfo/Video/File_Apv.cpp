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
#if defined(MEDIAINFO_APV_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Apv.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cmath>
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
static string Apv_pbu_type(int8u pbu_type)
{
    switch (pbu_type) {
    case  1: return "primary frame";
    case  2: return "non-primary frame";
    case 25: return "preview frame";
    case 26: return "depth frame";
    case 27: return "alpha frame";
    case 65: return "access unit information";
    case 66: return "metadata";
    case 67: return "filler";
    default: return std::to_string(pbu_type);
    }
}

//---------------------------------------------------------------------------
static string APV_Profile(int8u profile_idc)
{
    switch (profile_idc) {
    case 33: return "422-10";
    case 44: return "422-12";
    case 55: return "444-10";
    case 66: return "444-12";
    case 77: return "4444-10";
    case 88: return "4444-12";
    case 99: return "400-10";
    default: return std::to_string(profile_idc);
    }
}

//---------------------------------------------------------------------------
static string APV_Chroma(int8u chroma_format_idc)
{
    switch (chroma_format_idc) {
    case 0: return "4:0:0";
    case 2: return "4:2:2";
    case 3: return "4:4:4";
    case 4: return "4:4:4:4";
    default: return std::to_string(chroma_format_idc);
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Apv::File_Apv()
{
    StreamSource = IsStream;
    FrameIsAlwaysComplete = false;
}

//---------------------------------------------------------------------------
File_Apv::~File_Apv()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Apv::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "APV");

    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "APV");
}

//---------------------------------------------------------------------------
void File_Apv::Streams_Fill()
{
    Fill(Stream_Video, 0, Video_Format_Profile, APV_Profile(profile_idc));
    Fill(Stream_Video, 0, Video_Format_Level, (float)level_idc / 30, 1);
    Fill(Stream_Video, 0, "band_idc", band_idc);
    Fill(Stream_Video, 0, Video_Width, frame_width);
    Fill(Stream_Video, 0, Video_Height, frame_height);
    Fill(Stream_Video, 0, Video_ChromaSubsampling, APV_Chroma(chroma_format_idc));
    Fill(Stream_Video, 0, Video_BitDepth, bit_depth_minus8 + 8);
    if (color_description_present_flag) {
        Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(color_primaries));
        Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(transfer_characteristics));
        Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(matrix_coefficients));
        Fill(Stream_Video, 0, Video_colour_range, full_range_flag ? "Full" : "Limited");
    }
}

//---------------------------------------------------------------------------
void File_Apv::Streams_Finish()
{
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
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Apv::FileHeader_Begin()
{
    //Must have enough buffer for having header
    if (Buffer_Size < 8)
        return false; //Must wait for more data

    if (!IsSub
        && CC4(Buffer + 4) != 0x61507631) // signature = aPv1
    {
        Reject();
        return false;
    }

    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Apv::Read_Buffer_OutOfBand()
{
    Element_Begin1("APVDecoderConfigurationRecord");
    int8u number_of_configuration_entry, number_of_frame_info;
    Skip_B1(                                                    "configurationVersion");
    Get_B1 (number_of_configuration_entry,                      "number_of_configuration_entry");
    for (int8u i = 0; i < number_of_configuration_entry; ++i) {
        Element_Begin1("configuration_entry");
        Skip_B1(                                                "pbu_type");
        Get_B1 (number_of_frame_info,                           "number_of_frame_info");
        for (int8u j = 0; j < number_of_frame_info; ++j) {
            Element_Begin1("frame_info");
            BS_Begin();
            Skip_S1(6,                                          "reserved_zero_6bits");
            Get_SB (color_description_present_flag,             "color_description_present_flag");
            Skip_SB(                                            "capture_time_distance_ignored");
            BS_End();
            Get_B1 (profile_idc,                                "profile_idc");                 Param_Info1(APV_Profile(profile_idc));
            Get_B1 (level_idc,                                  "level_idc");                   Param_Info3((float)level_idc / 30, nullptr, 1);
            Get_B1 (band_idc,                                   "band_idc");
            Skip_B4(                                            "frame_width");
            Skip_B4(                                            "frame_height");
            BS_Begin();
            Get_S1 (4, chroma_format_idc,                       "chroma_format_idc");           Param_Info1(APV_Chroma(chroma_format_idc));
            Get_S1 (4, bit_depth_minus8,                        "bit_depth_minus8");
            BS_End();
            Skip_B1(                                            "capture_time_distance");
            if (color_description_present_flag) {
                Get_B1 (color_primaries,                        "color_primaries");             Param_Info1(Mpegv_colour_primaries(color_primaries));
                Get_B1 (transfer_characteristics,               "transfer_characteristics");    Param_Info1(Mpegv_transfer_characteristics(transfer_characteristics));
                Get_B1 (matrix_coefficients,                    "matrix_coefficients");         Param_Info1(Mpegv_matrix_coefficients(matrix_coefficients));
                BS_Begin();
                Get_SB (full_range_flag,                        "full_range_flag");
                Skip_S1(7,                                      "reserved_zero_7bits");
                BS_End();
            }
            Element_End0();
        }
        Element_End0();
    }
    Element_End0();

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Apv::Header_Parse()
{
    //Parsing
    DataMustAlwaysBeComplete = Element_Level > 3;
    if (Element_Level == 2) {
        Get_B4(au_size,                                         "au_size");
        Header_Fill_Size(au_size + Element_Offset);
        Header_Fill_Code(0, "raw_bitstream_access_unit");
        return;
    }
    if (Element_Level == 3) {
        int32u signature;
        Get_C4(signature,                                       "signature");
        if (signature != 0x61507631) Reject();
        Header_Fill_Size(au_size);
        Header_Fill_Code(0, "access_unit");
        return;
    }

    int32u pbu_size;
    int8u pbu_type;
    Get_B4 (pbu_size,                                           "pbu_size");
    Element_Begin1(                                             "pbu_header");
    Get_B1 (pbu_type,                                           "pbu_type");
    Skip_B2(                                                    "group_id");
    Skip_B1(                                                    "reserved_zero_8bits");
    Element_End0();

    FILLING_BEGIN();
    Header_Fill_Size(pbu_size + 4LL);
    Header_Fill_Code(pbu_type, Apv_pbu_type(pbu_type).c_str());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Apv::Data_Parse()
{
    if (Element_Level <= 2)
    {
        Element_ThisIsAList();
        return;
    }

    if ((1 <= Element_Code && Element_Code <= 2) ||
        (25 <= Element_Code && Element_Code <= 27))
        frame();
    else if (Element_Code == 65)
        au_info();
    else if (Element_Code == 66)
        metadata();
    else if (Element_Code == 67)
        filler();
    else
        Skip_XX(Element_Size,                                   "Data");
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Apv::frame()
{
    frame_header();
    for (int32u i = 0; i < NumTiles; ++i) {
        int32u tile_size;
        Get_B4(tile_size,                                       "tile_size[i]");
        auto Element_Size_Save = Element_Size;
        Element_Size = Element_Offset + tile_size;
        tile();
        Element_Size = Element_Size_Save;
    }
    filler();

    FILLING_BEGIN_PRECISE();
    Accept();
    FILLING_ELSE();
    Reject();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Apv::frame_header()
{
    Element_Begin1("frame_header");
    frame_info();
    Skip_B1 (                                                   "reserved_zero_8bits");
    BS_Begin();
    TEST_SB_GET(color_description_present_flag,                 "color_description_present_flag");
        Get_S1 ( 8, color_primaries,                            "color_primaries");             Param_Info1(Mpegv_colour_primaries(color_primaries));
        Get_S1 ( 8, transfer_characteristics,                   "transfer_characteristics");    Param_Info1(Mpegv_transfer_characteristics(transfer_characteristics));
        Get_S1 ( 8, matrix_coefficients,                        "matrix_coefficients");         Param_Info1(Mpegv_matrix_coefficients(matrix_coefficients));
        Get_SB (    full_range_flag,                            "full_range_flag");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "use_q_matrix");
    quantization_matrix();
    TEST_SB_END();
    tile_info();
    Skip_S1 ( 8,                                                "reserved_zero_8bits");
    byte_alignment();
    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::frame_info()
{
    Element_Begin1("frame_info");
    Get_B1 (    profile_idc,                                    "profile_idc");         Param_Info1(APV_Profile(profile_idc));
    Get_B1 (    level_idc,                                      "level_idc");           Param_Info3((float)level_idc / 30, nullptr, 1);
    BS_Begin();
    Get_S1 ( 3, band_idc,                                       "band_idc");
    Skip_S1( 5,                                                 "reserved_zero_5bits");
    BS_End();
    Get_B3 (    frame_width,                                    "frame_width");
    Get_B3 (    frame_height,                                   "frame_height");
    BS_Begin();
    Get_S1 ( 4, chroma_format_idc,                              "chroma_format_idc");   Param_Info1(APV_Chroma(chroma_format_idc));
    Get_S1 ( 4, bit_depth_minus8,                               "bit_depth_minus8");
    BS_End();
    Skip_B1(                                                    "capture_time_distance");
    Skip_B1(                                                    "reserved_zero_8bits");
    Element_End0();

    switch (chroma_format_idc) {
    case 0: NumComps = 1; break;
    case 2: NumComps = 3; break;
    case 3: NumComps = 3; break;
    case 4: NumComps = 4; break;
    default: NumComps = 0; break;
    }
}

//---------------------------------------------------------------------------
void File_Apv::quantization_matrix()
{
    Element_Begin1("quantization_matrix");
    for (int8u i = 0; i < NumComps; ++i) {
        for (int8u y = 0; y < 8; ++y) {
            for (int8u x = 0; x < 8; ++x) {
                Skip_S1(8,                                      "q_matrix[i][x][y]");
            }
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::tile_info()
{
    Element_Begin1("tile_info");
    int32u tile_width_in_mbs, tile_height_in_mbs;
    Get_S4(20, tile_width_in_mbs,                               "tile_width_in_mbs");
    Get_S4(20, tile_height_in_mbs,                              "tile_height_in_mbs");
    int32u FrameWidthInMbsY = static_cast<int32u>(ceil(frame_width / 16));
    int32u FrameHeightInMbsY = static_cast<int32u>(ceil(frame_height / 16));
    int32u tileCols = 0;
    int32u tileRows = 0;
    // Calculate number of columns
    for (int32u startMb = 0; startMb < FrameWidthInMbsY; startMb += tile_width_in_mbs) {
        ++tileCols;
    }

    // Calculate number of rows
    for (int32u startMb = 0; startMb < FrameHeightInMbsY; startMb += tile_height_in_mbs) {
        ++tileRows;
    }
    NumTiles = tileCols * tileRows;
    TEST_SB_SKIP(                                               "tile_size_present_in_fh_flag");
    for (int32u i = 0; i < NumTiles; ++i) {
        Skip_S4(32,                                             "tile_size_in_fh[i]");
    }
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::au_info()
{
    Element_Begin1("au_info");
    int16u num_frames;
    Get_B2 (num_frames,                                         "num_frames");
    for (int16u i = 0; i < num_frames; ++i) {
        Skip_B1(                                                "pbu_type");
        Skip_B2(                                                "group_id");
        Skip_B1(                                                "reserved_zero_8bits");
        frame_info();
    }
    Skip_B1(                                                    "reserved_zero_8bits");
    byte_alignment();
    filler();
    Element_End0();

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Apv::metadata()
{
    Element_Begin1("metadata");
    int32u metadata_size;
    Get_B4(metadata_size,                                       "metadata_size");
    auto Element_Size_Save = Element_Size;
    if (Element_Offset + metadata_size <= Element_Size)
        Element_Size = Element_Offset + metadata_size;
    else
        Trusted_IsNot("Size is wrong (metadata_size)");
    do {
        int64u payloadType{ 0 };
        while (Element_Offset < Element_Size && Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
            Skip_B1(                                            "ff_byte");
            payloadType += 0xFF;
        }
        int8u metadata_payload_type;
        Get_B1(metadata_payload_type,                           "metadata_payload_type");
        payloadType += metadata_payload_type;

        int64u payloadSize{ 0 };
        while (Element_Offset < Element_Size && Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
            Skip_B1(                                            "ff_byte");
            payloadSize += 0xFF;
        }
        int8u metadata_payload_size;
        Get_B1(metadata_payload_size,                           "metadata_payload_size");
        payloadSize += metadata_payload_size;
        metadata_payload(payloadType, payloadSize);
    } while (Element_Offset < Element_Size);
    Element_Size = Element_Size_Save;
    filler();
    Element_End0();

    FILLING_BEGIN_PRECISE();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Apv::filler()
{
    Element_Begin1("filler");
    while (Element_Offset < Element_Size && Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
        ++Element_Offset;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::tile()
{
    vector<int32u> tile_data_sizes;
    Element_Begin1("tile");
    Element_Begin1("tile_header");
    auto Element_Offset_begin = Element_Offset;
    int16u tile_header_size;
    Get_B2 (tile_header_size,                                   "tile_header_size");
    Skip_B2(                                                    "tile_index");
    for (int8u i = 0; i < NumComps; ++i) {
        int32u tile_data_size;
        Get_B4(tile_data_size,                                  "tile_data_size[i]");
        tile_data_sizes.push_back(tile_data_size);
    }
    for (int8u i = 0; i < NumComps; ++i) {
        Skip_B1(                                                "tile_qp[i]");
    }
    Skip_B1(                                                    "reserved_zero_8bits");
    byte_alignment();
    if (Element_Offset_begin + tile_header_size != Element_Offset)
        Trusted_IsNot("Size is wrong (tile_header_size)");
    Element_End0();
    for (int8u i = 0; i < NumComps; ++i) {
        Skip_XX(tile_data_sizes.at(i),                          "tile_data");
    }
    Skip_XX(Element_Size - Element_Offset,                      "tile_dummy_byte");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::byte_alignment()
{
    Element_Begin1("byte_alignment");
    while (BS->Remain() & 7)
        Mark_0();// alignment_bit_equal_to_zero
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_payload(int64u payloadType, int64u payloadSize)
{
    Element_Begin1("metadata_payload");
    auto Element_Size_Save = Element_Size;
    if (Element_Offset + payloadSize <= Element_Size)
        Element_Size = Element_Offset + payloadSize;
    else
        Trusted_IsNot("Size is wrong (payloadSize)");
    switch (payloadType) {
    case   4: metadata_itu_t_t35(); break;
    case   5: metadata_mdcv(); break;
    case   6: metadata_cll(); break;
    case  10: metadata_filler(); break;
    case 170: metadata_user_defined(); break;
    default: metadata_undefined(); break;
    }
    Skip_XX(Element_Size - Element_Offset, "(Not parsed)");
    Element_Size = Element_Size_Save;
    byte_alignment();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_filler()
{
    Element_Begin1("metadata_filler");
    while (Element_Offset < Element_Size && Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
        ++Element_Offset;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_itu_t_t35()
{
    Element_Begin1("metadata_itu_t_t35");
    // TODO: Use ITU T35 parser 
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_mdcv()
{
    Element_Begin1("metadata_mdcv");
    auto& HDR_Format = HDR[Video_HDR_Format][HdrFormat_SmpteSt2086];
    if (HDR_Format.empty())
    {
        HDR_Format = __T("SMPTE ST 2086");
        HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt2086] = "HDR10";
    }
    Get_MasteringDisplayColorVolume(HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_SmpteSt2086], HDR[Video_MasteringDisplay_Luminance][HdrFormat_SmpteSt2086], true);
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_cll()
{
    Element_Begin1("metadata_cll");
    Get_LightLevel(maximum_content_light_level, maximum_frame_average_light_level);
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_user_defined()
{
    Element_Begin1("metadata_user_defined");
    Skip_UUID(                                                  "uuid");
    Skip_XX(Element_Size - Element_Offset,                      "user_defined_data_payload");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Apv::metadata_undefined()
{
    Element_Begin1("metadata_undefined");
    Skip_XX(Element_Size - Element_Offset,                      "undefined_metadata_payload_byte");
    Element_End0();
}


//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_APV_YES
