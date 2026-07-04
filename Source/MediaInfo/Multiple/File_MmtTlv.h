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
#include <vector>
#include <map>
#include <set>
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
    void Parse_CompressedIp(const int8u* Data, size_t Size);
    void Parse_Mmtp(const int8u* Data, size_t Size);
    void Parse_SignalingMessages(int16u packet_id, int32u seq_num, const int8u* Data, size_t Size);
    void Parse_SignalingMessage(const int8u* Data, size_t Size);
    void Parse_Table(const int8u* Data, size_t Size);
    void Parse_Mpt(const int8u* Data, size_t Size);
    void Parse_MhEit(const int8u* Data, size_t Size);
    void Parse_MhTot(const int8u* Data, size_t Size);
    void Parse_MhSdt(const int8u* Data, size_t Size); // service (channel) name
    void Parse_Ntp(const int8u* Data, size_t Size); // IPv6/UDP -> NTP fallback clock
    void Note_StreamNow(int64s Utc);                // record earliest/latest "now"
    bool PidEncrypted(int16u PacketId) const;

    void Parse_Mpu(int16u packet_id, int32u seq_num, const int8u* Data, size_t Size);
    void Feed_DataUnit(int16u packet_id, const int8u* Data, size_t Size);

    //FIRST/MIDDLE/LAST fragments accumulated per packet_id with sequence-number
    //continuity. The MPT is fragmented across MMTP packets, so without this no
    //A/V/subtitle streams appear.
    struct fragment_assembler
    {
        enum state_t { Init, NotStarted, InFragment, Skip };
        state_t            State;
        int32u             LastSeq;
        std::vector<int8u> Data;
        fragment_assembler() : State(Init), LastSeq(0) {}
    };
    std::map<int16u, fragment_assembler> Assemblers;

    //As Assemblers, but the completed DU is framed and fed to a child ES parser
    //rather than dispatched as a table.
    std::map<int16u, fragment_assembler> MfuAssemblers;

    struct asset
    {
        int32u Type;        //asset_type FourCC (little-endian as read)
        int16u PacketId;    //location_type 0x00 packet_id, else 0
        bool   Superimpose; //stpp: 文字スーパー (true) vs 字幕 (false)
        //MH-audio-component descriptor (0x8014), MP4A assets only:
        Ztring Language;
        Ztring Title;         //component description (e.g. 主音声 / 解説)
        bool   MainComponent; //-> Default track
        int8u  Handicapped;   //audio_for_handicapped: 0b01 = VI commentary
        int8u  AudioMode;     //component_type & 0x1F (0=none)
        int8u  SamplingCode;  //sampling_rate 3-bit code (0=none)
        asset() : Type(0), PacketId(0), Superimpose(false),
                  MainComponent(false), Handicapped(0),
                  AudioMode(0), SamplingCode(0) {}
    };
    std::vector<asset> Assets;

    //Child ES parser per A/V asset, keyed by packet_id, Merge()d onto the
    //MPT-created stream at finish.
    struct media_parser
    {
        File__Analyze* Parser;      //owned; deleted in Streams_Finish
        stream_t       StreamKind;
        size_t         StreamPos;   //as created in Streams_Fill
        bool           IsAac;       //frame DUs as LOAS (else HEVC Annex-B)
        bool           Done;
        int64u         Fed;         //validated bytes fed (readability signal)
        int64u         MpuSeen;     //MPU packets routed here (scramble give-up)
        media_parser() : Parser(NULL), StreamKind(Stream_Video), StreamPos(0),
                         IsAac(false), Done(false), Fed(0), MpuSeen(0) {}
    };
    std::map<int16u, media_parser> MediaParsers;
    bool     Media_Probe_Done;
    int64u   Media_Bytes;        //global probe budget

    //PIDs flagged scrambled in the MMTP scramble sub-header.
    std::set<int16u> ScrambledPids;

    //An ECM (0x82/0x83) was seen -> CAS active. A scrambled PID counts as
    //encrypted only after this: a descrambled file's pre-ECM lead-in is still
    //scrambled but not encrypted.
    bool     Ecm_Seen;

    bool     Eit_Present_Found;
    Ztring   Eit_EventName;
    Ztring   Eit_EventText;      //short event description
    int16u   Eit_ServiceId;      //matches the SDT service
    bool     Eit_ServiceId_Found;
    int16u   Eit_EventId;
    int16u   Eit_StartDate;      //MJD
    int32u   Eit_StartTime;      //24-bit BCD HHMMSS
    int32u   Eit_Duration;       //24-bit BCD HHMMSS
    Ztring   Eit_Language;

    bool     Mpt_Found;

    //Per-channel, so it survives an EIT boundary re-scan (unlike the MPT).
    bool     Sdt_Found;
    Ztring   Sdt_ServiceName;
    Ztring   Sdt_Provider;

    //Stream wall clock, JST seconds since the Unix epoch. MH-TOT preferred, NTP
    //transmit timestamp fallback. -1 = not seen.
    int64s   Now_Utc;            //latest, drives the boundary check
    int64s   Now_First;
    int64s   Now_Last;           //incl. tail probe

    //Bounded so a bad stream is not walked forever.
    int      Eit_Boundary_Hops;

    enum probe_phase { Phase_Scan, Phase_Tail, Phase_Done };
    probe_phase Phase;

    //Bound on scanning past accept, so a huge file is not read end-to-end when
    //the present EIT never appears.
    int64u   Packets_Since_Accept;

    //When the core (MPT + present EIT + media probe) completed; a bounded grace
    //after it catches the less-frequent MH-SDT. -1 = not yet.
    int64u   Core_Done_At;
};

} //NameSpace

#endif
