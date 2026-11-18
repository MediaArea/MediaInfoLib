/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

// RegressionTest.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <vector>
#include "tchar.h"
#include "MediaInfoDLL/MediaInfoDLL.h"
#include "ZenLib/ZtringListListF.h"
#include "ZenLib/File.h"
#include "ZenLib/FileName.h"
#include "ZenLib/Dir.h"
#include "RegressionTest/RegressionTest.h"
#include "MediaInfo/MediaInfo_Events.h"
#include "ctime"
using namespace MediaInfoDLL;
using namespace ZenLib;
using namespace std;

/***************************************************************************/
/* Events                                                                  */
/***************************************************************************/

//Parsing times (Basic\Diff\Times.csv): a line per file of the list, with its complete name, an empty column, its size
//and the seconds of its parsing. A file referenced by another one (e.g. the files of a DCP) is parsed while the file
//referencing it is parsed, so its time is in the time of this file, and it has no line.
//The start event of a file has its name. The end events are sent by the parser of the file and by its sub-parsers,
//with their depth as the count of stream IDs: 1 for the parser of a file of the list, 1 more for a sub-parser and for
//the parser of a referenced file.

struct opened_file
{
    time_t Start;
    time_t End;
    int64u Size;
    bool   IsEnded;
};

struct UserHandle_struct
{
    File                Time_File;
    vector<opened_file> Opened; //The file of the list, then the files referenced by the previous one
    Ztring              Parser;
};

static void Basic_Close(struct UserHandle_struct* UserHandle, size_t Count)
{
    //Files after the first Count ones finished, the time of the file of the list written
    while (UserHandle->Opened.size()>Count)
    {
        if (UserHandle->Opened.size()==1)
        {
            const opened_file& Opened=UserHandle->Opened[0];
            time_t Diff=(Opened.IsEnded?Opened.End:time(NULL))-Opened.Start;
            UserHandle->Time_File.Write(__T(';')+Ztring::ToZtring(Opened.Size)+__T(';')+Ztring::ToZtring(Diff)+EOL);
        }
        UserHandle->Opened.pop_back();
    }
}

void Basic_General_Start_0 (struct MediaInfo_Event_General_Start_0* Event, struct UserHandle_struct* UserHandle)
{
    if (!Event->FileName_Unicode)
        return; //Parsing of a buffer, not of a file

    //Files whose parsing is finished
    while (!UserHandle->Opened.empty() && UserHandle->Opened.back().IsEnded)
        Basic_Close(UserHandle, UserHandle->Opened.size()-1);

    //A file of the list if no file is opened, else a file referenced by the last opened one
    if (UserHandle->Opened.empty())
        UserHandle->Time_File.Write(Ztring(Event->FileName_Unicode)+__T(';')); //Name written before the parsing, so the file of a crash or of a hang is known
    opened_file Opened;
    Opened.Start=time(NULL);
    Opened.Size=Event->Stream_Size;
    Opened.IsEnded=false;
    UserHandle->Opened.push_back(Opened);
    UserHandle->Parser.clear();
}

void Basic_General_End_0 (struct MediaInfo_Event_General_End_0* Event, struct UserHandle_struct* UserHandle)
{
    //End of the parser of an opened file (and of the files it references), the end of a sub-parser of the last opened
    //file is ignored, as the next ends of the parser of a file (e.g. from its sub-parsers)
    if (!Event->StreamIDs_Size || Event->StreamIDs_Size>UserHandle->Opened.size())
        return;
    size_t Depth=Event->StreamIDs_Size-1;
    Basic_Close(UserHandle, Depth+1);
    opened_file& Opened=UserHandle->Opened[Depth];
    if (!Opened.IsEnded)
    {
        Opened.End=time(NULL);
        Opened.IsEnded=true;
    }
    UserHandle->Parser.clear();
}

void Basic_General_Parser_Selected_0 (struct MediaInfo_Event_General_Parser_Selected_0* Event, struct UserHandle_struct* UserHandle)
{
    if (Event->Name)
        UserHandle->Parser.From_UTF8(Event->Name);
    else
        UserHandle->Parser.clear();
}

/***************************************************************************/
/* The callback function                                                   */
/***************************************************************************/

