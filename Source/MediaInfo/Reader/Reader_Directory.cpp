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
#include "ZenLib/Dir.h"
#include "ZenLib/FileName.h"
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
        if (Dir::Exists(File_Name) && FileName::FolderName_Get(File_Name+PathSeparator)==__T("CONTENT"))
            return P2_Format_Test(MI, File_Name);
    #endif //MEDIAINFO_P2_YES

    #ifdef MEDIAINFO_XDCAM_YES
        if (Dir::Exists(File_Name) && FileName::FolderName_Get(File_Name+PathSeparator)==__T("XDCAM"))
            return Xdcam_Format_Test(MI, File_Name);
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
    if (!MI->SelectFromExtension(__T("P2_Clip")))
        return 0;

    MI->Open(File_Name+__T("CLIP")+PathSeparator+__T("0013MM.XML"));

    return 1;
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
        if (Clip_Path.size()+10==List[File_Pos].size() && FileName::FolderName_Get(List[File_Pos])==__T("CLIP") && FileName::FolderName_Get(List[File_Pos], 1)==__T("CONTENTS"))
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
    if (!MI->SelectFromExtension(__T("Xdcam_Clip")))
        return 0;

    MI->Open(File_Name+__T("CLIP")+PathSeparator+__T("0013MM.XML"));

    return 1;
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

