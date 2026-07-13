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
#include "MediaInfo/Multiple/File_Mmt.h"
#include "MediaInfo/TimeCode.h" //Date_MJD(), Time_BCD()
#if defined(MEDIAINFO_HEVC_YES)
    #include "MediaInfo/Video/File_Hevc.h"
#endif
#if defined(MEDIAINFO_AAC_YES)
    #include "MediaInfo/Audio/File_Aac.h"
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

// Shared VUI strings (File_Mpegv.cpp) so descriptor and ES paths agree.
extern const char* Mpegv_colour_primaries(int8u colour_primaries);
extern const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
extern const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
// Shared AAC channel strings (File_Aac_Main.cpp), keyed by MPEG-4 channel_configuration.
extern int8u Aac_Channels_Get(int8u ChannelLayout);
extern std::string Aac_ChannelConfiguration_GetString(int8u ChannelLayout);
extern std::string Aac_ChannelConfiguration2_GetString(int8u ChannelLayout);
extern std::string Aac_ChannelLayout_GetString(int8u ChannelLayout, bool IsMpegh3da, bool IsTip);

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
    const int8u MMTP_PAYLOAD_MPU                = 0x00;
    const int8u MMTP_PAYLOAD_SIGNALING          = 0x02;

    // Signaling message ids
    const int16u MSG_PA_MESSAGE                 = 0x0000;
    const int16u MSG_M2_SECTION                 = 0x8000;

    // Table ids
    const int8u TABLE_MPT                       = 0x20; // MMT Package Table
    const int8u TABLE_ECM_0                     = 0x82; // ECM (CAS active)
    const int8u TABLE_ECM_1                     = 0x83; // ECM (alt)
    const int8u TABLE_MH_EIT                    = 0x8B; // MH-EIT[p/f]
    const int8u TABLE_MH_SDT                    = 0x9F; // MH-SDT (service name)
    const int8u TABLE_MH_TOT                    = 0xA1; // MH-TOT (current JST)

    // Descriptor tags
    const int16u DESC_MH_SHORT_EVENT            = 0xF001;

    // asset_type FourCCs, read little-endian
    const int32u ASSET_HEV1                     = 0x31766568; // 'hev1' (VPS/SPS/PPS in the MFU)
    const int32u ASSET_HVC1                     = 0x31637668; // 'hvc1' (VPS/SPS/PPS in MPU metadata)
    const int32u ASSET_MP4A                     = 0x6134706D; // 'mp4a'
    const int32u ASSET_STPP                     = 0x70707473; // 'stpp'

    // asset_type -> trace label for the MPU line: the printable 4CC (as the MPT declared it) plus a
    // friendly codec name when known. An un-mapped/future asset_type still shows its 4CC, so the MPU
    // payload is never an unlabelled block.
    inline Ztring Mmt_AssetLabel(int32u asset_type)
    {
        Ztring Label; // asset_type was read little-endian, so byte i spells 4CC character i
        for (int i = 0; i < 4; ++i)
        {
            int8u c = (int8u)(asset_type >> (8 * i));
            Label += (c >= 0x20 && c < 0x7F) ? (Char)c : (Char)__T('.');
        }
        switch (asset_type)
        {
            case ASSET_HEV1:
            case ASSET_HVC1: Label += __T(" (HEVC)"); break;
            case ASSET_MP4A: Label += __T(" (AAC)");  break;
            case ASSET_STPP: Label += __T(" (TTML)"); break;
            default:;
        }
        return Label;
    }

    // MMTP MPU fragmentation_indicator (ISO/IEC 23008-1): where this fragment sits in its data unit.
    inline const char* Mmt_fragmentation_indicator(int8u i)
    {
        switch (i)
        {
            case 0: return "Not fragmented";
            case 1: return "First";
            case 2: return "Middle";
            case 3: return "Last";
            default: return "";
        }
    }

    // Stop scanning after this many packets, so a multi-GB file is not read
    // end-to-end when the present EIT never arrives.
    const int64u GIVE_UP_AFTER_PACKETS          = 200000;

    // Feeding stops on SPS/ASC, so this only bounds a no-parse stream. Sized for
    // 8K, whose sparse RAPs can put the first in-band SPS tens of MB in.
    const int64u MEDIA_BYTES_BUDGET             = 64 * 1024 * 1024;

    // A scrambled PID feeds nothing decodable; give up after this many MPUs so
    // the probe (and tail) still finish.
    const int64u SCRAMBLED_GIVEUP_MPU           = 64;
    // Bytes fed that count as readable (not encrypted). Clear HEVC feeds 100s of
    // KB; a scrambled payload ~0, a spurious AAC accept a few hundred.
    const int64u READABLE_MIN_BYTES             = 4096;

    const int64u BOUNDARY_HOP_BYTES             = 8 * 1024 * 1024;
    const int    BOUNDARY_MAX_HOPS              = 8;
    // Present event with under this long left at "now" is the previous program's
    // tail, not the stream's main program. EPG times are minute-granular.
    const int64s BOUNDARY_GUARD_SECONDS         = 60;

    // Signaling near an event's start is unsettled (observed to settle within
    // this); scan this long past the start, newest wins. Also the no-EIT give-up.
    const int64s BOUNDARY_WINDOW_SECONDS        = 75;

    // First clock this far before the present-event start = not this event's boundary.
    // EPG times are minute-granular.
    const int64s BOUNDARY_MISALIGN_SECONDS      = 60;


    const int64u TAIL_PROBE_BYTES               = 4 * 1024 * 1024;

    // The boundary window has passed for this committed start: the values (and the p/f
    // itself) are stable. Unknown timing is the caller's decision.
    inline bool boundary_window_passed(int64s eit_start, int64s now_first, int64s now_utc)
    {
        if (eit_start < 0 || now_first < 0 || now_utc < 0)
            return false;
        int64s since_start = now_first - eit_start;
        if (since_start < -BOUNDARY_MISALIGN_SECONDS || since_start > BOUNDARY_WINDOW_SECONDS)
            return true;
        return now_utc - eit_start >= BOUNDARY_WINDOW_SECONDS;
    }

    // MH-SDT recurs less often than MPT/EIT, so scan this far past core
    // completion to catch it.
    const int64u SDT_GRACE_PACKETS              = 20000;

    // A recording that begins at its program start has the present event's start
    // ~= the first clock, so the boundary window would only settle after ~75 s of
    // stream (hundreds of MB at 8K). The present event is already captured at core
    // completion and the tail probe fixes the duration, so cap the settle wait:
    // finalize this far past core completion even if the window has not passed.
    const int64u SETTLE_GRACE_PACKETS           = 5000;

    const int64s NTP_UNIX_EPOCH_DELTA           = 2208988800LL;

    // NTP 32.32 -> us. The epoch offset cancels in a duration, so it is not
    // subtracted.
    inline int64s Ntp32_32_To_Us(int64u ntp)
    {
        int64u sec  = ntp >> 32;
        int64u frac = ntp & 0xFFFFFFFFULL;
        return (int64s)(sec * 1000000ULL + (frac * 1000000ULL) / 0x100000000ULL);
    }

    inline bool Is_Tlv_Type(int8u t)
    {
        return t == TLV_IPV4_PACKET || t == TLV_IPV6_PACKET
            || t == TLV_HEADER_COMPRESSED_IP_PACKET
            || t == TLV_TRANSMISSION_CONTROL_PACKET || t == TLV_NULL_PACKET;
    }

    // Video_Component_Descriptor geometry (0x8010, STD-B60).
    inline int VideoResolutionHeight(int8u code)
    {
        switch (code)
        {
            case 1: return 180;  case 2: return 240;  case 3: return 480;
            case 4: return 720;  case 5: return 1080; case 6: return 2160; case 7: return 4320;
            default: return 0;
        }
    }
    inline float64 VideoFrameRateFps(int8u code)
    {
        switch (code)
        {
            case 1: return 15;               case 2: return 24000.0 / 1001;
            case 3: return 24;               case 4: return 25;
            case 5: return 30000.0 / 1001;   case 6: return 30;
            case 7: return 50;               case 8: return 60000.0 / 1001;
            case 9: return 60;               case 10: return 100;
            case 11: return 120000.0 / 1001; case 12: return 120;
            default: return 0;
        }
    }

    // MH-audio-component sampling_rate 3-bit code -> Hz. 0/4 reserved.
    inline int32u AudioSamplingRate(int8u code)
    {
        switch (code)
        {
            case 0x1: return 16000;
            case 0x2: return 22050;
            case 0x3: return 24000;
            case 0x5: return 32000;
            case 0x6: return 44100;
            case 0x7: return 48000;
            default:  return 0;
        }
    }

    // MH-audio-component audio_mode (ARIB STD-B60 Table 7-60) -> MPEG-4 channel_configuration,
    // so the shared Aac_* strings apply. 0 = leave to the ES parser.
    inline int8u AudioModeToAacConfig(int8u mode)
    {
        switch (mode)
        {
            case 0x01: return 1;  // 1/0 mono
            case 0x02: return 8;  // 1/0+1/0 dual mono
            case 0x03: return 2;  // 2/0 stereo
            case 0x05: return 3;  // 3/0
            case 0x07: return 4;  // 3/1
            case 0x08: return 5;  // 3/2 (5.0)
            case 0x09: return 6;  // 3/2+LFE (5.1)
            case 0x0C: return 7;  // 5/2.1 (7.1)
            case 0x11: return 13; // 22.2
            default:   return 0;
        }
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
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
bool File_MmtTlv::PidEncrypted(int16u PacketId) const
{
    if (!PacketId || !ScrambledPids.count(PacketId))
        return false;
    //Bytes fed, not the accept flag: clear HEVC can finish on the byte budget
    //without IsAccepted, and File_Aac can accept a garbage LOAS frame. A readable
    //PID was descrambled in place, not encrypted.
    std::map<int16u, media_parser>::const_iterator It = MediaParsers.find(PacketId);
    if (It != MediaParsers.end())
    {
        if (It->second.Fed >= READABLE_MIN_BYTES)
            return false;
    }
    else
    {
        //No ES parser here (e.g. subtitle): readable iff any A/V parser was.
        for (std::map<int16u, media_parser>::const_iterator J = MediaParsers.begin();
             J != MediaParsers.end(); ++J)
            if (J->second.Fed >= READABLE_MIN_BYTES)
                return false;
    }
    //Gate on ECM: before it, a descrambled file's still-scrambled lead-in is not
    //yet encrypted.
    return Ecm_Seen;
}

//---------------------------------------------------------------------------
void File_MmtTlv::Streams_Fill()
{
    //Format identification. Only General_Format is set here; the extensions and the
    //descriptive info come from the format database (Format.csv -> MediaInfo_Config_Automatic).
    Fill(Stream_General, 0, General_Format, "MMT/TLV");

    //TLV stream id (transport_stream_id analog), as File_MpegTs exposes it: numeric + hex string.
    if (Tlv_Stream_Id >= 0)
    {
        Fill(Stream_General, 0, General_ID, Tlv_Stream_Id, 10, true);
        Fill(Stream_General, 0, General_ID_String, Get_Hex_ID((int32u)Tlv_Stream_Id), true);
    }

    //A/V/subtitle streams from the MPT asset list.
    //The Asset_Group_Descriptor (STD-B60 7.4.3.1) groups the main and robust-backup versions of a
    //stream under one group_identification (rain fade); selection_level 0 is the default. Expose
    //the group as the AlternateGroup and Default the level-0 member. Absent it GroupId is -1, so
    //Default falls back to the main_component flag alone.
    for (size_t i = 0; i < Assets.size(); ++i)
    {
        const asset& A = Assets[i];
        switch (A.Type)
        {
            case ASSET_HEV1:
            case ASSET_HVC1:
                Stream_Prepare(Stream_Video);
                Fill(Stream_Video, StreamPos_Last, Video_Format, "HEVC");
                Fill(Stream_Video, StreamPos_Last, Video_CodecID, A.Type == ASSET_HVC1 ? "hvc1" : "hev1");
                if (A.PacketId)
                {
                    Fill(Stream_Video, StreamPos_Last, Video_ID, A.PacketId, 10);
                    Fill(Stream_Video, StreamPos_Last, Video_ID_String, Get_Hex_ID(A.PacketId), true); // MMT packet_id is hex-native
                }
                if (PidEncrypted(A.PacketId))
                    Fill(Stream_Video, StreamPos_Last, "Encryption", "Encrypted");
                if (A.GroupId >= 0)
                {
                    Fill(Stream_Video, StreamPos_Last, Video_AlternateGroup, A.GroupId);
                    if (A.SelectionLevel == 0)
                        Fill(Stream_Video, StreamPos_Last, Video_Default, "Yes");
                }
                {
                    std::map<int16u, media_parser>::iterator It = MediaParsers.find(A.PacketId);
                    if (It != MediaParsers.end())
                    {
                        It->second.StreamPos = StreamPos_Last;
                        //Seed geometry from the descriptor; the ES overrides it at finish.
                        int h = VideoResolutionHeight(A.VideoResolution);
                        if (h)
                        {
                            It->second.DescHeight = h;
                            if (A.VideoAspect == 1)
                                It->second.DescWidth = h * 4 / 3;
                            else if (A.VideoAspect == 2 || A.VideoAspect == 3)
                                It->second.DescWidth = h * 16 / 9;
                        }
                        It->second.DescFrameRate = VideoFrameRateFps(A.VideoFrameRate);
                        It->second.DescScan      = A.VideoScan;
                    }
                }
                break;
            case ASSET_MP4A:
                Stream_Prepare(Stream_Audio);
                Fill(Stream_Audio, StreamPos_Last, Audio_Format, "AAC");
                Fill(Stream_Audio, StreamPos_Last, Audio_CodecID, "mp4a");
                if (A.PacketId)
                {
                    Fill(Stream_Audio, StreamPos_Last, Audio_ID, A.PacketId, 10);
                    Fill(Stream_Audio, StreamPos_Last, Audio_ID_String, Get_Hex_ID(A.PacketId), true);
                }
                //From the MH-audio-component descriptor (0x8014).
                if (!A.Language.empty())
                    Fill(Stream_Audio, StreamPos_Last, Audio_Language, A.Language);
                if (!A.Title.empty())
                    Fill(Stream_Audio, StreamPos_Last, Audio_Title, A.Title);
                if (A.GroupId >= 0)
                    Fill(Stream_Audio, StreamPos_Last, Audio_AlternateGroup, A.GroupId);
                if (A.MainComponent && A.SelectionLevel == 0)
                    Fill(Stream_Audio, StreamPos_Last, Audio_Default, "Yes");
                //audio_for_handicapped 0b01 = audio description. Only the VI case maps cleanly.
                if (A.Handicapped == 0x01)
                {
                    Fill(Stream_Audio, StreamPos_Last, Audio_ServiceKind, "VI");
                    Fill(Stream_Audio, StreamPos_Last, Audio_ServiceKind_String, "Visually Impaired");
                }
                //Seed channels from the descriptor (shared Aac_* strings); the ES overrides upward.
                {
                int8u Cfg  = AudioModeToAacConfig(A.AudioMode);
                int DescCh = Aac_Channels_Get(Cfg);
                if (DescCh)
                {
                    Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, DescCh);
                    Ztring Pos; Pos.From_UTF8(Aac_ChannelConfiguration_GetString(Cfg));
                    if (!Pos.empty())
                        Fill(Stream_Audio, StreamPos_Last, Audio_ChannelPositions, Pos);
                    Ztring Pos2; Pos2.From_UTF8(Aac_ChannelConfiguration2_GetString(Cfg));
                    if (!Pos2.empty())
                        Fill(Stream_Audio, StreamPos_Last, Audio_ChannelPositions_String2, Pos2);
                    Ztring Lay; Lay.From_UTF8(Aac_ChannelLayout_GetString(Cfg, false, false));
                    if (!Lay.empty())
                        Fill(Stream_Audio, StreamPos_Last, Audio_ChannelLayout, Lay);
                }
                //Encrypted: no ES detail, so the descriptor is all we have.
                if (PidEncrypted(A.PacketId))
                {
                    Fill(Stream_Audio, StreamPos_Last, "Encryption", "Encrypted");
                    int32u SamplingRate = AudioSamplingRate(A.SamplingCode);
                    if (SamplingRate)
                        Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, SamplingRate);
                }
                {
                    std::map<int16u, media_parser>::iterator It = MediaParsers.find(A.PacketId);
                    if (It != MediaParsers.end())
                    {
                        It->second.StreamPos = StreamPos_Last;
                        It->second.DescCh    = DescCh;
                        It->second.DescCfg   = Cfg;
                    }
                }
                }
                break;
            case ASSET_STPP:
                Stream_Prepare(Stream_Text);
                Fill(Stream_Text, StreamPos_Last, Text_Format, "TTML");
                Fill(Stream_Text, StreamPos_Last, Text_CodecID, "stpp");
                if (A.PacketId)
                {
                    Fill(Stream_Text, StreamPos_Last, Text_ID, A.PacketId, 10);
                    Fill(Stream_Text, StreamPos_Last, Text_ID_String, Get_Hex_ID(A.PacketId), true);
                }
                Fill(Stream_Text, StreamPos_Last, Text_Format_Profile,
                     A.Superimpose ? "Superimpose" : "Subtitle");
                if (!A.Language.empty()) //ISO_639 from the MH-data-component descriptor (0x8020)
                    Fill(Stream_Text, StreamPos_Last, Text_Language, A.Language);
                if (PidEncrypted(A.PacketId))
                    Fill(Stream_Text, StreamPos_Last, "Encryption", "Encrypted");
                break;
            default:
                break;
        }
    }

    if (Sdt_Found && !Sdt_ServiceName.empty())
    {
        Fill(Stream_General, 0, General_ServiceName, Sdt_ServiceName);
        if (!Sdt_Provider.empty())
            Fill(Stream_General, 0, General_ServiceProvider, Sdt_Provider);
        if (!Sdt_ServiceType.empty())
            Fill(Stream_General, 0, General_ServiceType, Sdt_ServiceType);
    }

    //Present event -> a Menu chapter, as File_MpegTs renders DVB/ATSC EPG: the
    //entry key is the scheduled start (UTC), the value a "/"-delimited record
    //(name / text / content / rating / duration / running_status).
    if (Eit_Present_Found)
    {
        Stream_Prepare(Stream_Menu);
        Fill(Stream_Menu, StreamPos_Last, Menu_ID, Eit_EventId, 10);
        Fill(Stream_Menu, StreamPos_Last, Menu_ID_String, Get_Hex_ID(Eit_EventId), true);
        if (!Eit_EventName.empty())
            Fill(Stream_General, 0, General_Title, Eit_EventName);
        if (!Eit_Language.empty())
            Fill(Stream_Menu, StreamPos_Last, Menu_Language, Eit_Language);
        if (Sdt_Found && !Sdt_ServiceName.empty())
            Fill(Stream_Menu, StreamPos_Last, Menu_ServiceName, Sdt_ServiceName);

        int64s Start = Mmt_DateTime_To_Seconds(Eit_StartDate, Eit_StartTime); // JST
        if (Start >= 0)
        {
            Ztring Time = Ztring().Date_From_Seconds_1970(Start - Mmt_JST_Offset_Seconds);
            Time.FindAndReplace(__T("UTC "), __T(""));
            Time += __T(" UTC");
            Ztring Dur; Dur.From_UTF8(Time_BCD(Eit_Duration).c_str());
            Ztring Event = Eit_EventName + __T(" / ") + Eit_EventText + __T(" /  /  / ") + Dur + __T(" / ");
            Fill(Stream_Menu, StreamPos_Last, Menu_Chapters_Pos_Begin, Count_Get(Stream_Menu, StreamPos_Last), 10, true);
            Fill(Stream_Menu, StreamPos_Last, Time.To_UTF8().c_str(), Event, true);
            Fill(Stream_Menu, StreamPos_Last, Menu_Chapters_Pos_End, Count_Get(Stream_Menu, StreamPos_Last), 10, true);
        }
    }
}