#define CASE(_PARSER,_EVENT,_VERSION) \
    case MediaInfo_Event_##_PARSER##_##_EVENT : if (EventVersion==_VERSION && Data_Size>=sizeof(struct MediaInfo_Event_##_PARSER##_##_EVENT##_##_VERSION)) _PARSER##_##_EVENT##_##_VERSION((struct MediaInfo_Event_##_PARSER##_##_EVENT##_##_VERSION*)Data_Content, UserHandle); break;

void __stdcall Basic_Event_CallBackFunction(unsigned char* Data_Content, size_t Data_Size, void* UserHandle_Void)
{
    /*Retrieving UserHandle*/
    struct UserHandle_struct*           UserHandle=(struct UserHandle_struct*)UserHandle_Void;
    struct MediaInfo_Event_Generic*     Event_Generic=(struct MediaInfo_Event_Generic*) Data_Content;
    unsigned char                       ParserID;
    unsigned short                      EventID;
    unsigned char                       EventVersion;

    /*integrity tests*/
    if (Data_Size<4)
        return; //There is a problem

    /*Retrieving EventID*/
    ParserID    =(unsigned char) ((Event_Generic->EventCode&0xFF000000)>>24);
    EventID     =(unsigned short)((Event_Generic->EventCode&0x00FFFF00)>>8 );
    EventVersion=(unsigned char) ( Event_Generic->EventCode&0x000000FF     );


    switch (ParserID)
    {
        case MediaInfo_Parser_None :
                switch (EventID)
                {
                    case MediaInfo_Event_General_Start                                          : if (EventVersion==0 && Data_Size>=sizeof(struct MediaInfo_Event_General_Start_0)) Basic_General_Start_0((struct MediaInfo_Event_General_Start_0*)Data_Content, UserHandle); break;
                    case MediaInfo_Event_General_End                                            : if (EventVersion==0 && Data_Size>=sizeof(struct MediaInfo_Event_General_End_0)) Basic_General_End_0((struct MediaInfo_Event_General_End_0*)Data_Content, UserHandle); break;
                    case MediaInfo_Event_General_Parser_Selected                                : if (EventVersion==0 && Data_Size>=sizeof(struct MediaInfo_Event_General_Parser_Selected_0)) Basic_General_Parser_Selected_0((struct MediaInfo_Event_General_Parser_Selected_0*)Data_Content, UserHandle); break;
                    default                                                                     : ;
                }
                break;
        default : ; //ParserID is unknown
    }
}

/***************************************************************************/
/* The callback function                                                   */
/***************************************************************************/


