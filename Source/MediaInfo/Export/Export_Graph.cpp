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
#if defined(MEDIAINFO_GRAPH_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Export/Export_Graph.h"
#include "MediaInfo/File__Analyse_Automatic.h"
#include "MediaInfo/MediaInfo_Config.h"
#include "MediaInfo/OutputHelpers.h"
#include <ctime>
#include <cmath>

#ifdef MEDIAINFO_GRAPHVIZ_YES
    #ifdef MEDIAINFO_GRAPHVIZ_DLL_RUNTIME
        #include "MediaInfo/Export/Export_Graph_gvc_Include.h"
    #else
        #include <gvc.h>
    #endif //MEDIAINFO_GRAPHVIZ_DLL_RUNTIME
#endif //MEDIAINFO_GRAPHVIZ_YES

using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
Ztring NewLine(size_t Level)
{
    return __T('\n')+Ztring(4*Level, __T(' '));
}

//---------------------------------------------------------------------------
Ztring Element2Html(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, Ztring Element, Ztring Format, Ztring FG, Ztring BG, Ztring HFG, Ztring HBG, bool HasNestedObjects=false)
{
    Ztring ToReturn;
    ToReturn+=__T("<table border='0' cellborder='0' cellspacing='0'>");
    ToReturn+=__T("<tr>");
    ToReturn+=__T("<td colspan='2' bgcolor='")+HBG+__T("'>");
    ToReturn+=__T("<font color='")+HFG+__T("'>");
    ToReturn+=__T("<b>")+XML_Encode(MI.Get(StreamKind, StreamPos, Element, Info_Name_Text))+__T("</b>");
    Ztring Sub=XML_Encode(MI.Get(StreamKind, StreamPos, Element, Info_Text));
    if (!HasNestedObjects || Sub!=__T("Yes"))
    {
        ToReturn+=__T("<br/>");
        if (Sub.size() && Sub[Sub.size()-1]==__T(')'))
        {
            if (Sub.FindAndReplace(__T("("), __T("<br/>"), 0))
                Sub.erase(Sub.size()-1);
        }
        Sub.FindAndReplace(__T(" / "), __T("<br/>"), 0, Ztring_Recursive);
        ToReturn+=Sub;
    }
    ToReturn+=__T("</font>");
    ToReturn+=__T("</td>");
    ToReturn+=__T("</tr>");

    for (size_t Pos=0; Pos<MI.Count_Get(StreamKind, StreamPos); Pos++)
    {
        Ztring Name=MI.Get(StreamKind, StreamPos, Pos, Info_Name);
        if (Name.find(Element+__T(" "))==0 && Name.SubString(Element+__T(" "), __T("")).find(__T(" "))==string::npos)
        {
            Ztring Text=XML_Encode(MI.Get(StreamKind, StreamPos, Pos, Info_Text));
            Text.FindAndReplace(__T(" / "), __T("<br align='left'/>"), 0, Ztring_Recursive);

            if (Text.empty() ||
                Name.find(Element+__T(" LinkedTo_"))==0 ||
                Name.find(Element+__T(" Title"))==0 ||
                (Format==__T("ED2") && Name.find(Element+__T(" Target"))==0) ||
                (MI.Get(StreamKind, StreamPos, Pos, Info_Options)[InfoOption_ShowInInform]!=__T('Y') && !MediaInfoLib::Config.Complete_Get()))
                continue;

            ToReturn+=__T("<tr>");
            ToReturn+=__T("<td align='text' bgcolor='")+BG+__T("' port='")+XML_Encode(Name.SubString(Element+__T(" "), __T("")))+__T("'>");
            ToReturn+=__T("<font color='")+FG+__T("'>")+XML_Encode(MI.Get(StreamKind, StreamPos, Pos, Info_Name_Text))+=__T("</font><br align='left'/>");
            ToReturn+=__T("</td>");
            ToReturn+=__T("<td align='text' bgcolor='")+BG+__T("'>");
            ToReturn+=__T("<font color='")+FG+__T("'>")+Text+=__T("</font><br align='left'/>");
            ToReturn+=__T("</td>");
            ToReturn+=__T("</tr>");
        }
    }
    ToReturn+=__T("</table>");

    return ToReturn;
}

