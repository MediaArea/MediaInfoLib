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
#if defined(MEDIAINFO_TTML_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_Ttml.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Config_MediaInfo.h"
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
#include "tinyxml2.h"
#include <cstring>
using namespace tinyxml2;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

int64u Ttml_str2timecode(const char* Value)
{
    size_t Length=strlen(Value);
         if (Length>=8
     && Value[0]>='0' && Value[0]<='9'
     && Value[1]>='0' && Value[1]<='9'
     && Value[2]==':'
     && Value[3]>='0' && Value[3]<='9'
     && Value[4]>='0' && Value[4]<='9'
     && Value[5]==':'
     && Value[6]>='0' && Value[6]<='9'
     && Value[7]>='0' && Value[7]<='9')
    {
        int64u ToReturn=(int64u)(Value[0]-'0')*10*60*60*1000000000
                       +(int64u)(Value[1]-'0')   *60*60*1000000000
                       +(int64u)(Value[3]-'0')   *10*60*1000000000
                       +(int64u)(Value[4]-'0')      *60*1000000000
                       +(int64u)(Value[6]-'0')      *10*1000000000
                       +(int64u)(Value[7]-'0')         *1000000000;
        if (Length>=9 && (Value[8]=='.' || Value[8]==','))
        {
            if (Length>9+9)
                Length=9+9; //Nanoseconds max
            const char* Value_End=Value+Length;
            Value+=9;
            int64u Multiplier=100000000;
            while (Value<Value_End)
            {
                ToReturn+=(int64u)(*Value-'0')*Multiplier;
                Multiplier/=10;
                Value++;
            }
        }

        return ToReturn;
    }
    else if (Length>=2
     && Value[Length-1]=='s')
    {
        return (int64u)(atof(Value)*1000000000);
    }
    else
        return (int64u)-1;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ttml::File_Ttml()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Ttml;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS

    //Init
    Frame_Count=0;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ttml::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "TTML");

    Stream_Prepare(Stream_Text);
    Fill(Stream_Text, 0, "Format", "TTML");
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ttml::Read_Buffer_Unsynched()
{
    GoTo(0);
}

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_Ttml::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    Open_Buffer_Unsynch();
    return 1;
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ttml::FileHeader_Begin()
{
    //All should be OK...
    return true;
}

