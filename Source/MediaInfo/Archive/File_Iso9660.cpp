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
#if defined(MEDIAINFO_ISO9660_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Archive/File_Iso9660.h"
#include "MediaInfo/MediaInfo_Internal.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Iso9660::~File_Iso9660()
{
    for (const auto& MI_Item : MI_MasterFiles)
        delete MI_Item.second; //MI_Item.second=NULL;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iso9660::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "ISO 9660");
}

//---------------------------------------------------------------------------
void File_Iso9660::Streams_Finish()
{
    //Merge
    if (MI_MasterFiles.empty())
        return;
    MediaInfo_Internal* MI=MI_MasterFiles.begin()->second;
    Ztring FileSizeS=Retrieve_Const(Stream_General, 0, General_FileSize);
    Merge(*(MI->Info));
    Merge(*(MI->Info), Stream_General, 0, 0);
    const Ztring &Format=Retrieve(Stream_General, 0, General_Format);
    Fill(Stream_General, 0, General_Format, __T("ISO 9660 / ")+Format, true);
    Fill(Stream_General, 0, General_FileSize, FileSizeS, true);
    Clear(Stream_General, 0, General_OverallBitRate);

    //Merge
    if (MI_DataFiles.empty())
        return;
    MI=MI_DataFiles.begin()->second;
    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
        for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind); Pos++)
            Merge(*(MI->Info), (stream_t)StreamKind, Pos, Pos);
}

//***************************************************************************
// File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iso9660::FileHeader_Parse()
{
    //Element_Size
    if (Buffer_Size<0x8000+6)
    {
        Element_WaitForMoreData();
        return;
    }

    int64u Magic=CC8(Buffer+32768);
    switch (Magic)
    {
        case 0x0143443030310100LL: //0x01+"CD001"+0x0100 (ECMA-119 / ISO-9660, CDROM)
            break;
        default:
            Reject("ISO 9660");
            return;
    }
    Skip_XX(0x8000,                                             "System area");

    //All should be OK...
    Accept("ISO 9660");

    //Temp
    Logical_Block_Size=2048;
    MI_Current=nullptr;
    Element_Code=0;
    #if MEDIAINFO_TRACE
        Trace_Activated_Save=Trace_Activated;
    #endif //MEDIAINFO_TRACE

    return;
}
//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iso9660::Header_Parse()
{
    if ((int32s)Element_Code<0)
    {
        Header_Fill_Size(Logical_Block_Size);
        Header_Fill_Code(Element_Code);
        return;
    }

    //Parsing
    int8u Type;
    Get_B1 (Type,                                               "Volume Descriptor Type");
    Skip_Local(5,                                               "Standard Identifier");
    Skip_B1(                                                    "Volume Descriptor Version");
    Skip_B1(                                                    "Unused field");

    Header_Fill_Code(Type, Ztring().From_CC1(Type));
    Header_Fill_Size(Logical_Block_Size);
}

