/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// MMT signaling tables (MPT / MH-EIT / MH-SDT / MH-TOT). Fed a reassembled
// signaling message by File_MmtTlv; parses the tables and writes results into
// the shared mmt_stream. Analogous to File_Mpeg_Psi for MPEG-TS PSI.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_MmtH
#define MediaInfo_File_MmtH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <vector>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Shared time helpers (used by both the container and the signaling parser).
//***************************************************************************
const int64s Mmt_JST_Offset_Seconds = 9 * 3600;

inline int Mmt_Bcd2(int8u b) { return (b >> 4) * 10 + (b & 0x0F); }

//MJD + 24-bit BCD HHMMSS -> seconds since the Unix epoch (in the table's own
//time zone; the caller removes the JST offset).
inline int64s Mmt_DateTime_To_Seconds(int16u mjd, int32u bcd_hhmmss)
{
    if (mjd == 0 || mjd == 0xFFFF)
        return -1;
    int64s days = (int64s)mjd - 40587;
    int hh = Mmt_Bcd2((int8u)(bcd_hhmmss >> 16));
    int mm = Mmt_Bcd2((int8u)(bcd_hhmmss >> 8));
    int ss = Mmt_Bcd2((int8u)(bcd_hhmmss));
    return days * 86400 + hh * 3600 + mm * 60 + ss;
}

//***************************************************************************
// One media/data asset from the MPT asset list, with its decoded descriptors.
//***************************************************************************
struct asset
{
    int32u Type = 0;        //asset_type FourCC (little-endian as read)
    int16u PacketId = 0;    //location_type 0x00 packet_id, else 0
    bool   Superimpose = false; //stpp: superimposed text (true) vs subtitle (false)
    //MH-audio-component descriptor (0x8014), MP4A assets only:
    Ztring Language;
    Ztring Title;         //component description (e.g. main audio / commentary)
    bool   MainComponent = false; //-> Default track
    int8u  Handicapped = 0;   //audio_for_handicapped: 0b01 = VI commentary
    int8u  AudioMode = 0;     //component_type & 0x1F (0=none)
    int8u  SamplingCode = 0;  //sampling_rate 3-bit code (0=none)
    //Video_Component_Descriptor (0x8010):
    int8u  VideoResolution = 0; //0=none, 6=2160, 7=4320
    int8u  VideoAspect = 0;
    int8u  VideoScan = 0xFF;       //0=interlaced, 1=progressive, 0xFF=none
    int8u  VideoFrameRate = 0;
    //Asset_Group_Descriptor (0x8000): main+backup of a stream share GroupId; level 0 = default.
    int    GroupId = -1;
    int8u  SelectionLevel = 0;
};

//***************************************************************************
// Shared state the container (File_MmtTlv) owns and the signaling parser
// writes into - analogous to File_Mpeg_Psi's complete_stream. Grows a field
// per migrated table.
//***************************************************************************
struct mmt_stream
{
    //MH-TOT: authoritative JST clock, as UTC seconds (-1 = not seen).
    int64s TotUtc;
    bool   TotSeen;

    //TLV stream id (the multiplex id, MPEG-TS transport_stream_id analog), from the MH-SDT/MH-EIT
    //section header. -1 = not seen.
    int32s TlvStreamId;       //out

    //MH-SDT: service (channel) name + provider, matched to the present-EIT
    //service_id when the container knows it (in), else the first service.
    int16u EitServiceId;      //in
    bool   EitServiceIdFound; //in
    Ztring ServiceName;       //out
    Ztring Provider;          //out
    Ztring ServiceType;       //out (decoded MH-service_descriptor service_type)
    bool   SdtFound;          //out

    //MPT: asset list (parsed here, committed by the container), plus the newest
    //0x8010 transfer nibble in the table (the container applies the phase gate).
    std::vector<asset> Assets;
    int    MptVersion;
    int8u  TransferLast;      //out (0 = none)
    bool   MptValid;          //out
    //Video MPU presentation-time span in this MPT, microseconds (-1 = none). In the clear, so it
    //works for scrambled video; the container keeps the global min/max.
    int64s PtsMinUs;          //out
    int64s PtsMaxUs;          //out

    //MH-EIT present event (parsed here; the container keeps the boundary/hop logic).
    bool   EitSvcIdFound;     //out
    int16u EitSvcId;          //out
    bool   EitParsed;         //out: a present-section first event was extracted
    int16u EitEventId;        //out
    int16u EitStartDate;      //out (MJD)
    int32u EitStartTime;      //out (BCD HHMMSS)
    int32u EitDuration;       //out (BCD HHMMSS)
    Ztring EitName;           //out
    Ztring EitText;           //out
    Ztring EitLanguage;       //out

    mmt_stream()
        : TotUtc(-1), TotSeen(false), TlvStreamId(-1),
          EitServiceId(0), EitServiceIdFound(false), SdtFound(false),
          MptVersion(-1), TransferLast(0), MptValid(false),
          PtsMinUs(-1), PtsMaxUs(-1),
          EitSvcIdFound(false), EitSvcId(0), EitParsed(false),
          EitEventId(0), EitStartDate(0), EitStartTime(0), EitDuration(0)
    {}
};

//***************************************************************************
// Class File_Mmt
//***************************************************************************

class File_Mmt : public File__Analyze
{
public :
    //In - set by the container before feeding a table buffer.
    mmt_stream* Complete_Stream = NULL;

private :
    //Buffer - Global
    void FileHeader_Parse() override;

    //Buffer - Per element (one signaling table)
    void Header_Parse() override;
    void Data_Parse() override;

    //Header_Parse stashes the table version byte for Mpt().
    int8u Table_Version;

    //Tables
    void Mpt();
    void MhEit();
    void MhTot();
    void MhSdt();
};

} //NameSpace

#endif
