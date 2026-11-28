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
#include "MediaInfo/Multiple/File_DcpCpl.h"
#include "MediaInfo/Multiple/File_DcpAm.h"
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#include "MediaInfo/XmlUtils.h"
#if defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/FileName.h"
#include "ThirdParty/tfsxml/tfsxml_tinyxml2.h"
#include <list>
#include <map>
#include <set>
#include <vector>
using namespace tinyxml2;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern string Jpeg2000_Rsiz(int16u Rsiz);
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DcpCpl::File_DcpCpl()
:File__Analyze()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_DcpCpl;
        StreamIDs_Width[0]=sizeof(size_t)*2;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent_Accept_Specific=true;
    #endif //MEDIAINFO_DEMUX

    //PKL
    PKL_Pos = (size_t)-1;
}

//---------------------------------------------------------------------------

static bool IsSmpteSt2067_2(const char* Ns)
{
    return Ns &&
           (!strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-2/2013") ||
            !strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-2/2016") ||
            !strcmp(Ns, "http://www.smpte-ra.org/ns/2067-2/2020") ||
            !strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-2/XXXX")); //Some muxers use XXXX instead of year

}

//---------------------------------------------------------------------------

static bool IsSmpteSt2067_3(const char* Ns)
{
    return Ns &&
           (!strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-3/2013") ||
            !strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-3/2016") ||
            !strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-3/XXXX")); //Some muxers use XXXX instead of year

}

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
//Profiles of the Track Files of an IMF Sequence, declared by the Composition Playlist
static string Imf_DeclaredProfiles(const char* Name, const char* NameSpace, const string& ApplicationIds)
{
    if (!NameSpace)
        return {};
    if (IsSmpteSt2067_2(NameSpace))
    {
        if (!strcmp(Name, "MainImageSequence"))
        {
            string Profiles("SMPTE_ST_2067-2");
            if (ApplicationIds.find("/2067-21/")!=string::npos) //Application #2E
                Profiles+=",SMPTE_ST_2067-21";
            if (ApplicationIds.find("/2067-50/")!=string::npos) //Application #5
                Profiles+=",SMPTE_ST_2067-50";
            return Profiles;
        }
        static const char* const TrackFileSequences[]=
        {
            "MainAudioSequence",
            "SubtitlesSequence",
            "HearingImpairedCaptionsSequence",
            "VisuallyImpairedTextSequence",
            "CommentarySequence",
            "KaraokeSequence",
            "ForcedNarrativeSequence",
            "AncillaryDataSequence",
        };
        for (const auto TrackFileSequence : TrackFileSequences)
            if (!strcmp(Name, TrackFileSequence))
                return "SMPTE_ST_2067-2";
        return {};
    }
    struct plugin
    {
        const char* Name;
        const char* NameSpace;
        const char* Profiles;
    };
    static const plugin Plugins[]=
    {
        { "IABSequence",            "http://www.smpte-ra.org/ns/2067-201/", "SMPTE_ST_2067-2,SMPTE_ST_2067-201" },
        { "ISXDSequence",           "http://www.smpte-ra.org/ns/2067-202/", "SMPTE_ST_2067-2,SMPTE_ST_2067-202" },
        { "MGASADMSignalSequence",  "http://www.smpte-ra.org/ns/2067-203/", "SMPTE_ST_2067-2,SMPTE_ST_2067-203" },
        { "ADMAudioSequence",       "http://www.smpte-ra.org/ns/2067-204/", "SMPTE_ST_2067-204" }, //Includes the overall Track File constraints
    };
    for (const auto& Plugin : Plugins)
        if (!strcmp(Name, Plugin.Name) && !strncmp(NameSpace, Plugin.NameSpace, strlen(Plugin.NameSpace)))
            return Plugin.Profiles;
    return {};
}
#endif //MEDIAINFO_CONFORMANCE

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DcpCpl::FileHeader_Begin()
{
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    XMLElement* Root=document.FirstChildElement();
    const char *NameSpace;
    if (!Root || strcmp(LocalName(Root, NameSpace), "CompositionPlaylist") || !NameSpace)
    {
        Reject("DcpCpl");
        return false;
    }

    bool IsDcp=false, IsImf=false;
    if (!strcmp(NameSpace, "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#") ||
        !strcmp(NameSpace, "http://www.smpte-ra.org/schemas/429-7/2006/CPL"))
    {
        IsDcp=true;
    }
    else if (IsSmpteSt2067_3(NameSpace))
    {
        IsImf=true;
    }
    else
    {
        Reject("DcpCpl");
        return false;
    }

    Accept("DcpCpl");
    Fill(Stream_General, 0, General_Format, IsDcp?"DCP CPL":"IMF CPL");
    #if MEDIAINFO_CONFORMANCE
        if (IsImf)
            Conformance_Composition(Root, NameSpace);
    #endif //MEDIAINFO_CONFORMANCE
    #if defined(MEDIAINFO_REFERENCES_YES)
    Config->File_ID_OnlyRoot_Set(false);

    ReferenceFiles_Accept(this, Config);

    vector<size_t> AssetCountPerReel;
    #if MEDIAINFO_CONFORMANCE
        string ApplicationIds; //Application IDs of SMPTE ST 2067-2, for the profiles of the Track Files
        if (IsImf)
            for (XMLElement* Item=Root->FirstChildElement(); Item; Item=Item->NextSiblingElement())
                if (MatchQName(Item, "ExtensionProperties", NameSpace))
                    for (XMLElement* Property=Item->FirstChildElement(); Property; Property=Property->NextSiblingElement())
                    {
                        const char* Property_NameSpace;
                        const char* Property_Name=LocalName(Property, Property_NameSpace);
                        if (IsSmpteSt2067_2(Property_NameSpace) && !strcmp(Property_Name, "ApplicationIdentification") && Property->GetText())
                        {
                            ApplicationIds+=Property->GetText();
                            ApplicationIds+=' ';
                        }
                    }
    #endif //MEDIAINFO_CONFORMANCE
                        
    //Parsing main elements
    for (XMLElement* CompositionPlaylist_Item=Root->FirstChildElement(); CompositionPlaylist_Item; CompositionPlaylist_Item=CompositionPlaylist_Item->NextSiblingElement())
    {
        const char* CompositionPlaylist_Item_Value=CompositionPlaylist_Item->Value();
        if (!CompositionPlaylist_Item_Value)
            continue;

        //CompositionTimecode
        if (IsImf && MatchQName(CompositionPlaylist_Item, "CompositionTimecode", NameSpace))
        {
            sequence* Sequence=new sequence;
            Sequence->StreamKind=Stream_Other;
            Sequence->Infos["Type"]=__T("Time code");
            Sequence->Infos["Format"]=__T("CPL TC");
            Sequence->Infos["TimeCode_Stripped"]=__T("Yes");
            bool IsDropFrame=false;

            for (XMLElement* CompositionTimecode_Item=CompositionPlaylist_Item->FirstChildElement(); CompositionTimecode_Item; CompositionTimecode_Item=CompositionTimecode_Item->NextSiblingElement())
            {
                const char* Text=CompositionTimecode_Item->GetText();
                if (!Text)
                    continue;
                const char *CtItemNs, *CtItemName = LocalName(CompositionTimecode_Item, CtItemNs);
                if (!CtItemNs || strcmp(CtItemNs, NameSpace))
                    continue; // item has wrong namespace

                //TimecodeDropFrame
                if (!strcmp(CtItemName, "TimecodeDropFrame"))
                {
                    if (!strcmp(Text, "true") || !strcmp(Text, "1"))
                        IsDropFrame=true;
                }

                //TimecodeRate
                if (!strcmp(CtItemName, "TimecodeRate"))
                    Sequence->Infos["FrameRate"].From_UTF8(Text);

                //TimecodeStartAddress
                if (!strcmp(CtItemName, "TimecodeStartAddress"))
                    Sequence->Infos["TimeCode_FirstFrame"].From_UTF8(Text);
            }

            //Adaptation
            if (IsDropFrame)
            {
                std::map<string, Ztring>::iterator Info=Sequence->Infos.find("TimeCode_FirstFrame");
                if (Info!=Sequence->Infos.end() && Info->second.size()>=11 && Info->second[8]!=__T(';'))
                    Info->second[8]=__T(';');
            }

            Sequence->StreamID=ReferenceFiles->Sequences_Size()+1;
            ReferenceFiles->AddSequence(Sequence);
            AssetCountPerReel.push_back(1); //Counts per reel are aligned with the list of sequences

            Stream_Prepare(Stream_Other);
            Fill(Stream_Other, StreamPos_Last, Other_ID, Sequence->StreamID);
            for (std::map<string, Ztring>::iterator Info=Sequence->Infos.begin(); Info!=Sequence->Infos.end(); ++Info)
                Fill(Stream_Other, StreamPos_Last, Info->first.c_str(), Info->second);
        }

        #if MEDIAINFO_ADVANCED
            //EssenceDescriptorList
            if (IsImf && !strcmp(CompositionPlaylist_Item_Value, "EssenceDescriptorList"))
            {
                for (XMLElement* EssenceDescriptorList_Item=CompositionPlaylist_Item->FirstChildElement(); EssenceDescriptorList_Item; EssenceDescriptorList_Item=EssenceDescriptorList_Item->NextSiblingElement())
                {
                    const char* EssenceDescriptorList_Item_Value=EssenceDescriptorList_Item->Value();
                    if (!EssenceDescriptorList_Item_Value)
                        continue;

                    //TimecodeDropFrame
                    if (!strcmp(EssenceDescriptorList_Item_Value, "EssenceDescriptor"))
                    {
                        string Id;
                        descriptor* Descriptor=new descriptor;

                        for (XMLElement* EssenceDescriptor_Item=EssenceDescriptorList_Item->FirstChildElement(); EssenceDescriptor_Item; EssenceDescriptor_Item=EssenceDescriptor_Item->NextSiblingElement())
                        {
                            const char* EssenceDescriptor_Item_Value=EssenceDescriptor_Item->Value();
                            const char* EssenceDescriptor_Item_Text=EssenceDescriptor_Item->GetText();
                            if (!EssenceDescriptor_Item_Value)
                                continue;
                            const char* EssenceDescriptor_Item_LocalName=LocalName(EssenceDescriptor_Item);

                            //Id
                            if (EssenceDescriptor_Item_Text && !strcmp(EssenceDescriptor_Item_Value, "Id"))
                                Id=EssenceDescriptor_Item_Text;

                            //CDCIDescriptor
                            if (!strcmp(EssenceDescriptor_Item_LocalName, "RGBADescriptor") || !strcmp(EssenceDescriptor_Item_LocalName, "CDCIDescriptor"))
                            {
                                for (XMLElement* Descriptor_Item=EssenceDescriptor_Item->FirstChildElement(); Descriptor_Item; Descriptor_Item=Descriptor_Item->NextSiblingElement())
                                {
                                    const char* Descriptor_Item_Value=Descriptor_Item->Value();
                                    if (!Descriptor_Item_Value)
                                        continue;

                                    //SubDescriptors
                                    if (!strcmp(LocalName(Descriptor_Item), "SubDescriptors"))
                                    {
                                        for (XMLElement* SubDescriptors_Item=Descriptor_Item->FirstChildElement(); SubDescriptors_Item; SubDescriptors_Item=SubDescriptors_Item->NextSiblingElement())
                                        {
                                            const char* SubDescriptors_Item_Value=SubDescriptors_Item->Value();
                                            if (!SubDescriptors_Item_Value)
                                                continue;

                                            descriptor* SubDescriptor=new descriptor;

                                            //JPEG2000PictureSubDescriptor
                                            if (!strcmp(LocalName(SubDescriptors_Item), "JPEG2000PictureSubDescriptor"))
                                            {
                                                for (XMLElement* JPEG2000PictureSubDescriptor_Item=SubDescriptors_Item->FirstChildElement(); JPEG2000PictureSubDescriptor_Item; JPEG2000PictureSubDescriptor_Item=JPEG2000PictureSubDescriptor_Item->NextSiblingElement())
                                                {
                                                    const char* JPEG2000PictureSubDescriptor_Item_Value=JPEG2000PictureSubDescriptor_Item->Value();
                                                    const char* JPEG2000PictureSubDescriptor_Item_Text=JPEG2000PictureSubDescriptor_Item->GetText();
                                                    if (!JPEG2000PictureSubDescriptor_Item_Value || !JPEG2000PictureSubDescriptor_Item_Text)
                                                        continue;

                                                    //Xsiz
                                                    if (!strcmp(LocalName(JPEG2000PictureSubDescriptor_Item), "Rsiz"))
                                                    {
                                                        SubDescriptor->Jpeg2000_Rsiz=(int16u)atoi(JPEG2000PictureSubDescriptor_Item_Text);
                                                    }
                                                }
                                            }

                                            Descriptor->SubDescriptors.push_back(SubDescriptor);
                                        }
                                    }
                                }
                            }
                        }

                        if (!Id.empty())
                        {
                            auto& Existing=EssenceDescriptorList[Id];
                            delete Existing;
                            Existing=Descriptor;
                        }
                        else
                            delete Descriptor; // Can not be associated
                    }
                }
            }
        #endif //MEDIAINFO_ADVANCED

        //ReelList / SegmentList
        if (MatchQName(CompositionPlaylist_Item, IsDcp?"ReelList":"SegmentList", NameSpace))
        {
            for (XMLElement* ReelList_Item=CompositionPlaylist_Item->FirstChildElement(); ReelList_Item; ReelList_Item=ReelList_Item->NextSiblingElement())
            {
                //Reel
                if (MatchQName(ReelList_Item, IsDcp?"Reel":"Segment", NameSpace))
                {
                    size_t AssetCount=0;

                    for (XMLElement* Reel_Item=ReelList_Item->FirstChildElement(); Reel_Item; Reel_Item=Reel_Item->NextSiblingElement())
                    {
                        //AssetList
                        if (MatchQName(Reel_Item, IsDcp?"AssetList":"SequenceList", NameSpace))
                        {
                            for (XMLElement* AssetList_Item=Reel_Item->FirstChildElement(); AssetList_Item; AssetList_Item=AssetList_Item->NextSiblingElement())
                            {
                                const char *AlItemNs, *AlItemName = LocalName(AssetList_Item, AlItemNs);
                                if (!AlItemNs)
                                    continue;
                                //File
                                //if ((IsDcp && (!strcmp(AssetList_Item->Value(), "MainPicture") || !strcmp(AssetList_Item->Value(), "MainSound")))
                                // || (IsImf && (!strcmp(AssetList_Item->Value(), "cc:MainImageSequence") || !strcmp(AssetList_Item->Value(), "cc:MainImage"))))
                                if (strcmp(AlItemName, "MarkerSequence")) //Ignoring MarkerSequence for the moment. TODO: check what to do with MarkerSequence
                                {
                                    sequence* Sequence=new sequence;
                                    Ztring Asset_Id;
                                    #if MEDIAINFO_CONFORMANCE
                                        if (IsImf)
                                        {
                                            //Track Files checked with the profiles of the Sequence, their compliance is mandatory
                                            const auto Profiles=Imf_DeclaredProfiles(AlItemName, AlItemNs, ApplicationIds);
                                            if (!Profiles.empty())
                                                Sequence->Config["File_Mxf_DeclaredProfiles"].From_UTF8(Profiles);
                                            Sequence->ConformanceFromGeneral=true;
                                        }
                                    #endif //MEDIAINFO_CONFORMANCE

                                    if (IsDcp && !strcmp(NameSpace, AlItemNs))
                                    {
                                        if (!strcmp(AlItemName, "MainPicture"))
                                            Sequence->StreamKind=Stream_Video;
                                        else if (!strcmp(AlItemName, "MainSound"))
                                            Sequence->StreamKind=Stream_Audio;
                                        else if (!strcmp(AlItemName, "MainSubtitle"))
                                            Sequence->StreamKind=Stream_Text;
                                    }
                                    else if (IsImf && IsSmpteSt2067_2(AlItemNs))
                                    {
                                        if (!strcmp(AlItemName, "MainImageSequence"))
                                            Sequence->StreamKind=Stream_Video;
                                        else if (!strcmp(AlItemName, "MainAudioSequence"))
                                            Sequence->StreamKind=Stream_Audio;
                                    }

                                    for (XMLElement* File_Item=AssetList_Item->FirstChildElement(); File_Item; File_Item=File_Item->NextSiblingElement())
                                    {
                                        //Id
                                        if (MatchQName(File_Item, "Id", NameSpace) && Asset_Id.empty())
                                            Asset_Id.From_UTF8(File_Item->GetText());

                                        //ResourceList
                                        if (IsImf && MatchQName(File_Item, "ResourceList", NameSpace))
                                        {
                                            for (XMLElement* ResourceList_Item=File_Item->FirstChildElement(); ResourceList_Item; ResourceList_Item=ResourceList_Item->NextSiblingElement())
                                            {
                                                //Resource
                                                if (MatchQName(ResourceList_Item, "Resource", NameSpace))
                                                {
                                                    Ztring Resource_Id;

                                                    resource* Resource=new resource;
                                                    for (XMLElement* Resource_Item=ResourceList_Item->FirstChildElement(); Resource_Item; Resource_Item=Resource_Item->NextSiblingElement())
                                                    {
                                                        const char* ResText=Resource_Item->GetText();
                                                        if (!ResText)
                                                            continue;
                                                        const char *ResItemNs, *ResItemName = LocalName(Resource_Item, ResItemNs);
                                                        if (!ResItemNs || strcmp(ResItemNs, NameSpace))
                                                            continue; // item has wrong namespace

                                                        //EditRate
                                                        if (!strcmp(ResItemName, "EditRate"))
                                                        {
                                                            Resource->EditRate=atof(ResText);
                                                            const char* EditRate2=strchr(ResText, ' ');
                                                            if (EditRate2!=NULL)
                                                            {
                                                                float64 EditRate2f=atof(EditRate2);
                                                                if (EditRate2f)
                                                                    Resource->EditRate/=EditRate2f;
                                                            }
                                                        }

                                                        //EntryPoint
                                                        if (!strcmp(ResItemName, "EntryPoint"))
                                                        {
                                                            Resource->IgnoreEditsBefore=atoi(ResText);
                                                            if (Resource->IgnoreEditsAfter!=(int64u)-1)
                                                                Resource->IgnoreEditsAfter+=Resource->IgnoreEditsBefore;
                                                        }

                                                        //Id
                                                        if (!strcmp(ResItemName, "Id") && Resource_Id.empty())
                                                            Resource_Id.From_UTF8(ResText);

                                                        //SourceDuration
                                                        if (!strcmp(ResItemName, "SourceDuration"))
                                                            Resource->IgnoreEditsAfter=Resource->IgnoreEditsBefore+atoi(ResText);

                                                        #if MEDIAINFO_ADVANCED
                                                            //SourceEncoding
                                                            if (!strcmp(ResItemName, "SourceEncoding"))
                                                                Resource->SourceEncodings.push_back(ResText);
                                                        #endif //MEDIAINFO_ADVANCED

                                                        //TrackFileId
                                                        if (!strcmp(ResItemName, "TrackFileId"))
                                                            Resource->FileNames.push_back(Ztring().From_UTF8(ResText));
                                                    }

                                                    if (Resource->FileNames.empty())
                                                        Resource->FileNames.push_back(Resource_Id);
                                                    Sequence->AddResource(Resource);
                                                }
                                            }
                                        }
                                    }

                                    if (Sequence->Resources.empty())
                                    {
                                        resource* Resource=new resource;
                                        Resource->FileNames.push_back(Asset_Id);
                                        Sequence->AddResource(Resource);
                                    }
                                    Sequence->StreamID=ReferenceFiles->Sequences_Size()+1;
                                    ReferenceFiles->AddSequence(Sequence);

                                    AssetCount++;
                                }
                            }
                        }
                    }

                    AssetCountPerReel.push_back(AssetCount);
                }
            }
        }
    }

    ReferenceFiles->DetectSameReels(AssetCountPerReel);

    //Getting files names
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
        }
    }

    ReferenceFiles->FilesForStorage=true;

    #if MEDIAINFO_ADVANCED
        for (std::map<string, descriptor*>::iterator EssenceDescriptor = EssenceDescriptorList.begin(); EssenceDescriptor != EssenceDescriptorList.end(); ++EssenceDescriptor)
            for (std::vector<descriptor*>::iterator SubDescriptor = EssenceDescriptor->second->SubDescriptors.begin(); SubDescriptor != EssenceDescriptor->second->SubDescriptors.end(); ++SubDescriptor)
                if ((*SubDescriptor)->Jpeg2000_Rsiz!=(int16u)-1)
                    ReferenceFiles->UpdateMetaDataFromSourceEncoding(EssenceDescriptor->first, "Format_Profile", Jpeg2000_Rsiz((*SubDescriptor)->Jpeg2000_Rsiz));
    #endif //MEDIAINFO_ADVANCED

    #endif //MEDIAINFO_REFERENCES_YES

    //All should be OK...
    Element_Offset=File_Size;
    return true;
}

