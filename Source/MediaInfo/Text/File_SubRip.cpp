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
#if defined(MEDIAINFO_SUBRIP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_SubRip.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Config_MediaInfo.h"
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

int64u SubRip_str2timecode(const char* Value)
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
File_SubRip::File_SubRip()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_SubRip;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS

    //Init
    Frame_Count=0;

    //Temp
    IsVTT=false;
    HasBOM=false;
    #if MEDIAINFO_DEMUX
        Items_Pos=0;
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_SubRip::FileHeader_Begin()
{
    if (!IsSub && (Buffer_Size<File_Size && Buffer_Size<65536))
    {
        Element_WaitForMoreData();
        return false;
    }

    ZtringListList List;
    List.Separator_Set(0, __T("\n\n"));
    List.Separator_Set(1, __T("\n"));

    if (Buffer_Size>=3
     && Buffer[0]==0xEF
     && Buffer[1]==0xBB
     && Buffer[2]==0xBF)
        HasBOM=true;
    bool IsLocal=false;
    Ztring Temp;
    Temp.From_UTF8((const char*)Buffer+(HasBOM?3:0), (Buffer_Size>65536?65536:Buffer_Size)-(HasBOM?3:0));
    if (Temp.empty())
    {
        #ifdef WINDOWS
        Temp.From_Local((const char*)Buffer+(HasBOM?3:0), (Buffer_Size>65536?65536:Buffer_Size)-(HasBOM?3:0)); // Trying from local code page
        #else //WINDOWS
        Temp.From_ISO_8859_1((const char*)Buffer+(HasBOM?3:0), (Buffer_Size>65536?65536:Buffer_Size)-(HasBOM?3:0));
        #endif //WINDOWS
        IsLocal=true;
    }
    Temp.FindAndReplace(__T("\r\n"), __T("\n"), 0, Ztring_Recursive);
    Temp.FindAndReplace(__T("\r"), __T("\n"), 0, Ztring_Recursive);
    List.Quote_Set(__T("\r")); // Should be empty string for indicating that there is no quote sepcial char, but old versions of ZenLib are buggy
    List.Write(Temp);
    Temp = List.Read();

    if (List(0, 0)==__T("WEBVTT FILE") || List(0, 0)==__T("WEBVTT"))
        IsVTT=true;

    if (!IsVTT)
    {
        size_t IsOk=0;
        size_t IsNok=0;
        int64u Number=1;
        for (size_t Pos=0; Pos<List.size(); Pos++)
        {
            int64u NewNumber=List(Pos, 0).To_int64u();
            if (NewNumber==Number)
                IsOk++;
            else
            {
                IsNok++;
                Number=NewNumber;
            }
            Number++;

            if (List(Pos, 1).size()>22 && List(Pos, 1)[2]==__T(':') && List(Pos, 1)[5]==__T(':') && List(Pos, 1).find(__T(" --> "))!=string::npos)
                IsOk++;
            else
                IsNok++;
        }

        if (!IsOk || IsNok>IsOk/2)
        {
            Reject();
            return true;
        }
    }

    if (!IsSub && File_Size!=(int64u)-1 && Buffer_Size!=File_Size)
    {
        Element_WaitForMoreData();
        return false;
    }

    if (!Status[IsAccepted])
    {
        Accept();
        Fill(Stream_General, 0, General_Format, IsVTT?"WebVTT":"SubRip");
        Stream_Prepare(Stream_Text);
        Fill(Stream_Text, 0, "Format", IsVTT?"WebVTT":"SubRip");
        Fill(Stream_Text, 0, "Codec", IsVTT?"WebVTT":"SubRip");
    }

    if (IsLocal)
        #ifdef WINDOWS
        Temp.From_Local((const char*)Buffer+(HasBOM?3:0), Buffer_Size-(HasBOM?3:0));
        #else //WINDOWS
        Temp.From_ISO_8859_1((const char*)Buffer+(HasBOM?3:0), Buffer_Size-(HasBOM?3:0));
        #endif //WINDOWS
    else
        Temp.From_UTF8((const char*)Buffer+(HasBOM?3:0), Buffer_Size-(HasBOM?3:0));
    Temp.FindAndReplace(__T("\r\n"), __T("\n"), 0, Ztring_Recursive);
    Temp.FindAndReplace(__T("\r"), __T("\n"), 0, Ztring_Recursive);
    List.Write(Temp);

    #if MEDIAINFO_DEMUX
        size_t Pos=0;
        for (;;)
        {
            if (Pos>=List.size())
                break;

            if (List[Pos].size()>=3 || (IsVTT && List[Pos].size()>=2))
            {
                Ztring PTS_Begin_String=List[Pos][IsVTT?0:1].SubString(Ztring(), __T(" --> "));
                Ztring PTS_End_String=List[Pos][IsVTT?0:1].SubString(__T(" --> "), Ztring());
                if (IsVTT)
                {
                    size_t Extra_Pos=PTS_End_String.find(__T(' '));
                    if (Extra_Pos!=string::npos)
                        PTS_End_String.resize(Extra_Pos); //Discarding positioning
                }
                item Item;
                Item.PTS_Begin=SubRip_str2timecode(PTS_Begin_String.To_UTF8().c_str());
                Item.PTS_End=SubRip_str2timecode(PTS_End_String.To_UTF8().c_str());
                for (size_t Pos2=IsVTT?1:2; Pos2<List[Pos].size(); Pos2++)
                {
                    List[Pos][Pos2].Trim();
                    Item.Content+=List[Pos][Pos2];
                    if (Pos2+1<List[Pos].size())
                        Item.Content+=EOL;
                }
                Items.push_back(Item);
            }

            Pos++;
        }
    #endif //MEDIAINFO_DEMUX

    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_SubRip::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    GoTo(0);
    #if MEDIAINFO_DEMUX
        Items_Pos=0;
    #endif //MEDIAINFO_DEMUX
    Open_Buffer_Unsynch();
    return 1;
}
#endif //MEDIAINFO_SEEK

