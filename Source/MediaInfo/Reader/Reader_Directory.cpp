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
#if defined(MEDIAINFO_DIRECTORY_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Reader/Reader_Directory.h"
#if defined(MEDIAINFO_FILE_YES)
    #include "MediaInfo/Reader/Reader_File.h"
#endif //defined(MEDIAINFO_FILE_YES)
#include "MediaInfo/File__Analyse_Automatic.h"
#include "ZenLib/Dir.h"
#include "ZenLib/FileName.h"
#include <algorithm>
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

size_t Reader_Directory::Format_Test(MediaInfo_Internal* MI, String File_Name)
{
    #ifdef MEDIAINFO_BDMV_YES
        if (Dir::Exists(File_Name) && FileName::FolderName_Get(File_Name+PathSeparator)==__T("BDMV"))
            return Bdmv_Format_Test(MI, File_Name);
    #endif //MEDIAINFO_BDMV_YES

    #ifdef MEDIAINFO_P2_YES
        if (Dir::Exists(File_Name) && P2_Format_Test(MI, File_Name))
            return 1;
    #endif //MEDIAINFO_P2_YES

    #ifdef MEDIAINFO_XDCAM_YES
        if (Dir::Exists(File_Name) && Xdcam_Format_Test(MI, File_Name))
            return 1;
    #endif //MEDIAINFO_XDCAM_YES

    return 0;
}

//---------------------------------------------------------------------------
void Reader_Directory::Directory_Cleanup(ZtringList &List)
{
    #ifdef MEDIAINFO_BDMV_YES
        Bdmv_Directory_Cleanup(List);
    #endif //MEDIAINFO_BDMV_YES

    #ifdef MEDIAINFO_P2_YES
        P2_Directory_Cleanup(List);
    #endif //MEDIAINFO_P2_YES

    #ifdef MEDIAINFO_XDCAM_YES
        Xdcam_Directory_Cleanup(List);
    #endif //MEDIAINFO_XDCAM_YES
}

//***************************************************************************
// Clips
//***************************************************************************

//---------------------------------------------------------------------------
// Analysis of a P2 card or of an XDCAM disc given as a directory: its longest clip, as the longest playlist of a Blu-ray disc
#if defined(MEDIAINFO_P2_YES) || defined(MEDIAINFO_XDCAM_YES)
int Reader_Directory::Clips_Format_Test(MediaInfo_Internal* MI, const ZtringList &Clips, const Char* Format)
{
    size_t MaxDuration_Pos=(size_t)-1;
    float64 MaxDuration=-1;
    for (size_t Pos=0; Pos<Clips.size(); Pos++)
    {
        MediaInfo_Internal MI_Clip;
        if (!MI_Clip.Open(Clips[Pos]) || MI_Clip.Get(Stream_General, 0, General_Format)!=Format)
            continue;
        float64 Duration=MI_Clip.Get(Stream_General, 0, General_Duration).To_float64();
        if (Duration>MaxDuration)
        {
            MaxDuration=Duration;
            MaxDuration_Pos=Pos;
        }
    }
    if (MaxDuration_Pos==(size_t)-1)
        return 0;

    //As a file given to this MediaInfo, without Open() which would wait for the end of the analysis in progress (in its thread with the Thread
    //option, the one running this code)
    #if defined(MEDIAINFO_FILE_YES)
        {
        CriticalSectionLocker CSL(MI->CS);
        if (MI->Reader)
            return 0; //There is a problem
        MI->Config.File_Names.clear();
        MI->Config.File_Names.push_back(Clips[MaxDuration_Pos]);
        MI->Reader=new Reader_File();
        }
        return MI->Reader->Format_Test(MI, Clips[MaxDuration_Pos])?1:0;
    #else //defined(MEDIAINFO_FILE_YES)
        return 0;
    #endif //defined(MEDIAINFO_FILE_YES)
}
#endif //defined(MEDIAINFO_P2_YES) || defined(MEDIAINFO_XDCAM_YES)

