/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

// RegressionTest.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <chrono>
#include <map>
#include <vector>
#include "tchar.h"
#include "MediaInfoDLL/MediaInfoDLL.h"
#include "ZenLib/ZtringListListF.h"
#include "ZenLib/File.h"
#include "ZenLib/FileName.h"
#include "ZenLib/Dir.h"
#include "RegressionTest/RegressionTest.h"
#include "MediaInfo/MediaInfo_Events.h"
using namespace MediaInfoDLL;
using namespace ZenLib;
using namespace std;

/***************************************************************************/
/* Events                                                                  */
/***************************************************************************/

//Parsing times (Basic\New\Times.csv): a line per file of the list, with its complete name, an empty column, its size,
//the seconds of its parsing, then the CPU seconds of the process, its count of read operations, its count of bytes read
//and its count of other input/output operations during the parsing. The files are parsed one by one, and the reads
//from the cache are counted, so the counts of a file do not depend on the disk, unlike the seconds.
//A file referenced by another one (e.g. the files of a DCP) is parsed while the file referencing it is parsed, so its
//time and counts are in the ones of this file, and it has no line.
//The start event of a file has its name. The end events are sent by the parser of the file and by its sub-parsers,
//with their depth as the count of stream IDs: 1 for the parser of a file of the list, 1 more for a sub-parser and for
//the parser of a referenced file.
//The log events of the library are stored as they are, each on its own line, after the line of the file being parsed
//if any. E.g. a file which can not be opened or which is not found has no start event, its name is in its log event.

struct process_counters
{
    float64 CPU; //Seconds, user and kernel
    int64u  Reads;
    int64u  Bytes;
    int64u  Others;
};

static process_counters Process_Counters_Get()
{
    process_counters Counters={};
    FILETIME Creation, Exit, Kernel, User;
    if (GetProcessTimes(GetCurrentProcess(), &Creation, &Exit, &Kernel, &User))
        Counters.CPU=((((int64u)Kernel.dwHighDateTime)<<32|Kernel.dwLowDateTime)+(((int64u)User.dwHighDateTime)<<32|User.dwLowDateTime))/10000000.0; //In 100 ns
    IO_COUNTERS IO;
    if (GetProcessIoCounters(GetCurrentProcess(), &IO))
    {
        Counters.Reads=IO.ReadOperationCount;
        Counters.Bytes=IO.ReadTransferCount;
        Counters.Others=IO.OtherOperationCount;
    }
    return Counters;
}

struct opened_file
{
    chrono::steady_clock::time_point Start;
    chrono::steady_clock::time_point End;
    process_counters Start_Counters;
    process_counters End_Counters;
    int64u Size;
    bool   IsEnded;
};

struct UserHandle_struct
{
    File                Time_File;
    vector<opened_file> Opened; //The file of the list, then the files referenced by the previous one
    Ztring              Parser;
    Ztring              Logs; //Log events during the parsing of the file of the list, written after its line
};

