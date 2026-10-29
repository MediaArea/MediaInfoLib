/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// MMT protocol over TLV packets (MMT/TLV): an MMTP stream (MPEG Media
// Transport, ISO/IEC 23008-1) framed in TLV packets (ARIB STD-B32). Japanese
// ISDB-S3 BS/CS 4K/8K. Surfaces A/V/subtitle streams (MPT asset list) and,
// under Stream_Menu, the present event as a chapter (name, description,
// scheduled start and duration; MH-EIT[p/f], table_id 0x8B).
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_MmtTlvH
#define MediaInfo_File_MmtTlvH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/Multiple/File_Mmt.h" //asset, mmt_stream (shared with the signaling sub-parser)
#include <vector>
#include <map>
#include <set>
#include <memory>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class File__Analyze;

//***************************************************************************
// Class File_MmtTlv
//***************************************************************************

class File_MmtTlv : public File__Analyze
{
public :
    File_MmtTlv();

private :
    //Streams management
    void Streams_Fill() override;
    void Streams_Finish() override;

    //Buffer - File header (probe)
    bool FileHeader_Begin() override;

    //Buffer - Synchro (per-TLV-packet synchronization; gives free resync)
    bool Synchronize() override;
    bool Synched_Test() override;

    //Buffer - Per element (one TLV packet)
    void Header_Parse() override;
    void Data_Parse() override;

    //Payload parsers (operate on an in-memory byte range; see .cpp)
    void Parse_CompressedIp();
    void Parse_Mmtp();
    void Parse_SignalingMessages(int16u packet_id, int32u seq_num, const int8u* Data, size_t Size);
    void Parse_SignalingMessage(const int8u* Data, size_t Size);
    void Parse_Table(const int8u* Data, size_t Size);
    void Parse_Ntp(); // IPv6/UDP -> NTP fallback clock
    void Note_StreamNow(int64s Utc);                // record earliest/latest "now"
    bool PidEncrypted(int16u PacketId) const;

    void Parse_Mpu(int16u packet_id, int32u seq_num);
    void Feed_DataUnit(int16u packet_id, const int8u* Data, size_t Size);
    void Feed_NalFragment(int16u packet_id, const int8u* Data, size_t Size, bool First);

    //Child ES parser factory: maps an MPT asset_type (STD-B60) to a MediaInfo
    //parser + stream kind + DU framing. Returns false for a type with no ES
    //parser (stpp/aapp today; a newly assigned asset_type is one added row).
    struct media_parser;
    bool Create_MediaParser(int32u asset_type, media_parser& M);

    //FIRST/MIDDLE/LAST fragments accumulated per packet_id with sequence-number
    //continuity. The MPT is fragmented across MMTP packets, so without this no
    //A/V/subtitle streams appear.
    struct fragment_assembler
    {
        enum state_t { Init, NotStarted, InFragment, Skip };
        state_t            State = Init;
        int32u             LastSeq = 0;
        std::vector<int8u> Data;
    };
    std::map<int16u, fragment_assembler> Assemblers;

    //As Assemblers, but the completed DU is framed and fed to a child ES parser
    //rather than dispatched as a table.
    std::map<int16u, fragment_assembler> MfuAssemblers;

    //asset struct now lives in File_Mmt.h (shared with the signaling sub-parser).
    std::vector<asset> Assets;

    //Data-unit framing of the child ES: HEVC emits Annex-B NAL, AAC LATM emits
    //LOAS. Keyed off asset_type at parser creation, so a new codec is one row.
    enum es_framing { Es_Nal, Es_Loas };

