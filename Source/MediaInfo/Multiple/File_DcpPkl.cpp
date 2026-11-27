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
#include "MediaInfo/Multiple/File_DcpPkl.h"
#include "MediaInfo/Multiple/File_DcpAm.h"
#include "MediaInfo/Multiple/File_DcpCpl.h"
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#include "MediaInfo/XmlUtils.h"
#if defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_REFERENCES_YES)
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
File_DcpPkl::File_DcpPkl()
:File__Analyze(), File__HasReferences()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_DcpPkl;
        StreamIDs_Width[0]=sizeof(size_t)*2;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent_Accept_Specific=true;
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_DcpPkl::Streams_Finish()
{
    if (Config->File_IsReferenced_Get())
        return;

    ReferenceFiles_Finish();

    // Detection of IMF CPL
    bool IsImf=false;
    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            if (Retrieve((stream_t)StreamKind, StreamPos, "MuxingMode").find(__T("IMF CPL"))==0)
                IsImf=true;
    if (IsImf)
    {
        Fill(Stream_General, 0, General_Format, "IMF PKL", Unlimited, true, true);
        Clear(Stream_General, 0, General_Format_Version);
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_DcpPkl::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    if (Config->File_IsReferenced_Get())
        return 0;

    return ReferenceFiles_Seek(Method, Value, ID);
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DcpPkl::FileHeader_Begin()
{
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    XMLElement* PackingList=document.FirstChildElement();
    const char *NameSpace;
    if (!PackingList || strcmp(LocalName(PackingList, NameSpace), "PackingList") || !NameSpace)
    {
        Reject("DcpPkl");
        return false;
    }
    if (strcmp(NameSpace, "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#") &&
        strcmp(NameSpace, "http://www.smpte-ra.org/schemas/429-8/2007/PKL") &&
        strcmp(NameSpace, "http://www.smpte-ra.org/schemas/2067-2/2016/PKL"))
    {
        Reject("DcpPkl");
        return false;
    }

    Accept("DcpPkl");
    Fill(Stream_General, 0, General_Format, "DCP PKL");
    #if defined(MEDIAINFO_REFERENCES_YES)
    Config->File_ID_OnlyRoot_Set(false);

    //Parsing main elements
    for (XMLElement* PackingList_Item=PackingList->FirstChildElement(); PackingList_Item; PackingList_Item=PackingList_Item->NextSiblingElement())
    {
        //AssetList
        if (MatchQName(PackingList_Item, "AssetList", NameSpace))
        {
            for (XMLElement* AssetList_Item=PackingList_Item->FirstChildElement(); AssetList_Item; AssetList_Item=AssetList_Item->NextSiblingElement())
            {
                //Asset
                if (MatchQName(AssetList_Item, "Asset", NameSpace))
                {
                    stream Stream;

                    for (XMLElement* File_Item=AssetList_Item->FirstChildElement(); File_Item; File_Item=File_Item->NextSiblingElement())
                    {
                        const char* Text=File_Item->GetText();
                        const char *FileItemNs, *FileItemName = LocalName(File_Item, FileItemNs);
                        if (!FileItemNs || strcmp(FileItemNs, NameSpace))
                            continue; // item has wrong namespace
                        //AnnotationText
                        if (Text && !strcmp(FileItemName, "AnnotationText"))
                            Stream.AnnotationText=Text;

                        //Id
                        if (Text && !strcmp(FileItemName, "Id"))
                            Stream.Id=Text;

                        //OriginalFileName
                        if (Text && !strcmp(FileItemName, "OriginalFileName"))
                            Stream.OriginalFileName=Text;

                        //Type
                        if (!strcmp(FileItemName, "Type"))
                        {
                            if (!Text)
                                Stream.StreamKind=Stream_Other;
                            else if (!strcmp(Text, "application/x-smpte-mxf;asdcpKind=Picture"))
                                Stream.StreamKind=Stream_Video;
                            else if (!strcmp(Text, "application/x-smpte-mxf;asdcpKind=Sound"))
                                Stream.StreamKind=Stream_Audio;
                            else if (!strcmp(Text, "text/xml") || !strcmp(Text, "text/xml;asdcpKind=CPL"))
                                Stream.StreamKind=Stream_Max+1; // Means CPL
                            else
                                Stream.StreamKind=Stream_Other;
                        }
                    }

                    Streams.push_back(Stream);
                }
            }
        }
    }

    //Merging with Assetmap
    #if MEDIAINFO_CONFORMANCE
        bool AssetMap_IsParsed=false;
    #endif //MEDIAINFO_CONFORMANCE
    if (!Config->File_IsReferenced_Get())
    {
        ReferenceFiles_Accept(this, Config);

        Ztring Assetmap_FileName=FileName::FromFolder_Get(File_Name, __T("ASSETMAP.xml"));
        bool IsOk=false;
        if (RelatedFiles_CanBeSearched(File_Name))
        {
            if (File::Exists(Assetmap_FileName))
                IsOk=true;
            else
            {
                Assetmap_FileName=FileName::FromFolder_Get(File_Name, __T("ASSETMAP")); //Old fashion, without ".xml"
                if (File::Exists(Assetmap_FileName))
                    IsOk=true;
            }
        }
        if (IsOk)
        {
            MediaInfo_Internal MI;
            MI.Option(__T("File_KeepInfo"), __T("1"));
            Ztring ParseSpeed_Save=MI.Option(__T("ParseSpeed_Get"), __T(""));
            Ztring Demux_Save=MI.Option(__T("Demux_Get"), __T(""));
            MI.Option(__T("ParseSpeed"), __T("0"));
            MI.Option(__T("Demux"), Ztring());
            MI.Option(__T("File_IsReferenced"), __T("1"));
            size_t MiOpenResult=ReferenceFiles->Open_Related(MI, Assetmap_FileName);
            MI.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
            MI.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
            if (MiOpenResult
                && (MI.Get(Stream_General, 0, General_Format)==__T("DCP AM")
                || MI.Get(Stream_General, 0, General_Format)==__T("IMF AM")))
            {
                MergeFromAm(((File_DcpAm*)MI.Info)->Streams);
                #if MEDIAINFO_CONFORMANCE
                    AssetMap_IsParsed=true;
                #endif //MEDIAINFO_CONFORMANCE
            }
        }
    }
    #if MEDIAINFO_CONFORMANCE
        if (!Config->File_IsReferenced_Get() && strcmp(NameSpace, "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#"))
            Conformance_PackingList(PackingList, NameSpace, AssetMap_IsParsed);
    #endif //MEDIAINFO_CONFORMANCE

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
void File_DcpPkl::MergeFromAm (File_DcpPkl::streams &StreamsToMerge)
{
    for (File_DcpPkl::streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
    {
        for (File_DcpPkl::streams::iterator StreamToMerge=StreamsToMerge.begin(); StreamToMerge!=StreamsToMerge.end(); ++StreamToMerge)
            if (StreamToMerge->Id==Stream->Id)
            {
                size_t StreamKind=Stream->StreamKind; //Keeping StreamKind from PKL
                *Stream=*StreamToMerge;
                Stream->StreamKind=StreamKind; //Keeping StreamKind from PKL
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
// Packing List (SMPTE ST 429-8:2007, IMF Packing List of SMPTE ST 2067-2:2020 Annex J and 7.3)
// Not checked: Hash values, Id of the assets against their content, signature
#if MEDIAINFO_CONFORMANCE
void File_DcpPkl::Conformance_PackingList(const XMLElement* PackingList, const char* NameSpace, bool AssetMap_IsParsed)
{
    conformance_errors Errors, Warnings_Imf;
    const auto IsImf=!strcmp(NameSpace, "http://www.smpte-ra.org/schemas/2067-2/2016/PKL");
    std::set<string> Ids;
    for (const auto* Item=PackingList->FirstChildElement(); Item; Item=Item->NextSiblingElement())
    {
        const char* Item_NameSpace;
        const char* Item_Name=LocalName(Item, Item_NameSpace);
        if (!Item_NameSpace || strcmp(Item_NameSpace, NameSpace))
            continue;
        if (!strcmp(Item_Name, "GroupId"))
            Warnings_Imf.Add("GroupId is present");
        if (strcmp(Item_Name, "AssetList"))
            continue;
        for (const auto* Asset=Item->FirstChildElement(); Asset; Asset=Asset->NextSiblingElement())
        {
            if (!MatchQName(Asset, "Asset", NameSpace))
                continue;
            string Id, Size;
            bool Hash_IsPresent=false, Size_IsPresent=false, Type_IsPresent=false;
            for (const auto* Asset_Item=Asset->FirstChildElement(); Asset_Item; Asset_Item=Asset_Item->NextSiblingElement())
            {
                if (MatchQName(Asset_Item, "Id", NameSpace))
                    Id=Dcp_Lower(Dcp_Text(Asset_Item));
                else if (MatchQName(Asset_Item, "Hash", NameSpace))
                    Hash_IsPresent=true;
                else if (MatchQName(Asset_Item, "Size", NameSpace))
                {
                    Size_IsPresent=true;
                    Size=Dcp_Text(Asset_Item);
                }
                else if (MatchQName(Asset_Item, "Type", NameSpace))
                    Type_IsPresent=true;
            }
            if (Id.empty())
                Errors.Add("Id of an Asset is missing");
            else if (!Ids.insert(Id).second)
                Errors.Add("Several Assets have the same Id (actual "+Id+")");
            if (!Hash_IsPresent)
                Errors.Add("Hash of an Asset is missing (Id "+Id+")");
            if (!Type_IsPresent)
                Errors.Add("Type of an Asset is missing (Id "+Id+")");
            const auto Size_IsValid=!Size.empty() && Size.size()<=19 && Size.find_first_not_of("0123456789")==string::npos;
            if (!Size_IsPresent)
                Errors.Add("Size of an Asset is missing (Id "+Id+")");
            else if (!Size_IsValid)
                Errors.Add("Size of an Asset is not a valid integer (actual \""+Size+"\")");

            //Location of the asset
            const File_DcpPkl::stream* Stream_Found=nullptr;
            for (const auto& Stream : Streams)
                if (Dcp_Lower(Stream.Id)==Id)
                    Stream_Found=&Stream;
            if (!AssetMap_IsParsed || !Stream_Found)
                continue;
            if (Stream_Found->ChunkList.empty())
            {
                Warnings_Imf.Add("Asset is not in the Asset Map (actual Id "+Id+")");
                continue;
            }
            const auto FullPath=Dcp_FullPath(File_Name, Stream_Found->ChunkList[0].Path);
            if (Size_IsValid && File::Exists(FullPath) && Size!=std::to_string(File::Size_Get(FullPath)))
                Errors.Add("Size of an Asset is not the size of its file (actual "+Size+", expected "+std::to_string(File::Size_Get(FullPath))+")");
        }
    }

    //Filling
    const auto File_Offset_Save=File_Offset;
    File_Offset=(int64u)-1-(Buffer_Offset+Element_Offset); //Errors are about the whole file, no offset is provided
    for (const auto& Error : Errors)
        Fill_Conformance("PKL", Error.Value, {}, Conformance_Error, Stream_General, 0);
    if (IsImf)
        for (const auto& Warning : Warnings_Imf)
            Fill_Conformance("PKL", "SMPTE_ST_2067-2: "+Warning.Value, {}, Conformance_Warning, Stream_General, 0);
    File_Offset=File_Offset_Save;
}
#endif //MEDIAINFO_CONFORMANCE

} //NameSpace

#endif //MEDIAINFO_DCP_YES