//---------------------------------------------------------------------------

#ifdef MEDIAINFO_GRAPHVIZ_YES
Ztring Dot2Svg(const Ztring& Dot)
{
    Ztring ToReturn;
    GVC_t* Context=NULL;
    graph_t* Graph=NULL;
    char* Buffer=NULL;
    unsigned int Size;

    if (!Export_Graph::Load())
        return ToReturn;

    if (Dot.empty())
       return ToReturn;

    Context=gvContext();
    if (!Context)
        return ToReturn;

    Graph=agmemread(Dot.To_UTF8().c_str()); 
    if (!Graph)
    {
        gvFinalize(Context);
        gvFreeContext(Context);
        return ToReturn;
    }

    if (gvLayout(Context, Graph, "dot")!=0)
    {
        agclose(Graph);
        gvFinalize(Context);
        gvFreeContext(Context);
        return ToReturn;
    }

    gvRenderData(Context, Graph, "svg", &Buffer, &Size);

    if (Buffer && Size)
        ToReturn=Ztring().From_UTF8(Buffer);

    gvFreeRenderData(Buffer);
    gvFreeLayout(Context, Graph);
    agclose(Graph);
    gvFinalize(Context);
    gvFreeContext(Context);

    return ToReturn;
}
#endif //MEDIAINFO_GRAPHVIZ_YES

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Export_Graph::Export_Graph ()
{
}

//---------------------------------------------------------------------------
Export_Graph::~Export_Graph ()
{
}


//***************************************************************************
// Dynamic load stuff
//***************************************************************************

bool Export_Graph::Load()
{
    //TODO: detect if graphviz library is usable
    #if defined(MEDIAINFO_GRAPHVIZ_DLL_RUNTIME)
        if (!gvc_Module)
        {
            DLOPEN(gvc_Module, GVCDLL_NAME)
            if (!gvc_Module)
                return false;

            bool Error=false;
            ASSIGN(gvc_Module, gvContext)
            ASSIGN(gvc_Module, gvFreeContext)
            ASSIGN(gvc_Module, gvLayout)
            ASSIGN(gvc_Module, gvFreeLayout)
            ASSIGN(gvc_Module, gvRenderData)
            ASSIGN(gvc_Module, gvFreeRenderData)
            ASSIGN(gvc_Module, gvFinalize)
            if (Error)
            {
                DLCLOSE(gvc_Module);
                return false;
            }
        }

        if (!cgraph_Module)
        {
            DLOPEN(cgraph_Module, CGRAPHDLL_NAME)
            if (!cgraph_Module)
            {
                DLCLOSE(gvc_Module);
                return false; 
            }

            bool Error=false;
            ASSIGN(cgraph_Module, agmemread)
            ASSIGN(cgraph_Module, agclose)
            if (Error)
            {
                DLCLOSE(gvc_Module);
                DLCLOSE(cgraph_Module);
                return false;
            }
        }
        return true;
    #elif defined(MEDIAINFO_GRAPHVIZ_YES)
        return true;
    #else
        return false;
    #endif //defined(MEDIAINFO_GRAPHVIZ_DLL_RUNTIME)
}

//***************************************************************************
// Generators
//***************************************************************************

