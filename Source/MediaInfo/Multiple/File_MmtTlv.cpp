/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// MMT protocol over TLV packets (MMT/TLV): an MMTP stream (MPEG Media
// Transport, ISO/IEC 23008-1) framed in TLV packets (ARIB STD-B32) to form a
// stored container. Japanese ISDB-S3 BS/CS 4K/8K broadcasting.
//
// Per-element synchronization re-syncs a stream that does not begin on a
// TLV sync byte.
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
#if defined(MEDIAINFO_MMTTLV_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_MmtTlv.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

namespace
{
    // TLV packet types
    const int8u TLV_HEADER_BYTE                 = 0x7F;
    const int8u TLV_IPV4_PACKET                 = 0x01;
    const int8u TLV_IPV6_PACKET                 = 0x02;
    const int8u TLV_HEADER_COMPRESSED_IP_PACKET = 0x03;
    const int8u TLV_TRANSMISSION_CONTROL_PACKET = 0xFE;
    const int8u TLV_NULL_PACKET                 = 0xFF;

    // Header-compression context identifiers
    const int8u CID_PARTIAL_IPV4_PARTIAL_UDP    = 0x20;
    const int8u CID_IPV4_HEADER                 = 0x21;
    const int8u CID_PARTIAL_IPV6_PARTIAL_UDP    = 0x60;
    const int8u CID_NO_COMPRESSED_HEADER        = 0x61;

    // Compressed-IP partial-header skip lengths: the compressed form
    // drops the first 4 UDP and first 2 IPv6 bytes.
    const size_t PARTIAL_UDP_HEADER_LENGTH      = 8 - 4;   // 4
    const size_t PARTIAL_IPV6_HEADER_LENGTH     = 40 - 2;  // 38

    // MMTP payload types
    const int8u MMTP_PAYLOAD_SIGNALING          = 0x02;

    // Signaling message ids
    const int16u MSG_PA_MESSAGE                 = 0x0000;
    const int16u MSG_M2_SECTION                 = 0x8000;

    // Table ids
    const int8u TABLE_MPT                       = 0x20; // MMT Package Table
    const int8u TABLE_MH_EIT                    = 0x8B; // MH-EIT[p/f]

    // Descriptor tags
    const int16u DESC_MH_SHORT_EVENT            = 0xF001;

    // asset_type FourCCs, read little-endian
    const int32u ASSET_HEV1                     = 0x31766568; // 'hev1'
    const int32u ASSET_MP4A                     = 0x6134706D; // 'mp4a'
    const int32u ASSET_STPP                     = 0x70707473; // 'stpp'

    // Stop scanning after this many packets, so a multi-GB file is not read
    // end-to-end when the present EIT never arrives.
    const int64u GIVE_UP_AFTER_PACKETS          = 200000;

    // Bounds checked by caller.
    inline int16u RB16(const int8u* p) { return ((int16u)p[0] << 8) | p[1]; }
    inline int32u RB24(const int8u* p)
    { return ((int32u)p[0] << 16) | ((int32u)p[1] << 8) | p[2]; }
    inline int32u RB32(const int8u* p)
    { return ((int32u)p[0] << 24) | ((int32u)p[1] << 16) | ((int32u)p[2] << 8) | p[3]; }
    inline int32u RL32(const int8u* p)
    { return ((int32u)p[3] << 24) | ((int32u)p[2] << 16) | ((int32u)p[1] << 8) | p[0]; }

