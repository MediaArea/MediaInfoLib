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
#if defined(MEDIAINFO_TELETEXT_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_Teletext.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Teletext::File_Teletext()
:File__Analyze()
{
    //Configuration
    ParserName="Teletext";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Teletext;
        StreamIDs_Width[0]=2;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    PTS_DTS_Needed=true;
    StreamSource=IsStream;
    MustSynchronize=true;

    //In
    #if defined(MEDIAINFO_MPEGPS_YES)
        FromMpegPs=false;
        Parser=NULL;
    #endif

    //Stream
    Stream_HasChanged=0;

    //Temp
    PageNumber=0xFF;
    SubCode=0x3F7F;
}

//---------------------------------------------------------------------------
File_Teletext::~File_Teletext()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Teletext::Streams_Fill()
{
}

//---------------------------------------------------------------------------
void File_Teletext::Streams_Finish()
{
    //Filling when it is from MpegPs
    //TODO: filter subtitles and non subtitles, some files have normal teletext in subtitles block.
    if (Parser)
    {
        Parser->Finish();
        for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
            for (size_t StreamPos=0; StreamPos<Parser->Count_Get((stream_t)StreamKind); StreamPos++)
            {
                Stream_Prepare((stream_t)StreamKind);
                Merge(*Parser, (stream_t)StreamKind, StreamPos_Last, StreamPos);
            }

        return;
    }

    if (Teletexts && !Teletexts->empty())
        for (std::map<int16u, teletext>::iterator Teletext=Teletexts->begin(); Teletext!=Teletexts->end(); ++Teletext)
        {
            std::map<std::string, Ztring>::iterator Info_Format=Teletext->second.Infos.find("Format");
            Stream_Prepare((Info_Format!=Teletext->second.Infos.end() && Info_Format->second==__T("Teletext"))?Stream_Other:Stream_Text);
            Fill(StreamKind_Last, StreamPos_Last, General_ID, Ztring::ToZtring(Teletext->first), true);
            Fill(StreamKind_Last, StreamPos_Last, General_ID_String, Ztring::ToZtring(Teletext->first), true);

            for (std::map<std::string, ZenLib::Ztring>::iterator Info=Teletext->second.Infos.begin(); Info!=Teletext->second.Infos.end(); ++Info)
            {
                if (Retrieve(StreamKind_Last, StreamPos_Last, Info->first.c_str()).empty())
                    Fill(StreamKind_Last, StreamPos_Last, Info->first.c_str(), Info->second);
            }
        }
    else
        for (streams::iterator Stream = Streams.begin(); Stream != Streams.end(); ++Stream)
        {
            Stream_Prepare(Stream->second.IsSubtitle?Stream_Text:Stream_Other);
            Fill(StreamKind_Last, StreamPos_Last, General_ID, Ztring::ToZtring(Stream->first, 16));
            Fill(StreamKind_Last, StreamPos_Last, "Format", Stream->second.IsSubtitle?"Teletext Subtitle":"Teletext");
        }
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Teletext::Synchronize()
{
    //Synchronizing
    while (Buffer_Offset+3<=Buffer_Size)
    {
        while (Buffer_Offset+3<=Buffer_Size)
        {
            if (Buffer[Buffer_Offset  ]==0x55
             && Buffer[Buffer_Offset+1]==0x55
             && Buffer[Buffer_Offset+2]==0x27)
                break; //while()

            Buffer_Offset++;
        }

        if (Buffer_Offset+3<=Buffer_Size) //Testing if size is coherant
        {
            if (Buffer_Offset+45==Buffer_Size)
                break;

            if (Buffer_Offset+45+3>Buffer_Size)
                return false; //Wait for more data

            if (Buffer[Buffer_Offset  ]==0x55
             && Buffer[Buffer_Offset+1]==0x55
             && Buffer[Buffer_Offset+2]==0x27)
                break; //while()

            Buffer_Offset++;
        }
    }

    //Must have enough buffer for having header
    if (Buffer_Offset+3>=Buffer_Size)
        return false;

    //Synched is OK
    if (!Status[IsAccepted])
    {
        //For the moment, we accept only if the file is in sync, the test is not strict enough
        if (Buffer_Offset)
        {
            Reject();
            return false;
        }

        Accept();
    }
    return true;
}

//---------------------------------------------------------------------------
bool File_Teletext::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+3>Buffer_Size)
        return false;

    //Quick test of synchro
    if (Buffer[Buffer_Offset  ]!=0x55
     || Buffer[Buffer_Offset+1]!=0x55
     || Buffer[Buffer_Offset+2]!=0x27)
    {
        Synched=false;
        return true;
    }

    //We continue
    return true;
}