//***************************************************************************
// Conformance
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
namespace
{

struct cpl_rational
{
    int64u Num=0;
    int64u Den=0;
    bool IsValid() const { return Num && Den; }
    bool operator==(const cpl_rational& Value) const { return Num*Value.Den==Value.Num*Den; } //Values are less than 2^31
    string ToString() const { return to_string(Num)+'/'+to_string(Den); }
};

struct cpl_resource
{
    string Id;
    string SourceEncoding;
    string TrackFileId;
    cpl_rational EditRate;
    int64u IntrinsicDuration=0;
    int64u EntryPoint=0;
    int64u SourceDuration=0;
    int64u RepeatCount=1;
    bool EditRate_IsPresent=false;
    bool IntrinsicDuration_IsPresent=false;
    bool SourceDuration_IsPresent=false;
    bool Hash_IsPresent=false;
    bool HashAlgorithm_IsPresent=false;
    bool IsTrackFile=false;
    bool IsValid=true; //Integer values are valid
};

struct cpl_sequence
{
    string Name;
    string TrackId;
    bool IsCoreConstraints=false; //Sequence defined by SMPTE ST 2067-2
    vector<cpl_resource> Resources;
};

struct cpl_segment
{
    string Id;
    vector<cpl_sequence> Sequences;
};

//---------------------------------------------------------------------------
string Cpl_Text(const XMLElement* Element)
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
string Cpl_Uuid(const XMLElement* Element)
{
    auto Value=Cpl_Text(Element);
    for (auto& Character : Value)
        if (Character>='A' && Character<='Z')
            Character+='a'-'A';
    return Value;
}

//---------------------------------------------------------------------------
bool Cpl_Integer(const string& Text, int64u& Value)
{
    if (Text.empty() || Text.size()>18 || Text.find_first_not_of("0123456789")!=string::npos)
        return false;
    Value=strtoull(Text.c_str(), nullptr, 10);
    return true;
}

//---------------------------------------------------------------------------
cpl_rational Cpl_Rational(const string& Text)
{
    cpl_rational Value;
    const auto Space=Text.find_first_of(" \t\r\n");
    if (Space==string::npos)
        return Value;
    int64u Num, Den;
    const auto Den_Begin=Text.find_first_not_of(" \t\r\n", Space);
    if (Den_Begin==string::npos || !Cpl_Integer(Text.substr(0, Space), Num) || !Cpl_Integer(Text.substr(Den_Begin), Den) || Num>0x7FFFFFFF || Den>0x7FFFFFFF)
        return Value;
    Value.Num=Num;
    Value.Den=Den;
    return Value;
}

//---------------------------------------------------------------------------
int64u Cpl_Gcd(int64u a, int64u b)
{
    while (b)
    {
        const auto Temp=a%b;
        a=b;
        b=Temp;
    }
    return a;
}

//---------------------------------------------------------------------------
//Adds Duration Resource Edit Units to Sum (fraction of Composition Edit Units), returns false if values are too big
bool Cpl_Duration_Add(int64u Duration, const cpl_rational& Resource_EditRate, const cpl_rational& Composition_EditRate, int64u& Sum_Num, int64u& Sum_Den)
{
    auto Num=Composition_EditRate.Num*Resource_EditRate.Den;
    auto Den=Composition_EditRate.Den*Resource_EditRate.Num;
    auto Gcd=Cpl_Gcd(Num, Den);
    Num/=Gcd;
    Den/=Gcd;
    Gcd=Cpl_Gcd(Duration, Den);
    Duration/=Gcd;
    Den/=Gcd;
    if (Duration>>32 || Num>>31 || Den>>31 || Sum_Num>>32)
        return false;
    Num*=Duration; //Duration*Num/Den Composition Edit Units
    if (Num>>32)
        return false;
    const auto Sum_Gcd=Cpl_Gcd(Sum_Den, Den);
    const auto Den_Common=Sum_Den/Sum_Gcd*Den;
    if (Den_Common>>31)
        return false;
    Sum_Num=Sum_Num*(Den_Common/Sum_Den)+Num*(Den_Common/Den);
    Sum_Den=Den_Common;
    Gcd=Cpl_Gcd(Sum_Num, Sum_Den);
    Sum_Num/=Gcd;
    Sum_Den/=Gcd;
    return true;
}

} //Namespace