//---------------------------------------------------------------------------
void File_MmtTlv::Streams_Finish()
{
    for (std::map<int16u, media_parser>::iterator It = MediaParsers.begin();
         It != MediaParsers.end(); ++It)
    {
        media_parser& M = It->second;
        if (!M.Parser)
            continue;
        Open_Buffer_Finalize(M.Parser.get());
        //Non-erasing: keep the container-level Format/CodecID/ID, add codec detail.
        Merge(*M.Parser, M.StreamKind, 0, M.StreamPos, false);
        //Keep the descriptor channels when the decoded ES read fewer (a boundary transient).
        if (M.StreamKind == Stream_Audio && M.DescCh)
        {
            int64u EsCh = Retrieve_Const(Stream_Audio, M.StreamPos, Audio_Channel_s_).To_int64u();
            if (EsCh < (int64u)M.DescCh)
            {
                Fill(Stream_Audio, M.StreamPos, Audio_Channel_s_, (int64u)M.DescCh, 10, true);
                Ztring Pos; Pos.From_UTF8(Aac_ChannelConfiguration_GetString(M.DescCfg));
                if (!Pos.empty())
                    Fill(Stream_Audio, M.StreamPos, Audio_ChannelPositions, Pos, true);
                Ztring Pos2; Pos2.From_UTF8(Aac_ChannelConfiguration2_GetString(M.DescCfg));
                if (!Pos2.empty())
                    Fill(Stream_Audio, M.StreamPos, Audio_ChannelPositions_String2, Pos2, true);
                Ztring Lay; Lay.From_UTF8(Aac_ChannelLayout_GetString(M.DescCfg, false, false));
                if (!Lay.empty())
                    Fill(Stream_Audio, M.StreamPos, Audio_ChannelLayout, Lay, true);
            }
        }
        //From 0x8010: transfer overrides the HEVC VUI; geometry and colour fill the ES's gaps.
        if (M.StreamKind == Stream_Video)
        {
            if (Transfer_Last >= 1 && Transfer_Last < 8)
            {
                //Descriptor -> VUI code (STD-B60 Table 7-51). The newest descriptor wins over
                //the ES: a head-bounded probe may only have decoded the opening segment's SPS.
                int8u vui = Transfer_Last == 5 ? 18 : Transfer_Last == 4 ? 16 : Transfer_Last == 3 ? 14 : Transfer_Last == 2 ? 11 : 1;
                Fill(Stream_Video, M.StreamPos, Video_transfer_characteristics, Mpegv_transfer_characteristics(vui), Unlimited, true, true);
                if (Transfer_Last == 3 || Transfer_Last == 4 || Transfer_Last == 5)
                {
                    if (Retrieve_Const(Stream_Video, M.StreamPos, Video_colour_primaries).empty())
                        Fill(Stream_Video, M.StreamPos, Video_colour_primaries, Mpegv_colour_primaries(9));
                    if (Retrieve_Const(Stream_Video, M.StreamPos, Video_matrix_coefficients).empty())
                        Fill(Stream_Video, M.StreamPos, Video_matrix_coefficients, Mpegv_matrix_coefficients(9));
                }
            }
            //Prefer the ES; fill only the gaps it left.
            if (M.DescHeight && Retrieve_Const(Stream_Video, M.StreamPos, Video_Height).empty())
            {
                Fill(Stream_Video, M.StreamPos, Video_Height, M.DescHeight);
                if (M.DescWidth)
                    Fill(Stream_Video, M.StreamPos, Video_Width, M.DescWidth);
            }
            if (M.DescFrameRate > 0 && Retrieve_Const(Stream_Video, M.StreamPos, Video_FrameRate).empty())
                Fill(Stream_Video, M.StreamPos, Video_FrameRate, M.DescFrameRate, 3);
            if (M.DescScan != 0xFF && Retrieve_Const(Stream_Video, M.StreamPos, Video_ScanType).empty())
                Fill(Stream_Video, M.StreamPos, Video_ScanType, M.DescScan ? "Progressive" : "Interlaced");
        }
    }
    MediaParsers.clear();

    //Stream start wall clock (UTC), the same field File_MpegTs uses.
    if (Now_First >= 0)
        Fill(Stream_General, 0, General_Duration_Start,
             Ztring().Date_From_Seconds_1970(Now_First));

    //Stream span, distinct from the Menu Duration (scheduled event length). Prefer the
    //MPU-PTS span (sub-second), but when the NTP clock ran materially longer the tail PTS was
    //never read - a scrambled service capture whose end blocks don't decode, or an early
    //give-up - so the whole-second clock is the true span, not the truncated PTS.
    float64 pts_span_ms = (Pts_First_Us >= 0 && Pts_Last_Us > Pts_First_Us)
                        ? (float64)(Pts_Last_Us - Pts_First_Us) / 1000.0 : -1; // us -> ms
    float64 ntp_span_ms = (Now_First >= 0 && Now_Last > Now_First)
                        ? (float64)(Now_Last - Now_First) * 1000.0 : -1;       // s -> ms
    float64 span_ms;
    if (pts_span_ms > 0 && (ntp_span_ms < 0 || ntp_span_ms <= pts_span_ms + 2000.0))
        span_ms = pts_span_ms;
    else
        span_ms = ntp_span_ms;
    if (span_ms > 0)
    {
        //Float ms keeps the sub-second precision.
        Fill(Stream_General, 0, General_Duration, span_ms, 3);
        //Same span per A/V track -> correct per-stream and overall bitrate.
        for (size_t Pos = 0; Pos < Count_Get(Stream_Video); ++Pos)
            Fill(Stream_Video, Pos, Video_Duration, span_ms, 3);
        for (size_t Pos = 0; Pos < Count_Get(Stream_Audio); ++Pos)
            Fill(Stream_Audio, Pos, Audio_Duration, span_ms, 3);
        //End clock = start + span, so Start/Duration/End stay consistent (the clock
        //is whole-second while the span is sub-second PTS, so the last raw clock can
        //differ by a second or two).
        if (Now_First >= 0)
            Fill(Stream_General, 0, General_Duration_End,
                 Ztring().Date_From_Seconds_1970(Now_First + (int64s)(span_ms / 1000.0 + 0.5)));
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
        int16u data_length = BigEndian2int16u(Buffer + i + 2);
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

    //Tail phase: near EOF for the last PTS and clock. Only signaling matters.
    if (Phase == Phase_Tail)
    {
        if (Element_Code == TLV_HEADER_COMPRESSED_IP_PACKET)
            Parse_CompressedIp();
        else if (Element_Code == TLV_IPV6_PACKET)
            Parse_Ntp();
        Skip_XX(Element_Size - Element_Offset, "Data");
        //With PTS, wait for a fresher one from the tail rather than stopping on
        //the earlier clock; else fall back to a distinct last "now".
        bool HavePts    = (Pts_First_Us >= 0);
        bool GotTailPts = (Pts_Last_Us > Pts_Last_At_Tail);
        bool Done = HavePts ? GotTailPts : (Now_Last > Now_First);
        if (Done || Packets_Since_Accept >= GIVE_UP_AFTER_PACKETS)
        {
            Phase = Phase_Done;
            Finish();
        }
        return;
    }

    // Settled once the boundary window has passed (from the later of the first clock and the
    // present-event start); a file opened mid-program, or with no EPG/clock, is already settled.
    auto probe_settled = [&]() -> bool {
        if (Eit_Start_Utc < 0 || Now_First < 0 || Now_Utc < 0)
            return true;
        return boundary_window_passed(Eit_Start_Utc, Now_First, Now_Utc);
    };
    bool Settled = probe_settled();
    switch ((int8u)Element_Code)
    {
        case TLV_HEADER_COMPRESSED_IP_PACKET:
            if (!(Mpt_Found && Eit_Present_Found && Media_Probe_Done && Sdt_Found && Settled))
                Parse_CompressedIp();
            break;
        case TLV_IPV6_PACKET:
            //Advance the clock from NTP until MH-TOT takes over; an NTP-only stream must not freeze.
            if (!Tot_Seen)
                Parse_Ntp();
            break;
        default:
            break;
    }

    Skip_XX(Element_Size - Element_Offset, "Data");

    //Probe done once every A/V parser has finished or the budget is spent.
    if (Mpt_Found && !Media_Probe_Done)
    {
        bool AllDone = true;
        for (std::map<int16u, media_parser>::iterator It = MediaParsers.begin();
             It != MediaParsers.end(); ++It)
            if (!It->second.Done)
            {
                AllDone = false;
                break;
            }
        if (AllDone || Media_Bytes >= MEDIA_BYTES_BUDGET)
            Media_Probe_Done = true;
    }
    if (Mpt_Found && MediaParsers.empty())
        Media_Probe_Done = true;

    //Note when the media probe finished with a clock in hand, to time-box the EIT wait below.
    if (Mpt_Found && Media_Probe_Done && Now_Utc >= 0 && Media_Done_Utc < 0)
        Media_Done_Utc = Now_Utc;

    //Grace past core completion for the less-frequent MH-SDT.
    bool CoreComplete = (Mpt_Found && Eit_Present_Found && Media_Probe_Done);
    if (CoreComplete && Core_Done_At == (int64u)-1)
        Core_Done_At = Packets_Since_Accept;

    // Recompute: this packet's EIT/TOT may have set the start or advanced the clock.
    Settled = probe_settled();
    bool SettledOrTimeout = Settled
                         || (Core_Done_At != (int64u)-1
                          && Packets_Since_Accept - Core_Done_At >= SETTLE_GRACE_PACKETS);
    bool ScanComplete = CoreComplete && SettledOrTimeout
                     && (Sdt_Found
                      || (Core_Done_At != (int64u)-1
                       && Packets_Since_Accept - Core_Done_At >= SDT_GRACE_PACKETS));

    // No EIT within the boundary window of the media probe (a raw service capture):
    // stop the forward scan instead of reading on to the give-up or EOF.
    bool NoEpgDone = !ScanComplete && Mpt_Found && Media_Probe_Done && !Eit_Present_Found
                  && Media_Done_Utc >= 0 && Now_Utc >= 0
                  && Now_Utc - Media_Done_Utc >= BOUNDARY_WINDOW_SECONDS;

    // Forward scan spent its packet budget before the boundary window could settle - a scrambled,
    // NULL-padded capture whose packet count outruns its timeline (the stuffing inflates the count
    // while the clock barely advances). Finalize like a completion rather than reading on to EOF.
    bool GaveUp = (Packets_Since_Accept >= GIVE_UP_AFTER_PACKETS);

    if (ScanComplete || NoEpgDone || GaveUp)
    {
        // A give-up or NoEpgDone stopped short of the end, so the tail is unread; a normal
        // completion only needs the tail when the forward scan has not reached it yet. The tail
        // probe is what fixes the span: it advances the clock (and PTS, when readable) to the true
        // end, so a give-up no longer clamps Duration to the boundary window.
        bool TailUnread = Now_First >= 0 && File_Size != (int64u)-1 && File_Size > TAIL_PROBE_BYTES
                       && ((NoEpgDone || GaveUp) || File_Offset + Buffer_Offset < File_Size - TAIL_PROBE_BYTES);
        if (TailUnread)
        {
            Phase = Phase_Tail;
            Pts_Last_At_Tail = Pts_Last_Us; //require a fresher PTS from the tail
            Packets_Since_Accept = 0;       //tail has its own budget
            GoToFromEnd(TAIL_PROBE_BYTES, "MmtTlv");
            return;
        }
        Finish();
    }
}

//***************************************************************************
// Compressed-IP TLV payload -> strip the (partial) IPv6+UDP headers -> MMTP.
//***************************************************************************

//---------------------------------------------------------------------------
void File_MmtTlv::Parse_CompressedIp()
{
    if (Element_Size - Element_Offset < 3)
        return;
    int8u header_type;
    Element_Begin1("Compressed IP");
        BS_Begin();
            Skip_S2(12,                                         "context_id");
            Skip_S1( 4,                                         "sequence_number");
        BS_End();
        Get_B1 (header_type,                                    "CID_header_type");
        switch (header_type)
        {
            case CID_PARTIAL_IPV6_PARTIAL_UDP:
                if (Element_Size - Element_Offset < PARTIAL_IPV6_HEADER_LENGTH + PARTIAL_UDP_HEADER_LENGTH)
                    { Element_End0(); return; }
                Skip_XX(PARTIAL_IPV6_HEADER_LENGTH,             "partial IPv6 header");
                Skip_XX(PARTIAL_UDP_HEADER_LENGTH,              "partial UDP header");
                break;
            case CID_NO_COMPRESSED_HEADER:
                break;
            default:
                Element_End0();
                return; // IPv4 variants not carried by MMT signaling here
        }
    Element_End0();

    Parse_Mmtp();
}

//---------------------------------------------------------------------------
// MMTP packet header layout.
void File_MmtTlv::Parse_Mmtp()
{
    if (Element_Size - Element_Offset < 12)
        return;

    bool   packet_counter_flag, extension_flag;
    int8u  payload_type;
    int16u packet_id;
    int32u seq_num;

    Element_Begin1("MMTP");
        BS_Begin();
            Skip_S1(2,                                          "version");
            Get_SB (   packet_counter_flag,                     "packet_counter_flag");
            Skip_S1(2,                                          "FEC_type");
            Skip_S1(1,                                          "reserved");
            Get_SB (   extension_flag,                          "extension_flag");
            Skip_SB(                                            "RAP_flag");
            Skip_S1(2,                                          "reserved");
            Get_S1 (6, payload_type,                            "payload_type");
        BS_End();
        Get_B2 (packet_id,                                      "packet_id");
        Skip_B4(                                                "distribute_timestamp");
        Get_B4 (seq_num,                                        "packet_sequence_number");
        if (packet_counter_flag)
        {
            if (Element_Size - Element_Offset < 4) { Element_End0(); return; }
            Skip_B4(                                            "packet_counter");
        }
        if (extension_flag)
        {
            if (Element_Size - Element_Offset < 4) { Element_End0(); return; }
            int16u ext_len;
            Skip_B2(                                            "extension_header_type");
            Get_B2 (ext_len,                                    "extension_header_length");
            if (Element_Size - Element_Offset < ext_len) { Element_End0(); return; }
            //Sub-header chain: (type & 0x7FFF)==1 is the scramble sub-header, whose first byte
            //carries the encryption_flag in bits 4-3 (>=2 means scrambled).
            int64u ext_end = Element_Offset + ext_len;
            while (Element_Offset + 4 <= ext_end)
            {
                int16u sub_type, sub_len;
                Get_B2 (sub_type,                               "header_extension_type");
                Get_B2 (sub_len,                                "header_extension_length");
                if (Element_Offset + sub_len > ext_end)
                    break;
                if ((sub_type & 0x7FFF) == 0x0001 && sub_len >= 1)
                {
                    int8u scrambling;
                    Get_B1 (scrambling,                         "scrambling_control");
                    if (((scrambling >> 3) & 0x03) >= 2)
                        ScrambledPids.insert(packet_id);
                    if (sub_len > 1)
                        Skip_XX(sub_len - 1,                    "header_extension_data");
                }
                else if (sub_len)
                    Skip_XX(sub_len,                            "header_extension_data");
            }
            if (Element_Offset < ext_end)
                Skip_XX(ext_end - Element_Offset,               "header_extension_data");
        }
    Element_End0();

    if (payload_type == MMTP_PAYLOAD_SIGNALING)
    {
        //The reassembler concatenates fragment payloads across packets, so it works on a raw
        //buffer rather than the per-element cursor.
        const int8u* P = Buffer + Buffer_Offset + (size_t)Element_Offset;
        size_t       N = (size_t)(Element_Size - Element_Offset);
        Parse_SignalingMessages(packet_id, seq_num, P, N);
    }
    else if (payload_type == MMTP_PAYLOAD_MPU && !Media_Probe_Done)
    {
        std::map<int16u, media_parser>::iterator It = MediaParsers.find(packet_id);
        if (It != MediaParsers.end() && !It->second.Done)
        {
            //The scramble flag can't gate feeding: a stream descrambled in place keeps it set. So
            //always probe; encrypted payload just fails the MFU/NAL checks. Give up on a scrambled
            //PID feeding nothing so the probe (and tail) finish.
            ++It->second.MpuSeen;
            Parse_Mpu(packet_id, seq_num);
            if (ScrambledPids.count(packet_id)
             && It->second.Fed < READABLE_MIN_BYTES
             && It->second.MpuSeen >= SCRAMBLED_GIVEUP_MPU)
                It->second.Done = true;
        }
    }
}

//***************************************************************************
// MPU (media) -> MFU data units -> per-asset HEVC/AAC child parser.
// MPU payload -> MFU data units (timed and non-timed).
//***************************************************************************

//---------------------------------------------------------------------------
void File_MmtTlv::Parse_Mpu(int16u packet_id, int32u seq_num)
{
    //Parses at the element cursor, which Parse_Mmtp left at the MPU payload.
    if (Element_Size - Element_Offset < 8)
        return;

    Element_Begin1("MPU");
    int16u length;
    Get_B2 (length,                                             "MPU_payload_length");
    if (length != Element_Size - Element_Offset) // must fill the MMTP payload
    {
        Element_End0();
        return;
    }
    int8u fragment_type, fragmentation_indicator;
    bool  timed_flag, aggregation_flag;
    BS_Begin();
        Get_S1 (4, fragment_type,                               "fragment_type");
        Get_SB (   timed_flag,                                  "timed_flag");
        Get_S1 (2, fragmentation_indicator,                     "fragmentation_indicator");
        Param_Info1(Mmt_fragmentation_indicator(fragmentation_indicator));
        Get_SB (   aggregation_flag,                            "aggregation_flag");
    BS_End();
    Skip_B1(                                                    "fragment_counter");
    Skip_B4(                                                    "MPU_sequence_number");

    if ((aggregation_flag && fragmentation_indicator != 0) || fragment_type != 2 /*MFU*/)
    {
        Element_End0();
        return;
    }

    // Name the MFU payload on the MPU line (hev1/mp4a/... + codec), so the child ES that follows is
    // not an unlabelled block. Sourced from the MPT's asset_type, so a type with no ES parser (or a
    // future one) still shows its 4CC.
    for (size_t i = 0; i < Assets.size(); ++i)
        if (Assets[i].PacketId == packet_id)
        {
            Element_Info1(Mmt_AssetLabel(Assets[i].Type));
            break;
        }

    // Per-DU header before each fragment:
    //   timed  -> movie_fragment_seq(32) sample(32) offset(32) prio(8) dep(8)
    //   !timed -> item_id(32)
    const int64u du_header = timed_flag ? (32 + 32 + 32 + 8 + 8) / 8 : 32 / 8;

    if (aggregation_flag)
    {
        // Run of length-prefixed COMPLETE DUs.
        while (Element_Size - Element_Offset >= 2)
        {
            int16u du_len;
            Get_B2 (du_len,                                     "data_unit_length");
            if (du_len > Element_Size - Element_Offset)
                break;
            if (du_len >= du_header)
                Feed_DataUnit(packet_id,
                              Buffer + Buffer_Offset + (size_t)Element_Offset + (size_t)du_header,
                              du_len - (size_t)du_header);
            Skip_XX(du_len,                                     "data_unit");
        }
        Element_End0();
        return;
    }

    if (Element_Size - Element_Offset < du_header)
    {
        Element_End0();
        return;
    }
    Skip_XX(du_header,                                          "data_unit_header");

    // The fragment payload is reassembled across MPUs (buffer-based, like the signaling
    // fragments); the trailing Skip_XX in Data_Parse consumes it from the cursor.
    const int8u* p = Buffer + Buffer_Offset + (size_t)Element_Offset;
    size_t       n = (size_t)(Element_Size - Element_Offset);
    Element_End0();

    fragment_assembler& Ass = MfuAssemblers[packet_id];

    //A seq jump or first-ever sighting drops any partial.
    if (Ass.State == fragment_assembler::Init)
        Ass.State = fragment_assembler::Skip;
    else if (seq_num != Ass.LastSeq + 1)
    {
        Ass.Data.clear();
        Ass.State = fragment_assembler::Skip;
    }
    Ass.LastSeq = seq_num;

    // HEVC is fed fragment-by-fragment, so each fragment is parsed at its real file offset. AAC is
    // not: its LOAS length header is unknown until the whole DU is reassembled.
    std::map<int16u, media_parser>::iterator MpIt = MediaParsers.find(packet_id);
    bool nal = (MpIt != MediaParsers.end() && MpIt->second.Framing == Es_Nal);

    switch (fragmentation_indicator)
    {
        case 0: // NOT_FRAGMENTED (complete DU)
            Ass.State = fragment_assembler::NotStarted;
            Ass.Data.clear();
            Feed_DataUnit(packet_id, p, n);
            break;
        case 1: // FIRST
            Ass.State = fragment_assembler::InFragment;
            if (nal)
                Feed_NalFragment(packet_id, p, n, true);
            else
                Ass.Data.assign(p, p + n);
            break;
        case 2: // MIDDLE
            if (Ass.State == fragment_assembler::InFragment)
            {
                if (nal)
                    Feed_NalFragment(packet_id, p, n, false);
                else
                    Ass.Data.insert(Ass.Data.end(), p, p + n);
            }
            break;
        case 3: // LAST
            if (Ass.State == fragment_assembler::InFragment)
            {
                if (nal)
                    Feed_NalFragment(packet_id, p, n, false);
                else
                {
                    Ass.Data.insert(Ass.Data.end(), p, p + n);
                    if (!Ass.Data.empty())
                        Feed_DataUnit(packet_id, &Ass.Data[0], Ass.Data.size());
                }
            }
            Ass.Data.clear();
            Ass.State = fragment_assembler::NotStarted;
            break;
        default:
            break;
    }
}

//---------------------------------------------------------------------------
// Child ES parser factory. asset_type (STD-B60 MPT) -> MediaInfo parser + stream
// kind + DU framing. STD-B60 signals HEVC video (hev1) and MPEG-4 AAC audio
// (mp4a); stpp (TTML) and aapp carry no ES parser. A newly assigned asset_type
// is a single added case.
bool File_MmtTlv::Create_MediaParser(int32u asset_type, media_parser& M)
{
    switch (asset_type)
    {
        case ASSET_HEV1:
        case ASSET_HVC1: // same File_Hevc path; for hvc1 the MFU carries slices without the
                         // parameter sets (those are in the MPU metadata), but geometry/colour
                         // still come from the 0x8010 video-component descriptor
            #if defined(MEDIAINFO_HEVC_YES)
            {
                File_Hevc* Parser = new File_Hevc;
                Parser->FrameIsAlwaysComplete = false; // a NAL spans several MFU fragments; stream them
                Parser->MustAdaptSubOffsets = true;    // trace each fragment at its real file offset
                M.Parser.reset(Parser);
                M.StreamKind = Stream_Video;
                M.Framing    = Es_Nal;
                Open_Buffer_Init(Parser);
                return true;
            }
            #else
                return false;
            #endif
        case ASSET_MP4A:
            #if defined(MEDIAINFO_AAC_YES)
            {
                File_Aac* Parser = new File_Aac;
                Parser->Mode = File_Aac::Mode_LATM;
                M.Parser.reset(Parser);
                M.StreamKind = Stream_Audio;
                M.Framing    = Es_Loas;
                Open_Buffer_Init(Parser);
                return true;
            }
            #else
                return false;
            #endif
        default:
            return false; // stpp/aapp and any un-assigned asset_type: no ES parser
    }
}

//---------------------------------------------------------------------------
// A COMPLETE data unit, framed for the child parser (Annex-B / LOAS) and fed.
void File_MmtTlv::Feed_DataUnit(int16u packet_id, const int8u* Data, size_t Size)
{
    std::map<int16u, media_parser>::iterator It = MediaParsers.find(packet_id);
    if (It == MediaParsers.end())
        return;
    media_parser& M = It->second;
    if (M.Done || !M.Parser)
        return;

    std::vector<int8u> Frame;
    if (M.Framing == Es_Loas)
    {
        // The DU is the LATM payload; prepend a LOAS sync header.
        if (Size == 0 || (Size >> 13))
            return;
        Frame.reserve(3 + Size);
        Frame.push_back(0x56);
        Frame.push_back((int8u)(0xE0 | (Size >> 8)));
        Frame.push_back((int8u)(Size & 0xFF));
        Frame.insert(Frame.end(), Data, Data + Size);
    }
    else
    {
        // The DU is be32 length + NAL; emit Annex-B.
        if (Size < 4)
            return;
        int32u nal_size = BigEndian2int32u(Data);
        const int8u* nal = Data + 4;
        size_t       nal_n = Size - 4;
        if (nal_size != nal_n)
            return;
        Frame.reserve(3 + nal_n);
        Frame.push_back(0x00);
        Frame.push_back(0x00);
        Frame.push_back(0x01);
        Frame.insert(Frame.end(), nal, nal + nal_n);
    }

    Open_Buffer_Continue(M.Parser.get(), &Frame[0], Frame.size());
    M.Fed       += Frame.size();
    Media_Bytes += Frame.size();

    if (M.Parser->Status[IsAccepted] || M.Parser->Status[IsFinished])
        M.Done = true;
}

//---------------------------------------------------------------------------
// Feeds one MFU fragment of a NAL to the child at the current element cursor, so its trace lands
// at the fragment's real file offset (MustAdaptSubOffsets re-aligns the child between feeds). The
// first fragment's leading be32 NAL length becomes the Annex-B start code; the rest is fed raw.
void File_MmtTlv::Feed_NalFragment(int16u packet_id, const int8u* Data, size_t Size, bool First)
{
    std::map<int16u, media_parser>::iterator It = MediaParsers.find(packet_id);
    if (It == MediaParsers.end())
        return;
    media_parser& M = It->second;
    if (M.Done || !M.Parser)
        return;

    std::vector<int8u> Frame;
    if (First)
    {
        if (Size < 4) // be32 length prefix
            return;
        Frame.reserve(3 + (Size - 4));
        Frame.push_back(0x00);
        Frame.push_back(0x00);
        Frame.push_back(0x01);
        Frame.insert(Frame.end(), Data + 4, Data + Size);
    }
    else
    {
        if (!Size)
            return;
        Frame.assign(Data, Data + Size);
    }
    if (Frame.empty())
        return;

    Open_Buffer_Continue(M.Parser.get(), &Frame[0], Frame.size());
    M.Fed       += Frame.size();
    Media_Bytes += Frame.size();

    if (M.Parser->Status[IsAccepted] || M.Parser->Status[IsFinished])
        M.Done = true;
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
            length = BigEndian2int32u(p); p += 4; n -= 4;
        }
        else
        {
            if (n < 2) return;
            length = BigEndian2int16u(p); p += 2; n -= 2;
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
    int16u msg_id = BigEndian2int16u(Data);

    if (msg_id == MSG_PA_MESSAGE)
    {
        // id(16) version(8) length(32)
        if (Size < 7) return;
        int32u length = BigEndian2int32u(Data + 3);
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
                consumed = (size_t)4 + BigEndian2int16u(p + 2);
            else
                consumed = (size_t)3 + (BigEndian2int16u(p + 1) & 0x0FFF);
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
        int16u length = BigEndian2int16u(Data + 3);
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
        case TABLE_MPT:
        {
            //The asset list + video MPU-PTS are parsed in File_Mmt; the container commits them here.
            mmt_stream Stream;
            File_Mmt   Signaling;
            Signaling.Complete_Stream = &Stream;
            Open_Buffer_Init(&Signaling);
            Open_Buffer_Continue(&Signaling, Data, Size);
            Open_Buffer_Finalize(&Signaling);

            //Timestamps come from every MPT (start + tail probe); keep the global min/max.
            if (Stream.PtsMinUs >= 0 && (Pts_First_Us < 0 || Stream.PtsMinUs < Pts_First_Us))
                Pts_First_Us = Stream.PtsMinUs;
            if (Stream.PtsMaxUs >= 0 && (Pts_Last_Us < 0 || Stream.PtsMaxUs > Pts_Last_Us))
                Pts_Last_Us = Stream.PtsMaxUs;

            if (!Stream.MptValid)
                break;

            //Transfer (0x8010): newest head-scan value wins; the tail probe's MPT is the next
            //program's, so it does not vote.
            if (Phase != Phase_Tail && Stream.TransferLast >= 1 && Stream.TransferLast < 8)
                Transfer_Last = Stream.TransferLast;

            //The MPT version identifies the current package. A change replaces the track set
            //(dropping what the new program no longer carries); within a version keep the fullest,
            //as the asset list builds up after a tune-in. The tail probe is a different program
            //near EOF, so skip it.
            const std::vector<asset>& Found = Stream.Assets;
            int Media = 0;
            for (size_t i = 0; i < Found.size(); ++i)
                if (Found[i].Type == ASSET_HEV1 || Found[i].Type == ASSET_HVC1 || Found[i].Type == ASSET_MP4A || Found[i].Type == ASSET_STPP)
                    ++Media;
            bool NewVersion = (Stream.MptVersion != Mpt_Version);
            if (Phase == Phase_Scan && Media && (NewVersion || Media > Mpt_AssetCount))
            {
                if (NewVersion)
                {
                    //Drop the previous package's parsers; its packet_ids and count may differ.
                    MediaParsers.clear();
                    Mpt_Version      = Stream.MptVersion;
                    Media_Probe_Done = false;
                    Core_Done_At     = (int64u)-1;
                    Media_Done_Utc   = -1;
                }
                Mpt_AssetCount = Media;
                Assets    = Found;
                Mpt_Found = true;

                //A child ES parser per A/V asset, keyed by packet_id (Text needs none). The
                //factory maps asset_type -> parser + framing, so a new codec is one added row.
                for (size_t i = 0; i < Assets.size(); ++i)
                {
                    const asset& A = Assets[i];
                    if (!A.PacketId || MediaParsers.count(A.PacketId))
                        continue;
                    media_parser M;
                    if (Create_MediaParser(A.Type, M))
                        MediaParsers[A.PacketId] = std::move(M);
                }
            }
            break;
        }
        case TABLE_MH_EIT:
        {
            //The p/f is itself a boundary-window value: re-read until the window passes, so a flip
            //re-commits and re-anchors (the first EIT can precede the first clock).
            if (Eit_Present_Found && boundary_window_passed(Eit_Start_Utc, Now_First, Now_Utc))
                break;

            //Field extraction is in File_Mmt; the boundary/hop logic stays here (it seeks the file).
            mmt_stream Stream;
            File_Mmt   Signaling;
            Signaling.Complete_Stream = &Stream;
            Open_Buffer_Init(&Signaling);
            Open_Buffer_Continue(&Signaling, Data, Size);
            Open_Buffer_Finalize(&Signaling);

            if (Stream.EitSvcIdFound && !Eit_ServiceId_Found) // matched later against MH-SDT
            {
                Eit_ServiceId       = Stream.EitSvcId;
                Eit_ServiceId_Found = true;
            }
            if (Stream.TlvStreamId >= 0 && Tlv_Stream_Id < 0)
                Tlv_Stream_Id = Stream.TlvStreamId;
            if (!Stream.EitParsed)
                break;

            //The boundary decision below compares the present event's window to the wall clock, so
            //it needs one. Do not assume the first EIT arrives before or after the first NTP/TOT:
            //without a clock, leave the present event uncommitted (Eit_Present_Found stays false, so
            //the forward scan cannot finalize) and re-decide on a later EIT once the clock is in hand.
            if (Now_Utc < 0)
                break;

            //If this present event has already ended at "now", it is the previous program's tail
            //(the stream started on a boundary): hop forward and re-scan for the one whose window
            //contains "now".
            bool hopped = false;
            int64s start_s = Mmt_DateTime_To_Seconds(Stream.EitStartDate, Stream.EitStartTime); // JST
            if (start_s >= 0)
            {
                start_s -= Mmt_JST_Offset_Seconds; // -> UTC, to compare with Now_Utc
                Eit_Start_Utc = start_s;
                int dur_h = Mmt_Bcd2((int8u)(Stream.EitDuration >> 16));
                int dur_m = Mmt_Bcd2((int8u)(Stream.EitDuration >> 8));
                int dur_s = Mmt_Bcd2((int8u)(Stream.EitDuration));
                int64s dur   = dur_h * 3600 + dur_m * 60 + dur_s;
                int64s end_s = start_s + dur;
                if (dur > 0 && end_s <= Now_Utc + BOUNDARY_GUARD_SECONDS
                 && Eit_Boundary_Hops < BOUNDARY_MAX_HOPS)
                {
                    //The boundary may reconfigure the A/V, so drop the prior program's probe
                    //and re-derive after the hop.
                    MediaParsers.clear();
                    MfuAssemblers.clear();
                    Assets.clear();
                    Mpt_Found        = false;
                    Mpt_AssetCount   = -1;
                    Transfer_Last    = 0xFF;
                    Media_Probe_Done = false;
                    Media_Bytes      = 0;
                    Core_Done_At     = (int64u)-1; // core re-derives at new pos
                    Media_Done_Utc   = -1;

                    ++Eit_Boundary_Hops;
                    int64u here = File_Offset + Buffer_Offset;
                    //Size the hop to land just past the event's end, from the byte rate
                    //observed so far; a fixed nudge cannot cross a minute-long tail.
                    int64u hop = BOUNDARY_HOP_BYTES;
                    if (Now_Utc > Now_First && end_s + 2 > Now_Utc)
                    {
                        int64u rate = here / (int64u)(Now_Utc - Now_First);
                        if (rate > 0 && (int64u)(end_s + 2 - Now_Utc) > BOUNDARY_HOP_BYTES / rate)
                            hop = rate * (int64u)(end_s + 2 - Now_Utc);
                    }
                    int64u target = here + hop;
                    if (File_Size != (int64u)-1 && target >= File_Size)
                        target = here + BOUNDARY_HOP_BYTES / 4; // small final nudge
                    GoTo(target, "MmtTlv");
                    hopped = true;
                }
            }
            if (hopped)
                break;

            Eit_EventId   = Stream.EitEventId;
            Eit_StartDate = Stream.EitStartDate;
            Eit_StartTime = Stream.EitStartTime;
            Eit_Duration  = Stream.EitDuration;
            Eit_Language  = Stream.EitLanguage;
            Eit_EventName = Stream.EitName;
            Eit_EventText = Stream.EitText;
            Eit_Present_Found = true; // present event only
            break;
        }
        case TABLE_MH_SDT:
        {
            //Migrated to File_Mmt (+ File_Mmt_Descriptors for the loop). The container passes the
            //present-EIT service_id in and keeps the once-only latch on the recovered name.
            if (Sdt_Found)
                break;
            mmt_stream Stream;
            Stream.EitServiceId      = Eit_ServiceId;
            Stream.EitServiceIdFound = Eit_ServiceId_Found;
            File_Mmt Signaling;
            Signaling.Complete_Stream = &Stream;
            Open_Buffer_Init(&Signaling);
            Open_Buffer_Continue(&Signaling, Data, Size);
            Open_Buffer_Finalize(&Signaling);
            if (Stream.SdtFound)
            {
                Sdt_Found       = true;
                Sdt_ServiceName = Stream.ServiceName;
                Sdt_Provider    = Stream.Provider;
                Sdt_ServiceType = Stream.ServiceType;
            }
            if (Stream.TlvStreamId >= 0 && Tlv_Stream_Id < 0)
                Tlv_Stream_Id = Stream.TlvStreamId;
            break;
        }
        case TABLE_MH_TOT:
        {
            //Migrated to the File_Mmt signaling sub-parser (idiomatic, traced). The container
            //keeps its authoritative clock; the sub-parser only reports the decoded MH-TOT time.
            mmt_stream Stream;
            File_Mmt   Signaling;
            Signaling.Complete_Stream = &Stream;
            Open_Buffer_Init(&Signaling);
            Open_Buffer_Continue(&Signaling, Data, Size);
            Open_Buffer_Finalize(&Signaling);
            if (Stream.TotSeen)
            {
                Tot_Seen = true;
                Now_Utc  = Stream.TotUtc; // authoritative, overrides NTP
                Note_StreamNow(Stream.TotUtc);
            }
            break;
        }
        case TABLE_ECM_0:   // presence = CAS active; not parsed
        case TABLE_ECM_1:   Ecm_Seen = true; // and name it in the trace, below
            [[fallthrough]];
        default:
            #if MEDIAINFO_TRACE
            //Not parsed; route through the signaling sub-parser only for the
            //trace, so recognized B60 tables (Table 4-8) show by name.
            if (Trace_Activated)
            {
                mmt_stream Stream;
                File_Mmt   Signaling;
                Signaling.Complete_Stream = &Stream;
                Open_Buffer_Init(&Signaling);
                Open_Buffer_Continue(&Signaling, Data, Size);
                Open_Buffer_Finalize(&Signaling);
            }
            #endif //MEDIAINFO_TRACE
            break;
    }
}

//***************************************************************************
// MH-TOT: current wall clock (JST), in the clear.
//***************************************************************************


//***************************************************************************
// Fallback clock: the NTP transmit timestamp in the IPv6/UDP time-distribution
// packet, used until MH-TOT arrives. Only a coarse "now" is needed.
//***************************************************************************

//---------------------------------------------------------------------------
void File_MmtTlv::Parse_Ntp()
{
    const int64u IPV6_HEADER = 40;
    const int64u UDP_HEADER  = 8;
    const int64u NTP_MIN     = 48;
    if (Element_Size - Element_Offset < IPV6_HEADER + UDP_HEADER + NTP_MIN)
        return;
    // Gate before parsing: IPv6 (version 6, next_header UDP=17) carrying a plausible NTP payload
    // (non-zero mode, sane stratum). Peeked so a non-NTP datagram leaves no half-trace.
    const int8u* P   = Buffer + Buffer_Offset + (size_t)Element_Offset;
    const int8u* Ntp = P + IPV6_HEADER + UDP_HEADER;
    if ((P[0] >> 4) != 6 || P[6] != 17 || (Ntp[0] & 0x07) == 0 || Ntp[1] > 15)
        return;

    int32u ntp_secs;
    Element_Begin1("IPv6/UDP/NTP");
        Skip_XX(IPV6_HEADER,                                    "IPv6 header");
        Skip_XX(UDP_HEADER,                                     "UDP header");
        Element_Begin1("NTP");
            Skip_B1(                                            "LI/VN/Mode");
            Skip_B1(                                            "Stratum");
            Skip_B1(                                            "Poll");
            Skip_B1(                                            "Precision");
            Skip_B4(                                            "Root Delay");
            Skip_B4(                                            "Root Dispersion");
            Skip_B4(                                            "Reference ID");
            Skip_XX(8,                                          "Reference Timestamp");
            Skip_XX(8,                                          "Origin Timestamp");
            Skip_XX(8,                                          "Receive Timestamp");
            Get_B4 (ntp_secs,                                   "Transmit Timestamp (seconds)");
            Skip_B4(                                            "Transmit Timestamp (fraction)");
        Element_End0();
    Element_End0();

    if (ntp_secs == 0)
        return;
    int64s utc = (int64s)ntp_secs - NTP_UNIX_EPOCH_DELTA;
    if (!Tot_Seen)
        Now_Utc = utc; // provisional until MH-TOT
    Note_StreamNow(utc);
}

//---------------------------------------------------------------------------
void File_MmtTlv::Note_StreamNow(int64s Utc)
{
    if (Now_First < 0 || Utc < Now_First)
        Now_First = Utc;
    if (Now_Last < 0 || Utc > Now_Last)
        Now_Last = Utc;
}

} //NameSpace

#endif //MEDIAINFO_MMTTLV_YES
