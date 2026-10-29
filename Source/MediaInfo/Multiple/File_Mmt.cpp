/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_MMTTLV_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Mmt.h"
#include "MediaInfo/Multiple/File_Mmt_Descriptors.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Local helpers (temporary; consolidated into a shared header when the
// MPT/MH-EIT tables migrate here from File_MmtTlv)
//***************************************************************************

namespace
{
    const int8u TABLE_MPT = 0x20; // MPT: table_id(8) version(8) length(16)

    //MMT-SI table_id -> name (ARIB STD-B60 Table 4-8); "" if unrecognized.
    const char* Mmt_Table_Name(int8u table_id)
    {
        switch (table_id)
        {
            case 0x00 : return "PA";
            case 0x01 : return "MPI (subset 0)";
            case 0x10 : return "MPI (complete)";
            case 0x20 : return "MPT";
            case 0x21 : return "CRI";
            case 0x22 : return "DCI";
            case 0x80 : return "PLT";
            case 0x81 : return "LCT";
            case 0x86 : return "CAT (MH)";
            case 0x8B : return "MH-EIT";
            case 0x9C : return "MH-AIT";
            case 0x9D : return "MH-BIT";
            case 0x9E : return "MH-SDTT";
            case 0x9F : return "MH-SDT";
            case 0xA0 : return "MH-SDT (other stream)";
            case 0xA1 : return "MH-TOT";
            case 0xA2 : return "MH-CDT";
            case 0xA3 : return "DDM Table";
            case 0xA4 : return "DAM Table";
            case 0xA5 : return "DCC Table";
            case 0xA6 : return "EMT";
            case 0xA7 : return "MH-DIT";
            case 0xA8 : return "MH-SIT";
            default   : break;
        }
        if (table_id >= 0x02 && table_id <= 0x0F) return "MPI (subset)";
        if (table_id >= 0x11 && table_id <= 0x1F) return "MPT (subset)";
        if (table_id >= 0x82 && table_id <= 0x83) return "ECM";
        if (table_id >= 0x84 && table_id <= 0x85) return "EMM";
        if (table_id >= 0x87 && table_id <= 0x88) return "DCM";
        if (table_id >= 0x89 && table_id <= 0x8A) return "DMM";
        if (table_id >= 0x8C && table_id <= 0x9B) return "MH-EIT (schedule)";
        return "";
    }

