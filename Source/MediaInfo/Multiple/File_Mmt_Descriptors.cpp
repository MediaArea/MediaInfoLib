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
#include "MediaInfo/Multiple/File_Mmt_Descriptors.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Descriptor tag -> name, using the exact B60 syntax-structure identifiers
// (STD-B60 Table 4-10 and the descriptor bodies); "" if unrecognized.
//***************************************************************************

namespace
{
    const char* Mmt_Descriptor_Name(int16u tag)
    {
        switch (tag)
        {
            case 0x0000 : return "CRI_Descriptor";
            case 0x0001 : return "MPU_Timestamp_Descriptor";
            case 0x0002 : return "Dependency_Descriptor";
            case 0x0003 : return "GFDT_Descriptor";
            case 0x8000 : return "Asset_Group_Descriptor";
            case 0x8001 : return "Event_Package_Descriptor";
            case 0x8002 : return "Background_Color_Descriptor";
            case 0x8003 : return "MPU_Presentation_Region_Descriptor";
            case 0x8004 : return "Access_Control_Descriptor";
            case 0x8005 : return "Scrambler_Descriptor";
            case 0x8006 : return "Message_Authentication_Method_Descriptor";
            case 0x8007 : return "Emergency_Information_Descriptor";
            case 0x8008 : return "MH-MPEG-4_Audio_Descriptor";
            case 0x8009 : return "MH-MPEG-4_Audio_Extension_Descriptor";
            case 0x800A : return "MH-HEVC_Descriptor";
            case 0x800C : return "MH-Event_Group_Descriptor";
            case 0x800D : return "MH-Service_List_Descriptor";
            case 0x8010 : return "Video_Component_Descriptor";
            case 0x8011 : return "MH-Stream_Identifier_Descriptor";
            case 0x8012 : return "MH-Content_Descriptor";
            case 0x8013 : return "MH-Parental_Rating_Descriptor";
            case 0x8014 : return "MH-Audio_Component_Descriptor";
            case 0x8015 : return "MH-Target_Region_Descriptor";
            case 0x8016 : return "MH-Series_Descriptor";
            case 0x8017 : return "MH-SI_Parameter_Descriptor";
            case 0x8018 : return "MH-Broadcaster_Name_Descriptor";
            case 0x8019 : return "MH-Service_Descriptor";
            case 0x801A : return "IP_Data_Flow_Descriptor";
            case 0x801B : return "MH-CA_Startup_Descriptor";
            case 0x801C : return "MH-Type_Descriptor";
            case 0x801D : return "MH-Info_Descriptor";
            case 0x801E : return "MH-Expire_Descriptor";
            case 0x801F : return "MH-Compression_Type_Descriptor";
            case 0x8020 : return "MH-Data_Component_Descriptor";
            case 0x8021 : return "UTC-NPT_Reference_Descriptor";
            case 0x8023 : return "MH-Local_Time_Offset_Descriptor";
            case 0x8024 : return "MH-Component_Group_Descriptor";
            case 0x8025 : return "MH-Logo_Transmission_Descriptor";
            case 0x8026 : return "MPU_Extended_Timestamp_Descriptor";
            case 0x8027 : return "MPU_Download_Content_Descriptor";
            case 0x8028 : return "MH-Network_Download_Content_Descriptor";
            case 0x8029 : return "MH-Application_Descriptor";
            case 0x802A : return "MH-Transport_Protocol_Descriptor";
            case 0x802B : return "MH-Simple_Application_Location_Descriptor";
            case 0x802C : return "MH-Application_Boundary_and_Permission_Descriptor";
            case 0x802D : return "MH-Autostart_Priority_Descriptor";
            case 0x802E : return "MH-Cache_Control_Info_Descriptor";
            case 0x802F : return "MH-Randomized_Latency_Descriptor";
            case 0x8030 : return "Linked_PU_Descriptor";
            case 0x8031 : return "Locked_Cache_Descriptor";
            case 0x8032 : return "Unlocked_Cache_Descriptor";
            case 0x8033 : return "MH-DL_Protection_Descriptor";
            case 0x8034 : return "Application_Service_Descriptor";
            case 0x8035 : return "MPU_Node_Descriptor";
            case 0x8036 : return "PU_Structure_Descriptor";
            case 0x8037 : return "MH-Hierarchy_Descriptor";
            case 0x8038 : return "Content_Copy_Control_Descriptor";
            case 0x8039 : return "Content_Usage_Control_Descriptor";
            case 0x803A : return "MH-External_Application_Control_Descriptor";
            case 0x803B : return "MH-Playback_Application_Descriptor";
            case 0x803C : return "MH-Simple_Playback_Application_Location_Descriptor";
            case 0x803D : return "MH-Application_Expiration_Descriptor";
            case 0x803E : return "Related_Broadcaster_Descriptor";
            case 0x803F : return "Multimedia_Service_Info_Descriptor";
            case 0x8040 : return "Emergency_News_Descriptor";
            case 0x8041 : return "MH-CA_Contract_Info_Descriptor";
            case 0x8042 : return "MH-CA_Service_Descriptor";
            case 0xF000 : return "MH-Linkage_Descriptor";
            case 0xF001 : return "MH-Short_Event_Descriptor";
            case 0xF002 : return "MH-Extended_Event_Descriptor";
            case 0xF003 : return "Event_Message_Descriptor";
            case 0xF004 : return "MH-stuffing_descriptor";
            case 0xF005 : return "MH-broadcast_id_descriptor";
            case 0xF006 : return "MH-network_identification_descriptor";
            default     : return "";
        }
    }

