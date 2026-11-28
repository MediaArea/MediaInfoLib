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
#if defined(MEDIAINFO_DCP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_DcpAm.h"
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#include "MediaInfo/Multiple/File_DcpCpl.h"
#include "MediaInfo/XmlUtils.h"
#include "ZenLib/FileName.h"
#if MEDIAINFO_CONFORMANCE
#include "ZenLib/File.h"
#include <set>
#endif //MEDIAINFO_CONFORMANCE
#include "ThirdParty/tfsxml/tfsxml_tinyxml2.h"
using namespace tinyxml2;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DcpAm::File_DcpAm()
:File__Analyze(), File__HasReferences()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_DcpAm;
        StreamIDs_Width[0]=sizeof(size_t)*2;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent_Accept_Specific=true;
    #endif //MEDIAINFO_DEMUX

    //PKL
    PKL_Pos=(size_t)-1;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_DcpAm::Streams_Finish()
{
    ReferenceFiles_Finish();

    // Detection of IMF CPL
    bool IsImf=false;
    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            if (Retrieve((stream_t)StreamKind, StreamPos, "MuxingMode").find(__T("IMF CPL"))==0)
                IsImf=true;
    if (IsImf)
    {
        Fill(Stream_General, 0, General_Format, "IMF AM", Unlimited, true, true);
        Clear(Stream_General, 0, General_Format_Version);
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DcpAm::FileHeader_Begin()
{
    static const char *InteropNs="http://www.digicine.com/PROTO-ASDCP-AM-20040311#";
    static const char *SmpteNs="http://www.smpte-ra.org/schemas/429-9/2007/AM";
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    XMLElement* AssetMap=document.FirstChildElement();
    const char *NameSpace;
    if (!AssetMap || strcmp(LocalName(AssetMap, NameSpace), "AssetMap") || !NameSpace)
    {
        Reject("DcpAm");
        return false;
    }
    std::string FmtVersion;
    if (!strcmp(NameSpace, InteropNs))
    {
        FmtVersion="Interop";
    }
    else if (!strcmp(NameSpace, SmpteNs))
    {
        FmtVersion="SMPTE";
    }
    else
    {
        Reject("DcpAm");
        return false;
    }

    Accept("DcpAm");
    Fill(Stream_General, 0, General_Format, "DCP AM");
    Fill(Stream_General, 0, General_Format_Version, FmtVersion);
    #if MEDIAINFO_CONFORMANCE
        if (FmtVersion=="SMPTE" && !Config->File_IsReferenced_Get())
            Conformance_AssetMap(AssetMap, NameSpace);
    #endif //MEDIAINFO_CONFORMANCE
    #if defined(MEDIAINFO_REFERENCES_YES)
    Config->File_ID_OnlyRoot_Set(false);

    ReferenceFiles_Accept(this, Config);

    //Parsing main elements
    for (XMLElement* AssetMap_Item=AssetMap->FirstChildElement(); AssetMap_Item; AssetMap_Item=AssetMap_Item->NextSiblingElement())
    {
        const char *AmItemNs, *AmItemName = LocalName(AssetMap_Item, AmItemNs);
        if (!AmItemNs || strcmp(AmItemNs, NameSpace))
            continue; // item has wrong namespace

        //AssetList
        if (!strcmp(AmItemName, "AssetList"))
        {
            for (XMLElement* AssetList_Item=AssetMap_Item->FirstChildElement(); AssetList_Item; AssetList_Item=AssetList_Item->NextSiblingElement())
            {
                //Asset
                if (MatchQName(AssetList_Item, "Asset", NameSpace))
                {
                    File_DcpPkl::stream Stream;

                    for (XMLElement* Asset_Item=AssetList_Item->FirstChildElement(); Asset_Item; Asset_Item=Asset_Item->NextSiblingElement())
                    {
                        //ChunkList
                        if (MatchQName(Asset_Item, "ChunkList", NameSpace))
                        {
                            for (XMLElement* ChunkList_Item=Asset_Item->FirstChildElement(); ChunkList_Item; ChunkList_Item=ChunkList_Item->NextSiblingElement())
                            {
                                //Chunk
                                if (MatchQName(ChunkList_Item, "Chunk", NameSpace))
                                {
                                    File_DcpPkl::stream::chunk Chunk;

                                    for (XMLElement* Chunk_Item=ChunkList_Item->FirstChildElement(); Chunk_Item; Chunk_Item=Chunk_Item->NextSiblingElement())
                                    {
                                        //Path
                                        if (MatchQName(Chunk_Item, "Path", NameSpace))
                                            if (const char* Text=Chunk_Item->GetText())
                                                Chunk.Path=Text;
                                    }

                                    Stream.ChunkList.push_back(Chunk);
                                }
                            }
                        }

                        //Id
                        if (MatchQName(Asset_Item, "Id", NameSpace))
                            if (const char* Text=Asset_Item->GetText())
                                Stream.Id=Text;

                        //PackingList
                        if (MatchQName(Asset_Item, "PackingList", NameSpace) &&
                            Asset_Item->GetText() && !strcmp(Asset_Item->GetText(), "true"))
                        {
                            PKL_Pos=Streams.size();
                            Stream.StreamKind=Stream_Max+2; // Means PKL
                        }
                    }

                    Streams.push_back(Stream);
                 }
            }
        }

        //Creator
        if (!strcmp(AmItemName, "Creator"))
            Fill(Stream_General, 0, General_Encoded_Library, AssetMap_Item->GetText());

        //IssueDate
        if (!strcmp(AmItemName, "IssueDate"))
            Fill(Stream_General, 0, General_Encoded_Date, AssetMap_Item->GetText());

        //Issuer
        if (!strcmp(AmItemName, "Issuer"))
            Fill(Stream_General, 0, General_EncodedBy, AssetMap_Item->GetText());
    }

    //Merging with PKL
    if (RelatedFiles_CanBeSearched(File_Name) && PKL_Pos<Streams.size() && Streams[PKL_Pos].ChunkList.size()==1)
    {
        Ztring PKL_FileName; PKL_FileName.From_UTF8(Streams[PKL_Pos].ChunkList[0].Path);
        if (PKL_FileName.find(__T("file://"))==0 && PKL_FileName.find(__T("file:///"))==string::npos)
            PKL_FileName.erase(0, 7); //TODO: better handling of relative and absolute file naes
        MediaInfo_Internal MI;
        MI.Option(__T("File_KeepInfo"), __T("1"));
        Ztring ParseSpeed_Save=MI.Option(__T("ParseSpeed_Get"), __T(""));
        Ztring Demux_Save=MI.Option(__T("Demux_Get"), __T(""));
        MI.Option(__T("ParseSpeed"), __T("0"));
        MI.Option(__T("Demux"), Ztring());
        MI.Option(__T("File_IsReferenced"), __T("1"));
        size_t MiOpenResult=ReferenceFiles->Open_Related(MI, FileName::FromFolder_Get(File_Name, PKL_FileName));
        MI.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
        MI.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
        if (MiOpenResult
            && (MI.Get(Stream_General, 0, General_Format)==__T("DCP PKL")
            ||  MI.Get(Stream_General, 0, General_Format)==__T("IMF PKL")))
        {
            MergeFromPkl(((File_DcpPkl*)MI.Info)->Streams);

            for (size_t Pos=0; Pos<MI.Count_Get(Stream_Other); ++Pos)
            {
                Stream_Prepare(Stream_Other);
                Merge(*MI.Info, Stream_Other, Pos, StreamPos_Last);
            }
        }
    }

    //Creating the playlist
    if (!Config->File_IsReferenced_Get())
    {
        for (File_DcpPkl::streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
            if (Stream->StreamKind==Stream_Max+1 && Stream->ChunkList.size()==1) // Means CPL
            {
                sequence* Sequence=new sequence;
                Sequence->FileNames.push_back(Ztring().From_UTF8(Stream->ChunkList[0].Path));

                Sequence->StreamID=ReferenceFiles->Sequences_Size()+1;
                ReferenceFiles->AddSequence(Sequence);
            }

        ReferenceFiles->FilesForStorage=true;
    }
    #endif //MEDIAINFO_REFERENCES_YES

    //All should be OK...
    Element_Offset=File_Size;
    return true;
}

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
void File_DcpAm::MergeFromPkl (File_DcpPkl::streams &StreamsToMerge)
{
    for (File_DcpPkl::streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
    {
        for (File_DcpPkl::streams::iterator StreamToMerge=StreamsToMerge.begin(); StreamToMerge!=StreamsToMerge.end(); ++StreamToMerge)
            if (StreamToMerge->Id==Stream->Id)
            {
                if (Stream->StreamKind==Stream_Max)
                    Stream->StreamKind=StreamToMerge->StreamKind;
                if (Stream->OriginalFileName.empty())
                    Stream->OriginalFileName=StreamToMerge->OriginalFileName;
                if (Stream->Type.empty())
                    Stream->Type=StreamToMerge->Type;
                if (Stream->AnnotationText.empty())
                    Stream->AnnotationText=StreamToMerge->AnnotationText;
            }
    }
}

//***************************************************************************
// Conformance
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
static string Dcp_Text(const XMLElement* Element)
{
    const char* Text=Element->GetText();
    if (!Text)
        return {};
    string Value(Text);
    const auto Begin=Value.find_first_not_of(" \t\r\n");
    if (Begin==string::npos)
        return {};
    Value.erase(Value.find_last_not_of(" \t\r\n")+1);
    Value.erase(0, Begin);
    return Value;
}

//---------------------------------------------------------------------------
static string Dcp_Lower(string Value)
{
    for (auto& Character : Value)
        if (Character>='A' && Character<='Z')
            Character+='a'-'A';
    return Value;
}

//---------------------------------------------------------------------------
static Ztring Dcp_FullPath(const Ztring& Reference_FileName, const string& Path)
{
    Ztring Directory=ZenLib::FileName(Reference_FileName).Path_Get();
    if (!Directory.empty())
        Directory+=PathSeparator;
    Ztring Path_Local;
    Path_Local.From_UTF8(Path);
    #ifdef WINDOWS
        Path_Local.FindAndReplace(__T("/"), __T("\\"), 0, Ztring_Recursive);
    #endif //WINDOWS
    return Directory+Path_Local;
}
#endif //MEDIAINFO_CONFORMANCE

//---------------------------------------------------------------------------
// Asset Map (SMPTE ST 429-9:2014), with the Map Profile for file systems (Annex A)
// Not checked: PackingList element against the kind of the asset, case of the file names on the file system
#if MEDIAINFO_CONFORMANCE
void File_DcpAm::Conformance_AssetMap(const XMLElement* AssetMap, const char* NameSpace)
{
    conformance_errors Errors;

    //File name (A.2)
    const Ztring Name(ZenLib::FileName(File_Name).Name_Get()+__T('.')+ZenLib::FileName(File_Name).Extension_Get());
    if (Name!=__T("ASSETMAP.xml"))
        Errors.Add("File name is not ASSETMAP.xml (actual "+Name.To_UTF8()+")");

    bool VolumeCount_IsPresent=false;
    std::set<string> Ids, Paths;
    for (const auto* Item=AssetMap->FirstChildElement(); Item; Item=Item->NextSiblingElement())
    {
        const char* Item_NameSpace;
        const char* Item_Name=LocalName(Item, Item_NameSpace);
        if (!Item_NameSpace || strcmp(Item_NameSpace, NameSpace))
            continue;
        if (!strcmp(Item_Name, "VolumeCount"))
        {
            VolumeCount_IsPresent=true;
            const auto VolumeCount=Dcp_Text(Item);
            if (VolumeCount!="1")
                Errors.Add("VolumeCount is not 1 (actual "+VolumeCount+")");
        }
        if (strcmp(Item_Name, "AssetList"))
            continue;
        for (const auto* Asset=Item->FirstChildElement(); Asset; Asset=Asset->NextSiblingElement())
        {
            if (!MatchQName(Asset, "Asset", NameSpace))
                continue;
            string Id, Path, VolumeIndex, Offset, Length;
            bool Length_IsPresent=false;
            size_t Chunk_Count=0;
            for (const auto* Asset_Item=Asset->FirstChildElement(); Asset_Item; Asset_Item=Asset_Item->NextSiblingElement())
            {
                if (MatchQName(Asset_Item, "Id", NameSpace))
                    Id=Dcp_Lower(Dcp_Text(Asset_Item));
                if (!MatchQName(Asset_Item, "ChunkList", NameSpace))
                    continue;
                for (const auto* Chunk=Asset_Item->FirstChildElement(); Chunk; Chunk=Chunk->NextSiblingElement())
                {
                    if (!MatchQName(Chunk, "Chunk", NameSpace))
                        continue;
                    if (Chunk_Count++)
                        continue;
                    for (const auto* Chunk_Item=Chunk->FirstChildElement(); Chunk_Item; Chunk_Item=Chunk_Item->NextSiblingElement())
                    {
                        if (MatchQName(Chunk_Item, "Path", NameSpace))
                            Path=Dcp_Text(Chunk_Item);
                        else if (MatchQName(Chunk_Item, "VolumeIndex", NameSpace))
                            VolumeIndex=Dcp_Text(Chunk_Item);
                        else if (MatchQName(Chunk_Item, "Offset", NameSpace))
                            Offset=Dcp_Text(Chunk_Item);
                        else if (MatchQName(Chunk_Item, "Length", NameSpace))
                        {
                            Length_IsPresent=true;
                            Length=Dcp_Text(Chunk_Item);
                        }
                    }
                }
            }
            if (!Id.empty() && !Ids.insert(Id).second)
                Errors.Add("Several Assets have the same Id (actual "+Id+")");
            if (Chunk_Count!=1)
                Errors.Add("ChunkList does not contain one Chunk (actual "+std::to_string(Chunk_Count)+", Id "+Id+")");
            if (!Chunk_Count)
                continue;
            if (!VolumeIndex.empty() && VolumeIndex!="1")
                Errors.Add("VolumeIndex is not 1 (actual "+VolumeIndex+")");
            if (!Offset.empty() && Offset!="0")
                Errors.Add("Offset is not 0 (actual "+Offset+")");

            //Path (A.3)
            if (Path.empty())
            {
                Errors.Add("Path is missing (Id "+Id+")");
                continue;
            }
            if (!Paths.insert(Dcp_Lower(Path)).second)
                Errors.Add("Several Assets have the same Path (actual "+Path+")");
            if (Path.find_first_of("?#")!=string::npos)
                Errors.Add("Path contains a query or a fragment (actual "+Path+")");
            if (Path[0]=='/' || Path.find(':')!=string::npos)
                Errors.Add("Path is not a relative-path reference (actual "+Path+")");
            if (Path.size()>100)
                Errors.Add("Path is longer than 100 characters (actual "+std::to_string(Path.size())+")");
            if (Path.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_./")!=string::npos)
                Errors.Add("Path contains characters other than a-z, A-Z, 0-9, dash, underscore and period (actual "+Path+")");
            size_t Segment_Count=0;
            int Depth=0;
            bool IsOutside=false;
            for (size_t Begin=0; Begin<=Path.size();)
            {
                auto End=Path.find('/', Begin);
                if (End==string::npos)
                    End=Path.size();
                const auto Segment=Path.substr(Begin, End-Begin);
                Segment_Count++;
                if (Segment.size()>100)
                    Errors.Add("Path segment is longer than 100 characters (actual "+std::to_string(Segment.size())+")");
                if (Segment=="..")
                    IsOutside|=--Depth<0;
                else if (!Segment.empty() && Segment!=".")
                    Depth++;
                Begin=End+1;
            }
            if (Segment_Count>10)
                Errors.Add("Path has more than 10 segments (actual "+std::to_string(Segment_Count)+")");
            if (IsOutside)
                Errors.Add("Path is outside the directory of the Asset Map (actual "+Path+")");

            //File
            const auto FullPath=Dcp_FullPath(File_Name, Path);
            if (!File::Exists(FullPath))
                Errors.Add("File of an Asset is missing (actual "+Path+")");
            else if (Length_IsPresent && Length!=std::to_string(File::Size_Get(FullPath)))
                Errors.Add("Length is not the size of the file (actual "+Length+", expected "+std::to_string(File::Size_Get(FullPath))+")");
        }
    }
    if (!VolumeCount_IsPresent)
        Errors.Add("VolumeCount is missing");

    //Filling
    const auto File_Offset_Save=File_Offset;
    File_Offset=(int64u)-1-(Buffer_Offset+Element_Offset); //Errors are about the whole file, no offset is provided
    for (const auto& Error : Errors)
        Fill_Conformance("AM", Error.Value, {}, Conformance_Error, Stream_General, 0);
    File_Offset=File_Offset_Save;
}
#endif //MEDIAINFO_CONFORMANCE

} //NameSpace

#endif //MEDIAINFO_DCP_YES