#define OBJECT_START(NAME, COUNTER, FOREGROUND_COLOR, BACKGROUND_COLOR, TITLE_FOREGROUND_COLOR, TITLE_BACKGROUND_COLOR) \
    {\
        Temp+=NewLine(Level++)+__T("rank=same {"); \
        size_t NAME##s=MI.Get(Stream_Audio, StreamPos, __T(COUNTER), Info_Text).To_int64u(); \
        if (NAME##s) Empty=false; \
        for (size_t NAME##Pos=NAME##s; NAME##Pos; NAME##Pos--) \
        {\
            Ztring Object=__T(#NAME)+Ztring().Ztring::ToZtring(NAME##Pos-1); \
            Temp+=NewLine(Level++)+Stream+__T("_")+Object+__T(" ["); \
            Temp+=NewLine(Level)+__T("shape=plaintext"); \
            Temp+=NewLine(Level)+__T("label=<"); \
            Temp+=Element2Html(MI, Stream_Audio, StreamPos, Object, Format, __T(FOREGROUND_COLOR), __T(BACKGROUND_COLOR), __T(TITLE_FOREGROUND_COLOR), __T(TITLE_BACKGROUND_COLOR), true); \
            Temp+=__T(">");\
            Temp+=NewLine(--Level)+__T("]");

#define OBJECT_LINK_TO(NAME, COLOR) \
            { \
                ZtringList Linked##NAME##s; \
                Linked##NAME##s.Separator_Set(0, __T(" + ")); \
                Linked##NAME##s.Write(MI.Get(Stream_Audio, StreamPos, Object+__T(" LinkedTo_" #NAME "_Pos"), Info_Text)); \
                for (size_t NAME##Pos=0; NAME##Pos<Linked##NAME##s.size(); NAME##Pos++) \
                    Relations.push_back(relation(Stream+__T("_")+Object, Stream+__T("_" #NAME)+Linked##NAME##s[NAME##Pos], __T("[color=\"" COLOR "\"]"))); \
            }

#define OBJECT_LINK_TO2(NAME, NAME2, COLOR) \
            { \
                ZtringList Linked##NAME##s; \
                Linked##NAME##s.Separator_Set(0, __T(" + ")); \
                Linked##NAME##s.Write(MI.Get(Stream_Audio, StreamPos, Object+__T(" LinkedTo_" #NAME2 "_Pos"), Info_Text)); \
                for (size_t NAME##Pos=0; NAME##Pos<Linked##NAME##s.size(); NAME##Pos++) \
                    Relations.push_back(relation(Stream+__T("_")+Object, Stream+__T("_" #NAME)+Linked##NAME##s[NAME##Pos], __T("[color=\"" COLOR "\", style=\"dashed\"]"))); \
            }

#define OBJECT_END() \
        } \
        Temp+=NewLine(--Level)+__T("}"); \
    }

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AC4_YES)
Ztring Export_Graph::Ac4_Graph(MediaInfo_Internal &MI, size_t StreamPos, size_t Level)
{
    Ztring ToReturn;
    if (MI.Get(Stream_Audio, StreamPos, Audio_Format)!=__T("AC-4"))
        return ToReturn;

    Ztring Format=__T("AC4");

    vector<relation> Relations;
    Ztring Stream=MI.Get(Stream_Audio, StreamPos, __T("StreamKind"))+MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos"));
    int8u Version=MI.Get(Stream_Audio, StreamPos, __T("Format_Version"), Info_Text).SubString(__T("Version "), __T("")).To_int8u();

    bool Empty=true;
    Ztring Temp;

    Temp+=NewLine(Level++)+__T("subgraph cluster_")+Ztring().From_Number(StreamPos)+__T(" {");
    Temp+=NewLine(Level)+__T("label=<<b>")+MI.Get(Stream_Audio, StreamPos, __T("StreamKind/String"));
    if (!MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos")).empty())
        Temp+=__T(" ")+MediaInfoLib::Config.Language_Get(__T("  Config_Text_NumberTag"))+MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos"));
    Temp+=__T(" (")+MI.Get(Stream_Audio, StreamPos, Audio_Format)+__T(")</b>>");

    OBJECT_START(Presentation, "NumberOfPresentations", "#000000", "#c5cae9", "#ffffff", "#303f9f")
    if (Version<2)
        OBJECT_LINK_TO(Substream,  "#c5cae9")
    else
        OBJECT_LINK_TO(Group,  "#c5cae9")
    OBJECT_END()

    if (Version>=2)
    {
        OBJECT_START(Group, "NumberOfGroups", "#000000", "#bbdefb", "#ffffff", "#1976d2")
        OBJECT_LINK_TO(Substream,  "#bbdefb")
        OBJECT_END()
    }

    OBJECT_START(Substream, "NumberOfSubstreams", "#000000", "#b3e5fc", "#ffffff", "#0288d1")
    OBJECT_END()

    for (size_t Pos=0; Pos<Relations.size(); Pos++)
        Temp+=NewLine(Level)+Relations[Pos].Src+__T("--")+Relations[Pos].Dst+__T(" ")+Relations[Pos].Opts;

    Temp+=NewLine(--Level)+__T("}");

    if (!Empty)
        ToReturn+=Temp;

    return ToReturn;
}
#endif //defined(MEDIAINFO_AC4_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_DOLBYE_YES)
Ztring Export_Graph::Ed2_Graph(MediaInfo_Internal &MI, size_t StreamPos, size_t Level)
{
    Ztring ToReturn;
    if (MI.Get(Stream_Audio, StreamPos, Audio_Format)!=__T("Dolby ED2"))
        return ToReturn;

    Ztring Format=__T("ED2");

    vector<relation> Relations;
    Ztring Stream=MI.Get(Stream_Audio, StreamPos, __T("StreamKind"))+MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos"));

    bool Empty=true;
    Ztring Temp;

    Temp+=NewLine(Level++)+__T("subgraph cluster_")+Ztring().From_Number(StreamPos)+__T(" {");
    Temp+=NewLine(Level)+__T("label=<<b>")+MI.Get(Stream_Audio, StreamPos, __T("StreamKind/String"));
    if (!MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos")).empty())
        Temp+=__T(" ")+MediaInfoLib::Config.Language_Get(__T("  Config_Text_NumberTag"))+MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos"));
    Temp+=__T(" (")+MI.Get(Stream_Audio, StreamPos, Audio_Format)+__T(")</b>>");

    OBJECT_START(Presentation, "NumberOfPresentations", "#000000", "#c5cae9", "#ffffff", "#303f9f")
    OBJECT_END()

    // Targets
    {
        Temp+=NewLine(Level++)+__T("rank=same {");
        size_t Presentations=MI.Get(Stream_Audio, StreamPos,__T("NumberOfPresentations"), Info_Text).To_int64u();
        for (size_t PresentationPos=Presentations; PresentationPos; PresentationPos--)
        {
            Ztring Object=__T("Presentation")+Ztring().Ztring::ToZtring(PresentationPos-1);
            size_t Targets=MI.Get(Stream_Audio, StreamPos, Object+__T(" NumberOfTargets"), Info_Text).To_int64u();
            for (size_t TargetPos=Targets; TargetPos; TargetPos--)
            {
                Ztring SubObject=__T("Target")+Ztring().Ztring::ToZtring(TargetPos-1);
                Temp+=NewLine(Level++)+Stream+__T("_")+Object+__T("_")+SubObject+__T(" [");
                Temp+=NewLine(Level)+__T("shape=plaintext");
                Temp+=NewLine(Level)+__T("label=<");
                Temp+=Element2Html(MI, Stream_Audio, StreamPos, Object + __T(" ") + SubObject, __T("#000000"), Format, __T("#bbdefb"), __T("#ffffff"), __T("#1976d2"));
                Temp+=__T(">");
                Temp+=NewLine(--Level)+__T("]");

                Relations.push_back(relation(Stream+__T("_")+Object, Stream+__T("_")+Object+__T("_")+SubObject, __T("[color=\"#c5cae9\"]")));

                ZtringList LinkedBeds;
                LinkedBeds.Separator_Set(0, __T(" + "));
                LinkedBeds.Write(MI.Get(Stream_Audio, StreamPos, Object+__T(" ")+SubObject+__T(" LinkedTo_Bed_Pos"), Info_Text));
                for (size_t BedPos=0; BedPos<LinkedBeds.size(); BedPos++)
                {
                    if (LinkedBeds[BedPos].find(__T("-"), 0)==string::npos)
                        Relations.push_back(relation(Stream+__T("_")+Object+__T("_")+SubObject, Stream+__T("_Bed")+LinkedBeds[BedPos], __T("[color=\"#bbdefb\"]")));
                    else
                    {
                        Ztring Bed=LinkedBeds[BedPos].SubString(__T(""), __T("-"));
                        Ztring Alt=LinkedBeds[BedPos].SubString(__T("-"), __T(""));
                        Relations.push_back(relation(Stream+__T("_")+Object+__T("_")+SubObject, Stream+__T("_Bed")+Bed+__T(":Alt")+Alt, __T("[color=\"#bbdefb\"]")));
                    }
                }

                ZtringList LinkedObjects;
                LinkedObjects.Separator_Set(0, __T(" + "));
                LinkedObjects.Write(MI.Get(Stream_Audio, StreamPos, Object+__T(" ")+SubObject+__T(" LinkedTo_Object_Pos"), Info_Text));
                for (size_t ObjectPos=0; ObjectPos<LinkedObjects.size(); ObjectPos++)
                {
                    if (LinkedObjects[ObjectPos].find(__T("-"), 0)==string::npos)
                        Relations.push_back(relation(Stream+__T("_")+Object+__T("_")+SubObject, Stream+__T("_Object")+LinkedObjects[ObjectPos], __T("[color=\"#bbdefb\"]")));
                    else
                    {
                        Ztring Obj=LinkedObjects[ObjectPos].SubString(__T(""), __T("-"));
                        Ztring Alt=LinkedObjects[ObjectPos].SubString(__T("-"), __T(""));
                        Relations.push_back(relation(Stream+__T("_")+Object+__T("_")+SubObject, Stream+__T("_Object")+Obj+__T(":Alt")+Alt, __T("[color=\"#bbdefb\"]")));
                    }
                }
            }
        }
        Temp+=NewLine(--Level)+__T("}");
    }



    OBJECT_START(Bed, "NumberOfBeds", "#000000", "#b3e5fc", "#ffffff", "#0288d1")
    OBJECT_END()

    // Beds Alts
    {
        Temp+=NewLine(Level++)+__T("rank=same {");
        size_t Beds=MI.Get(Stream_Audio, StreamPos,__T("NumberOfBeds"), Info_Text).To_int64u();
        for (size_t BedPos=Beds; BedPos; BedPos--)
        {
            Ztring Object=__T("Bed")+Ztring().Ztring::ToZtring(BedPos-1);
            size_t Alts=MI.Get(Stream_Audio, StreamPos, Object+__T(" NumberOfAlts"), Info_Text).To_int64u();
            for (size_t AltPos=Alts; AltPos; AltPos--)
            {
                Ztring SubObject=__T("Alt")+Ztring().Ztring::ToZtring(AltPos-1);
                Temp+=NewLine(Level++)+Stream+__T("_")+Object+__T("_")+SubObject+__T(" [");
                Temp+=NewLine(Level)+__T("shape=plaintext");
                Temp+=NewLine(Level)+__T("label=<");
                Temp+=Element2Html(MI, Stream_Audio, StreamPos, Object + __T(" ") + SubObject, Format, __T("#000000"), __T("#b2ebf2"), __T("#ffffff"), __T("#00796b"));
                Temp+=__T(">");
                Temp+=NewLine(--Level)+__T("]");

                Relations.push_back(relation(Stream+__T("_")+Object, Stream+__T("_")+Object+__T("_")+SubObject, __T("[color=\"#b3e5fc\"]")));
            }
        }
        Temp+=NewLine(--Level)+__T("}");
    }

    OBJECT_START(Object, "NumberOfObjects", "#000000", "#b3e5fc", "#ffffff", "#0288d1")
    OBJECT_END()

    // Objects Alts
    {
        Temp+=NewLine(Level++)+__T("rank=same {");
        size_t Objects=MI.Get(Stream_Audio, StreamPos,__T("NumberOfObjects"), Info_Text).To_int64u();
        for (size_t ObjectPos=Objects; ObjectPos; ObjectPos--)
        {
            Ztring Object=__T("Object")+Ztring().Ztring::ToZtring(ObjectPos-1);
            size_t Alts=MI.Get(Stream_Audio, StreamPos, Object+__T(" NumberOfAlts"), Info_Text).To_int64u();
            for (size_t AltPos=Alts; AltPos; AltPos--)
            {
                Ztring SubObject=__T("Alt")+Ztring().Ztring::ToZtring(AltPos-1);
                Temp+=NewLine(Level++)+Stream+__T("_")+Object+__T("_")+SubObject+__T(" [");
                Temp+=NewLine(Level)+__T("shape=plaintext");
                Temp+=NewLine(Level)+__T("label=<");
                Temp+=Element2Html(MI, Stream_Audio, StreamPos, Object + __T(" ") + SubObject, Format, __T("#000000"), __T("#b2dfdb"), __T("#ffffff"), __T("#0288d1"));
                Temp+=__T(">");
                Temp+=NewLine(--Level)+__T("]");

                Relations.push_back(relation(Stream+__T("_")+Object, Stream+__T("_")+Object+__T("_")+SubObject, __T("[color=\"#b2ebf2\"]")));
            }
        }
        Temp+=NewLine(--Level)+__T("}");
    }

    for (size_t Pos=0; Pos<Relations.size(); Pos++)
        Temp+=NewLine(Level)+Relations[Pos].Src+__T("--")+Relations[Pos].Dst+__T(" ")+Relations[Pos].Opts;

    Temp+=NewLine(--Level)+__T("}");

    if (!Empty)
        ToReturn+=Temp;

    return ToReturn;
}
#endif //defined(MEDIAINFO_DOLBYE_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_ADM_YES)
Ztring Export_Graph::Adm_Graph(MediaInfo_Internal &MI, size_t StreamPos, size_t Level)
{
    Ztring ToReturn;
    if (MI.Get(Stream_Audio, StreamPos, __T("Metadata_Format")).find(__T("ADM, "), 0)!=0
     && MI.Get(Stream_Audio, StreamPos, __T("Metadata_Format")).find(__T("S-ADM, "), 0)!=0
     && MI.Get(Stream_Audio, StreamPos, __T("Format"))!=__T("MGA")
     && MI.Get(Stream_Audio, StreamPos, __T("Format"))!=__T("IAB"))
        return ToReturn;

    Ztring Format=__T("ADM");

    vector<relation> Relations;
    Ztring Stream=MI.Get(Stream_Audio, StreamPos, __T("StreamKind"))+MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos"));

    bool Empty=true;
    Ztring Temp;

    Temp+=NewLine(Level++)+__T("subgraph cluster_")+Ztring().From_Number(StreamPos)+__T(" {");
    Temp+=NewLine(Level)+__T("label=<<b>")+MI.Get(Stream_Audio, StreamPos, __T("StreamKind/String"));
    if (!MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos")).empty())
        Temp+=__T(" ")+MediaInfoLib::Config.Language_Get(__T("  Config_Text_NumberTag"))+MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos"));
    Temp+=__T(" (")+MI.Get(Stream_Audio, StreamPos, Audio_Format)+__T(")</b>>");

    OBJECT_START(Programme, "NumberOfProgrammes", "#000000", "#c5cae9", "#ffffff", "#303f9f")
    OBJECT_LINK_TO(Content, "#c5cae9")
    OBJECT_LINK_TO(PackFormat, "#c5cae9")
    OBJECT_END()

    OBJECT_START(Content, "NumberOfContents", "#000000", "#bbdefb", "#ffffff", "#1976d2")
    OBJECT_LINK_TO(Object, "#bbdefb")
    OBJECT_END()

    OBJECT_START(Object, "NumberOfObjects", "#000000", "#b3e5fc", "#ffffff", "#0288d1")
    OBJECT_LINK_TO(Object, "black")
    OBJECT_LINK_TO2(Object, ComplementaryObject, "black")
    OBJECT_LINK_TO(PackFormat, "#b3e5fc")
    if (MediaInfoLib::Config.Graph_Adm_ShowTrackUIDs_Get())
        OBJECT_LINK_TO(TrackUID, "#b3e5fc")
    OBJECT_END()

    if (MediaInfoLib::Config.Graph_Adm_ShowTrackUIDs_Get())
    {
        OBJECT_START(TrackUID, "NumberOfTrackUIDs", "#000000", "#b2ebf2", "#ffffff", "#0097a7")
        OBJECT_LINK_TO(PackFormat, "#b2ebf2")
        OBJECT_LINK_TO(TrackFormat, "#b2ebf2")
        OBJECT_END()

        OBJECT_START(TrackFormat, "NumberOfTrackFormats", "#000000", "#b2dfdb", "#ffffff", "#00796b")
        OBJECT_LINK_TO(StreamFormat,  "#b2dfdb")
        OBJECT_END()

        OBJECT_START(StreamFormat, "NumberOfStreamFormats", "#000000", "#c8e6c9", "#ffffff", "#388e3c")
        OBJECT_LINK_TO(ChannelFormat, "#c8e6c9")
        OBJECT_LINK_TO(PackFormat, "#c8e6c9")
        OBJECT_LINK_TO(TrackFormat, "#c8e6c9")
        OBJECT_END()
    }

    OBJECT_START(PackFormat, "NumberOfPackFormats", "#000000", "#dcedc8", "#ffffff", "#689f38")
    if (MediaInfoLib::Config.Graph_Adm_ShowChannelFormats_Get())
        OBJECT_LINK_TO(ChannelFormat, "#dcedc8")
    OBJECT_END()

    if (MediaInfoLib::Config.Graph_Adm_ShowChannelFormats_Get())
    {
        OBJECT_START(ChannelFormat, "NumberOfChannelFormats", "#000000", "#f0f4c3", "#ffffff", "#afb42b")
        OBJECT_END()
    }

    for (size_t Pos=0; Pos<Relations.size(); Pos++)
        Temp+=NewLine(Level)+Relations[Pos].Src+__T("--")+Relations[Pos].Dst+__T(" ")+Relations[Pos].Opts;

    Temp+=NewLine(--Level)+__T("}");

    if (!Empty)
        ToReturn+=Temp;

    return ToReturn;
}
#endif //defined(MEDIAINFO_ADM_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_MPEGH3DA_YES)
Ztring Export_Graph::Mpegh3da_Graph(MediaInfo_Internal &MI, size_t StreamPos, size_t Level)
{
    Ztring ToReturn;
    if (MI.Get(Stream_Audio, StreamPos, Audio_Format)!=__T("MPEG-H 3D Audio"))
        return ToReturn;

    Ztring Format=__T("MPEG3DA");

    vector<relation> Relations;
    Ztring Stream=MI.Get(Stream_Audio, StreamPos, __T("StreamKind"))+MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos"));

    bool Empty=true;
    Ztring Temp;

    Temp+=NewLine(Level++)+__T("subgraph cluster_")+Ztring().From_Number(StreamPos)+__T(" {");
    Temp+=NewLine(Level)+__T("label=<<b>")+MI.Get(Stream_Audio, StreamPos, __T("StreamKind/String"));
    if (!MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos")).empty())
        Temp+=__T(" ")+MediaInfoLib::Config.Language_Get(__T("  Config_Text_NumberTag"))+MI.Get(Stream_Audio, StreamPos, __T("StreamKindPos"));
    Temp+=__T(" (")+MI.Get(Stream_Audio, StreamPos, Audio_Format)+__T(")</b>>");

    OBJECT_START(SwitchGroup, "SwitchGroupCount", "#000000", "#c5cae9", "#ffffff", "#303f9f")
    OBJECT_LINK_TO(Group, "#c5cae9")
    OBJECT_END()

    OBJECT_START(Group, "GroupCount", "#000000", "#bbdefb", "#ffffff", "#1976d2")
    OBJECT_LINK_TO(SignalGroup, "#bbdefb")
    OBJECT_END()

    OBJECT_START(SignalGroup, "SignalGroupCount", "#000000", "#b3e5fc", "#ffffff", "#0288d1")
    OBJECT_END()

    for (size_t Pos=0; Pos<Relations.size(); Pos++)
        Temp+=NewLine(Level)+Relations[Pos].Src+__T("--")+Relations[Pos].Dst+__T(" ")+Relations[Pos].Opts;

    Temp+=NewLine(--Level)+__T("}");

    if (!Empty)
        ToReturn+=Temp;

    return ToReturn;
}
#endif //defined(MEDIAINFO_MPEGH3DA_YES)

//***************************************************************************
// Input
//***************************************************************************

//---------------------------------------------------------------------------
Ztring Export_Graph::Transform(MediaInfo_Internal &MI, Export_Graph::graph Graph, Export_Graph::format Format)
{
    Ztring ToReturn;
    size_t Level=1;

    bool ExpandSub_Old=MI.Config.File_ExpandSubs_Get();
    MI.Config.File_ExpandSubs_Set(false);

    Ztring FileName=XML_Encode(MI.Get(Stream_General, 0, General_FileNameExtension));
    if (FileName.empty())
        FileName=__T("&nbsp;");

    ToReturn+=__T("graph {");
    ToReturn+=NewLine(Level)+__T("color=\"#1565c0\"");
    ToReturn+=NewLine(Level)+__T("fontcolor=\"#1565c0\"");
    ToReturn+=NewLine(Level)+__T("labelloc=t");
    ToReturn+=NewLine(Level)+__T("label=<<b>")+FileName+__T("</b>>");

    Ztring Temp;
    for (size_t StreamPos=0; StreamPos<(size_t)MI.Count_Get(Stream_Audio); StreamPos++)
    {
        #if defined(MEDIAINFO_AC4_YES)
            if (Graph==Graph_All || Graph==Graph_Ac4)
                Temp+=Ac4_Graph(MI, StreamPos, Level);
        #endif //defined(MEDIAINFO_AC4_YES)
        #if defined(MEDIAINFO_DOLBYE_YES)
            if (Graph==Graph_All || Graph==Graph_Ed2)
                Temp+=Ed2_Graph(MI, StreamPos, Level);
        #endif //defined(MEDIAINFO_DOLBYE_YES)
        #if defined(MEDIAINFO_ADM_YES)
            if (Graph==Graph_All || Graph==Graph_Adm)
                Temp+=Adm_Graph(MI, StreamPos, Level);
        #endif //defined(MEDIAINFO_ADM_YES)
        #if defined(MEDIAINFO_MPEGH3DA_YES)
            if (Graph==Graph_All || Graph==Graph_Mpegh3da)
                Temp+=Mpegh3da_Graph(MI, StreamPos, Level);
        #endif //defined(MEDIAINFO_MPEGH3DA_YES)
    }
    if (!Temp.empty())
        ToReturn+=Temp;
    else
        ToReturn+=NewLine(Level)+__T("message [shape=plaintext, fontcolor=\"#1565c0\", label=<<b>Graphs are currently available for AC-4, MPEG-H, Dolby ED2, IAB and ADM formats.</b>>]");
    ToReturn+=__T("\n}");

#ifdef MEDIAINFO_GRAPHVIZ_YES
    if (Format==Format_Svg)
        ToReturn=Dot2Svg(ToReturn);
#endif //MEDIAINFO_GRAPHVIZ_YES

    MI.Config.File_ExpandSubs_Set(ExpandSub_Old);

    return ToReturn;
}

//***************************************************************************
//
//***************************************************************************

} //NameSpace

#endif
