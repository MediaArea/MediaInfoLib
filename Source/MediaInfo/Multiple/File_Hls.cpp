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
#if defined(MEDIAINFO_HLS_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Hls.h"
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#if defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/FileName.h"
#include <map>
#include <vector>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

#if defined(MEDIAINFO_REFERENCES_YES)
//---------------------------------------------------------------------------
//Attribute list (RFC 8216 section 4.2), a quoted string may contain commas
static std::map<Ztring, Ztring> Hls_Attributes(const Ztring &List)
{
    std::map<Ztring, Ztring> Attributes;
    size_t Pos=0;
    while (Pos<List.size())
    {
        size_t Name_End=List.find_first_of(__T("=,"), Pos);
        if (Name_End==string::npos)
            break;
        Ztring Name=List.substr(Pos, Name_End-Pos);
        Name.Trim();
        Pos=Name_End+1;
        if (List[Name_End]==__T(','))
            continue; //No value
        while (Pos<List.size() && List[Pos]==__T(' '))
            Pos++;
        Ztring Value;
        size_t Value_End;
        if (Pos<List.size() && List[Pos]==__T('"'))
        {
            size_t Quote_End=List.find(__T('"'), Pos+1);
            if (Quote_End==string::npos)
                Quote_End=List.size();
            Value=List.substr(Pos+1, Quote_End-Pos-1);
            Value_End=List.find(__T(','), Quote_End);
        }
        else
        {
            Value_End=List.find(__T(','), Pos);
            Value=List.substr(Pos, Value_End==string::npos?string::npos:(Value_End-Pos));
            Value.Trim();
        }
        if (!Name.empty())
            Attributes[Name]=Value;
        if (Value_End==string::npos)
            break;
        Pos=Value_End+1;
    }
    return Attributes;
}

//---------------------------------------------------------------------------
static void Hls_Info(std::map<std::string, Ztring> &Infos, const char* Name, const Ztring &Value)
{
    if (!Value.empty() && Infos.find(Name)==Infos.end())
        Infos[Name]=Value;
}