//---------------------------------------------------------------------------
// Composition Playlist (SMPTE ST 2067-3:2020, SMPTE ST 2067-3:2016 and SMPTE ST 2067-3:2013), Composition constraints of SMPTE ST 2067-2:2020 6 if the Composition Playlist uses the Sequences of SMPTE ST 2067-2
// Not checked: Sequences or Resources with the same Id but not identical, values of the Track Files (Edit Rate, duration, homogeneous audio, identification), signature
void File_DcpCpl::Conformance_Composition(const XMLElement* Root, const char* NameSpace)
{
    conformance_errors Errors, Errors_Core;
    cpl_rational EditRate;
    string EditRate_Text;
    bool EditRate_IsPresent=false;
    bool Signer_IsPresent=false;
    bool Signature_IsPresent=false;
    size_t ApplicationIdentification_Count=0;
    map<string, size_t> EssenceDescriptors; //Id, count of references
    set<string> EssenceDescriptors_Duplicates;
    vector<cpl_segment> Segments;
    for (const auto* Item=Root->FirstChildElement(); Item; Item=Item->NextSiblingElement())
    {
        const char* Item_NameSpace;
        const char* Item_Name=LocalName(Item, Item_NameSpace);
        if (!Item_NameSpace)
            continue;
        if (!strcmp(Item_Name, "Signature") && !strcmp(Item_NameSpace, "http://www.w3.org/2000/09/xmldsig#"))
            Signature_IsPresent=true;
        if (strcmp(Item_NameSpace, NameSpace))
            continue;
        if (!strcmp(Item_Name, "EditRate"))
        {
            EditRate_IsPresent=true;
            EditRate_Text=Cpl_Text(Item);
            EditRate=Cpl_Rational(EditRate_Text);
        }
        else if (!strcmp(Item_Name, "Signer"))
            Signer_IsPresent=true;
        else if (!strcmp(Item_Name, "EssenceDescriptorList"))
        {
            for (const auto* Descriptor=Item->FirstChildElement(); Descriptor; Descriptor=Descriptor->NextSiblingElement())
                if (MatchQName(Descriptor, "EssenceDescriptor", NameSpace))
                    for (const auto* Descriptor_Item=Descriptor->FirstChildElement(); Descriptor_Item; Descriptor_Item=Descriptor_Item->NextSiblingElement())
                        if (MatchQName(Descriptor_Item, "Id", NameSpace))
                        {
                            const auto Id=Cpl_Uuid(Descriptor_Item);
                            if (!EssenceDescriptors.emplace(Id, 0).second)
                                EssenceDescriptors_Duplicates.insert(Id);
                            break;
                        }
        }
        else if (!strcmp(Item_Name, "ExtensionProperties"))
        {
            for (const auto* Property=Item->FirstChildElement(); Property; Property=Property->NextSiblingElement())
            {
                const char* Property_NameSpace;
                const char* Property_Name=LocalName(Property, Property_NameSpace);
                if (IsSmpteSt2067_2(Property_NameSpace) && !strcmp(Property_Name, "ApplicationIdentification"))
                    ApplicationIdentification_Count++;
            }
        }
        else if (!strcmp(Item_Name, "SegmentList"))
        {
            for (const auto* Segment_Element=Item->FirstChildElement(); Segment_Element; Segment_Element=Segment_Element->NextSiblingElement())
            {
                if (!MatchQName(Segment_Element, "Segment", NameSpace))
                    continue;
                Segments.emplace_back();
                auto& Segment=Segments.back();
                for (const auto* Segment_Item=Segment_Element->FirstChildElement(); Segment_Item; Segment_Item=Segment_Item->NextSiblingElement())
                {
                    if (MatchQName(Segment_Item, "Id", NameSpace))
                        Segment.Id=Cpl_Uuid(Segment_Item);
                    if (!MatchQName(Segment_Item, "SequenceList", NameSpace))
                        continue;
                    for (const auto* Sequence_Element=Segment_Item->FirstChildElement(); Sequence_Element; Sequence_Element=Sequence_Element->NextSiblingElement())
                    {
                        const char* Sequence_NameSpace;
                        const char* Sequence_Name=LocalName(Sequence_Element, Sequence_NameSpace);
                        Segment.Sequences.emplace_back();
                        auto& Sequence=Segment.Sequences.back();
                        Sequence.Name=Sequence_Name;
                        Sequence.IsCoreConstraints=IsSmpteSt2067_2(Sequence_NameSpace);
                        for (const auto* Sequence_Item=Sequence_Element->FirstChildElement(); Sequence_Item; Sequence_Item=Sequence_Item->NextSiblingElement())
                        {
                            if (MatchQName(Sequence_Item, "TrackId", NameSpace))
                                Sequence.TrackId=Cpl_Uuid(Sequence_Item);
                            if (!MatchQName(Sequence_Item, "ResourceList", NameSpace))
                                continue;
                            for (const auto* Resource_Element=Sequence_Item->FirstChildElement(); Resource_Element; Resource_Element=Resource_Element->NextSiblingElement())
                            {
                                if (!MatchQName(Resource_Element, "Resource", NameSpace))
                                    continue;
                                Sequence.Resources.emplace_back();
                                auto& Resource=Sequence.Resources.back();
                                const char* Type=Resource_Element->Attribute("xsi:type");
                                if (Type && strstr(Type, "TrackFileResourceType"))
                                    Resource.IsTrackFile=true;
                                for (const auto* Resource_Item=Resource_Element->FirstChildElement(); Resource_Item; Resource_Item=Resource_Item->NextSiblingElement())
                                {
                                    const char* Resource_Item_NameSpace;
                                    const char* Resource_Item_Name=LocalName(Resource_Item, Resource_Item_NameSpace);
                                    if (!Resource_Item_NameSpace || strcmp(Resource_Item_NameSpace, NameSpace))
                                        continue;
                                    if (!strcmp(Resource_Item_Name, "Id"))
                                        Resource.Id=Cpl_Uuid(Resource_Item);
                                    else if (!strcmp(Resource_Item_Name, "EditRate"))
                                    {
                                        Resource.EditRate_IsPresent=true;
                                        Resource.EditRate=Cpl_Rational(Cpl_Text(Resource_Item));
                                        if (!Resource.EditRate.IsValid())
                                            Errors.Add("EditRate of a Resource is not a valid rational (actual \""+Cpl_Text(Resource_Item)+"\")");
                                    }
                                    else if (!strcmp(Resource_Item_Name, "IntrinsicDuration"))
                                    {
                                        Resource.IntrinsicDuration_IsPresent=true;
                                        if (!Cpl_Integer(Cpl_Text(Resource_Item), Resource.IntrinsicDuration))
                                            Resource.IsValid=false;
                                    }
                                    else if (!strcmp(Resource_Item_Name, "EntryPoint"))
                                    {
                                        if (!Cpl_Integer(Cpl_Text(Resource_Item), Resource.EntryPoint))
                                            Resource.IsValid=false;
                                    }
                                    else if (!strcmp(Resource_Item_Name, "SourceDuration"))
                                    {
                                        Resource.SourceDuration_IsPresent=true;
                                        if (!Cpl_Integer(Cpl_Text(Resource_Item), Resource.SourceDuration))
                                            Resource.IsValid=false;
                                    }
                                    else if (!strcmp(Resource_Item_Name, "RepeatCount"))
                                    {
                                        if (!Cpl_Integer(Cpl_Text(Resource_Item), Resource.RepeatCount) || !Resource.RepeatCount)
                                            Resource.IsValid=false;
                                    }
                                    else if (!strcmp(Resource_Item_Name, "SourceEncoding"))
                                    {
                                        Resource.SourceEncoding=Cpl_Uuid(Resource_Item);
                                        Resource.IsTrackFile=true;
                                    }
                                    else if (!strcmp(Resource_Item_Name, "TrackFileId"))
                                    {
                                        Resource.TrackFileId=Cpl_Uuid(Resource_Item);
                                        Resource.IsTrackFile=true;
                                    }
                                    else if (!strcmp(Resource_Item_Name, "Hash"))
                                        Resource.Hash_IsPresent=true;
                                    else if (!strcmp(Resource_Item_Name, "HashAlgorithm"))
                                        Resource.HashAlgorithm_IsPresent=true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //Composition (6.1)
    if (!EditRate_IsPresent)
        Errors.Add("EditRate is missing");
    else if (!EditRate.IsValid())
        Errors.Add("EditRate is not a valid rational (actual \""+EditRate_Text+"\")");
    for (const auto& Id : EssenceDescriptors_Duplicates)
        Errors.Add("Several EssenceDescriptor elements have the same Id (actual "+Id+")");
    if (Signer_IsPresent && !Signature_IsPresent)
        Errors.Add("Signature is missing (Signer is present)");
    if (!Signer_IsPresent && Signature_IsPresent)
        Errors.Add("Signer is missing (Signature is present)");

    //Segments (6.9, 7.2), Sequences (6.10, 7.3), Resources (6.11, 6.12, 7.4)
    bool IsCoreConstraints=ApplicationIdentification_Count!=0;
    set<string> Segment_Ids;
    map<string, size_t> TrackIds; //TrackId, count of Segments
    set<string> MainImage_TrackIds;
    for (const auto& Segment : Segments)
    {
        if (!Segment.Id.empty() && !Segment_Ids.insert(Segment.Id).second)
            Errors.Add("Several Segments have the same Id (actual "+Segment.Id+")");
        set<string> Segment_TrackIds;
        bool Segment_Duration_IsKnown=EditRate.IsValid();
        vector<string> Segment_Durations;
        int64u Segment_Duration=(int64u)-1;
        for (const auto& Sequence : Segment.Sequences)
        {
            if (Sequence.IsCoreConstraints)
                IsCoreConstraints=true;
            if (Sequence.TrackId.empty())
                Errors.Add("TrackId of a Sequence is missing");
            else if (!Segment_TrackIds.insert(Sequence.TrackId).second)
                Errors.Add("Several Sequences of a Segment have the same TrackId (actual "+Sequence.TrackId+")");
            const auto IsMainImage=Sequence.IsCoreConstraints && Sequence.Name=="MainImageSequence";
            if (IsMainImage && !Sequence.TrackId.empty())
                MainImage_TrackIds.insert(Sequence.TrackId);
            int64u Duration_Num=0, Duration_Den=1;
            auto Duration_IsKnown=EditRate.IsValid();
            for (const auto& Resource : Sequence.Resources)
            {
                if (!Resource.IsValid)
                    Errors.Add("Integer value of a Resource is not valid (IntrinsicDuration, EntryPoint, SourceDuration or RepeatCount)");
                if (!Resource.IntrinsicDuration_IsPresent)
                    Errors.Add("IntrinsicDuration of a Resource is missing");
                else if (Resource.IsValid && Resource.EntryPoint>Resource.IntrinsicDuration)
                    Errors.Add("EntryPoint is greater than IntrinsicDuration (actual "+to_string(Resource.EntryPoint)+", IntrinsicDuration "+to_string(Resource.IntrinsicDuration)+")");
                else if (Resource.IsValid && Resource.SourceDuration_IsPresent && Resource.SourceDuration>Resource.IntrinsicDuration-Resource.EntryPoint)
                    Errors.Add("SourceDuration is greater than IntrinsicDuration minus EntryPoint (actual "+to_string(Resource.SourceDuration)+", expected at most "+to_string(Resource.IntrinsicDuration-Resource.EntryPoint)+")");
                if (Resource.IsTrackFile)
                {
                    if (Resource.SourceEncoding.empty())
                        Errors.Add("SourceEncoding of a Track File Resource is missing");
                    else
                    {
                        const auto EssenceDescriptor=EssenceDescriptors.find(Resource.SourceEncoding);
                        if (EssenceDescriptor==EssenceDescriptors.end())
                            Errors.Add("SourceEncoding does not reference an EssenceDescriptor (actual "+Resource.SourceEncoding+")");
                        else
                            EssenceDescriptor->second++;
                    }
                    if (Resource.TrackFileId.empty())
                        Errors.Add("TrackFileId of a Track File Resource is missing");
                    if (Resource.Hash_IsPresent && !Resource.HashAlgorithm_IsPresent)
                        Errors.Add("HashAlgorithm is missing (Hash is present)");
                    if (!Resource.Hash_IsPresent && Resource.HashAlgorithm_IsPresent)
                        Errors.Add("HashAlgorithm is present without Hash");
                }
                const auto& Resource_EditRate=Resource.EditRate_IsPresent?Resource.EditRate:EditRate;
                if (IsMainImage && EditRate.IsValid() && Resource_EditRate.IsValid() && !(Resource_EditRate==EditRate))
                    Errors_Core.Add("EditRate is not the Edit Rate of the Main Image Virtual Track (actual "+EditRate.ToString()+", expected "+Resource_EditRate.ToString()+")");
                if (!Duration_IsKnown || !Resource.IsValid || !Resource.IntrinsicDuration_IsPresent || !Resource_EditRate.IsValid() || Resource.EntryPoint>Resource.IntrinsicDuration || Resource.RepeatCount>>16)
                {
                    Duration_IsKnown=false;
                    continue;
                }
                const auto SourceDuration=Resource.SourceDuration_IsPresent?Resource.SourceDuration:(Resource.IntrinsicDuration-Resource.EntryPoint);
                if (!Cpl_Duration_Add(SourceDuration*Resource.RepeatCount, Resource_EditRate, EditRate, Duration_Num, Duration_Den))
                    Duration_IsKnown=false;
            }
            if (!Duration_IsKnown)
            {
                Segment_Duration_IsKnown=false;
                continue;
            }
            if (Duration_Den!=1)
            {
                Errors.Add("Duration of a Sequence is not an integer count of Composition Edit Units (actual "+to_string(Duration_Num)+'/'+to_string(Duration_Den)+")");
                Segment_Duration_IsKnown=false;
                continue;
            }
            Segment_Durations.push_back(to_string(Duration_Num));
            if (Segment_Duration==(int64u)-1)
                Segment_Duration=Duration_Num;
            else if (Segment_Duration!=Duration_Num)
                Segment_Duration=(int64u)-2;
        }
        if (Segment_Duration==(int64u)-2)
        {
            string List;
            for (const auto& Duration : Segment_Durations)
                List+=(List.empty()?"":" / ")+Duration;
            Errors.Add("Sequences of a Segment do not have the same duration (actual "+List+")");
        }
        else if (Segment_Duration_IsKnown && !Segment_Duration)
            Errors_Core.Add("Duration of a Segment is less than the duration of one frame of the Main Image Virtual Track");
        for (const auto& TrackId : Segment_TrackIds)
            TrackIds[TrackId]++;
    }
    for (const auto& TrackId : TrackIds)
        if (TrackId.second!=Segments.size())
            Errors.Add("Virtual Track is not present in all Segments (actual TrackId "+TrackId.first+", "+to_string(TrackId.second)+" of "+to_string(Segments.size())+" Segments)");
    for (const auto& EssenceDescriptor : EssenceDescriptors)
        if (!EssenceDescriptor.second)
            Errors.Add("EssenceDescriptor is not referenced by a Track File Resource (actual Id "+EssenceDescriptor.first+")");

    //Composition constraints of IMF (SMPTE ST 2067-2 6)
    if (IsCoreConstraints)
    {
        if (ApplicationIdentification_Count!=1)
            Errors_Core.Add("ExtensionProperties does not contain one ApplicationIdentification element (actual "+to_string(ApplicationIdentification_Count)+")");
        if (MainImage_TrackIds.size()!=1)
            Errors_Core.Add("Composition does not contain one Main Image Virtual Track (actual "+to_string(MainImage_TrackIds.size())+")");
    }
    else
        Errors_Core.clear();

    //Filling
    const auto File_Offset_Save=File_Offset;
    File_Offset=(int64u)-1-(Buffer_Offset+Element_Offset); //Errors are about the whole file, no offset is provided
    for (const auto& Error : Errors)
        Fill_Conformance("CPL", Error.Value, {}, Conformance_Error, Stream_General, 0);
    for (const auto& Error : Errors_Core)
        Fill_Conformance("CPL", "SMPTE_ST_2067-2: "+Error.Value, {}, Conformance_Error, Stream_General, 0);
    File_Offset=File_Offset_Save;
}
#endif //MEDIAINFO_CONFORMANCE

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_REFERENCES_YES)
void File_DcpCpl::MergeFromAm (File_DcpPkl::streams &StreamsToMerge)
{
    for (File_DcpPkl::streams::iterator StreamToMerge=StreamsToMerge.begin(); StreamToMerge!=StreamsToMerge.end(); ++StreamToMerge)
        if (!StreamToMerge->ChunkList.empty()) // Note: ChunkLists with more than 1 file are not yet supported)
            ReferenceFiles->UpdateFileName(Ztring().From_UTF8(StreamToMerge->Id), Ztring().From_UTF8(StreamToMerge->ChunkList[0].Path));
}
#endif //MEDIAINFO_REFERENCES_YES

} //NameSpace

#endif //MEDIAINFO_DCP_YES