//---------------------------------------------------------------------------
void File_SubRip::Read_Buffer_Continue()
{
    #if MEDIAINFO_DEMUX
        if (Buffer)
            Demux(Buffer+((HasBOM && Buffer_Size>=3)?3:0), Buffer_Size-((HasBOM && Buffer_Size>=3)?3:0), ContentType_MainStream);
    #endif //MEDIAINFO_DEMUX

    // Output
    #if MEDIAINFO_EVENTS
        for (; Items_Pos<Items.size(); Items_Pos++)
        {
            Frame_Count_NotParsedIncluded=Frame_Count;
            EVENT_BEGIN (Global, SimpleText, 0)
                Event.DTS=Items[Items_Pos].PTS_Begin;
                Event.PTS=Event.DTS;
                Event.DUR=Items[Items_Pos].PTS_End-Items[Items_Pos].PTS_Begin;
                Event.Content=Items[Items_Pos].Content.To_Unicode().c_str();
                Event.Flags=IsVTT?1:0;
                Event.MuxingMode=(int8u)-1;
                Event.Service=(int8u)-1;
                Event.Row_Max=0;
                Event.Column_Max=0;
                Event.Row_Values=NULL;
                Event.Row_Attributes=NULL;
            EVENT_END   ()
            
            if (Items_Pos+1==Items.size() || Items[Items_Pos].PTS_End!=Items[Items_Pos+1].PTS_Begin)
            {
                EVENT_BEGIN (Global, SimpleText, 0)
                    Event.DTS=Items[Items_Pos].PTS_End;
                    Event.PTS=Event.DTS;
                    Event.DUR=0;
                    Event.Content=L"";
                    Event.Flags=IsVTT?1:0;
                    Event.MuxingMode=(int8u)-1;
                    Event.Service=(int8u)-1;
                    Event.Row_Max=0;
                    Event.Column_Max=0;
                    Event.Row_Values=NULL;
                    Event.Row_Attributes=NULL;
                EVENT_END   ()
            }

            Frame_Count++;
        }
    #endif //MEDIAINFO_EVENTS

    Buffer_Offset=Buffer_Size;
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_SUBRIP_YES