//---------------------------------------------------------------------------
static Ztring Hls_YesNo(const Ztring &Value)
{
    if (Value==__T("YES"))
        return __T("Yes");
    if (Value==__T("NO"))
        return __T("No");
    return Ztring();
}
#endif //MEDIAINFO_REFERENCES_YES

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Hls::File_Hls()
:File__Analyze(), File__HasReferences()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Hls;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent_Accept_Specific=true;
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Hls::FileHeader_Begin()
{
    //Element_Size
    if (File_Size>1024*1024 || File_Size<10)
    {
        Reject("HLS");
        return false; //HLS files are not big
    }

    if (Buffer_Size<File_Size)
        return false; //Wait for complete file

    Ztring Document; Document.From_UTF8((char*)Buffer, Buffer_Size);
    ZtringList Lines;
    size_t LinesSeparator_Pos=Document.find_first_of(__T("\r\n"));
    if (LinesSeparator_Pos>File_Size-1)
    {
        Reject("HLS");
        return false;
    }
    Ztring LinesSeparator;
    if (Document[LinesSeparator_Pos]==__T('\r') && LinesSeparator_Pos+1<Document.size() && Document[LinesSeparator_Pos+1]==__T('\n'))
        LinesSeparator=__T("\r\n");
    else if (Document[LinesSeparator_Pos]==__T('\r'))
        LinesSeparator=__T("\r");
    else if (Document[LinesSeparator_Pos]==__T('\n'))
        LinesSeparator=__T("\n");
    else
    {
        Reject("HLS");
        return false;
    }
    Lines.Separator_Set(0, LinesSeparator);
    Lines.Write(Document);

    if (Lines(0)!=__T("#EXTM3U"))
    {
        Reject("HLS");
        return false;
    }

    Accept("HLS");
    Fill(Stream_General, 0, General_Format, "HLS");

    ReferenceFiles_Accept(this, Config);

    #if defined(MEDIAINFO_REFERENCES_YES)
    if (!IsSub)
        ReferenceFiles->ContainerHasNoId=true;

    sequence* Sequence=new sequence;

    bool IsGroup=false;
    Ztring PreviousFile;
    std::map<Ztring, sequence*> Variants; //A variant may be listed several times (e.g. once per group of audio renditions), it is parsed once
    std::vector<std::map<Ztring, Ztring> > Renditions; //Added after the variants
    for (size_t Line=0; Line<Lines.size(); Line++)
    {
        if (!Lines[Line].empty())
        {
            if (Lines[Line].find(__T("#EXT-X-KEY:"))==0)
            {
                ZtringListList List;
                List.Separator_Set(0, __T(","));
                List.Separator_Set(1, __T("="));
                List.Write(Lines[Line].substr(11, string::npos));
                for (size_t Pos=0; Pos<List.size(); ++Pos)
                {
                    if (List[Pos](0)==__T("METHOD"))
                    {
                        if (List[Pos](1).find(__T("AES-128"))==0)
                        {
                            Fill(Stream_General, 0, General_Encryption_Format, "AES");
                            Fill(Stream_General, 0, General_Encryption_Length, "128");
                            Fill(Stream_General, 0, General_Encryption_Method, "Segment");
                            Fill(Stream_General, 0, General_Encryption_Mode, "CBC");
                            Fill(Stream_General, 0, General_Encryption_Padding, "PKCS7");
                            Fill(Stream_General, 0, General_Encryption_InitializationVector, "Sequence number");

                            #if MEDIAINFO_AES
                            //Trying to get the key from FileName.FileExt.key
                            if (Config->Encryption_Key_Get().empty())
                            {
                                File KeyFile;
                                if (KeyFile.Open(File_Name+__T(".key")))
                                {
                                    if (KeyFile.Size_Get()==16)
                                    {
                                        int8u Key[16];
                                        if (KeyFile.Read(Key, 16)==16)
                                            Config->Encryption_Key_Set(Key, 16);
                                    }
                                    else
                                        Fill(Stream_General, 0, "Encryption_Key_Problem", KeyFile.Size_Get());
                                }
                            }
                            #endif
                        }
                        Fill(Stream_General, 0, General_Encryption, List[Pos](1));
                    }
                }
            }
            else if (Lines[Line].find(__T("#EXT-X-STREAM-INF:"))==0)
            {
                IsGroup=true;
            }
            else if (Lines[Line].find(__T("#EXT-X-MEDIA:"))==0)
            {
                Renditions.push_back(Hls_Attributes(Lines[Line].substr(13)));
            }
            else if (Lines[Line][0]==__T('#'))
                ;
            else
            {
                if (IsGroup)
                {
                    IsGroup=false;
                    sequence* &Variant=Variants[Lines[Line]];
                    if (!Variant)
                    {
                        Variant=Sequence;
                        Sequence->AddFileName(Lines[Line]);
                        Sequence->StreamID=ReferenceFiles->Sequences_Size()+1;
                        ReferenceFiles->AddSequence(Sequence);
                        Sequence=new sequence();
                        #if MEDIAINFO_EVENTS
                            ParserIDs[0]=MediaInfo_Parser_HlsIndex;
                            StreamIDs_Width[0]=sizeof(size_t);
                        #endif //MEDIAINFO_EVENTS
                    }
                }
                else if (Lines[Line]!=PreviousFile)
                {
                    PreviousFile=Lines[Line];
                    Sequence->AddFileName(Lines[Line]);
                }
            }
        }
    }

    if (!Sequence->FileNames.empty())
    {
        ReferenceFiles->AddSequence(Sequence);
        Fill(Stream_General, 0, General_Format_Profile, "Media");
    }
    else
    {
        delete Sequence;
        Fill(Stream_General, 0, General_Format_Profile, "Master");

        //Renditions with their own playlist
        for (size_t Pos=0; Pos<Renditions.size(); Pos++)
        {
            std::map<Ztring, Ztring> &Rendition=Renditions[Pos];
            const Ztring &URI=Rendition[__T("URI")];
            if (URI.empty())
                continue; //In the variant streams
            sequence* &Rendition_Sequence=Variants[URI];
            if (!Rendition_Sequence)
            {
                Rendition_Sequence=new sequence;
                Rendition_Sequence->AddFileName(URI);
                Rendition_Sequence->PathOnly=true; //The playlists of the renditions and of the variants often have the same name in their folders
                Rendition_Sequence->StreamID=ReferenceFiles->Sequences_Size()+1;
                ReferenceFiles->AddSequence(Rendition_Sequence);
                #if MEDIAINFO_EVENTS
                    ParserIDs[0]=MediaInfo_Parser_HlsIndex;
                    StreamIDs_Width[0]=sizeof(size_t);
                #endif //MEDIAINFO_EVENTS
            }

            //Metadata from the first rendition with this playlist, also if it is a variant, for the streams of the type of the rendition
            if (!Rendition_Sequence->Infos.empty())
                continue;
            const Ztring &Type=Rendition[__T("TYPE")];
            if (Type==__T("AUDIO"))
                Rendition_Sequence->Infos_StreamKind=Stream_Audio;
            else if (Type==__T("VIDEO"))
                Rendition_Sequence->Infos_StreamKind=Stream_Video;
            else if (Type==__T("SUBTITLES"))
                Rendition_Sequence->Infos_StreamKind=Stream_Text;
            else
                continue;
            Hls_Info(Rendition_Sequence->Infos, "Language", Rendition[__T("LANGUAGE")]);
            Hls_Info(Rendition_Sequence->Infos, "Title", Rendition[__T("NAME")]);
            Hls_Info(Rendition_Sequence->Infos, "Default", Hls_YesNo(Rendition[__T("DEFAULT")]));
            Hls_Info(Rendition_Sequence->Infos, "Forced", Hls_YesNo(Rendition[__T("FORCED")]));
        }
    }
    #endif //MEDIAINFO_REFERENCES_YES

    Element_Offset=File_Size;

    //All should be OK...
    Config->File_TestContinuousFileNames_Set(false);
    Config->File_TestDirectory_Set(false);
    return true;
}

} //NameSpace

#endif //MEDIAINFO_HLS_YES