//---------------------------------------------------------------------------
void File_Teletext::Synched_Init()
{
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Teletext::Read_Buffer_Unsynched()
{
    for (streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
        {
            Stream_HasChanged=0;
            for (size_t PosY=0; PosY<26; ++PosY)
                for (size_t PosX=0; PosX<40; ++PosX)
                    if (Stream->second.CC_Displayed_Values[PosY][PosX]!=L' ')
                    {
                        Stream->second.CC_Displayed_Values[PosY][PosX]=L' ';
                        Stream_HasChanged=Stream->first;
                    }

            if (Stream_HasChanged)
            {
                HasChanged();
                Stream_HasChanged=0;
            }
        }

    //Unsynching when it is from MpegPs
    if (Parser)
        Parser->Open_Buffer_Unsynch();
}

//---------------------------------------------------------------------------
void File_Teletext::Read_Buffer_Continue()
{
    #if defined(MEDIAINFO_MPEGPS_YES)
        if (FromMpegPs)
        {
            if (!Status[IsAccepted])
            {
                Accept();
                MustSynchronize=false;
            }

            Skip_B1(                                            "data_identifier");
            while (Element_Offset<Element_Size)
            {
                int8u data_unit_id, data_unit_length;
                Get_B1 (data_unit_id,                           "data_unit_id");
                Get_B1 (data_unit_length,                       "data_unit_length");
                if (data_unit_length)
                {
                Skip_B1(                                        "field/line");
                if (data_unit_id==0x03 && data_unit_length==0x2C)
                {
                    int8u Data[43];
                    for (int8u Pos=0; Pos<43; ++Pos)
                        Data[Pos]=ReverseBits(Buffer[Buffer_Offset+(size_t)Element_Offset+Pos]);

                    if (Parser==NULL)
                    {
                        Parser=new File_Teletext();
                        Parser->MustSynchronize=false;
                        //Parser->IsSubtitle=data_unit_id==0x03; //Not needed, there is a Subtitle flag in Teletext directly
                        Parser->Teletexts=Teletexts;
                        Open_Buffer_Init(Parser);
                        Parser->Accept(); //Force to be accepted because there is no other synchronization layer (MustSynchronize set to false)
                    }
                    Element_Code=data_unit_id;
                    int8u Temp[2];
                    Temp[0]=0x55;
                    Temp[1]=0x55;
                    Demux(Temp, 2, ContentType_MainStream);
                    Demux(Data, 43, ContentType_MainStream);
                    Parser->FrameInfo=FrameInfo;
                    Open_Buffer_Continue(Parser, Data, 43);
                    Element_Offset+=43;
                }
                else
                    Skip_XX(data_unit_length-1,                 "Data");
                }
            }
        }
    #endif
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Teletext::Header_Parse()
{
    //Parsing
    if (MustSynchronize)
        Skip_B2(                                                "Clock run-in");
    Skip_B1(                                                    "Framing code");

    //Magazine and Packet Number (for all packets)
    X=0, Y=0;
    bool P1, D1, P2, D2, P3, D3, P4, D4;
    bool B;
    BS_Begin_LE();
    Element_Begin1("Magazine (X or M)");
        Get_TB (P1,                                             "Hamming 8/4");
        Get_TB (D1,                                             "Magazine 0");
        if (D1)
            X|=1<<0;
        Get_TB (P2,                                             "Hamming 8/4");
        Get_TB (D2,                                             "Magazine 1");
        if (D2)
            X|=1<<1;
        Get_TB (P3,                                             "Hamming 8/4");
        Get_TB (D3,                                             "Magazine 2");
        if (D3)
            X|=1<<2;
    Element_Info1(X);
    Element_End0();
    Element_Begin1("Packet Number (Y)");
        Get_TB (P4,                                             "Hamming 8/4");
        Get_TB (D4,                                             "Packet Number 0");
        if (D4)
            Y|=1<<0;
        /*
        {
            //Hamming 8/4
            bool A=P1^D1^D3^D4;
            bool B=D1^P2^D2^D4;
            bool C=D1^D2^P3^D3;
            bool D=P1^D1^P2^D2^P3^D3^P4^D4;
            if (A && B && C && D)
            {
            }
            else
            {
            }
        }
        */
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Packet Number 1");
        if (B)
            Y|=1<<1;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Packet Number 2");
        if (B)
            Y|=1<<2;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Packet Number 3");
        if (B)
            Y|=1<<3;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Packet Number 4");
        if (B)
            Y|=1<<4;
        if (X==0)
            X=8; // A packet with a magazine value of 0 is referred to as belonging to magazine 8
    Element_Info1(Y);
    Element_End0();

    //Page header
    if (Y==0)
    {
        C.reset();
        CharacterSubset=0;

        Element_Begin1("Page header");
        int8u PU=0, PT=0;
        bool B;
        Element_Begin1("Page Units");
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Page Units 0");
        if (B)
            PU|=1<<0;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Page Units 1");
        if (B)
            PU|=1<<1;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Page Units 2");
        if (B)
            PU|=1<<2;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Page Units 3");
        if (B)
            PU|=1<<3;
        Element_Info1(PU);
        Element_End0();
        Element_Begin1("Page Tens");
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Page Tens 0");
        if (B)
            PT|=1<<0;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Page Tens 1");
        if (B)
            PT|=1<<1;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Page Tens 2");
        if (B)
            PT|=1<<2;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "Page Tens 3");
        if (B)
            PT|=1<<3;
        Element_Info1(PT);
        Element_End0();
        PageNumber=(PT<<4)|PU;
        Element_Info1(Ztring::ToZtring(PageNumber, 16));

        int8u S1=0, S2=0, S3=0, S4=0;
        Element_Begin1("Page sub-code 1");
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S1 0");
        if (B)
            S1|=1<<0;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S1 1");
        if (B)
            S1|=1<<1;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S1 2");
        if (B)
            S1|=1<<2;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S1 3");
        if (B)
            S1|=1<<3;
        Element_Info1(S1);
        Element_End0();
        Element_Begin1("Page sub-code 2");
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S2 0");
        if (B)
            S2|=1<<0;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S2 1");
        if (B)
            S2|=1<<1;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S2 2");
        if (B)
            S2|=1<<2;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C4 - Erase Page");
        if (B)
            C[4]=true;
        Element_Info1(S2);
        Element_End0();
        Element_Begin1("Page sub-code 3");
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S3 0");
        if (B)
            S3|=1<<0;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S3 1");
        if (B)
            S3|=1<<1;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S3 2");
        if (B)
            S3|=1<<2;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S3 3");
        if (B)
            S3|=1<<3;
        Element_Info1(S3);
        Element_End0();
        Element_Begin1("Page sub-code 4");
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S4 0");
        if (B)
            S4|=1<<0;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "S4 1");
        if (B)
            S4|=1<<1;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C5 - Newsflash");
        if (B)
            C[5]=true;
        #if MEDIAINFO_TRACE
            if (B)
                Element_Info1("Newsflash");
        #endif //MEDIAINFO_TRACE
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C6 - Subtitle");
        if (B)
            C[6]=true;
        Element_Info1(S4);
        Element_End0();
        Element_Begin1("Control bits");
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C7 - Suppress Header");
        if (B)
            C[7]=true;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C8 - Update Indicator");
        if (B)
            C[8]=true;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C9 - Interrupted Sequence");
        if (B)
            C[9]=true;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C10 - Inhibit Display");
        if (B)
            C[10]=true;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C11 - Magazine Serial");
        if (B)
            C[11]=true;
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C12 - Character Subset");
        if (B)
            CharacterSubset |= (1<<2);
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C13 - Character Subset");
        if (B)
            CharacterSubset |= (1<<1);
        Skip_TB(                                                "Hamming 8/4");
        Get_TB (B,                                              "C14 - Character Subset");
        if (B)
            CharacterSubset |= (1<<0);
        Element_End0();

        SubCode=(S4<<12)|(S3<<8)|(S2<<4)|S1;

        Element_End0();
    }
    BS_End_LE();

    #if MEDIAINFO_TRACE
        if (C[4])
            Element_Info1("Erase Page");
        if (C[5])
            Element_Info1("Newsflash");
        if (C[6])
            Element_Info1("Subtitle");
        if (C[7])
            Element_Info1("Suppress Header");
        if (C[8])
            Element_Info1("Update Indicator");
        if (C[9])
            Element_Info1("Interrupted Sequence");
        if (C[10])
            Element_Info1("Inhibit Display");
        if (C[11])
            Element_Info1("Magazine Serial");
        Element_Info1(Ztring::ToZtring((X<<8)|PageNumber, 16)+__T(':')+Ztring().From_CC2(SubCode));
        Element_Info1(Y);
    #endif // MEDIAINFO_TRACE

    Header_Fill_Size(45-(MustSynchronize?0:2));

    if (Y==0)
    {
        if (Stream_HasChanged)
        {
            HasChanged();
            Stream_HasChanged=0;
        }

        if (C[6] && PageNumber!=0xFF)
            Streams[(X<<8)|PageNumber].IsSubtitle=true;

        if (C[4] && PageNumber!=0xFF)
        {
            stream &Stream=Streams[(X<<8)|PageNumber];
            for (size_t PosY=0; PosY<26; ++PosY)
                for (size_t PosX=0; PosX<40; ++PosX)
                    if (Stream.CC_Displayed_Values[PosY][PosX]!=L' ')
                    {
                        Stream.CC_Displayed_Values[PosY][PosX]=L' ';
                        Stream_HasChanged=(X<<8)|PageNumber;
                    }
        }
    }
}

//---------------------------------------------------------------------------
void File_Teletext::Data_Parse()
{
    if (PageNumber==0xFF)
    {
        Skip_XX(Y?40:32,                                            "Junk");
    }
    else if (Y>=26)
    {
        Skip_XX(40,                                                 "Special commands");
    }
    else
    {
        Element_Begin1("Data bytes");
        stream &Stream=Streams[(X<<8)|PageNumber];
        size_t PosX=Y?0:8;
        for (; PosX<40; ++PosX)
        {
            int8u byte;
            Get_B1(byte,                                            "Byte");
            byte&=0x7F;
            if (byte<0x20)
                byte=0x20;
            Param_Info1(Ztring().From_UTF8((const char*)&byte, 1));
            if (byte!=Stream.CC_Displayed_Values[Y][PosX] && (!C[7] || Y)) // C[7] is "Suppress Header", to be tested when Y==0
            {
                wchar_t Uni;
                switch (CharacterSubset)
                {
                    case 0x00:  //English
                                switch(byte)
                                {
                                    case 0x23: Uni = L'\xA3'; break;
                                    case 0x24: Uni = L'$'; break;
                                    case 0x40: Uni = L'@'; break;
                                    case 0x5B: Uni = L'\x2190'; break;
                                    case 0x5C: Uni = L'\xBD'; break;
                                    case 0x5D: Uni = L'\x2192'; break;
                                    case 0x5E: Uni = L'\x2191'; break;
                                    case 0x5F: Uni = L'#'; break;
                                    case 0x60: Uni = L'-'; break;
                                    case 0x7B: Uni = L'\xBC'; break;
                                    case 0x7C: Uni = L'|'; break;
                                    case 0x7D: Uni = L'\xBE'; break;
                                    case 0x7E: Uni = L'\xF7'; break;
                                    default: Uni=byte;
                                }
                                break;
                    case 0x04:  //French
                                switch(byte)
                                {
                                    case 0x23: Uni = L'\xE9'; break;
                                    case 0x24: Uni = L'\xEF'; break;
                                    case 0x40: Uni = L'\xE0'; break;
                                    case 0x5B: Uni = L'\xEB'; break;
                                    case 0x5C: Uni = L'\xEA'; break;
                                    case 0x5D: Uni = L'\xF9'; break;
                                    case 0x5E: Uni = L'\xEE'; break;
                                    case 0x5F: Uni = L'#'; break;
                                    case 0x60: Uni = L'\xE8'; break;
                                    case 0x7B: Uni = L'\xE2'; break;
                                    case 0x7C: Uni = L'\xF4'; break;
                                    case 0x7D: Uni = L'\xFB'; break;
                                    case 0x7E: Uni = L'\xE7'; break;
                                    default: Uni=byte;
                                }
                                break;
                    case 0x05:  //Portuguese/Spanish
                                switch(byte)
                                {
                                    case 0x23: Uni = L'\xE7'; break;
                                    case 0x24: Uni = L'$'; break;
                                    case 0x40: Uni = L'\xA1'; break;
                                    case 0x5B: Uni = L'\xE1'; break;
                                    case 0x5C: Uni = L'\xE9'; break;
                                    case 0x5D: Uni = L'\xED'; break;
                                    case 0x5E: Uni = L'\xF3'; break;
                                    case 0x5F: Uni = L'\xFA'; break;
                                    case 0x60: Uni = L'\xBF'; break;
                                    case 0x7B: Uni = L'\xFC'; break;
                                    case 0x7C: Uni = L'\xF1'; break;
                                    case 0x7D: Uni = L'\xE8'; break;
                                    case 0x7E: Uni = L'\xE0'; break;
                                    default: Uni=byte;
                                }
                                break;
                    default: Uni=byte;
                }

                Stream.CC_Displayed_Values[Y][PosX]=Uni;
                Stream_HasChanged=(X<<8)|PageNumber;
            }
        }
        Element_End0();
    }

    #if MEDIAINFO_TRACE
        if (PageNumber==0xFF)
        {
            Element_Name("Skip");
        }
        else
        {
            Element_Name(Ztring::ToZtring((X<<8)|PageNumber, 16)+__T(':')+Ztring().From_CC2(SubCode));
            Element_Info1(Y);
            if (Y<26)
            {
                Element_Info1(Ztring().From_Unicode(Streams[(X<<8)|PageNumber].CC_Displayed_Values[Y]));
                if (Y==0)
                {
                    if (C[4])
                        Element_Info1("Erase Page");
                    if (C[5])
                        Element_Info1("Newsflash");
                    if (C[6])
                        Element_Info1("Subtitle");
                    if (C[7])
                        Element_Info1("Suppress Header");
                    if (C[8])
                        Element_Info1("Update Indicator");
                    if (C[9])
                        Element_Info1("Interrupted Sequence");
                    if (C[10])
                        Element_Info1("Inhibit Display");
                    if (C[11])
                        Element_Info1("Magazine Serial");
                }
            }
        }
    #endif //MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Teletext::HasChanged()
{
    #if MEDIAINFO_EVENTS
        EVENT_BEGIN (Global, SimpleText, 0)
            wstring Content;
            stream &Stream=Streams[Stream_HasChanged];
            const wchar_t* Row_Values[26];
            for (size_t PosY=0; PosY<26; ++PosY)
            {
                if (PosY)
                    Content+=Ztring(EOL).To_Unicode().c_str();
                Content+=Stream.CC_Displayed_Values[PosY];
                Row_Values[PosY]=Stream.CC_Displayed_Values[PosY].c_str();
            }
            Event.StreamIDs[StreamIDs_Size-1]=Stream_HasChanged;
            Event.DTS=FrameInfo.DTS;
            Event.PTS=Event.DTS;
            Event.DUR=(int64u)-1;
            Event.Content=Content.c_str();
            Event.Flags=0;
            if (StreamIDs_Size>1 && Event.ParserIDs[StreamIDs_Size-2]==MediaInfo_Parser_Sdp)
                Event.MuxingMode=12; //Ancillary data / OP-47 / SDP
            else
                Event.MuxingMode=14; //Usually Teletext in MPEG-TS
            Event.Service=(int8u)-1;
            Event.Row_Max=26;
            Event.Column_Max=40;
            Event.Row_Values=(wchar_t**)&Row_Values;
            Event.Row_Attributes=NULL;
        EVENT_END   ()
    #endif //MEDIAINFO_EVENTS
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_TELETEXT_YES