void RegressionTest_Basic(Ztring Files, Ztring DataBaseDirectory, int32u Scenario)
{
    /*
    //One per one
    ZtringList List=Dir::GetAllFileNames(Files);
    for (size_t Pos=0; Pos<List.size(); Pos++)
    {
        std::cout<<List[Pos].To_Local()<<endl;
        MediaInfoDLL_Load();
        MediaInfoList* MIL=new MediaInfoList;
        MIL->Open(List[Pos]);
        delete MIL;
        MediaInfoDLL_UnLoad();
        File::Move(L"C:\\Temp\\Debug_MemoryLeak.txt", L"C:\\Temp\\"+ZenLib::FileName(List[Pos]).Name_Get()+L"."+ZenLib::FileName(List[Pos]).Extension_Get()+L"."+Ztring::ToZtring(Pos)+L".txt");
    }
    */

    /* Old
    ZtringListListF* Ref=new ZtringListListF[Stream_Max];
    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
        if (File::Exists(DataBaseDirectory+__T("\\Basic\\Ref\\")+Ztring::ToZtring(StreamKind)+__T(".csv")))
            Ref[StreamKind].Load(DataBaseDirectory+__T("\\Basic\\Ref\\")+Ztring::ToZtring(StreamKind)+__T(".csv"));

    ZtringListListF* New=new ZtringListListF[Stream_Max];

    MediaInfoList MIL;
    struct UserHandle_struct UserHandle;

    //Times
    wostringstream Event_CallBackFunction_Text;
    Event_CallBackFunction_Text<<__T("CallBack=memory://")<<(MediaInfo_int64u)Basic_Event_CallBackFunction<<__T(";UserHandler=memory://")<<(MediaInfo_int64u)&UserHandle;
    MIL.Option(__T("File_Event_CallBackFunction"), Event_CallBackFunction_Text.str());
    if (!Dir::Exists(DataBaseDirectory+__T("\\Basic\\Diff")))
        Dir::Create(DataBaseDirectory+__T("\\Basic\\Diff"));
    File::Delete(DataBaseDirectory+__T("\\Basic\\Diff\\Times.csv"));
    UserHandle.Time_File.Open(DataBaseDirectory+__T("\\Basic\\Diff\\Times.csv"), File::Access_Write_Append);

    cout<<" Analyzing"<<endl;
    MIL.Open(Files);

    cout<<" Retrieving new data"<<endl;
    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
    {
        for (size_t FilePos=0; FilePos<MIL.Count_Get(); FilePos++)
            if (MIL.Count_Get(FilePos, (stream_t)StreamKind))
            {
                New[StreamKind].push_back(ZtringList());
                New[StreamKind].at(New[StreamKind].size()-1).push_back(Ztring()); //CompleteFileName
                for (size_t LinePos=0; LinePos<MIL.Count_Get(FilePos, (stream_t)StreamKind, 0); LinePos++)
                    New[StreamKind].at(New[StreamKind].size()-1).push_back(MIL.Get(FilePos, (stream_t)StreamKind, 0, LinePos, Info_Name));
                break;
            }
    }
    for (size_t FilePos=0; FilePos<MIL.Count_Get(); FilePos++)
        for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
            for (size_t StreamPos=0; StreamPos<MIL.Count_Get(FilePos, (stream_t)StreamKind); StreamPos++)
            {
                New[StreamKind].push_back(ZtringList());
                New[StreamKind].at(New[StreamKind].size()-1).push_back(MIL.Get(FilePos, Stream_General, 0, __T("CompleteName")));
                for (size_t LinePos=0; LinePos<MIL.Count_Get(FilePos, (stream_t)StreamKind, StreamPos); LinePos++)
                    New[StreamKind].at(New[StreamKind].size()-1).push_back(MIL.Get(FilePos, (stream_t)StreamKind, StreamPos, LinePos));
            }

    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
    {
        if (!Dir::Exists(DataBaseDirectory+__T("\\Basic\\New")))
            Dir::Create(DataBaseDirectory+__T("\\Basic\\New"));
        New[StreamKind].Save(DataBaseDirectory+__T("\\Basic\\New\\")+Ztring::ToZtring(StreamKind)+__T(".csv"));
    }

    cout<<" Diff"<<endl;
    ZtringListListF* Diff=new ZtringListListF[Stream_Max];
    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
    {
        size_t Ref_Pos=1;
        size_t New_Pos=1;
        while (Ref_Pos<Ref[StreamKind].size() && New_Pos<New[StreamKind].size())
        {
            if (New[StreamKind](New_Pos, 0)!=Ref[StreamKind](Ref_Pos, 0))
            {
                size_t Ref_PosNext=Ref_Pos;
                while (Ref_PosNext<Ref[StreamKind].size() && New[StreamKind](New_Pos, 0)!=Ref[StreamKind](Ref_PosNext, 0))
                    Ref_PosNext++;
                if (Ref_PosNext==Ref[StreamKind].size()) //Ref file not found
                {
                    Diff[StreamKind].push_back(New[StreamKind](New_Pos));
                    New_Pos++;
                }
                else
                {
                    for (; Ref_Pos<Ref_PosNext; Ref_Pos++)
                        Diff[StreamKind].push_back(Ref[StreamKind](Ref_Pos));
                }
            }
            else
            {
                bool IsDiff=false;
                size_t LinePos_Max=max(New[StreamKind](New_Pos).size(), Ref[StreamKind](Ref_Pos).size());
                for (size_t LinePos=1; LinePos<LinePos_Max; LinePos++)
                    if (New[StreamKind](New_Pos, LinePos)!=Ref[StreamKind](Ref_Pos, LinePos))
                    {
                        if (!IsDiff)
                        {
                            Diff[StreamKind].push_back(ZtringList());
                            Diff[StreamKind](Diff[StreamKind].size()-1, 0)=New[StreamKind](New_Pos, 0);
                            IsDiff=true;
                        }
                        Diff[StreamKind](Diff[StreamKind].size()-1, LinePos)=New[StreamKind](New_Pos, LinePos)+__T(" --- ")+Ref[StreamKind](Ref_Pos, LinePos);
                    }

                Ref_Pos++;
                New_Pos++;
            }

        }

        for (; Ref_Pos<Ref[StreamKind].size(); Ref_Pos++)
        {
            Diff[StreamKind].push_back(ZtringList());
            Diff[StreamKind](Diff[StreamKind].size()-1, 0)=Ref[StreamKind](Ref_Pos, 0);
            for (size_t LinePos=1; LinePos<Ref[StreamKind][Ref_Pos].size(); LinePos++)
                if (!Ref[StreamKind](Ref_Pos, LinePos).empty())
                    Diff[StreamKind](Diff[StreamKind].size()-1, LinePos)=__T(" --- ")+Ref[StreamKind](Ref_Pos, LinePos);
        }
        for (; New_Pos<New[StreamKind].size(); New_Pos++)
        {
            Diff[StreamKind].push_back(ZtringList());
            Diff[StreamKind](Diff[StreamKind].size()-1, 0)=New[StreamKind](New_Pos, 0);
            for (size_t LinePos=1; LinePos<New[StreamKind][New_Pos].size(); LinePos++)
                if (!New[StreamKind](New_Pos, LinePos).empty())
                    Diff[StreamKind](Diff[StreamKind].size()-1, LinePos)=New[StreamKind](New_Pos, LinePos)+__T(" --- ");
        }
    }

    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
    {
        if (!Diff[StreamKind].empty())
        {
            Diff[StreamKind].insert(Diff[StreamKind].begin(), New[StreamKind](0));
            if (!Dir::Exists(DataBaseDirectory+__T("\\Basic\\Diff")))
                Dir::Create(DataBaseDirectory+__T("\\Basic\\Diff"));
            Diff[StreamKind].Save(DataBaseDirectory+__T("\\Basic\\Diff\\")+Ztring::ToZtring(StreamKind)+__T(".csv"));
        }
    }
    */

    MediaInfoList MIL;
    struct UserHandle_struct UserHandle;

    //Times
    wostringstream Event_CallBackFunction_Text;
    Event_CallBackFunction_Text<<__T("CallBack=memory://")<<(MediaInfo_int64u)Basic_Event_CallBackFunction<<__T(";UserHandler=memory://")<<(MediaInfo_int64u)&UserHandle;
    MIL.Option(__T("File_Event_CallBackFunction"), Event_CallBackFunction_Text.str());
    if (!Dir::Exists(DataBaseDirectory+__T("\\Basic\\Diff")))
        Dir::Create(DataBaseDirectory+__T("\\Basic\\Diff"));
    File::Delete(DataBaseDirectory+__T("\\Basic\\Diff\\Times.csv"));
    UserHandle.Time_File.Open(DataBaseDirectory+__T("\\Basic\\Diff\\Times.csv"), File::Access_Write_Append);

    cout<<" Analyzing"<<endl;
    MIL.Open(Files);
    Basic_Close(&UserHandle, 0); //Time of the last file
    UserHandle.Time_File.Close();

    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
    {
        ZtringListListF New;
        ZtringListListF Ref;
        ZtringListListF Diff;

        cout<<" Retrieving ref data"<<endl;
        if (File::Exists(DataBaseDirectory+__T("\\Basic\\Ref\\")+Ztring::ToZtring(StreamKind)+__T(".csv")))
            Ref.Load(DataBaseDirectory+__T("\\Basic\\Ref\\")+Ztring::ToZtring(StreamKind)+__T(".csv"));

        cout<<" Retrieving new data"<<endl;
        for (size_t FilePos=0; FilePos<MIL.Count_Get(); FilePos++)
            if (MIL.Count_Get(FilePos, (stream_t)StreamKind))
            {
                New.push_back(ZtringList());
                New.at(New.size()-1).push_back(Ztring()); //CompleteFileName
                for (size_t LinePos=0; LinePos<MIL.Count_Get(FilePos, (stream_t)StreamKind, 0); LinePos++)
                    New.at(New.size()-1).push_back(MIL.Get(FilePos, (stream_t)StreamKind, 0, LinePos, Info_Name));
                break;
            }

        for (size_t FilePos=0; FilePos<MIL.Count_Get(); FilePos++)
            for (size_t StreamPos=0; StreamPos<MIL.Count_Get(FilePos, (stream_t)StreamKind); StreamPos++)
            {
                New.push_back(ZtringList());
                New.at(New.size()-1).push_back(MIL.Get(FilePos, Stream_General, 0, __T("CompleteName")));
                for (size_t LinePos=0; LinePos<MIL.Count_Get(FilePos, (stream_t)StreamKind, StreamPos); LinePos++)
                {
                    Ztring Value=MIL.Get(FilePos, (stream_t)StreamKind, StreamPos, LinePos);
                    if (Value.find(__T('\r')) != string::npos || Value.find(__T('\n')) != string::npos)
                    {
                        Value.FindAndReplace(__T("\r\n"), __T(" / "), 0, Ztring_Recursive);
                        Value.FindAndReplace(__T("\r"), __T(" / "), 0, Ztring_Recursive);
                        Value.FindAndReplace(__T("\n"), __T(" / "), 0, Ztring_Recursive);
                        if (Value.size()>=3 && Value.rfind(__T(" / "))== Value.size()-3)
                            Value.resize(Value.size()-3);
                    }
                    New.at(New.size()-1).push_back(Value);
                }
            }

        if (!Dir::Exists(DataBaseDirectory+__T("\\Basic\\New")))
            Dir::Create(DataBaseDirectory+__T("\\Basic\\New"));
        New.Save(DataBaseDirectory+__T("\\Basic\\New\\")+Ztring::ToZtring(StreamKind)+__T(".csv"));

        cout<<" Diff"<<endl;
        size_t Ref_Pos=1;
        size_t New_Pos=1;
        while (Ref_Pos<Ref.size() && New_Pos<New.size())
        {
            if (New(New_Pos, 0)!=Ref(Ref_Pos, 0))
            {
                size_t Ref_PosNext=Ref_Pos;
                while (Ref_PosNext<Ref.size() && New(New_Pos, 0)!=Ref(Ref_PosNext, 0))
                    Ref_PosNext++;
                if (Ref_PosNext==Ref.size()) //Ref file not found
                {
                    Diff.push_back(New(New_Pos));
                    New_Pos++;
                }
                else
                {
                    for (; Ref_Pos<Ref_PosNext; Ref_Pos++)
                        Diff.push_back(Ref(Ref_Pos));
                }
            }
            else
            {
                bool IsDiff=false;
                size_t LinePos_Max=max(New(New_Pos).size(), Ref(Ref_Pos).size());
                for (size_t LinePos=1; LinePos<LinePos_Max; LinePos++)
                    if (New(New_Pos, LinePos)!=Ref(Ref_Pos, LinePos))
                    {
                        if (!IsDiff)
                        {
                            Diff.push_back(ZtringList());
                            Diff(Diff.size()-1, 0)=New(New_Pos, 0);
                            IsDiff=true;
                        }
                        Diff(Diff.size()-1, LinePos)=New(New_Pos, LinePos)+__T(" --- ")+Ref(Ref_Pos, LinePos);
                    }

                Ref_Pos++;
                New_Pos++;
            }

        }

        for (; Ref_Pos<Ref.size(); Ref_Pos++)
        {
            Diff.push_back(ZtringList());
            Diff(Diff.size()-1, 0)=Ref(Ref_Pos, 0);
            for (size_t LinePos=1; LinePos<Ref[Ref_Pos].size(); LinePos++)
                if (!Ref(Ref_Pos, LinePos).empty())
                    Diff(Diff.size()-1, LinePos)=__T(" --- ")+Ref(Ref_Pos, LinePos);
        }
        for (; New_Pos<New.size(); New_Pos++)
        {
            Diff.push_back(ZtringList());
            Diff(Diff.size()-1, 0)=New(New_Pos, 0);
            for (size_t LinePos=1; LinePos<New[New_Pos].size(); LinePos++)
                if (!New(New_Pos, LinePos).empty())
                    Diff(Diff.size()-1, LinePos)=New(New_Pos, LinePos)+__T(" --- ");
        }

        if (!Diff.empty())
        {
            Diff.insert(Diff.begin(), New(0));
            if (!Dir::Exists(DataBaseDirectory+__T("\\Basic\\Diff")))
                Dir::Create(DataBaseDirectory+__T("\\Basic\\Diff"));
            Diff.Save(DataBaseDirectory+__T("\\Basic\\Diff\\")+Ztring::ToZtring(StreamKind)+__T(".csv"));
        }
        else
            File::Delete(DataBaseDirectory+__T("\\Basic\\Diff\\")+Ztring::ToZtring(StreamKind)+__T(".csv")); //No difference, the file of a previous run would be stale
    }
}