//---------------------------------------------------------------------------
void File_Ttml::Read_Buffer_Continue()
{
    if (!IsSub && File_Offset+Buffer_Offset+Element_Offset)
        return; //Already parsed, no change

    tinyxml2::XMLDocument document;

    if (!FileHeader_Begin_XML(document))
        return;

    XMLElement* Root=document.FirstChildElement("tt");
    if (!Root)
    {
        Reject();
        return;
    }

    if (!Status[IsAccepted])
    {
        Accept();

        #if MEDIAINFO_EVENTS
            MuxingMode=(int8u)-1;
            if (StreamIDs_Size>=2 && ParserIDs[StreamIDs_Size-2]==MediaInfo_Parser_Mpeg4)
                MuxingMode=11; //MPEG-4
            if (StreamIDs_Size>2 && ParserIDs[StreamIDs_Size-2]==MediaInfo_Parser_Mxf) //Only if referenced MXF
                MuxingMode=13; //MXF
        #endif //MEDIAINFO_EVENTS

        #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
            if (Config->NextPacket_Get() && Config->Event_CallBackFunction_IsSet())
                return; // Waiting for NextPacket
        #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
    }

    tinyxml2::XMLElement*       div=NULL;
    #if MEDIAINFO_EVENTS
    tinyxml2::XMLElement*       p=NULL;
    #endif //MEDIAINFO_EVENTS
    for (XMLElement* tt_element=Root->FirstChildElement(); tt_element; tt_element=tt_element->NextSiblingElement())
    {
        //body
        if (!strcmp(tt_element->Value(), "body"))
        {
            for (XMLElement* body_element=tt_element->FirstChildElement(); body_element; body_element=body_element->NextSiblingElement())
            {
                //div
                if (!strcmp(body_element->Value(), "div"))
                {
                    for (XMLElement* div_element=body_element->FirstChildElement(); div_element; div_element=div_element->NextSiblingElement())
                    {
                        //p
                        if (!strcmp(div_element->Value(), "p"))
                        {
                            div=body_element;
                            #if MEDIAINFO_EVENTS
                                p=div_element;
                            #endif //MEDIAINFO_EVENTS
                            break;
                        }
                    }

                    if (div)
                        break;
                }
            }

            if (div)
                break;
        }
    }

    #if MEDIAINFO_DEMUX
        Demux(Buffer, Buffer_Size, ContentType_MainStream);
    #endif //MEDIAINFO_DEMUX

    // Output
    #if MEDIAINFO_EVENTS
        for (; p; p=p->NextSiblingElement())
        {
            //p
            if (!strcmp(p->Value(), "p"))
            {
                int64u DTS_Begin=(int64u)-1;
                const char* Attribute=p->Attribute("begin");
                if (Attribute)
                    DTS_Begin=Ttml_str2timecode(Attribute);
                int64u DTS_End=(int64u)-1;
                Attribute=p->Attribute("end");
                if (Attribute)
                    DTS_End=Ttml_str2timecode(Attribute);
                string ContentUtf8;
                XMLPrinter printer;
                p->Accept(&printer);
                ContentUtf8+=printer.CStr();
                while (!ContentUtf8.empty() && (ContentUtf8[ContentUtf8.size()-1]=='\r' || ContentUtf8[ContentUtf8.size()-1]=='\n'))
                    ContentUtf8.resize(ContentUtf8.size()-1);
                Ztring Content; Content.From_UTF8(ContentUtf8.c_str());

                Frame_Count_NotParsedIncluded=Frame_Count;
                EVENT_BEGIN (Global, SimpleText, 0)
                    //Hack: remove "p", "span", "br"
                    Content.FindAndReplace(__T("\r"), Ztring(), 0, ZenLib::Ztring_Recursive);
                    Content.FindAndReplace(__T("\n"), Ztring(), 0, ZenLib::Ztring_Recursive);
                    for (;;)
                    {
                        size_t Span_Begin=Content.find(__T("<p"));
                        if (Span_Begin==string::npos)
                            break;
                        
                        size_t Span_End=Content.find(__T('>'), Span_Begin+5);
                        if (Span_End==string::npos)
                            break;

                        size_t ShlashSpan_Begin=Content.find(__T("</p>"), Span_End+1);
                        if (ShlashSpan_Begin==string::npos)
                            break;

                        Content.erase(ShlashSpan_Begin, 7);
                        Content.erase(Span_Begin, Span_End-Span_Begin+1);
                    }
                    for (;;)
                    {
                        size_t Span_Begin=Content.find(__T("<span"));
                        if (Span_Begin==string::npos)
                            break;
                        
                        size_t Span_End=Content.find(__T('>'), Span_Begin+5);
                        if (Span_End==string::npos)
                            break;

                        size_t ShlashSpan_Begin=Content.find(__T("</span>"), Span_End+1);
                        if (ShlashSpan_Begin==string::npos)
                            break;

                        Content.erase(ShlashSpan_Begin, 7);
                        Content.erase(Span_Begin, Span_End-Span_Begin+1);
                    }
                    Content.FindAndReplace(__T("<br>"), EOL, 0, ZenLib::Ztring_Recursive);
                    Content.FindAndReplace(__T("<br/>"), EOL, 0, ZenLib::Ztring_Recursive);
                    Content.FindAndReplace(__T("<br />"), EOL, 0, ZenLib::Ztring_Recursive);

                    Event.DTS=DTS_Begin;
                    Event.PTS=Event.DTS;
                    Event.DUR=DTS_End-DTS_Begin;
                    Event.Content=Content.To_Unicode().c_str();
                    Event.Flags=0;
                    Event.MuxingMode=MuxingMode;
                    Event.Service=(int8u)Element_Code;
                    Event.Row_Max=0;
                    Event.Column_Max=0;
                    Event.Row_Values=NULL;
                    Event.Row_Attributes=NULL;
                EVENT_END   ()
                EVENT_BEGIN (Global, SimpleText, 0)
                    Event.DTS=DTS_End;
                    Event.PTS=Event.DTS;
                    Event.DUR=0;
                    Event.Content=L"";
                    Event.Flags=0;
                    Event.MuxingMode=MuxingMode;
                    Event.Service=(int8u)Element_Code;
                    Event.Row_Max=0;
                    Event.Column_Max=0;
                    Event.Row_Values=NULL;
                    Event.Row_Attributes=NULL;
                EVENT_END   ()

                Frame_Count++;
            }
        }
    #endif //MEDIAINFO_EVENTS

    Buffer_Offset=Buffer_Size;
}

} //NameSpace

#endif //MEDIAINFO_TTML_YES