    //Coded-field meanings (ARIB STD-B60); "" leaves the field with no annotation.
    const char* Mmt_video_resolution(int8u c)
    {
        switch (c)
        {
            case 1 : return "180";
            case 2 : return "240";
            case 3 : return "480 (525)";
            case 4 : return "720 (750)";
            case 5 : return "1080 (1125)";
            case 6 : return "2160";
            case 7 : return "4320";
            default: return "";
        }
    }
    const char* Mmt_video_aspect_ratio(int8u c)
    {
        switch (c)
        {
            case 1 : return "4:3";
            case 2 : return "16:9 with pan vector";
            case 3 : return "16:9 without pan vector";
            case 4 : return "> 16:9";
            default: return "";
        }
    }
    const char* Mmt_video_frame_rate(int8u c)
    {
        switch (c)
        {
            case 1 : return "15";
            case 2 : return "24/1.001";
            case 3 : return "24";
            case 4 : return "25";
            case 5 : return "30/1.001";
            case 6 : return "30";
            case 7 : return "50";
            case 8 : return "60/1.001";
            case 9 : return "60";
            case 10: return "100";
            case 11: return "120/1.001";
            case 12: return "120";
            default: return "";
        }
    }
    const char* Mmt_video_transfer(int8u c)
    {
        switch (c)
        {
            case 1 : return "BT.709";
            case 2 : return "IEC 61966-2-4";
            case 3 : return "BT.2020";
            case 4 : return "BT.2100 PQ";
            case 5 : return "BT.2100 HLG";
            default: return "";
        }
    }
    const char* Mmt_stream_content(int8u c)
    {
        switch (c)
        {
            case 2 : return "Sound (system not specified)";
            case 3 : return "MPEG-4 AAC";
            case 4 : return "MPEG-4 ALS";
            default: return "";
        }
    }
    const char* Mmt_audio_mode(int8u c) //component_type bits 4-0 (Table 7-60)
    {
        switch (c & 0x1F)
        {
            case 0x01: return "1/0 mode (single mono)";
            case 0x02: return "1/0 + 1/0 mode (dual mono)";
            case 0x03: return "2/0 mode (stereo)";
            case 0x04: return "2/1 mode";
            case 0x05: return "3/0 mode";
            case 0x06: return "2/2 mode";
            case 0x07: return "3/1 mode";
            case 0x08: return "3/2 mode";
            case 0x09: return "3/2 + LFE mode (3/2.1 mode)";
            case 0x0A: return "3/3.1 mode";
            case 0x0B: return "2/0/0-2/0/2-0.1 mode";
            case 0x0C: return "5/2.1 mode";
            case 0x0D: return "3/2/2.1 mode";
            case 0x0E: return "2/0/0-3/0/2-0.1 mode";
            case 0x0F: return "0/2/0-3/0/2-0.1 mode";
            case 0x10: return "2/0/0-3/2/3-0.2 mode";
            case 0x11: return "3/3/3-5/2/3-3/0/0.2 mode";
            default  : return "";
        }
    }
    const char* Mmt_sampling_rate(int8u c)
    {
        switch (c)
        {
            case 1 : return "16 kHz";
            case 2 : return "22.05 kHz";
            case 3 : return "24 kHz";
            case 5 : return "32 kHz";
            case 6 : return "44.1 kHz";
            case 7 : return "48 kHz";
            default: return "";
        }
    }
    const char* Mmt_data_component_id(int16u c)
    {
        switch (c)
        {
            case 0x0020: return "Caption (2nd generation)";
            case 0x0021: return "Multimedia (2nd generation)";
            default    : return "";
        }
    }
    const char* Mmt_service_type(int8u c)
    {
        switch (c)
        {
            case 0x01: return "Digital TV service";
            case 0x02: return "Digital audio service";
            case 0xA1: return "Special video service";
            case 0xA4: return "Engineering service";
            case 0xC0: return "Data service";
            case 0xC1: return "Storage type service using TLV";
            case 0xC2: return "Multimedia service";
            default  : return "";
        }
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::FileHeader_Parse()
{
    Accept();
}

//***************************************************************************
// Buffer - Per element (one descriptor)
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Header_Parse()
{
    //MMT descriptor: 16-bit tag. The length field is 16-bit for the 0xF000-0xFFFF
    //tags (MH-EIT short/extended event) and 8-bit below that (STD-B60 tag table).
    int16u descriptor_tag;
    Get_B2 (descriptor_tag,                                     "descriptor_tag");
    if (descriptor_tag >= 0xF000)
    {
        int16u descriptor_length;
        Get_B2 (descriptor_length,                              "descriptor_length");
        Header_Fill_Size((int64u)4 + descriptor_length);
    }
    else
    {
        int8u descriptor_length;
        Get_B1 (descriptor_length,                              "descriptor_length");
        Header_Fill_Size((int64u)3 + descriptor_length);
    }
    Header_Fill_Code(descriptor_tag, Ztring().From_Number(descriptor_tag, 16));
}

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Data_Parse()
{
    switch (Element_Code)
    {
        case 0x0001 : Element_Name("MPU_Timestamp_Descriptor");       Descriptor_0001(); break;
        case 0x8000 : Element_Name("Asset_Group_Descriptor");         Descriptor_8000(); break;
        case 0x8010 : Element_Name("Video_Component_Descriptor");     Descriptor_8010(); break;
        case 0x8014 : Element_Name("MH-Audio_Component_Descriptor");  Descriptor_8014(); break;
        case 0x8019 : Element_Name("MH-Service_Descriptor");          Descriptor_8019(); break;
        case 0x8020 : Element_Name("MH-Data_Component_Descriptor");   Descriptor_8020(); break;
        case 0xF001 : Element_Name("MH-Short_Event_Descriptor");      Descriptor_F001(); break;
        default     :
            {
                const char* Name = Mmt_Descriptor_Name((int16u)Element_Code);
                if (Name[0])
                    Element_Name(Name);
                Skip_XX(Element_Size - Element_Offset, "Data");
            }
            break;
    }
}

//***************************************************************************
// Descriptors
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Descriptor_0001()
{
    //Repeated { mpu_sequence_number(32), presentation_time(64, NTP 32.32) }. Only
    //the video (HEVC) asset's timestamps define the stream span.
    if (!Complete_Stream || !CurrentAsset
     || (CurrentAsset->Type != 0x31766568 /*'hev1'*/ && CurrentAsset->Type != 0x31637668 /*'hvc1'*/))
    {
        Skip_XX(Element_Size - Element_Offset,                  "Data");
        return;
    }
    while (Element_Size - Element_Offset >= 12)
    {
        int32u seconds, fraction;
        Skip_B4(                                                "mpu_sequence_number");
        Get_B4 (seconds,                                        "presentation_time (seconds)");
        Get_B4 (fraction,                                       "presentation_time (fraction)");
        int64s us = (int64s)((int64u)seconds * 1000000ULL
                  + ((int64u)fraction * 1000000ULL) / 0x100000000ULL);
        if (Complete_Stream->PtsMinUs < 0 || us < Complete_Stream->PtsMinUs) Complete_Stream->PtsMinUs = us;
        if (Complete_Stream->PtsMaxUs < 0 || us > Complete_Stream->PtsMaxUs) Complete_Stream->PtsMaxUs = us;
    }
    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "Data");
}

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Descriptor_8000()
{
    //group_identification(8) selection_level(8): main + rain-fade backup of a
    //stream share the group; selection_level 0 is the default member.
    if (Element_Size - Element_Offset >= 2 && CurrentAsset && CurrentAsset->GroupId < 0)
    {
        int8u group_id, selection_level;
        Get_B1 (group_id,                                       "group_identification");
        Get_B1 (selection_level,                                "selection_level");
        CurrentAsset->GroupId        = group_id;
        CurrentAsset->SelectionLevel = selection_level;
    }
    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "Data");
}

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Descriptor_8010()
{
    if (Element_Size - Element_Offset < 2 || !CurrentAsset)
    {
        if (Element_Offset < Element_Size)
            Skip_XX(Element_Size - Element_Offset,              "Data");
        return;
    }
    int8u video_resolution, video_aspect_ratio, video_scan_flag, video_frame_rate;
    BS_Begin();
    Get_S1 (4, video_resolution,                                "video_resolution");   Param_Info1(Mmt_video_resolution(video_resolution));
    Get_S1 (4, video_aspect_ratio,                              "video_aspect_ratio"); Param_Info1(Mmt_video_aspect_ratio(video_aspect_ratio));
    Get_S1 (1, video_scan_flag,                                 "video_scan_flag");    Param_Info1(video_scan_flag ? "Progressive" : "Interlaced");
    Skip_S1(2,                                                  "reserved");
    Get_S1 (5, video_frame_rate,                                "video_frame_rate");   Param_Info1(Mmt_video_frame_rate(video_frame_rate));
    BS_End();
    CurrentAsset->VideoResolution = video_resolution;
    CurrentAsset->VideoAspect     = video_aspect_ratio;
    CurrentAsset->VideoScan       = video_scan_flag;
    CurrentAsset->VideoFrameRate  = video_frame_rate;
    if (Element_Size - Element_Offset >= 3)
    {
        int8u transfer;
        Skip_B2(                                                "component_tag");
        BS_Begin();
        Get_S1 (4, transfer,                                    "video_transfer_characteristics"); Param_Info1(Mmt_video_transfer(transfer));
        Skip_S1(4,                                              "reserved");
        BS_End();
        if (Complete_Stream && transfer >= 1 && transfer < 8)
            Complete_Stream->TransferLast = transfer; // container applies the phase gate
    }
    if (Element_Size - Element_Offset >= 3)
    {
        Ztring Language;
        Get_UTF8(3, Language,                                   "ISO_639_language_code");
    }
    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "text");
}

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Descriptor_8014()
{
    //component_type packs the handicapped mode (bits 6-5) and audio_mode (bits
    //4-0); the flags byte carries ES_multi, main_component and the sampling_rate
    //code, then the 3-byte language (a second when ES_multi) and the UTF-8 text.
    if (Element_Size - Element_Offset < 10 || !CurrentAsset)
    {
        if (Element_Offset < Element_Size)
            Skip_XX(Element_Size - Element_Offset,              "Data");
        return;
    }
    int8u component_type, sampling_rate;
    bool  es_multi, main_component;
    int8u stream_content;
    BS_Begin();
    Skip_S1(4,                                                  "reserved_future_use");
    Get_S1 (4, stream_content,                                  "stream_content"); Param_Info1(Mmt_stream_content(stream_content));
    BS_End();
    Get_B1 (component_type,                                     "component_type"); Param_Info1(Mmt_audio_mode(component_type));
    Skip_B2(                                                    "component_tag");
    Skip_B1(                                                    "stream_type");
    Skip_B1(                                                    "simulcast_group_tag");
    BS_Begin();
    Get_SB (es_multi,                                           "ES_multi_lingual_flag");
    Get_SB (main_component,                                     "main_component_flag");
    Skip_S1(2,                                                  "quality_indicator");
    Get_S1 (3, sampling_rate,                                   "sampling_rate"); Param_Info1(Mmt_sampling_rate(sampling_rate));
    Skip_SB(                                                    "reserved_future_use");
    BS_End();
    CurrentAsset->Handicapped   = (component_type >> 5) & 0x03;
    CurrentAsset->AudioMode     = component_type & 0x1F;
    CurrentAsset->MainComponent = main_component;
    CurrentAsset->SamplingCode  = sampling_rate;
    Ztring Language;
    Get_UTF8(3, Language,                                       "ISO_639_language_code");
    CurrentAsset->Language = Language;
    if (es_multi && Element_Size - Element_Offset >= 3)
        Skip_UTF8(3,                                            "ISO_639_language_code_2");
    if (Element_Offset < Element_Size)
    {
        Ztring Text;
        Get_UTF8(Element_Size - Element_Offset, Text,           "text");
        CurrentAsset->Title = Text;
    }
}

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Descriptor_8020()
{
    //data_component_id 0x0020 = caption; the Additional_Arib_Subtitle_Info (STD-B60 Table 9-4)
    //carries the ISO_639_language_code, then a type byte whose bits 7-6 are 01 = superimposed
    //text, 00 = subtitle.
    if (Element_Size - Element_Offset >= 8 && CurrentAsset)
    {
        int16u data_component_id;
        Get_B2 (data_component_id,                              "data_component_id"); Param_Info1(Mmt_data_component_id(data_component_id));
        if (data_component_id == 0x0020)
        {
            Ztring Language;
            int8u  subtitle_type;
            Skip_XX(2,                                          "Additional_Arib_Subtitle_Info");
            Get_UTF8(3, Language,                               "ISO_639_language_code");
            BS_Begin();
            Get_S1 (2, subtitle_type,                           "subtitle_type");
            Skip_S1(6,                                          "reserved");
            BS_End();
            CurrentAsset->Language    = Language;
            CurrentAsset->Superimpose = subtitle_type == 0x01;
        }
    }
    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "Data");
}

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Descriptor_F001()
{
    //ISO_639_language(24) event_name_length(8) event_name text_length(16) text.
    //Decrypted MMT SI text is UTF-8 (no ARIB STD-B24 decode). text_length is
    //16-bit here - an 8-bit read would hit the high 0x00 and yield empty text.
    if (Element_Size - Element_Offset < 4 || !Complete_Stream)
    {
        Skip_XX(Element_Size - Element_Offset,                  "Data");
        return;
    }
    Ztring Language, Name, Text;
    int8u  event_name_length;
    Get_UTF8(3, Language,                                       "ISO_639_language_code");
    Complete_Stream->EitLanguage = Language;
    Get_B1 (event_name_length,                                  "event_name_length");
    if (event_name_length && Element_Offset + event_name_length <= Element_Size)
    {
        Get_UTF8(event_name_length, Name,                       "event_name");
        Complete_Stream->EitName = Name;
    }
    if (Element_Offset + 2 <= Element_Size)
    {
        int16u text_length;
        Get_B2 (text_length,                                    "text_length");
        if (text_length && Element_Offset + text_length <= Element_Size)
        {
            Get_UTF8(text_length, Text,                         "text");
            Complete_Stream->EitText = Text;
        }
    }
    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "Data");
}