//***************************************************************************
// Blu-ray stuff
//***************************************************************************

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_BDMV_YES
int Reader_Directory::Bdmv_Format_Test(MediaInfo_Internal* MI, const String &File_Name)
{
    if (!MI->SelectFromExtension(__T("Bdmv")))
        return 0;

    MI->Open_Buffer_Init(0, File_Name);

    MI->Open_Buffer_Continue(NULL, 0);

    MI->Open_Buffer_Finalize();

    return 1;
}
#endif //MEDIAINFO_BDMV_YES

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_BDMV_YES
void Reader_Directory::Bdmv_Directory_Cleanup(ZtringList &List)
{
    //if there is a BDMV folder, this is blu-ray, for each disc of the list
    for (size_t File_Pos=0; File_Pos<List.size(); File_Pos++)
    {
        size_t Name_Pos=List[File_Pos].size()-FileName::FullName_Get(List[File_Pos]).size();
        if (Name_Pos && Name_Pos+10==List[File_Pos].size() && !List[File_Pos].compare(Name_Pos, 10, __T("index.bdmv")) && FileName::FolderName_Get(List[File_Pos])==__T("BDMV"))
        {
            //This is a BDMV index, parsing the directory only if index and movie objects are BOTH present
            Ztring Path=List[File_Pos].substr(0, Name_Pos); //"%CompletePath%\BDMV\"
            if (List.Find(Path+__T("MovieObject.bdmv"))!=string::npos)
            {
                //We want the folder instead of the files
                List[File_Pos]=FileName::FolderPath_Get(List[File_Pos]); //only %CompletePath%\BDMV

                for (size_t Pos=0; Pos<List.size(); Pos++)
                {
                    if (!List[Pos].compare(0, Path.size(), Path)) //Remove all subdirs of the BDMV folder, not the ones of a folder whose name starts with BDMV
                    {
                        //Removing the file in the blu-ray directory
                        List.erase(List.begin()+Pos);
                        if (Pos<File_Pos)
                            File_Pos--;
                        Pos--;
                    }
                }
            }
        }
    }
}
#endif //MEDIAINFO_BDMV_YES

//***************************************************************************
// P2 stuff
//***************************************************************************

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_P2_YES
int Reader_Directory::P2_Format_Test(MediaInfo_Internal* MI, const String &File_Name)
{
    //CONTENTS folder, the clip files are in its CLIP folder, the names of the folders being in upper or lower case as the P2 Clip parser
    //accepts them
    Ztring Folder=FileName::FolderName_Get(File_Name+PathSeparator);
    Folder.MakeUpperCase();
    if (Folder!=__T("CONTENTS"))
        return 0;
    ZtringList Files, Clips;
    ZtringList Dirs=Dir::GetAllFileNames(File_Name, Dir::Include_Dirs);
    for (size_t Dirs_Pos=0; Dirs_Pos<Dirs.size(); Dirs_Pos++)
    {
        Ztring Name=FileName::FullName_Get(Dirs[Dirs_Pos]);
        Name.MakeUpperCase();
        if (Name==__T("CLIP"))
            Files+=Dir::GetAllFileNames(Dirs[Dirs_Pos], Dir::Include_Files);
    }
    for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
    {
        Ztring Extension=FileName(Files[Files_Pos]).Extension_Get();
        Extension.MakeUpperCase();
        if (FileName::FullName_Get(Files[Files_Pos]).size()==10 && Extension==__T("XML")) //e.g. "0001AB.XML"
            Clips.push_back(Files[Files_Pos]);
    }
    std::sort(Clips.begin(), Clips.end());

    int ToReturn=Clips_Format_Test(MI, Clips, __T("P2 Clip"));
    Dir::ReleaseFileNames(Dirs);
    Dir::ReleaseFileNames(Files);
    return ToReturn;
}
#endif //MEDIAINFO_P2_YES

//---------------------------------------------------------------------------
// The file (name of 10 characters) is a clip file in the CONTENTS/CLIP folder of a P2 card; in another case than the one of the folders of a
// card, as the P2 Clip parser accepts them, the card must also have its VIDEO or AUDIO folder
#ifdef MEDIAINFO_P2_YES
static bool P2_IsClip(const Ztring &File_Name)
{
    Ztring Extension=FileName(File_Name).Extension_Get();
    Extension.MakeUpperCase();
    if (Extension!=__T("XML"))
        return false;
    Ztring Clip_Folder=FileName::FolderName_Get(File_Name);
    Ztring Contents_Folder=FileName::FolderName_Get(File_Name, 1);
    if (Clip_Folder==__T("CLIP") && Contents_Folder==__T("CONTENTS"))
        return true;
    Clip_Folder.MakeUpperCase();
    Contents_Folder.MakeUpperCase();
    if (Clip_Folder!=__T("CLIP") || Contents_Folder!=__T("CONTENTS"))
        return false;
    ZtringList Dirs=Dir::GetAllFileNames(FileName::FolderPath_Get(File_Name, 1), Dir::Include_Dirs);
    bool ToReturn=false;
    for (size_t Pos=0; Pos<Dirs.size(); Pos++)
    {
        Ztring Name=FileName::FullName_Get(Dirs[Pos]);
        Name.MakeUpperCase();
        if (Name==__T("VIDEO") || Name==__T("AUDIO"))
            ToReturn=true;
    }
    Dir::ReleaseFileNames(Dirs); //Only their names are used
    return ToReturn;
}
#endif //MEDIAINFO_P2_YES

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_P2_YES
void Reader_Directory::P2_Directory_Cleanup(ZtringList &List)
{
    //if there is a CONTENTS/CLIP folder, this is P2
    for (size_t File_Pos=0; File_Pos<List.size(); File_Pos++)
    {
        Ztring Clip_Path=FileName::FromFolder_Get(List[File_Pos], Ztring()); //"%CompletePath%/CONTENTS/CLIP/"
        if (Clip_Path.size()+10!=List[File_Pos].size())
            continue;
        if (P2_IsClip(List[File_Pos]))
        {
            //This is a P2 CLIP, all the listed files are in the card if its folder is not in their names (e.g. "." for CONTENTS)
            Ztring Card_Path=FileName::FromFolder_Get(List[File_Pos], Ztring(), 2);
            Ztring Path_Begin=List[File_Pos].compare(0, Card_Path.size(), Card_Path)?Ztring():Card_Path;
            bool HasChanged=false;
            for (size_t Pos=0; Pos<List.size(); Pos++)
            {
                if (List[Pos].find(Path_Begin)==0 && List[Pos].find(Clip_Path)!=0) //Remove all subdirs of Path_Begin but not with CLIP
                {
                    //Removing the file in the P2 directory
                    List.erase(List.begin()+Pos);
                    Pos--;
                    HasChanged=true;
                }
            }
            if (HasChanged)
                File_Pos=0;
        }
    }
}
#endif //MEDIAINFO_P2_YES