    //running_status meaning (ARIB STD-B60 Table 7-19); "" if not annotated.
    const char* Mmt_running_status(int8u c)
    {
        switch (c)
        {
            case 1 : return "In non-operation";
            case 2 : return "It will start within several seconds";
            case 3 : return "Out of operation";
            case 4 : return "In operation";
            default: return "";
        }
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mmt::FileHeader_Parse()
{
    Accept();
}

//***************************************************************************
// Buffer - Per element (one signaling table)
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mmt::Header_Parse()
{
    //Frame one table. MPT carries a 16-bit length after an 8-bit version; the
    //M2 section tables (MH-EIT/SDT/TOT) carry the 12-bit section_length.
    int8u table_id;
    Get_B1 (table_id,                                           "table_id");
    int64u Size;
    if (table_id == TABLE_MPT)
    {
        int16u length;
        Get_B1 (Table_Version,                                  "version");
        Get_B2 (length,                                         "length");
        Size = (int64u)4 + length;
    }
    else
    {
        int16u section_length;
        BS_Begin();
        Skip_SB(                                                "section_syntax_indicator");
        Skip_SB(                                                "reserved_future_use");
        Skip_S1(2,                                              "reserved");
        Get_S2 (12, section_length,                             "section_length");
        BS_End();
        Size = (int64u)3 + section_length;
    }
    if (Element_Size && Size > Element_Size)
        Size = Element_Size; // a truncated final table: parse against what we have
    Header_Fill_Size(Size);
    Header_Fill_Code(table_id, Ztring().From_Number(table_id, 16));
}

//---------------------------------------------------------------------------
void File_Mmt::Data_Parse()
{
    switch (Element_Code)
    {
        case 0x20 : Element_Name("MPT");    Mpt();  break; // asset -> codec map
        case 0x8B : Element_Name("MH-EIT"); MhEit(); break; // present event
        case 0x9F : Element_Name("MH-SDT"); MhSdt(); break; // service (channel) name
        case 0xA1 : Element_Name("MH-TOT"); MhTot(); break; // current JST clock
        default   :
            {
                //Named-but-unparsed: recognized in the trace, body left opaque.
                const char* Name = Mmt_Table_Name((int8u)Element_Code);
                if (Name[0])
                    Element_Name(Name);
                Skip_XX(Element_Size - Element_Offset, "Data");
            }
            break;
    }
}

//***************************************************************************
// Tables
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mmt::MhTot()
{
    //JST_time (40): MJD(16) + BCD HHMMSS(24), then a descriptor loop and a CRC
    //we do not verify.
    if (Element_Size - Element_Offset < 5)
    {
        Skip_XX(Element_Size - Element_Offset,                  "Data");
        return;
    }
    int16u mjd;
    int32u hms;
    Get_B2 (mjd,                                                "MJD");
    Get_B3 (hms,                                                "JST_time (BCD)");
    if (Element_Size - Element_Offset >= 2)
    {
        int16u dll;
        BS_Begin();
        Skip_S1(4,                                              "reserved");
        Get_S2 (12, dll,                                        "descriptors_loop_length");
        BS_End();
        if (Element_Offset + dll > Element_Size)
            dll = (int16u)(Element_Size - Element_Offset);
        if (dll)
            Skip_XX(dll,                                        "descriptors");
    }
    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "CRC32");

    int64s utc = Mmt_DateTime_To_Seconds(mjd, hms); // JST
    if (utc < 0 || !Complete_Stream)
        return;
    Complete_Stream->TotUtc  = utc - Mmt_JST_Offset_Seconds;
    Complete_Stream->TotSeen = true;
}

//---------------------------------------------------------------------------
void File_Mmt::Mpt()
{
    //MPT body: MPT_mode(8) MMT_package_id_length(8) id MPT_descriptors_length(16)
    //descriptors number_of_assets(8), then per asset: identifier_type(8)
    //asset_id_scheme(32) asset_id_length(8) id asset_type(32,LE) clock_flag(8)
    //location_count(8) locations asset_descriptors_length(16) descriptors.
    if (!Complete_Stream || Element_Size - Element_Offset < 2)
        return;
    int8u pkg_id_len, number_of_assets;
    int16u mpt_desc_len;
    BS_Begin();
    Skip_S1(6,                                                  "reserved");
    Skip_S1(2,                                                  "MPT_mode");
    BS_End();
    Get_B1 (pkg_id_len,                                         "MMT_package_id_length");
    if (Element_Offset + pkg_id_len > Element_Size) return;
    Skip_XX(pkg_id_len,                                         "MMT_package_id");
    if (Element_Size - Element_Offset < 2) return;
    Get_B2 (mpt_desc_len,                                       "MPT_descriptors_length");
    if (Element_Offset + mpt_desc_len > Element_Size) return;
    if (mpt_desc_len)
    {
        //Package-level descriptors: not extracted, but named in the trace.
        File_Mmt_Descriptors Desc;
        Desc.Complete_Stream = Complete_Stream;
        Element_Begin1("MPT_descriptors");
        Open_Buffer_Init(&Desc);
        Open_Buffer_Continue(&Desc, mpt_desc_len);
        Element_End0();
    }
    if (Element_Size - Element_Offset < 1) return;
    Get_B1 (number_of_assets,                                   "number_of_assets");

    Complete_Stream->TransferLast = 0; //newest wins within this MPT
    std::vector<asset> Found;
    for (int i = 0; i < number_of_assets && Element_Offset < Element_Size; ++i)
    {
        //A mid-asset shortfall stops the loop with prior assets intact.
        Element_Begin1("asset");
        if (Element_Size - Element_Offset < 6) { Element_End0(); break; }
        int8u asset_id_len, location_count;
        int32u asset_type;
        Skip_B1(                                                "identifier_type");
        Skip_B4(                                                "asset_id_scheme");
        Get_B1 (asset_id_len,                                   "asset_id_length");
        if (Element_Offset + asset_id_len > Element_Size) { Element_End0(); break; }
        Skip_XX(asset_id_len,                                   "asset_id");
        if (Element_Size - Element_Offset < 4) { Element_End0(); break; }
        Get_L4 (asset_type,                                     "asset_type"); //FourCC (little-endian)
        if (Element_Size - Element_Offset < 1) { Element_End0(); break; }
        BS_Begin();
        Skip_S1(7,                                              "reserved");
        Skip_SB(                                                "asset_clock_relation_flag");
        BS_End();
        if (Element_Size - Element_Offset < 1) { Element_End0(); break; }
        Get_B1 (location_count,                                 "location_count");

        int16u packet_id = 0;
        bool   packet_id_set = false;
        for (int j = 0; j < location_count && Element_Offset < Element_Size; ++j)
        {
            int8u location_type;
            Get_B1 (location_type,                              "location_type");
            size_t loc_len;
            switch (location_type)
            {
                case 0x00: loc_len = 2; break;           //packet_id(16)
                case 0x01: loc_len = 4 + 2 + 2; break;   //ipv4 (approx)
                case 0x02: loc_len = 16 + 16 + 2; break; //ipv6 (approx)
                case 0x05: loc_len = 2 + 2 + 2; break;   //url-ish (best-effort)
                default:   loc_len = 0; break;
            }
            if (Element_Offset + loc_len > Element_Size)
                break;
            if (location_type == 0x00 && !packet_id_set)
            {
                Get_B2 (packet_id,                              "packet_id");
                packet_id_set = true;
                if (loc_len > 2)
                    Skip_XX(loc_len - 2,                        "location");
            }
            else if (loc_len)
                Skip_XX(loc_len,                                "location");
        }

        if (Element_Size - Element_Offset < 2) { Element_End0(); break; }
        int16u asset_desc_len;
        Get_B2 (asset_desc_len,                                 "asset_descriptors_length");
        if (Element_Offset + asset_desc_len > Element_Size) { Element_End0(); break; }

        asset a;
        a.Type     = asset_type;
        a.PacketId = packet_id;
        if (asset_desc_len)
        {
            File_Mmt_Descriptors Desc;
            Desc.Complete_Stream = Complete_Stream;
            Desc.CurrentAsset    = &a;
            Element_Begin1("asset_descriptors");
            Open_Buffer_Init(&Desc);
            Open_Buffer_Continue(&Desc, asset_desc_len);
            Element_End0();
        }
        Found.push_back(a);
        Element_End0();
    }

    Complete_Stream->Assets     = Found;
    Complete_Stream->MptVersion = Table_Version;
    Complete_Stream->MptValid   = true;
}

//---------------------------------------------------------------------------
void File_Mmt::MhEit()
{
    //The present event leads section 0 (current_next==1). The container keeps
    //the boundary-window/hop logic; here we only extract fields.
    if (!Complete_Stream || Element_Size - Element_Offset < 11)
        return;
    int16u service_id, tlv_stream_id;
    int8u  section_number;
    bool   current_next_indicator;
    Get_B2 (service_id,                                         "service_id");
    BS_Begin();
    Skip_S1(2,                                                  "reserved");
    Skip_S1(5,                                                  "version_number");
    Get_SB (current_next_indicator,                             "current_next_indicator");
    BS_End();
    Get_B1 (section_number,                                     "section_number");
    Skip_B1(                                                    "last_section_number");
    Get_B2 (tlv_stream_id,                                      "TLV_stream_id");
    Skip_B2(                                                    "original_network_id");
    Skip_B1(                                                    "segment_last_section_number");
    Skip_B1(                                                    "last_table_id");
    Complete_Stream->EitSvcId      = service_id;
    Complete_Stream->EitSvcIdFound = true;
    Complete_Stream->TlvStreamId   = tlv_stream_id;

    if (section_number != 0 || !current_next_indicator)
        return;

    int64u End = Element_Size >= 4 ? Element_Size - 4 : Element_Size; // drop CRC32
    if (Element_Offset + 12 > End)
        return;
    Element_Begin1("event");
    int16u event_id, start_date, dll;
    int32u start_time, duration;
    Get_B2 (event_id,                                           "event_id");
    Get_B2 (start_date,                                         "start_time (MJD)");
    Get_B3 (start_time,                                         "start_time (BCD HHMMSS)");
    Get_B3 (duration,                                           "duration (BCD HHMMSS)");
    int8u running_status;
    BS_Begin();
    Get_S1 (3, running_status,                                  "running_status"); Param_Info1(Mmt_running_status(running_status));
    Skip_SB(                                                    "free_CA_mode");
    Get_S2 (12, dll,                                            "descriptors_loop_length");
    BS_End();
    int16u desc_loop_len = dll;
    if (Element_Offset + desc_loop_len > End)
        desc_loop_len = (int16u)(End - Element_Offset);
    if (desc_loop_len)
    {
        File_Mmt_Descriptors Desc;
        Desc.Complete_Stream = Complete_Stream; //short_event writes name/text into mmt_stream
        Element_Begin1("descriptors");
        Open_Buffer_Init(&Desc);
        Open_Buffer_Continue(&Desc, desc_loop_len);
        Element_End0();
    }
    Element_End0();

    Complete_Stream->EitEventId   = event_id;
    Complete_Stream->EitStartDate = start_date;
    Complete_Stream->EitStartTime = start_time;
    Complete_Stream->EitDuration  = duration;
    Complete_Stream->EitParsed    = true;
}

//---------------------------------------------------------------------------
void File_Mmt::MhSdt()
{
    //SDT header (8 bytes) then a services loop, then a trailing CRC32. The
    //service is matched to the present-EIT service_id when the container knows
    //it, else the first service with a name wins.
    if (Element_Size - Element_Offset < 8 + 4)
    {
        Skip_XX(Element_Size - Element_Offset,                  "Data");
        return;
    }
    int16u tlv_stream_id;
    Get_B2 (tlv_stream_id,                                      "TLV_stream_id");
    BS_Begin();
    Skip_S1(2,                                                  "reserved");
    Skip_S1(5,                                                  "version_number");
    Skip_SB(                                                    "current_next_indicator");
    BS_End();
    Skip_B1(                                                    "section_number");
    Skip_B1(                                                    "last_section_number");
    Skip_B2(                                                    "original_network_id");
    Skip_B1(                                                    "reserved_future_use");
    if (Complete_Stream)
        Complete_Stream->TlvStreamId = tlv_stream_id;

    int64u End = Element_Size >= 4 ? Element_Size - 4 : Element_Size; // exclude CRC32
    while (Element_Offset + 5 <= End)
    {
        Element_Begin1("service");
        int16u service_id, dll_word;
        Get_B2 (service_id,                                     "service_id");
        int8u running_status;
        BS_Begin();
        Skip_S1(3,                                              "reserved_future_use");
        Skip_S1(3,                                              "EIT_user_defined_flags");
        Skip_SB(                                                "EIT_schedule_flag");
        Skip_SB(                                                "EIT_present_following_flag");
        Get_S1 (3, running_status,                              "running_status"); Param_Info1(Mmt_running_status(running_status));
        Skip_SB(                                                "free_CA_mode");
        Get_S2 (12, dll_word,                                   "descriptors_loop_length");
        BS_End();
        int16u desc_len = dll_word;
        if (Element_Offset + desc_len > End)
            desc_len = (int16u)(End - Element_Offset);

        bool take = Complete_Stream && (Complete_Stream->EitServiceIdFound
                    ? (service_id == Complete_Stream->EitServiceId) : true);
        if (take && desc_len)
        {
            File_Mmt_Descriptors Desc;
            Desc.Complete_Stream = Complete_Stream;
            Element_Begin1("descriptors");
            Open_Buffer_Init(&Desc);
            Open_Buffer_Continue(&Desc, desc_len);
            Element_End0();
        }
        else if (desc_len)
            Skip_XX(desc_len,                                   "descriptors");
        Element_End0();

        if (take && Complete_Stream && !Complete_Stream->ServiceName.empty())
        {
            Complete_Stream->SdtFound = true;
            break;
        }
    }
    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "CRC32");
}

} //NameSpace

#endif //MEDIAINFO_MMTTLV_YES