//---------------------------------------------------------------------------
void File_Mmt_Descriptors::Descriptor_8019()
{
    //service_type(8) provider_name_length(8) provider(UTF-8)
    //service_name_length(8) service_name(UTF-8)
    if (Element_Size - Element_Offset < 2)
    {
        Skip_XX(Element_Size - Element_Offset,                  "Data");
        return;
    }
    Ztring Provider, ServiceName;
    int8u  service_type, provider_name_length, service_name_length;
    Get_B1 (service_type,                                       "service_type"); Param_Info1(Mmt_service_type(service_type));
    Get_B1 (provider_name_length,                               "provider_name_length");
    if (provider_name_length && Element_Offset + provider_name_length <= Element_Size)
        Get_UTF8(provider_name_length, Provider,                "service_provider_name");
    //A malformed provider length leaves the rest to the trailing skip.
    if (Element_Offset < Element_Size)
    {
        Get_B1 (service_name_length,                            "service_name_length");
        if (service_name_length && Element_Offset + service_name_length <= Element_Size)
            Get_UTF8(service_name_length, ServiceName,          "service_name");
    }
    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "Data");

    if (Complete_Stream)
    {
        if (!Provider.empty())
            Complete_Stream->Provider = Provider;
        if (!ServiceName.empty())
            Complete_Stream->ServiceName = ServiceName;
        const char* Type = Mmt_service_type(service_type);
        if (Type[0])
            Complete_Stream->ServiceType = Ztring().From_UTF8(Type);
    }
}

} //NameSpace

#endif //MEDIAINFO_MMTTLV_YES