    inline bool Is_Tlv_Type(int8u t)
    {
        return t == TLV_IPV4_PACKET || t == TLV_IPV6_PACKET
            || t == TLV_HEADER_COMPRESSED_IP_PACKET
            || t == TLV_TRANSMISSION_CONTROL_PACKET || t == TLV_NULL_PACKET;
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_MmtTlv::File_MmtTlv()
: File__Analyze()
{
    //Configuration
    MustSynchronize          = true;

    //Init
    Eit_Present_Found        = false;
    Eit_EventId              = 0;
    Eit_StartDate            = 0;
    Eit_StartTime            = 0;
    Eit_Duration             = 0;
    Mpt_Found                = false;
    Packets_Since_Accept     = 0;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_MmtTlv::Streams_Fill()
{
    //Format identification
    Fill(Stream_General, 0, General_Format, "MMT/TLV");
    Fill(Stream_General, 0, General_Format_Info, "MMT protocol over TLV packets (ARIB STD-B32)");

    //A/V/subtitle streams from the MPT asset list.
    for (size_t i = 0; i < Assets.size(); ++i)
    {
        const asset& A = Assets[i];
        switch (A.Type)
        {
            case ASSET_HEV1:
                Stream_Prepare(Stream_Video);
                Fill(Stream_Video, StreamPos_Last, Video_Format, "HEVC");
                Fill(Stream_Video, StreamPos_Last, Video_CodecID, "hev1");
                if (A.PacketId)
                    Fill(Stream_Video, StreamPos_Last, Video_ID, A.PacketId, 10);
                break;
            case ASSET_MP4A:
                Stream_Prepare(Stream_Audio);
                Fill(Stream_Audio, StreamPos_Last, Audio_Format, "AAC");
                Fill(Stream_Audio, StreamPos_Last, Audio_CodecID, "mp4a");
                if (A.PacketId)
                    Fill(Stream_Audio, StreamPos_Last, Audio_ID, A.PacketId, 10);
                break;
            case ASSET_STPP:
                Stream_Prepare(Stream_Text);
                Fill(Stream_Text, StreamPos_Last, Text_Format, "TTML");
                Fill(Stream_Text, StreamPos_Last, Text_CodecID, "stpp");
                if (A.PacketId)
                    Fill(Stream_Text, StreamPos_Last, Text_ID, A.PacketId, 10);
                Fill(Stream_Text, StreamPos_Last, Text_Format_Profile,
                     A.Superimpose ? "Superimpose" : "Subtitle");
                break;
            default:
                break;
        }
    }
}

//***************************************************************************
// Buffer - File header (format detection / probe)
//***************************************************************************

//---------------------------------------------------------------------------
bool File_MmtTlv::FileHeader_Begin()
{
    //Scan up to 100 TLV packets, require recognized
    //compressed-IP / NULL packets. Resync byte-wise (a stream may not start on a
    //sync byte).
    if (Buffer_Size < 188)
        return false;

    size_t i          = 0;
    int    processed  = 0;
    int    recognized = 0;

    while (i + 4 < Buffer_Size && processed < 100)
    {
        if (Buffer[i] != TLV_HEADER_BYTE)
        {
            ++i;
            continue;
        }
        ++processed;

        int8u  packet_type = Buffer[i + 1];
        int16u data_length = RB16(Buffer + i + 2);
        i += 4;

        if (packet_type == TLV_HEADER_COMPRESSED_IP_PACKET)
        {
            if (data_length >= 3 && i + 2 < Buffer_Size)
            {
                switch (Buffer[i + 2])
                {
                    case CID_PARTIAL_IPV4_PARTIAL_UDP:
                    case CID_IPV4_HEADER:
                    case CID_PARTIAL_IPV6_PARTIAL_UDP:
                    case CID_NO_COMPRESSED_HEADER:
                        ++recognized;
                }
            }
            i += data_length;
        }
        else if (packet_type == TLV_NULL_PACKET)
        {
            bool AllFF = true;
            for (size_t j = i; j < i + data_length && j < Buffer_Size; ++j)
                if (Buffer[j] != 0xFF) { AllFF = false; break; }
            if (AllFF)
                ++recognized;
            i += data_length;
        }
        else if (packet_type == TLV_IPV6_PACKET || packet_type == TLV_IPV4_PACKET)
        {
            i += data_length;
        }
        // else: unknown byte after a false 0x7F; keep scanning byte-wise.
    }

    //Ratio test rejects files that merely contain a stray 0x7F.
    if (processed >= 4 && recognized > 0
     && recognized * 100 / (processed < 10 ? 10 : processed) >= 25)
    {
        Accept("MmtTlv");
        return true;
    }

    //Not enough confidence yet. A stream may begin with a long non-TLV region;
    //keep requesting data until a bounded search budget is spent, then give up.
    const int64u DetectionBudget = 512 * 1024; // bytes from start of file
    if (File_Offset + Buffer_Size < DetectionBudget && File_Offset + Buffer_Size < File_Size)
        return false; //Wait for more data

    Reject("MmtTlv");
    return false;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_MmtTlv::Synchronize()
{
    //Sync byte followed by a known packet type.
    while (Buffer_Offset + 4 <= Buffer_Size
        && !(Buffer[Buffer_Offset] == TLV_HEADER_BYTE
          && Is_Tlv_Type(Buffer[Buffer_Offset + 1])))
        ++Buffer_Offset;

    if (Buffer_Offset + 4 > Buffer_Size)
        return false; //Need more data

    return true;
}

//---------------------------------------------------------------------------
bool File_MmtTlv::Synched_Test()
{
    if (Buffer_Offset + 4 > Buffer_Size)
        return false;

    if (!(Buffer[Buffer_Offset] == TLV_HEADER_BYTE
       && Is_Tlv_Type(Buffer[Buffer_Offset + 1])))
        Synched = false;

    return true;
}

//***************************************************************************
// Buffer - Per element (one TLV packet)
//***************************************************************************

//---------------------------------------------------------------------------
void File_MmtTlv::Header_Parse()
{
    //TLV header: sync(8)=0x7F packet_type(8) data_length(16).
    int8u  packet_type;
    int16u data_length;
    Skip_B1(                                                    "sync (0x7F)");
    Get_B1 (packet_type,                                        "packet_type");
    Get_B2 (data_length,                                        "data_length");

    Header_Fill_Size((int64u)4 + data_length);
    Header_Fill_Code(packet_type, "TLV packet");
}

//---------------------------------------------------------------------------
void File_MmtTlv::Data_Parse()
{
    ++Packets_Since_Accept;

    const int8u* Payload = Buffer + Buffer_Offset;
    size_t       Size    = (size_t)Element_Size;

    switch ((int8u)Element_Code)
    {
        case TLV_HEADER_COMPRESSED_IP_PACKET:
            if (!(Mpt_Found && Eit_Present_Found))
                Parse_CompressedIp(Payload, Size);
            break;
        default:
            break; // IPv6 (NTP), NULL, control: no naming/schedule info
    }

    Skip_XX(Element_Size, "Data");

    //Stop once we have both the codec map and the present program, or after a
    //bounded packet count (a stream may carry no in-band MPT and/or no present
    //EIT; do not read a multi-GB file end to end).
    if ((Mpt_Found && Eit_Present_Found)
     || Packets_Since_Accept >= GIVE_UP_AFTER_PACKETS)
        Finish();
}

//***************************************************************************
// Compressed-IP TLV payload -> strip the (partial) IPv6+UDP headers -> MMTP.
//***************************************************************************

//---------------------------------------------------------------------------
void File_MmtTlv::Parse_CompressedIp(const int8u* Data, size_t Size)
{
    if (Size < 3)
        return;
    //context_id(12) + type nibble = 2 bytes; 3rd byte = header type.
    int8u header_type = Data[2];
    const int8u* p    = Data + 3;
    size_t       n    = Size - 3;

    switch (header_type)
    {
        case CID_PARTIAL_IPV6_PARTIAL_UDP:
            if (n < PARTIAL_IPV6_HEADER_LENGTH + PARTIAL_UDP_HEADER_LENGTH)
                return;
            p += PARTIAL_IPV6_HEADER_LENGTH + PARTIAL_UDP_HEADER_LENGTH;
            n -= PARTIAL_IPV6_HEADER_LENGTH + PARTIAL_UDP_HEADER_LENGTH;
            break;
        case CID_NO_COMPRESSED_HEADER:
            break;
        default:
            return; // IPv4 variants not carried by MMT signaling here
    }

    Parse_Mmtp(p, n);
}

//***************************************************************************
// MMTP
//***************************************************************************

//---------------------------------------------------------------------------
// MMTP packet header layout.
void File_MmtTlv::Parse_Mmtp(const int8u* Data, size_t Size)
{
    if (Size < 12)
        return;

    int8u b0               = Data[0];
    bool  packet_counter   = (b0 >> 5) & 1;
    bool  extension_header = (b0 >> 1) & 1;
    int8u payload_type     = Data[1] & 0x3F;

    const int8u* p = Data + 2;
    size_t       n = Size - 2;

    // packet_id(16) + distribute_timestamp(32) + packet_sequence_number(32)
    if (n < 10)
        return;
    int16u packet_id  = RB16(p);
    int32u seq_num    = RB32(p + 6); // after 32-bit distribute_timestamp
    p += 10; n -= 10;

    if (packet_counter)
    {
        if (n < 4) return;
        p += 4; n -= 4;
    }
    if (extension_header)
    {
        if (n < 4) return;
        int16u ext_len = RB16(p + 2); // extension_header_type(16) + length(16)
        p += 4; n -= 4;
        if (n < ext_len) return;
        p += ext_len; n -= ext_len;
    }

    if (payload_type == MMTP_PAYLOAD_SIGNALING)
        Parse_SignalingMessages(packet_id, seq_num, p, n);
    // MPU (media) carries no naming/schedule info; ignored.
}

//---------------------------------------------------------------------------
// Reassembly keyed by packet_id; aggregation and fragmentation are mutually
// exclusive. A completed message goes to Parse_SignalingMessage.
void File_MmtTlv::Parse_SignalingMessages(int16u packet_id, int32u seq_num, const int8u* Data, size_t Size)
{
    if (Size < 2)
        return;

    int8u b                 = Data[0];
    int   fragmentation_ind = b >> 6;   // 0 complete, 1 first, 2 middle, 3 last
    bool  length_extension  = (b >> 1) & 1;
    bool  aggregation       = b & 1;

    const int8u* p = Data + 2; // fragmentation byte + fragment counter byte
    size_t       n = Size - 2;

    fragment_assembler& Ass = Assemblers[packet_id];

    //A seq jump or first-ever sighting drops any partial; a complete/first
    //fragment recovers the run.
    if (Ass.State == fragment_assembler::Init)
        Ass.State = fragment_assembler::Skip;
    else if (seq_num != Ass.LastSeq + 1)
    {
        Ass.Data.clear();
        Ass.State = fragment_assembler::Skip;
    }
    Ass.LastSeq = seq_num;

    if (!aggregation)
    {
        switch (fragmentation_ind)
        {
            case 0: // NOT_FRAGMENTED (complete)
                Ass.State = fragment_assembler::NotStarted;
                Ass.Data.clear();
                Parse_SignalingMessage(p, n);
                break;
            case 1: // FIRST
                Ass.State = fragment_assembler::InFragment;
                Ass.Data.assign(p, p + n);
                break;
            case 2: // MIDDLE
                if (Ass.State == fragment_assembler::InFragment)
                    Ass.Data.insert(Ass.Data.end(), p, p + n);
                break;
            case 3: // LAST
                if (Ass.State == fragment_assembler::InFragment)
                {
                    Ass.Data.insert(Ass.Data.end(), p, p + n);
                    if (!Ass.Data.empty())
                        Parse_SignalingMessage(&Ass.Data[0], Ass.Data.size());
                }
                Ass.Data.clear();
                Ass.State = fragment_assembler::NotStarted;
                break;
            default:
                break;
        }
        return;
    }

    //Aggregated: length-prefixed COMPLETE messages.
    if (fragmentation_ind != 0)
        return;
    Ass.State = fragment_assembler::NotStarted;
    Ass.Data.clear();
    while (n > 0)
    {
        int32u length;
        if (length_extension)
        {
            if (n < 4) return;
            length = RB32(p); p += 4; n -= 4;
        }
        else
        {
            if (n < 2) return;
            length = RB16(p); p += 2; n -= 2;
        }
        if (length > n) return;
        Parse_SignalingMessage(p, (size_t)length);
        p += length; n -= length;
    }
}

//---------------------------------------------------------------------------
// PA message (0x0000, a table list) or M2 section (0x8000, one table).
void File_MmtTlv::Parse_SignalingMessage(const int8u* Data, size_t Size)
{
    if (Size < 4)
        return;
    int16u msg_id = RB16(Data);

    if (msg_id == MSG_PA_MESSAGE)
    {
        // id(16) version(8) length(32)
        if (Size < 7) return;
        int32u length = RB32(Data + 3);
        const int8u* body = Data + 7;
        if (length > Size - 7) length = (int32u)(Size - 7);
        size_t n = length;

        if (n < 1) return;
        int8u num_of_tables = body[0];
        const int8u* p = body + 1; n -= 1;
        // per-table index: table_id(8) table_version(8) table_length(16)
        for (int i = 0; i < num_of_tables && n >= 4; ++i) { p += 4; n -= 4; }
        // Concatenated tables. Length lives in each table header, width by table:
        //  - MPT (0x20): table_id(8) version(8) length(16) -> 4+len
        //  - M2 section tables: table_id(8) + 16-bit, low 12 = section_length
        //    -> 3+section_length
        while (n >= 4)
        {
            int8u  tid = p[0];
            size_t consumed;
            if (tid == TABLE_MPT)
                consumed = (size_t)4 + RB16(p + 2);
            else
                consumed = (size_t)3 + (RB16(p + 1) & 0x0FFF);
            if (consumed < 4)
                break;
            //Parse a truncated final table against the bytes we have.
            size_t take = consumed > n ? n : consumed;
            Parse_Table(p, take);
            if (consumed > n)
                break;
            p += consumed; n -= consumed;
        }
    }
    else if (msg_id == MSG_M2_SECTION)
    {
        // id(16) version(8) length(16)
        if (Size < 5) return;
        int16u length = RB16(Data + 3);
        const int8u* body = Data + 5;
        if (length > Size - 5) length = (int16u)(Size - 5);
        Parse_Table(body, length);
    }
}

//---------------------------------------------------------------------------
void File_MmtTlv::Parse_Table(const int8u* Data, size_t Size)
{
    if (Size < 1)
        return;
    switch (Data[0])
    {
        case TABLE_MPT:     Parse_Mpt(Data, Size);   break;
        case TABLE_MH_EIT:  Parse_MhEit(Data, Size); break;
        default: break;
    }
}

//***************************************************************************
// MMT Package Table: the asset -> codec map.
//***************************************************************************

//---------------------------------------------------------------------------
void File_MmtTlv::Parse_Mpt(const int8u* Data, size_t Size)
{
    if (Mpt_Found)
        return;

    // table_id(8) version(8) length(16)
    if (Size < 4) return;
    int16u length = RB16(Data + 2);
    const int8u* body = Data + 4;
    if (length > Size - 4) length = (int16u)(Size - 4);

    const int8u* p = body; size_t n = length;

    // MPT_mode(8) + MMT_package_id_length(8)
    if (n < 2) return;
    p += 1; n -= 1;
    int8u pkg_id_len = *p++; n -= 1;
    if (pkg_id_len > n) return;
    p += pkg_id_len; n -= pkg_id_len;

    // MPT_descriptors_length(16) + descriptors
    if (n < 2) return;
    int16u mpt_desc_len = RB16(p); p += 2; n -= 2;
    if (mpt_desc_len > n) return;
    p += mpt_desc_len; n -= mpt_desc_len;

    if (n < 1) return;
    int8u number_of_assets = *p++; n -= 1;

    //Collect locally, commit only once the whole list is read: the MPT recurs
    //~10x/sec, so a partial/corrupt parse returns and a later complete copy wins
    //-- no duplicate/partial streams.
    std::vector<asset> Found;
    for (int i = 0; i < number_of_assets && n > 0; ++i)
    {
        //A mid-asset shortfall stops the loop with prior assets intact (media
        //assets lead the list; trailing data assets run us out).
        // identifier_type(8) asset_id_scheme(32) asset_id_length(8)
        if (n < 6) break;
        p += 5; n -= 5;
        int8u asset_id_len = *p++; n -= 1;
        if (asset_id_len > n) break;
        p += asset_id_len; n -= asset_id_len;

        // asset_type(32, little-endian FourCC)
        if (n < 4) break;
        int32u asset_type = RL32(p); p += 4; n -= 4;

        // asset_clock_relation_flag(8)
        if (n < 1) break;
        p += 1; n -= 1;

        // location_count(8) + entries; take packet_id from the first type-0x00.
        if (n < 1) break;
        int8u  location_count = *p++; n -= 1;
        int16u packet_id      = 0;
        bool   packet_id_set  = false;
        for (int j = 0; j < location_count && n >= 1; ++j)
        {
            int8u location_type = *p++; n -= 1;
            size_t loc_len = 0;
            switch (location_type)
            {
                case 0x00: loc_len = 2; break;           // packet_id(16)
                case 0x01: loc_len = 4 + 2 + 2; break;   // ipv4 (approx)
                case 0x02: loc_len = 16 + 16 + 2; break; // ipv6 (approx)
                case 0x05: loc_len = 2 + 2 + 2; break;   // url-ish (best-effort)
                default:   loc_len = 0; break;
            }
            if (loc_len > n) { n = 0; break; }
            if (location_type == 0x00 && !packet_id_set)
            {
                packet_id     = RB16(p);
                packet_id_set = true;
            }
            p += loc_len; n -= loc_len;
        }

        // asset_descriptors_length(16) + descriptors.
        if (n < 2) break;
        int16u asset_desc_len = RB16(p); p += 2; n -= 2;
        if (asset_desc_len > n) break;

        bool superimpose = false;
        {
            const int8u* d = p; size_t dn = asset_desc_len;
            while (dn >= 3)
            {
                int16u dtag = RB16(d);
                int8u  dlen = d[2]; // 8-bit length for these MMT descriptor tags
                if ((size_t)3 + dlen > dn) break;
                if (dtag == 0x8020) // MH_DATA_COMPONENT_DESCRIPTOR
                {
                    const int8u* db = d + 3;
                    // data_component_id 0x0020 = 2nd-gen closed caption; then
                    // Additional_Arib_Subtitle_Info type (top 2 bits of body[5])
                    // == 0b01 is superimpose (ARIB STD-B60).
                    if (dlen >= 8 && RB16(db) == 0x0020)
                        superimpose = ((db[7] >> 6) & 0x03) == 0x01;
                }
                d += 3 + dlen; dn -= 3 + dlen;
            }
        }
        p += asset_desc_len; n -= asset_desc_len;

        asset a;
        a.Type        = asset_type;
        a.PacketId    = packet_id;
        a.Superimpose = superimpose;
        Found.push_back(a);
    }

    //Require a media asset before committing, so a stray/empty table doesn't latch.
    bool HasMedia = false;
    for (size_t i = 0; i < Found.size(); ++i)
        if (Found[i].Type == ASSET_HEV1 || Found[i].Type == ASSET_MP4A || Found[i].Type == ASSET_STPP)
        {
            HasMedia = true;
            break;
        }
    if (HasMedia)
    {
        Assets    = Found;
        Mpt_Found = true;
    }
}

//***************************************************************************
// MH-EIT[p/f]: present program name + schedule.
//***************************************************************************

//---------------------------------------------------------------------------
void File_MmtTlv::Parse_MhEit(const int8u* Data, size_t Size)
{
    if (Eit_Present_Found)
        return;
    // table_id(8) section_syntax+len(16); section_length = low 12 bits.
    if (Size < 3) return;
    int16u section_length = RB16(Data + 1) & 0x0FFF;
    if (section_length < 4)
        return;
    const int8u* body = Data + 3;

    // Drop the trailing CRC32; a reassembled fragment may be short, so clamp.
    // The present event leads the section.
    size_t n = (size_t)section_length - 4;
    size_t avail = Size - 3;
    if (n > avail)
        n = avail;
    const int8u* p = body;

    // service_id(16) reserved+version+current_next_indicator(8)
    // section_number(8) last_section_number(8) TLV_stream_id(16)
    // original_network_id(16) segment_last_section_number(8) last_table_id(8)
    if (n < 11) return;
    int8u version_byte   = p[2];
    int8u section_number = p[3];
    p += 11; n -= 11;

    // Present event is section 0 with current_next_indicator == 1.
    if (section_number != 0 || !(version_byte & 1))
        return;

    while (n > 0)
    {
        // event_id(16) start_time(40) duration(24) desc_loop_len(16, low12)
        if (n < 12) return;
        int16u event_id      = RB16(p);
        int16u start_date    = RB16(p + 2);      // MJD
        int32u start_time    = RB24(p + 4);      // BCD HHMMSS
        int32u duration      = RB24(p + 7);      // BCD HHMMSS
        int16u desc_loop_len = RB16(p + 10) & 0x0FFF;
        p += 12; n -= 12;
        if (desc_loop_len > n) return;

        Eit_EventId   = event_id;
        Eit_StartDate = start_date;
        Eit_StartTime = start_time;
        Eit_Duration  = duration;

        const int8u* d = p; size_t dn = desc_loop_len;
        while (dn >= 4)
        {
            int16u desc_tag = RB16(d);
            int16u desc_len = RB16(d + 2); // MH descriptors use a 16-bit length
            const int8u* dbody = d + 4;
            if (desc_len > dn - 4) break;

            if (desc_tag == DESC_MH_SHORT_EVENT && desc_len >= 4)
            {
                // ISO_639_language(24) event_name_length(8) event_name
                // text_length(16) text
                Eit_Language.From_UTF8(std::string((const char*)dbody, 3));
                int8u name_len = dbody[3];
                if (4 + (size_t)name_len <= desc_len)
                {
                    // Decrypted MMT SI text is UTF-8 (no ARIB STD-B24 decode).
                    Eit_EventName.From_UTF8(std::string((const char*)(dbody + 4), name_len));
                    // text_length is 16-bit here; an 8-bit read hits the high
                    // 0x00 byte and yields empty text.
                    size_t text_at = 4 + (size_t)name_len;
                    if (text_at + 2 <= desc_len)
                    {
                        int16u text_len = RB16(dbody + text_at);
                        if (text_at + 2 + (size_t)text_len <= desc_len)
                            Eit_EventText.From_UTF8(std::string((const char*)(dbody + text_at + 2), text_len));
                    }
                }
            }

            d += 4 + desc_len; dn -= 4 + desc_len;
        }

        p += desc_loop_len; n -= desc_loop_len;

        Eit_Present_Found = true; // present event only
        return;
    }
}

} //NameSpace

#endif //MEDIAINFO_MMTTLV_YES
