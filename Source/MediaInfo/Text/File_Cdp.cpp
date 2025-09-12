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
#if defined(MEDIAINFO_CDP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_Cdp.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if defined(MEDIAINFO_EIA608_YES)
    #include "MediaInfo/Text/File_Eia608.h"
#endif
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
using namespace std;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Cdp_cc_type(int8u cc_type)
{
    switch (cc_type)
    {
        case  0 : return "CEA-608 line 21 field 1 closed captions"; //closed caption 3 if this is second field
        case  1 : return "CEA-608 line 21 field 2 closed captions"; //closed caption 4 if this is second field
        case  2 : return "DTVCC Channel Packet Data";
        case  3 : return "DTVCC Channel Packet Start";
        default : return "";
    }
}

//---------------------------------------------------------------------------
static float64 Cdp_cdp_frame_rate(int8u cdp_frame_rate)
{
    switch (cdp_frame_rate)
    {
        case  1 : return (float64)24/1.001;
        case  2 : return (float64)24;
        case  3 : return (float64)25;
        case  4 : return (float64)30/1.001;
        case  5 : return (float64)30;
        case  6 : return (float64)50;
        case  7 : return (float64)60/1.001;
        case  8 : return (float64)60;
        default : return (float64) 0;
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Cdp::File_Cdp()
:File__Analyze()
{
    //Config
    PTS_DTS_Needed=true;
    MustSynchronize=true;

    //In
    AspectRatio=0;

    //Temp
    ParserName="CDP";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Cdp;
        StreamIDs_Width[0]=1;
    #endif //MEDIAINFO_EVENTS
    Streams.resize(3); //CEA-608 Field 1, CEA-608 Field 2, CEA-708 Channel
    Streams_Count=0;

    //In
    WithAppleHeader=false;
    AspectRatio=0;

    //Temp
    cdp_frame_rate=(int8u)-1;

    //Descriptors
    #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
        ServiceDescriptors=new File__Analyze::servicedescriptors;
    #endif

    //cdp_length
    cdp_length_Min=(int8u)-1;
    cdp_length_Max=0;
}

//---------------------------------------------------------------------------
File_Cdp::~File_Cdp()
{
    for (size_t Pos=0; Pos<Streams.size(); Pos++)
        delete Streams[Pos]; //Streams[Pos]=NULL

    //EIA-708 descriptors
    #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
        delete ServiceDescriptors;
    #endif
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Cdp::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, WithAppleHeader?"Final Cut CDP":"CDP");
}

//---------------------------------------------------------------------------
void File_Cdp::Streams_Update()
{
    Clear(Stream_Text);

    //Per stream
    for (size_t Pos=0; Pos<Streams.size(); Pos++)
        if (Streams[Pos] && Streams[Pos]->Parser && Streams[Pos]->Parser->Status[IsFilled] /*&& Streams[Pos]->Parser->Status[IsUpdated]*/ && Streams[Pos]->Parser->Count_Get(Stream_Text))
            Streams_Update_PerStream(Pos);
}

//---------------------------------------------------------------------------
void File_Cdp::Streams_Update_PerStream(size_t Pos)
{
    if (Streams[Pos]==NULL)
        return;

    Update(Streams[Pos]->Parser);

    if (Streams[Pos]->Parser)
    {
        for (size_t Pos2=0; Pos2<Streams[Pos]->Parser->Count_Get(Stream_Text); Pos2++)
        {
            Stream_Prepare(Stream_Text);
            Merge(*Streams[Pos]->Parser, Stream_Text, Pos2, StreamPos_Last);
            if (WithAppleHeader)
                Fill(Stream_Text, StreamPos_Last, "MuxingMode", "Final Cut");
            Fill(Stream_Text, StreamPos_Last, "MuxingMode", "CDP");
            if (cdp_frame_rate!=(int8u)-1)
                Fill(Stream_Text, StreamPos_Last, Text_FrameRate, Cdp_cdp_frame_rate(cdp_frame_rate));
            Fill(Stream_Text, StreamPos_Last, Text_ID, Streams[Pos]->Parser->Retrieve(Stream_Text, Pos2, Text_ID), true);

            //cdp_length
            if (cdp_length_Min<=cdp_length_Max)
            {
                Fill(Stream_Text, StreamPos_Last, "cdp_length_Min", cdp_length_Min, 10, true);
                Fill(Stream_Text, StreamPos_Last, "cdp_length_Max", cdp_length_Max, 10, true);
                Fill_SetOptions(Stream_Text, StreamPos_Last, "cdp_length_Min", "N NT");
                Fill_SetOptions(Stream_Text, StreamPos_Last, "cdp_length_Max", "N NT");
            }
        }

        Ztring LawRating=Streams[Pos]->Parser->Retrieve(Stream_General, 0, General_LawRating);
        if (!LawRating.empty())
            Fill(Stream_General, 0, General_LawRating, LawRating, true);
        Ztring Title=Streams[Pos]->Parser->Retrieve(Stream_General, 0, General_Title);
        if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
            Fill(Stream_General, 0, General_Title, Title);
    }
}

//---------------------------------------------------------------------------
void File_Cdp::Streams_Finish()
{
    Clear(Stream_Text);

    //Per stream
    for (size_t Pos=0; Pos<Streams.size(); Pos++)
        if (Streams[Pos] && Streams[Pos]->Parser && Streams[Pos]->Parser->Status[IsAccepted] /*&& Streams[Pos]->Parser->Status[IsUpdated]*/)
        {
            Finish(Streams[Pos]->Parser);
            Streams_Update_PerStream(Pos);
        }
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
void File_Cdp::Read_Buffer_Unsynched()
{
    //Parsing
    for (size_t Pos=0; Pos<Streams.size(); Pos++)
        if (Streams[Pos] && Streams[Pos]->Parser)
            Streams[Pos]->Parser->Open_Buffer_Unsynch();
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Cdp::Synchronize()
{
    //Synchronizing
    while (Buffer_Offset+3<=Buffer_Size)
    {
        while (Buffer_Offset+3<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x96
                                             || Buffer[Buffer_Offset+1]!=0x69))
        {
            Buffer_Offset++;
            while (Buffer_Offset<Buffer_Size && Buffer[Buffer_Offset]!=0x96)
                Buffer_Offset++;
        }

        if (Buffer_Offset+3>Buffer_Size)
            break;

        //CRC
        int8u CRC=0;
        int8u cdp_length=Buffer[Buffer_Offset+2];

        //Must have enough buffer for having header
        if (Buffer_Offset+cdp_length>Buffer_Size)
            return false;

        const int8u* Buffer_Temp=Buffer+Buffer_Offset;
        const int8u* Buffer_End=Buffer+Buffer_Offset+cdp_length;
        while (Buffer_Temp<Buffer_End)
            CRC+=*(Buffer_Temp++);
        if (!CRC)
            break;

        Buffer_Offset++;
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+3>Buffer_Size)
    {
        if (Buffer_Offset+3==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x9669)
            Buffer_Offset++;
        if (Buffer_Offset+2==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x9669)
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && CC1(Buffer+Buffer_Offset)!=0x96)
            Buffer_Offset++;
        return false;
    }

    //Synched is OK
    return true;
}

//---------------------------------------------------------------------------
bool File_Cdp::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+3>Buffer_Size)
        return false;

    //Quick test of synchro
    if (Buffer[Buffer_Offset  ]!=0x96
     || Buffer[Buffer_Offset+1]!=0x69)
        Synched=false;

    if (Synched)
    {
        //CRC
        int8u CRC=0;
        int8u cdp_length=Buffer[Buffer_Offset+2];

        //Must have enough buffer for having header
        if (Buffer_Offset+cdp_length>Buffer_Size)
            return false;

        const int8u* Buffer_Temp=Buffer+Buffer_Offset;
        const int8u* Buffer_End=Buffer+Buffer_Offset+cdp_length;
        while (Buffer_Temp<Buffer_End)
            CRC+=*(Buffer_Temp++);
        if (CRC)
            Synched=false;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Cdp::Header_Parse()
{
    //Parsing
    int32u cdp_identifier_length; // For trace feature, we don't want to parse with classic Get() and cdp_identifier was already tested during Sync test
    Peek_B3(cdp_identifier_length);

    Header_Fill_Code(0, "CDP packet");
    Header_Fill_Size(cdp_identifier_length&0xFF);
}

//---------------------------------------------------------------------------
void File_Cdp::Data_Parse()
{
    if (!Status[IsAccepted])
        Accept("CDP");

    while(Element_Offset<Element_Size)
    {
        if (!IsSub)
            Element_Begin1("CDP");

        cdp_header();
        int64u End=cdp_length;
        if (End>Element_Size)
            End=Element_Size;
        while(Element_Offset<End)
        {
            int8u section_id;
            Peek_L1(section_id);
            switch (section_id)
            {
                case 0x71 : time_code_section(); break;
                case 0x72 : ccdata_section(); break;
                case 0x73 : ccsvcinfo_section(); break;
                case 0x74 : cdp_footer(); break;
                case 0xFF : Skip_B1("Padding?"); break;
                default   : if (section_id>=0x75 && section_id<=0xEF)
                                future_section();
                            else
                                Skip_XX(Element_Size-Element_Offset, "Unknown");
            }
        }

        if (!IsSub)
            Element_End1("CDP");
    }

    FILLING_BEGIN();
        if (!Status[IsFilled] && Frame_Count>=1024 && Config->ParseSpeed<1.0)
            Fill();
        if (!IsSub && Config->ParseSpeed<1.0 && Frame_Count>=300)
            Finish();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Cdp::Read_Buffer_Continue()
{
    if (Buffer_Size==0)
        return;

    if (WithAppleHeader)
    {
        int32u Size, Magic;
        Get_B4 (Size,                                           "Size");
        Get_B4 (Magic,                                          "Magic");

        FILLING_BEGIN();
            if (Magic!=0x63636470)
            {
                Reject("CDP");
                Skip_XX(Element_Size-Element_Offset,            "Junk");
                return;
            }
        FILLING_END();
    }
}

//***************************************************************************
// Functions
//***************************************************************************

//---------------------------------------------------------------------------
void File_Cdp::cdp_header()
{
    Element_Begin1("cdp_header");
    int16u cdp_identifier;
    Get_B2 (   cdp_identifier,                                  "cdp_identifier");
    Get_B1 (   cdp_length,                                      "cdp_length");
    BS_Begin();
    Get_S1 (4, cdp_frame_rate,                                  "cdp_frame_rate"); Param_Info1(Ztring::ToZtring(Cdp_cdp_frame_rate(cdp_frame_rate))+__T(" fps"));
    Skip_S1(4,                                                  "Reserved");
    Skip_SB(                                                    "time_code_present");
    Skip_SB(                                                    "ccdata_present");
    Skip_SB(                                                    "svcinfo_present");
    Skip_SB(                                                    "svc_info_start");
    Skip_SB(                                                    "svc_info_change");
    Skip_SB(                                                    "svc_info_complete");
    Skip_SB(                                                    "caption_service_active");
    Skip_SB(                                                    "Reserved");
    BS_End();
    Skip_B2(                                                    "cdp_hdr_sequence_cntr");
    Element_End0();

    FILLING_BEGIN();
        //cdp_length
        if (cdp_length>cdp_length_Max)
            cdp_length_Max=cdp_length;
        if (cdp_length<cdp_length_Min)
            cdp_length_Min=cdp_length;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Cdp::time_code_section()
{
    Element_Begin1("time_code_section");
    Skip_B1(                                                    "time_code_section_id");
    BS_Begin();
    Mark_1();
    Mark_1();
    Skip_S1(2,                                                  "tc_10hrs");
    Skip_S1(4,                                                  "tc_1hrs");
    Mark_1();
    Skip_S1(3,                                                  "tc_10min");
    Skip_S1(4,                                                  "tc_1min");
    Skip_SB(                                                    "tc_field_flag");
    Skip_S1(3,                                                  "tc_10sec");
    Skip_S1(4,                                                  "tc_1sec");
    Skip_SB(                                                    "drop_frame_flag");
    Mark_0();
    Skip_S1(2,                                                  "tc_10fr");
    Skip_S1(4,                                                  "tc_1fr");
    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Cdp::ccdata_section()
{
    //Parsing
    int8u cc_count;
    Element_Begin1("ccdata_section");
    Skip_B1(                                                    "ccdata_id");
    BS_Begin();
    Mark_1();
    Mark_1();
    Mark_1();
    Get_S1 (5, cc_count,                                        "cc_count");
    BS_End();
    for (int8u Pos=0; Pos<cc_count; Pos++)
    {
        Element_Begin1("cc");
        int8u cc_type;
        bool  cc_valid;
        BS_Begin();
        Mark_1();
        Mark_1();
        Mark_1();
        Mark_1();
        Mark_1();
        Get_SB (   cc_valid,                                    "cc_valid");
        Get_S1 (2, cc_type,                                     "cc_type"); Param_Info1(Cdp_cc_type(cc_type));
        BS_End();

        #if MEDIAINFO_ADVANCED
            if (cc_type>=2 && !Streams[2] && Config->File_DisplayCaptions_Get()==DisplayCaptions_Stream)
                CreateStream(2);
        #endif //MEDIAINFO_ADVANCED

        if (cc_valid)
        {
            Element_Begin1("cc_data");
                //Calculating the parser position
                int8u Parser_Pos=cc_type==3?2:cc_type; //cc_type 2 and 3 are for the same text

                //Stream creation
                if (Streams[Parser_Pos]==NULL)
                    CreateStream(Parser_Pos);

                //Parsing
                #if MEDIAINFO_DEMUX
                    Element_Code=Parser_Pos;
                    Demux(Buffer+(size_t)(Buffer_Offset+Element_Offset), 2, ContentType_MainStream);
                #endif //MEDIAINFO_DEMUX
                if (!Streams[Parser_Pos]->Parser->Status[IsFinished])
                {
                    if (Streams[Parser_Pos]->Parser->PTS_DTS_Needed)
                    {
                        Streams[Parser_Pos]->Parser->FrameInfo.PCR=FrameInfo.PCR;
                        Streams[Parser_Pos]->Parser->FrameInfo.PTS=FrameInfo.PTS;
                        Streams[Parser_Pos]->Parser->FrameInfo.DTS=FrameInfo.DTS;
                    }
                    if (Parser_Pos==2)
                    {
                        #if defined(MEDIAINFO_EIA708_YES)
                            ((File_Eia708*)Streams[2]->Parser)->cc_type=cc_type;
                            if (AspectRatio)
                                ((File_Eia708*)Streams[2]->Parser)->AspectRatio=AspectRatio;
                        #endif //defined(MEDIAINFO_EIA708_YES)
                    }
                    #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
                        Streams[Parser_Pos]->Parser->ServiceDescriptors=ServiceDescriptors;
                    #endif
                    Open_Buffer_Continue(Streams[Parser_Pos]->Parser, Buffer+(size_t)(Buffer_Offset+Element_Offset), 2);
                    Element_Offset+=2;

                    //Filled
                    if (!Streams[Parser_Pos]->IsFilled && Streams[Parser_Pos]->Parser->Status[IsFilled])
                    {
                        if (Count_Get(Stream_General)==0)
                            Accept("CDP");
                        Streams_Count++;
                        if (Streams_Count==3)
                            Fill("CDP");
                        Streams[Parser_Pos]->IsFilled=true;
                    }
                }
                else
                    Skip_XX(2,                                  "Data");
            Element_End0();
        }
        else
            Skip_XX(2,                                          "Junk");
        Element_End0();
    }
    Element_End0();

    Frame_Count++;
    Frame_Count_InThisBlock++;
    if (Frame_Count_NotParsedIncluded!=(int64u)-1)
        Frame_Count_NotParsedIncluded++;
}

//---------------------------------------------------------------------------
void File_Cdp::ccsvcinfo_section()
{
    //Parsing
    int8u svc_count;
    Element_Begin1("ccsvcinfo_section");
    Skip_B1(                                                    "ccsvcinfo_id");
    BS_Begin();
    Skip_SB(                                                    "reserved");
    Skip_SB(                                                    "svc_info_start");
    Skip_SB(                                                    "svc_info_change");
    Skip_SB(                                                    "svc_info_complete");
    Get_S1 (4, svc_count,                                       "svc_count");
    BS_End();
    bool line21_field_Found[2]={false, false};
    string line21_languages[2];
    for (int8u Pos=0; Pos<svc_count; Pos++)
    {
        Element_Begin1("svc");
        bool  csn_size;
        BS_Begin();
        Skip_SB(                                                "reserved");
        Get_SB (   csn_size,                                    "csn_size");
        if (csn_size)
        {
            Skip_SB(                                            "reserved");
            Skip_S1(5,                                          "caption_service_number");
        }
        else
            Skip_S1(6,                                          "caption_service_number");
        BS_End();

        //svc_data_byte - caption_service_descriptor
        Element_Begin1("service");
        string language;
        int8u caption_service_number=0;
        bool digital_cc, wide_aspect_ratio, line21_field;
        Get_String(3, language,                                 "language");
        BS_Begin();
        Get_SB (digital_cc,                                     "digital_cc");
        Skip_SB(                                                "reserved");
        if (digital_cc) //line21
            Get_S1 (6, caption_service_number,                  "caption_service_number");
        else
        {
            Skip_S1(5,                                          "reserved");
            Get_SB (   line21_field,                            "line21_field");
        }
        Skip_SB(                                                "easy_reader");
        Get_SB (wide_aspect_ratio,                              "wide_aspect_ratio");
        Skip_S2(14,                                             "reserved");
        BS_End();
        Element_End0();
        Element_End0();

        FILLING_BEGIN();
            #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
                if (digital_cc)
                {
                    #if defined(MEDIAINFO_EIA708_YES)
                        ServiceDescriptors->ServiceDescriptors708[caption_service_number].language=language;
                        ServiceDescriptors->ServiceDescriptors708[caption_service_number].wide_aspect_ratio.set(0);
                        ServiceDescriptors->ServiceDescriptors708[caption_service_number].wide_aspect_ratio.set(1, wide_aspect_ratio);
                        if (!Streams[2])
                            CreateStream(2);
                    #endif
                }
                else
                {
                    #if defined(MEDIAINFO_EIA608_YES)
                        line21_field_Found[line21_field]=true;
                        line21_languages[line21_field]=language;
                    #endif
                }
            #endif
        FILLING_END();
    }
    Element_End0();

    #if defined(MEDIAINFO_EIA608_YES)
        // From specs: line21_field is deprecated and language is indicated as having no meaning if digital_cc is clear
        // but some encoders fill language for CC1 and CC3, let's try to keep this metadata
        // but some encoders also fill line21_field to 1 for CC1 alone so let's ignore this field if there is only 1 CC announced
        if (!line21_field_Found[0] && line21_field_Found[1])
        {
            swap(line21_field_Found[0], line21_field_Found[1]);
            swap(line21_languages[0], line21_languages[1]);
        }
        for (int i=0; i<=1; i++)
            if (line21_field_Found[i])
            {
                ServiceDescriptors->ServiceDescriptors608[i].language=line21_languages[i];
                if (!Streams[i])
                    CreateStream(i);
            }
    #endif
}

//---------------------------------------------------------------------------
void File_Cdp::cdp_footer()
{
    Element_Begin1("cdp_footer");
    Skip_B1(                                                    "cdp_footer_id");
    Skip_B2(                                                    "cdp_ftr_sequence_cntr");
    Skip_B1(                                                    "packet_checksum");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Cdp::future_section()
{
    //Parsing
    int8u length;
    Element_Begin1("future_section");
    Skip_B1(                                                    "future_section_id");
    Get_B1 (length,                                             "length");
    Skip_XX(length,                                             "Unknown");
    Element_End0();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Cdp::CreateStream(int8u Parser_Pos)
{
    if (Streams[Parser_Pos])
        return; //Already done

    //Parsing
    #if MEDIAINFO_DEMUX
        Element_Code=Parser_Pos;
    #endif //MEDIAINFO_DEMUX

    //Creation of the parser
    Streams[Parser_Pos]=new stream;
    #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
        if (Parser_Pos<2)
        {
            #if defined(MEDIAINFO_EIA608_YES)
                Streams[Parser_Pos]->Parser=new File_Eia608();
                ((File_Eia608*)Streams[Parser_Pos]->Parser)->cc_type=Parser_Pos;
            #else //defined(MEDIAINFO_EIA608_YES)
                Streams[Parser_Pos]->Parser=new File__Analyze();
            #endif //defined(MEDIAINFO_EIA608_YES)
        }
        else
        {
            #if defined(MEDIAINFO_EIA708_YES)
                Streams[Parser_Pos]->Parser=new File_Eia708();
            #else //defined(MEDIAINFO_EIA708_YES)
                Streams[Parser_Pos]->Parser=new File__Analyze();
            #endif //defined(MEDIAINFO_EIA708_YES)
        }
    #else //defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
        Streams[Parser_Pos]->Parser=new File__Analyze();
    #endif //defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)

    //Init
    Streams[Parser_Pos]->Parser->ServiceDescriptors=ServiceDescriptors;
    Open_Buffer_Init(Streams[Parser_Pos]->Parser);
    Streams[Parser_Pos]->Parser->Accept();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_CDP_YES
