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
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

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

    struct asset
    {
        int32u Type;        //asset_type FourCC (little-endian as read)
        int16u PacketId;    //location_type 0x00 packet_id, else 0
        bool   Superimpose; //stpp: 文字スーパー (true) vs 字幕 (false)
        asset() : Type(0), PacketId(0), Superimpose(false) {}
    };
    std::vector<asset> Assets;

    bool     Eit_Present_Found;
    Ztring   Eit_EventName;
    Ztring   Eit_EventText;      //short event description
    int16u   Eit_EventId;
    int16u   Eit_StartDate;      //MJD
    int32u   Eit_StartTime;      //24-bit BCD HHMMSS
    int32u   Eit_Duration;       //24-bit BCD HHMMSS
    Ztring   Eit_Language;

    bool     Mpt_Found;

    //Bound on scanning past accept, so a huge file is not read end-to-end when
    //the present EIT never appears.
    int64u   Packets_Since_Accept;
};

} //NameSpace

#endif