//***************************************************************************
// P2 stuff
//***************************************************************************

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_XDCAM_YES
int Reader_Directory::Xdcam_Format_Test(MediaInfo_Internal* MI, const String &File_Name)
{
    //Folder of the disc, the clip files are in its Clip folder
    Ztring Disc_Path=File_Name+PathSeparator;
    if (!Dir::Exists(Disc_Path+__T("Clip")) || !Dir::Exists(Disc_Path+__T("Edit")) || !Dir::Exists(Disc_Path+__T("General")) || !Dir::Exists(Disc_Path+__T("Sub")))
        return 0;
    ZtringList Clips;
    ZtringList Files=Dir::GetAllFileNames(Disc_Path+__T("Clip"), Dir::Include_Files);
    for (size_t Files_Pos=0; Files_Pos<Files.size(); Files_Pos++)
        if (FileName(Files[Files_Pos]).Extension_Get()==__T("XML"))
            Clips.push_back(Files[Files_Pos]);
    std::sort(Clips.begin(), Clips.end());

    int ToReturn=Clips_Format_Test(MI, Clips, __T("XDCAM Clip"));
    Dir::ReleaseFileNames(Files);
    return ToReturn;
}
#endif //MEDIAINFO_XDCAM_YES

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_XDCAM_YES
void Reader_Directory::Xdcam_Directory_Cleanup(ZtringList &List)
{
    //if there is a XDCAM/Clip folder, this is Xdcam
    for (size_t File_Pos=0; File_Pos<List.size(); File_Pos++)
    {
        FileName Xdcam_FileName(List[File_Pos]);
        if ((Xdcam_FileName.Extension_Get()==__T("XML")) && FileName::FolderName_Get(List[File_Pos])==__T("Clip"))
        {
            //This is a XDCAM CLIP
            Ztring Disc_Path=FileName::FromFolder_Get(List[File_Pos], Ztring(), 1);
            if (Dir::Exists(Disc_Path+__T("Edit")) && Dir::Exists(Disc_Path+__T("General")) && Dir::Exists(Disc_Path+__T("Sub")))
            {
                //All the listed files are in the disc if its folder is not in their names (e.g. "." for Clip)
                Ztring Path_Begin=List[File_Pos].compare(0, Disc_Path.size(), Disc_Path)?Ztring():Disc_Path;
                Ztring Clip_Path=FileName::FromFolder_Get(List[File_Pos], Ztring());
                bool HasChanged=false;
                for (size_t Pos=0; Pos<List.size(); Pos++)
                {
                    if (List[Pos].find(Path_Begin)==0 && (List[Pos].find(Clip_Path)!=0 || FileName(List[Pos]).Extension_Get()!=__T("XML"))) //Remove all subdirs of Path_Begin but not with the right XML
                    {
                        //Removing the file in the XDCAM directory
                        List.erase(List.begin()+Pos);
                        Pos--;
                        HasChanged=true;
                    }
                }
                if (HasChanged)
                    File_Pos=0;
            }
        }
    }
}
#endif //MEDIAINFO_XDCAM_YES

} //NameSpace

#endif //MEDIAINFO_DIRECTORY_YES