static void Basic_Close(struct UserHandle_struct* UserHandle, size_t Count)
{
    //Files after the first Count ones finished, the time of the file of the list written
    while (UserHandle->Opened.size()>Count)
    {
        if (UserHandle->Opened.size()==1)
        {
            const opened_file& Opened=UserHandle->Opened[0];
            float64 Seconds=chrono::duration<float64>((Opened.IsEnded?Opened.End:chrono::steady_clock::now())-Opened.Start).count();
            process_counters End=Opened.IsEnded?Opened.End_Counters:Process_Counters_Get();
            UserHandle->Time_File.Write(__T(';')+Ztring::ToZtring(Opened.Size)+__T(';')+Ztring::ToZtring(Seconds, 3)
                                       +__T(';')+Ztring::ToZtring(End.CPU-Opened.Start_Counters.CPU, 3)
                                       +__T(';')+Ztring::ToZtring(End.Reads-Opened.Start_Counters.Reads)
                                       +__T(';')+Ztring::ToZtring(End.Bytes-Opened.Start_Counters.Bytes)
                                       +__T(';')+Ztring::ToZtring(End.Others-Opened.Start_Counters.Others)
                                       +EOL+UserHandle->Logs);
            UserHandle->Logs.clear();
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
    Opened.Start=chrono::steady_clock::now();
    Opened.Start_Counters=Process_Counters_Get();
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
        Opened.End=chrono::steady_clock::now();
        Opened.End_Counters=Process_Counters_Get();
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

void Basic_Log_0 (struct MediaInfo_Event_Log_0* Event, struct UserHandle_struct* UserHandle)
{
    if (!Event->MessageStringU)
        return;
    Ztring Message;
    Message.From_Unicode(Event->MessageStringU);

    //Files whose parsing is finished
    while (!UserHandle->Opened.empty() && UserHandle->Opened.back().IsEnded)
        Basic_Close(UserHandle, UserHandle->Opened.size()-1);

    //Between the lines of the files of the list, else after the line of the file being parsed
    if (UserHandle->Opened.empty())
        UserHandle->Time_File.Write(Message+EOL);
    else
        UserHandle->Logs+=Message+EOL;
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
                    case MediaInfo_Event_Log                                                    : if (EventVersion==0 && Data_Size>=sizeof(struct MediaInfo_Event_Log_0)) Basic_Log_0((struct MediaInfo_Event_Log_0*)Data_Content, UserHandle); break;
                    default                                                                     : ;
                }
                break;
        default : ; //ParserID is unknown
    }
}

/***************************************************************************/
/* Times                                                                   */
/***************************************************************************/

struct time_line
{
    Ztring Name;
    Ztring Size;
    Ztring Seconds;
    Ztring CPU; //This one and the next ones are empty in a line written without them
    Ztring Reads;
    Ztring Bytes;
    Ztring Others;
};

static vector<time_line> Basic_Times_Load(const Ztring& FileName)
{
    //Lines of a Times.csv file, the fields are read from the end as the complete name may have a ';', with the CPU time
    //and the counts after the seconds, or without them in a line written before they were added
    vector<time_line> Lines;
    File F;
    if (!F.Open(FileName))
        return Lines;
    string Data((size_t)F.Size_Get(), '\0');
    if (!Data.empty())
        Data.resize(F.Read((int8u*)&Data[0], Data.size()));
    Ztring Text;
    Text.From_UTF8(Data.c_str(), 0, Data.size());
    const size_t EOL_Size=Ztring(EOL).size();
    for (size_t Begin=0; Begin<Text.size();)
    {
        size_t End=Text.find(EOL, Begin);
        if (End==string::npos)
            End=Text.size();
        Ztring Line=Text.substr(Begin, End-Begin);
        Begin=End+EOL_Size;

        //Positions of the last separators, from the end
        size_t Pos[7];
        size_t Pos_Count=0;
        size_t Search=Line.size();
        while (Pos_Count<7 && Search)
        {
            Search=Line.rfind(__T(';'), Search-1);
            if (Search==string::npos)
                break;
            Pos[Pos_Count++]=Search;
        }
        size_t Fields; //After the empty column
        if (Pos_Count>=3 && Pos[2]+1==Pos[1])
            Fields=2; //Size and seconds, tested first as a line with the counts never has an empty read count
        else if (Pos_Count==7 && Pos[6]+1==Pos[5])
            Fields=6; //Size, seconds, CPU time and counts
        else
            continue; //Not a line of times with its empty column, e.g. the name of a file whose parsing did not end or a log
        time_line Item;
        Item.Name=Line.substr(0, Pos[Fields]);
        Ztring* Items[6]={&Item.Size, &Item.Seconds, &Item.CPU, &Item.Reads, &Item.Bytes, &Item.Others};
        for (size_t Field=0; Field<Fields; Field++)
        {
            size_t Field_Begin=Pos[Fields-1-Field]+1;
            size_t Field_End=Field+1<Fields?Pos[Fields-2-Field]:Line.size();
            *Items[Field]=Line.substr(Field_Begin, Field_End-Field_Begin);
        }
        Lines.push_back(Item);
    }
    return Lines;
}

static Ztring Basic_Times_Counts_Diff(const time_line& N, const time_line& R)
{
    //CPU time and counts as "new --- ref", if one of the lines has them
    if (N.CPU.empty() && R.CPU.empty())
        return Ztring();
    return __T(';')+N.CPU+__T(" --- ")+R.CPU+__T(';')+N.Reads+__T(" --- ")+R.Reads+__T(';')+N.Bytes+__T(" --- ")+R.Bytes+__T(';')+N.Others+__T(" --- ")+R.Others;
}

static void Basic_Times_Diff(const Ztring& DataBaseDirectory)
{
    //Basic\Diff\Times.csv: the files whose parsing time is different by more than 1 second from Basic\Ref\Times.csv,
    //whose size is different, whose CPU time is different by more than 25% and 0.5 second, or whose counts are
    //different, as "new --- ref", and the files only in one of them. The CPU time and the counts are compared only if
    //both lines have them. The CPU time is counted by steps of 15.625 ms, and the parsing of a file waiting for the disk
    //does not take the same CPU time in each run, hence its tolerance.
    Ztring Ref_FileName=DataBaseDirectory+__T("\\Basic\\Ref\\Times.csv");
    if (!File::Exists(Ref_FileName))
        return; //No reference times, New\Times.csv may be copied as Ref\Times.csv
    vector<time_line> New=Basic_Times_Load(DataBaseDirectory+__T("\\Basic\\New\\Times.csv"));
    vector<time_line> Ref=Basic_Times_Load(Ref_FileName);

    //Lines with the same name taken in the same order, a name may be in several lines
    map<Ztring, vector<size_t> > Ref_Lines;
    for (size_t Pos=0; Pos<Ref.size(); Pos++)
        Ref_Lines[Ref[Pos].Name].push_back(Pos);
    map<Ztring, size_t> Ref_Lines_Used;
    vector<bool> Ref_IsUsed(Ref.size());

    Ztring Diff;
    for (size_t Pos=0; Pos<New.size(); Pos++)
    {
        const time_line& N=New[Pos];
        map<Ztring, vector<size_t> >::iterator Ref_Line=Ref_Lines.find(N.Name);
        size_t& Used=Ref_Lines_Used[N.Name];
        if (Ref_Line==Ref_Lines.end() || Used>=Ref_Line->second.size())
        {
            Diff+=N.Name+__T(";;")+N.Size+__T(" --- ;")+N.Seconds+__T(" --- ")+Basic_Times_Counts_Diff(N, time_line())+EOL;
            continue;
        }
        const time_line& R=Ref[Ref_Line->second[Used]];
        Ref_IsUsed[Ref_Line->second[Used]]=true;
        Used++;
        float64 Seconds_Diff=N.Seconds.To_float64()-R.Seconds.To_float64();
        bool IsSizeDiff=N.Size!=R.Size;
        bool IsCountsDiff=false;
        if (!N.CPU.empty() && !R.CPU.empty())
        {
            float64 CPU_Diff=N.CPU.To_float64()-R.CPU.To_float64();
            float64 CPU_Tolerance=R.CPU.To_float64()/4;
            if (CPU_Tolerance<0.5)
                CPU_Tolerance=0.5;
            IsCountsDiff=CPU_Diff>CPU_Tolerance || CPU_Diff<-CPU_Tolerance || N.Reads!=R.Reads || N.Bytes!=R.Bytes || N.Others!=R.Others;
        }
        if (Seconds_Diff>1 || Seconds_Diff<-1 || IsSizeDiff || IsCountsDiff) //1 second of tolerance
            Diff+=N.Name+__T(";;")+(IsSizeDiff?(N.Size+__T(" --- ")+R.Size):N.Size)+__T(';')+N.Seconds+__T(" --- ")+R.Seconds+Basic_Times_Counts_Diff(N, R)+EOL;
    }
    for (size_t Pos=0; Pos<Ref.size(); Pos++)
        if (!Ref_IsUsed[Pos])
            Diff+=Ref[Pos].Name+__T(";; --- ")+Ref[Pos].Size+__T("; --- ")+Ref[Pos].Seconds+Basic_Times_Counts_Diff(time_line(), Ref[Pos])+EOL;

    if (!Diff.empty())
    {
        File F;
        if (F.Create(DataBaseDirectory+__T("\\Basic\\Diff\\Times.csv")))
            F.Write(Diff);
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
    MIL.Option(__T("Event_CallBackFunction"), Event_CallBackFunction_Text.str()); //Log events of the library, global
    if (!Dir::Exists(DataBaseDirectory+__T("\\Basic\\New")))
        Dir::Create(DataBaseDirectory+__T("\\Basic\\New"));
    if (!Dir::Exists(DataBaseDirectory+__T("\\Basic\\Diff")))
        Dir::Create(DataBaseDirectory+__T("\\Basic\\Diff"));
    File::Delete(DataBaseDirectory+__T("\\Basic\\New\\Times.csv"));
    File::Delete(DataBaseDirectory+__T("\\Basic\\Diff\\Times.csv")); //Diff of a previous run
    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
        File::Delete(DataBaseDirectory+__T("\\Basic\\Diff\\")+Ztring::ToZtring(StreamKind)+__T(".csv")); //Diff of a previous run, stale during this run and if this run does not finish
    UserHandle.Time_File.Open(DataBaseDirectory+__T("\\Basic\\New\\Times.csv"), File::Access_Write_Append);
    TIME_ZONE_INFORMATION Time_Zone;
    GetTimeZoneInformation(&Time_Zone); //The first call of the process (e.g. for the local dates of a file) reads the time zone data, not counted in the input/output operations of the first file

    cout<<" Analyzing"<<endl;
    MIL.Open(Files);
    MIL.Option(__T("Event_CallBackFunction")); //Removed, the user handle is local
    Basic_Close(&UserHandle, 0); //Time of the last file
    UserHandle.Time_File.Close();

    cout<<" Times diff"<<endl;
    Basic_Times_Diff(DataBaseDirectory);

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
    }
}