//---------------------------------------------------------------------------
void File_Iso9660::Data_Parse()
{
    switch (Element_Code)
    {
        case 0x00000001 : Primary_Volume_Descriptor(); break;
        case 0x80000000 : Directory(); break;
        case 0x80000001 : File(); break;
        default         : ForceFinish();
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iso9660::Primary_Volume_Descriptor()
{
    Element_Name("Primary Volume Descriptor");

    //Parsing
    Ztring VolumeIdentifier;
    int32u Volume_Space_Size, Location_Of_Path_Table;
    Skip_Local(32,                                              "System Identifier");
    Get_Local (32, VolumeIdentifier,                            "Volume Identifier");
    Skip_XX(8,                                                  "Unused field");
    Get_D4(Volume_Space_Size,                                   "Volume Space Size"); Param_Info2(Volume_Space_Size*(int64u)Logical_Block_Size, " bytes");
    Skip_XX(32,                                                 "Unused field");
    Skip_D2(                                                    "Volume Set Size");
    Skip_D2(                                                    "Volume Sequence Number");
    Get_D2 (Logical_Block_Size,                                 "Logical Block Size");
    Skip_D4(                                                    "Path Table Size");
    Get_L4 (Location_Of_Path_Table,                             "Location of Occurrence of Type L Path Table");
    Skip_L4(                                                    "Location of Optional Occurrence of Type L Path Table");
    Skip_B4(                                                    "Location of Occurrence of Type M Path Table");
    Skip_B4(                                                    "Location of Optional Occurrence of Type M Path Table");
    Directory_Record(34,                                        "Directory Record for Root Directory");
    Skip_Local(128,                                             "Volume Set Identifier");
    Skip_Local(128,                                             "Publisher Identifier");
    Skip_Local(128,                                             "Data Preparer Identifier");
    Skip_Local(128,                                             "Application Identifier");
    Skip_Local(37,                                              "Copyright File Identifier");
    Skip_Local(37,                                              "Abstract File Identifier");
    Skip_Local(37,                                              "Bibliographic File Identifier");
    Skip_XX(17,                                                 "Volume Creation Date and Time");

    Fill(Stream_General, 0, "Title", VolumeIdentifier);

    if (!NotParsed.empty())
    {
        Element_Code=0x80000000;
        GoTo(((int64u)*NotParsed.begin())*Logical_Block_Size);
        return;
    }

    ForceFinish();
}

//---------------------------------------------------------------------------
void File_Iso9660::Path_Table()
{
    Element_Name("Path Table");

    //Parsing
    Skip_L1(                                                    "x");
    Skip_L1(                                                    "x");

    Finish();
}

//---------------------------------------------------------------------------
void File_Iso9660::Directory()
{
    int32u ThisLocation=(File_Offset+Buffer_Offset)/Logical_Block_Size;
    NotParsed.erase(ThisLocation);
    Parsed.insert(ThisLocation);

    //Parsing
    Element_Name("Directory");
    while (Element_Offset<Element_Size)
    {
        int8u Len_DR;
        Peek_L1(Len_DR);
        if (!Len_DR)
        {
            Skip_XX(Element_Size-Element_Offset,                "Padding");
            break;
        }
        Directory_Record();
    }

    if (!NotParsed.empty())
    {
        GoTo(((int64u)*NotParsed.begin())*Logical_Block_Size);
        return;
    }

    //Checking know structures
    Manage_MasterFiles();
}

//---------------------------------------------------------------------------
void File_Iso9660::File()
{
    Element_Name("File");

    if (!MI_Current)
    {
        Element_Info1(MI_DataFiles.empty()?MI_MasterFileInfos.empty()?Ztring():MI_MasterFileInfos.begin()->first:MI_DataFileInfos.begin()->first);
        MI_Current=new MediaInfo_Internal;
        MI_Current->Option(__T("FormatDetection_MaximumOffset"), __T("1048576"));
        MI_Current->Option(__T("File_IsReferenced"), __T("1"));
        MI_Current->Open_Buffer_Init(MI_Current_EndOffset-MI_Current_StartOffset);
    }

    //Preparing to fill MediaInfo with a buffer
    //MI->Open_Buffer_Position_Set(File_Offset+Buffer_Offset);

    //Sending the buffer to MediaInfo
    auto Status= MI_Current->Open_Buffer_Continue(Buffer+Buffer_Offset, (size_t)Element_Size);

    //Details
    #if MEDIAINFO_TRACE
    if (Config_Trace_Level)
    {
        if (!MI_Current->Inform().empty())
            Element_Show_Add(MI_Current->Info);
    }
    #endif //MEDIAINFO_TRACE

    //Testing if MediaInfo always need data
    File_GoTo= MI_Current->Open_Buffer_Continue_GoTo_Get();
    if (File_GoTo!=(int64u)-1)
        GoTo(MI_Current_StartOffset+File_GoTo);
    else if (Status[Config->ParseSpeed>=1?IsFinished:IsFilled] || File_Offset+Buffer_Offset+Element_Size>=MI_Current_EndOffset)
    {
        MI_Current->Info->Open_Buffer_Finalize();
        Manage_Files();
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iso9660::Directory_Record(int32u Size, const char* Name)
{
    //Parsing
    Element_Begin1(Name?Name:"Directory Record");
    auto End=Element_Offset+Size;
    int32u Location, Length;
    int8u Len_DR, Flags, Len_FileID;
    bool Directory;
    Get_L1 (Len_DR,                                             "Length of Directory Record");
    if (!Size || (Len_DR && Len_DR<Size))
        End=Element_Offset+Len_DR-1;
    Skip_L1(                                                    "Extended Attribute Record Length");
    Get_D4 (Location,                                           "Location of Extent"); Param_Info1(__T("0x")+Ztring::ToZtring((Location)*(int64u)Logical_Block_Size, 16));
    Get_D4 (Length,                                             "Data Length");
    Skip_B7(                                                    "Recording Date and Time");
    Get_L1 (Flags,                                              "File Flags");
    Get_Flags (Flags, 1, Directory,                             "Directory");
    Skip_L1(                                                    "File Unit Size");
    Skip_L1(                                                    "Interleave Gap Size");
    Skip_D2(                                                    "Volume Sequence Number");
    Get_L1 (Len_FileID,                                         "Length of File Identifier");
    Ztring FileID;
    if (Directory && Len_FileID==1)
    {
        int8u Probe;
        Peek_L1(Probe);
        if (Probe<=1)
        {
            int8u RootID;
            Get_L1 (RootID,                                     "File Identifier");
            FileID+=(Char)RootID;
            Element_Info1("(Root)");
            Element_Info1(Probe);
            Len_FileID--;
        }
    }
    if (Len_FileID)
    {
        bool IsUnicode=false;
        for (size_t i=0; i<Len_FileID; i++)
        {
            auto Value=Buffer[Buffer_Offset+Element_Offset+i];
            if (!Value || Value>=0x80)
                IsUnicode=true;
        }
        if (IsUnicode && !(Len_FileID&1))
            Get_UTF16B(Len_FileID, FileID,                      "File Identifier");
        else
            Get_ISO_8859_1(Len_FileID, FileID,                  "File Identifier");
        Element_Info1(FileID);
        if (!FileID.empty())
        {
            size_t i=FileID.size()-1;
            for (; i; i--)
            {
                auto Value=FileID[i];
                if (Value<'0' || Value>'9')
                    break;
            }
            if (i && i<FileID.size()-1 && FileID[i]==';')
                FileID.resize(i); // Remove version
        }
    }
    if (Element_Offset<End)
        Skip_XX(End-Element_Offset,                             "Padding");
    Element_End0();

    int32u ThisLocation=(File_Offset+Buffer_Offset)/Logical_Block_Size;
    if (Len_FileID)
    {
        auto& ThisRecord=Records[ThisLocation];
        ThisRecord.push_back({ Location, Length, FileID, Flags });
    }
    if (Directory)
    {
        if (ThisLocation==0x10)
            Root_Location=Location;
        if (Parsed.find(Location)==Parsed.end())
            NotParsed.insert(Location);
    }
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iso9660::Manage_Files()
{
    //Data files
    if (!MI_DataFileInfos.empty())
    {
        std::swap(MI_DataFiles[MI_DataFileInfos.begin()->first], MI_Current);
        MI_DataFileInfos.erase(MI_DataFileInfos.begin());
        if (Manage_File(MI_MasterFileInfos))
        {
            #if MEDIAINFO_TRACE
                Trace_Activated=Trace_Activated_Save;
            #endif //MEDIAINFO_TRACE
            ForceFinish(); // Nothing else to do
        }
        return;
    }

    //Master files
    if (!MI_MasterFileInfos.empty())
    {
        std::swap(MI_MasterFiles[MI_MasterFileInfos.begin()->first], MI_Current);
        MI_MasterFileInfos.erase(MI_MasterFileInfos.begin());
        if (Manage_File(MI_MasterFileInfos))
            Manage_DataFiles();
        return;
    }

    //Nothing already done, so
    Manage_MasterFiles();
}

//---------------------------------------------------------------------------
void File_Iso9660::Manage_MasterFiles()
{
    const auto& Root=Records[Root_Location];

    //DVD Video
    for (auto& Level1 : Root)
        if (Level1.Name==__T("VIDEO_TS") && Level1.Flags&0x2) // Directory
            for (auto& Level2 : Records[Level1.Location])
                if (Level2.Name.size()>=4 && Level2.Name.find(__T(".IFO"), Level2.Name.size()-4)!=string::npos && !(Level2.Flags & 0x2))
                    MI_MasterFileInfos[Level1.Name+PathSeparator+Level2.Name]=&Level2;

    if (Manage_File(MI_MasterFileInfos))
        ForceFinish(); // Nothing else to do
}

//---------------------------------------------------------------------------
void File_Iso9660::Manage_DataFiles()
{
    #if MEDIAINFO_TRACE
        Trace_Activated_Save=Trace_Activated;
        Trace_Activated=false; //It is too big, disabling trace for now for full USAC parsing
    #endif //MEDIAINFO_TRACE

    Ztring MI_FileName;
    if (MI_MasterFiles.size()>1)
    {
        int64u MaxDuration=0;
        MediaInfo_Internal* MI_MaxDuration=nullptr;
        for (const auto& MI_Item : MI_MasterFiles)
        {
            int64u Duration=MI_Item.second->Get(Stream_General, 0, General_Duration).To_int64u();
            if (MaxDuration<Duration)
            {
                MaxDuration=Duration;
                MI_MaxDuration=MI_Item.second;
                MI_FileName=MI_Item.first;
            }
        }
        if (!MaxDuration)
        {
            ForceFinish(); // Nothing else to do
            return;
        }
        MI_MasterFiles.clear();
        MI_MasterFiles[MI_FileName]=MI_MaxDuration;
    }

    //DVD Video
    if (MI_FileName.size()>=5 && MI_FileName.find(__T("0.IFO"), MI_FileName.size()-5)!=string::npos && MI_FileName.rfind(Ztring(__T("VIDEO_TS"))+PathSeparator, 0)!=string::npos)
    {
        auto FileSize=MI_MasterFiles.begin()->second->Get(Stream_General, 0, General_FileSize);
        MI_FileName.erase(0, 9);
        MI_FileName.erase(MI_FileName.size()-5);
        const auto& Root=Records[Root_Location];
        for (auto& Level1 : Root)
            if (Level1.Name==__T("VIDEO_TS") && Level1.Flags&0x2) // Directory
                for (auto& Level2 : Records[Level1.Location])
                    if (Level2.Name.size()>=4 && Level2.Name.rfind(MI_FileName, 0)!=string::npos && !(Level2.Flags & 0x2))
                        if (Level2.Name==MI_FileName+__T("1.VOB"))
                            MI_DataFileInfos[Level1.Name+PathSeparator+Level2.Name]=&Level2;
    }
 
    if (Manage_File(MI_DataFileInfos))
        ForceFinish(); // Nothing else to do
    #if MEDIAINFO_TRACE
    else
    {
        Trace_Activated_Save=Trace_Activated;
        Trace_Activated=false; //It is too big, disabling trace for now for full USAC parsing
    }
    #endif //MEDIAINFO_TRACE

}

//---------------------------------------------------------------------------
bool File_Iso9660::Manage_File(file_infos& MI_FileInfos)
{
    if (!MI_FileInfos.empty())
    {
        Element_Code=0x80000001;
        const auto& Record=*MI_FileInfos.begin()->second;
        MI_Current_StartOffset=((int64u)Record.Location)*Logical_Block_Size;
        GoTo(MI_Current_StartOffset);
        MI_Current_EndOffset=MI_Current_StartOffset+Record.Length;
        return false;
    }

    return true;
}

} //NameSpace

#endif //MEDIAINFO_ISO9660_YES