    //Child ES parser per A/V asset, keyed by packet_id, Merge()d onto the
    //MPT-created stream at finish.
    struct media_parser
    {
        std::unique_ptr<File__Analyze> Parser; //owned; freed with the map entry
        stream_t       StreamKind = Stream_Video;
        size_t         StreamPos = 0;   //as created in Streams_Fill
        es_framing     Framing = Es_Nal;     //how Feed_DataUnit frames a DU for the parser
        bool           Done = false;
        int64u         Fed = 0;         //validated bytes fed (readability signal)
        int64u         MpuSeen = 0;     //MPU packets routed here (scramble give-up)
        int            DescCh = 0;      //audio-component channel count
        int8u          DescCfg = 0;     //MPEG-4 channel_configuration for the shared Aac_* strings
        //Video geometry from 0x8010; fills the ES's gaps at finish.
        int            DescWidth = 0, DescHeight = 0;
        float64        DescFrameRate = 0;
        int8u          DescScan = 0xFF;              //0xFF=none
    };
    std::map<int16u, media_parser> MediaParsers;
    bool     Media_Probe_Done = false;
    int64u   Media_Bytes = 0;        //global probe budget

    //PIDs flagged scrambled in the MMTP scramble sub-header.
    std::set<int16u> ScrambledPids;

    //An ECM (0x82/0x83) was seen -> CAS active. A scrambled PID counts as
    //encrypted only after this: a descrambled file's pre-ECM lead-in is still
    //scrambled but not encrypted.
    bool     Ecm_Seen = false;

    bool     Eit_Present_Found = false;
    Ztring   Eit_EventName;
    Ztring   Eit_EventText;      //short event description
    int16u   Eit_ServiceId = 0;      //matches the SDT service
    bool     Eit_ServiceId_Found = false;
    int16u   Eit_EventId = 0;
    int16u   Eit_StartDate = 0;      //MJD
    int32u   Eit_StartTime = 0;      //24-bit BCD HHMMSS
    int64s   Eit_Start_Utc = -1;     //present event start, UTC (settle grace)
    int32u   Eit_Duration = 0;       //24-bit BCD HHMMSS
    Ztring   Eit_Language;

    bool     Mpt_Found = false;
    int      Mpt_AssetCount = -1;    //media assets in the committed MPT version (fullest within it)
    int      Mpt_Version = -1;       //committed MPT version; a change replaces the track set
    int8u    Transfer_Last = 0xFF;   //newest 0x8010 transfer code (STD-B60 7-51); 0xFF = none seen

    //Per-channel, so it survives an EIT boundary re-scan (unlike the MPT).
    bool     Sdt_Found = false;
    Ztring   Sdt_ServiceName;
    Ztring   Sdt_Provider;
    Ztring   Sdt_ServiceType;

    //TLV stream id (transport_stream_id analog) from the MH-SDT/MH-EIT header. -1 = not seen.
    int32s   Tlv_Stream_Id = -1;

    //Stream wall clock, JST seconds since the Unix epoch. MH-TOT preferred, NTP
    //transmit timestamp fallback. -1 = not seen.
    bool     Tot_Seen = false;   //authoritative MH-TOT clock seen; until then NTP advances it
    int64s   Now_Utc = -1;            //latest, drives the boundary check
    int64s   Now_First = -1;
    int64s   Now_Last = -1;           //incl. tail probe

    //Min/max video MPU presentation time, us. In the clear, so it works for
    //scrambled video. -1 = none seen.
    int64s   Pts_First_Us = -1;
    int64s   Pts_Last_Us = -1;
    int64s   Pts_Last_At_Tail = -1; //snapshot on entering the tail probe

    //Bounded so a bad stream is not walked forever.
    int      Eit_Boundary_Hops = 0;

    enum probe_phase { Phase_Scan, Phase_Tail, Phase_Done };
    probe_phase Phase = Phase_Scan;

    //Bound on scanning past accept, so a huge file is not read end-to-end when
    //the present EIT never appears.
    int64u   Packets_Since_Accept = 0;

    //When the core (MPT + present EIT + media probe) completed; a bounded grace
    //after it catches the less-frequent MH-SDT. -1 = not yet.
    int64u   Core_Done_At = (int64u)-1;
    int64s   Media_Done_Utc = -1;     //clock when MPT+media first done, to time-box the EIT wait
};

} //NameSpace

#endif
