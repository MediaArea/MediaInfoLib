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
#if defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_DolbyVisionMetadata.h"
#include "tinyxml2.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <set>
using namespace tinyxml2;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
static void TID_Collect(XMLElement* Element, std::set<string>& TIDs)
{
    for (XMLElement* Item = Element->FirstChildElement(); Item; Item = Item->NextSiblingElement())
    {
        if (!strcmp(Item->Name(), "TID"))
        {
            if (const char* Text = Item->GetText())
                TIDs.insert(Text);
        }
        else
            TID_Collect(Item, TIDs);
    }
}

//---------------------------------------------------------------------------
// Values separated by spaces (version 4.x or more) or commas (version 2.x)
static std::vector<float64> Values_Get(const char* Text)
{
    std::vector<float64> Values;
    for (;;)
    {
        while (*Text == ' ' || *Text == ',')
            Text++;
        if (!*Text)
            break;
        const char* End = Text;
        while (*End && *End != ' ' && *End != ',')
            End++;
        Values.push_back(Ztring().From_UTF8(Text, End - Text).To_float64());
        Text = End;
    }
    return Values;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DolbyVisionMetadata::File_DolbyVisionMetadata()
{
    //Configuration
    StreamSource=IsContainerExtra;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DolbyVisionMetadata::FileHeader_Begin()
{
    //Configuration
    if (!IsSub)
        StreamSource=IsContainer; // Standalone file, not extra metadata of a container

    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    string Version;
    float32 AspectRatio=0;
    XMLElement* DolbyVisionGlobalData = document.FirstChildElement();
    if (!DolbyVisionGlobalData)
    {
        Reject("DolbyVisionMetadata");
        return false;
    }

    XMLElement* MasterElements[2] = {};
    
    const char* Format = nullptr;
    if (!strcmp(DolbyVisionGlobalData->Name(), "gsp:DolbyVisionGlobalDataGSP"))
    {
        for (DolbyVisionGlobalData = DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData; DolbyVisionGlobalData = DolbyVisionGlobalData->NextSiblingElement())
        {
            if (!strcmp(DolbyVisionGlobalData->Name(), "gsp:Version"))
            {
                if (const char* Text = DolbyVisionGlobalData->GetText())
                    Version = Text;
            }
            if (!strcmp(DolbyVisionGlobalData->Name(), "gsp:Track"))
            {
                MasterElements[1]=DolbyVisionGlobalData;
                break;
            }
        }
        Format = "Dolby Vision Global Data GSP";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:DolbyVisionIntegratedData"))
    {
        for (DolbyVisionGlobalData = DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData; DolbyVisionGlobalData = DolbyVisionGlobalData->NextSiblingElement())
        {
            if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:Version"))
            {
                if (const char* Text = DolbyVisionGlobalData->GetText())
                    Version = Text;
            }
            if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:DolbyVisionGlobalData"))
            {
                for (DolbyVisionGlobalData = DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData; DolbyVisionGlobalData = DolbyVisionGlobalData->NextSiblingElement())
                {
                    if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:CanvasAspectRatio"))
                    {
                        if (const char* Text = DolbyVisionGlobalData->GetText())
                            if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                                AspectRatio = TextF;
                    }
                    if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:Track"))
                    {
                        MasterElements[1]=DolbyVisionGlobalData;
                        break;
                    }
                }
                break;
            }
        }
        Format = "Dolby Vision Integrated Data";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionIntegratedWrapper"))
    {
        for (DolbyVisionGlobalData = DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData; DolbyVisionGlobalData = DolbyVisionGlobalData->NextSiblingElement())
        {
            if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionGlobalData"))
            {
                MasterElements[0] = DolbyVisionGlobalData;
                Format = "Dolby Vision Global Data";
            }
            if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionFrameData"))
            {
                Format = "Dolby Vision Frame Data";
                MasterElements[1]=DolbyVisionGlobalData;
                break; // Frame Data has priority over Global Data
            }
        }
        if (!Format)
            Format = "Dolby Vision Integrated Wrapper";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionGlobalData"))
    {
        MasterElements[0] = DolbyVisionGlobalData;
        Format = "Dolby Vision Global Data";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionFrameData") || !strcmp(DolbyVisionGlobalData->Name(), "fw:DolbyVisionFrameData"))
    {
        MasterElements[1] = DolbyVisionGlobalData;
        Format = "Dolby Vision Frame Data";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyLabsMDF"))
    {
        Format = "Dolby Vision MDF";
    }
    else
        DolbyVisionGlobalData = NULL;

    if (!DolbyVisionGlobalData)
    {
        Reject("DolbyVisionMetadata");
        return false;
    }
    if (const char* Text = DolbyVisionGlobalData->Attribute("version"))
        Version=Text;

    // Dolby Vision XML metadata, root element DolbyLabsMDF (CM v2.9 is version 2.x, CM v4.0 is version 4.x or more)
    XMLElement* MDF_Output = nullptr;
    XMLElement* MDF_Revision_Last = nullptr;
    if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyLabsMDF"))
    {
        for (XMLElement* MDF_Item = DolbyVisionGlobalData->FirstChildElement(); MDF_Item; MDF_Item = MDF_Item->NextSiblingElement())
        {
            if (!strcmp(MDF_Item->Name(), "Version"))
            {
                if (const char* Text = MDF_Item->GetText())
                    Version = Text;
            }
            if (!strcmp(MDF_Item->Name(), "RevisionHistory"))
            {
                for (XMLElement* Revision = MDF_Item->FirstChildElement("Revision"); Revision; Revision = Revision->NextSiblingElement("Revision"))
                    MDF_Revision_Last = Revision;
            }
            if (!strcmp(MDF_Item->Name(), "Outputs"))
            {
                if (XMLElement* Output = MDF_Item->FirstChildElement("Output"))
                {
                    MDF_Output = Output;
                    if (XMLElement* Video = Output->FirstChildElement("Video"))
                        MasterElements[1] = Video->FirstChildElement("Track");
                }
            }
        }
        if (!MDF_Output)
        {
            Reject("DolbyVisionMetadata");
            return false;
        }
    }

    Accept("DolbyVisionMetadata");
    Fill(Stream_General, 0, General_Format, "Dolby Vision Metadata");
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_HDR_Format, "Dolby Vision Metadata");
    Stream_Prepare(Stream_Other);
    if (Format)
        Fill(Stream_Other, 0, Other_Format, Format);
    if (!Version.empty())
        Fill(Stream_Video, 0, Video_HDR_Format_Version, Version);
    if (AspectRatio)
        Fill(Stream_Video, 0, Video_DisplayAspectRatio, AspectRatio);

    // Dolby Vision metadata, displayed the same way as the metadata from the RPU
    DV_RPU DV;
    int8u TargetDisplay_WithoutID = 255;
    auto TargetDisplay_Fill = [&](XMLElement* TargetDisplay)
    {
        const char* Name = TargetDisplay->Attribute("name");
        const char* ID = TargetDisplay->Attribute("id");
        float64 PeakBrightness = 0, MinimumBrightness = 0;
        for (XMLElement* TargetDisplay_Item = TargetDisplay->FirstChildElement(); TargetDisplay_Item; TargetDisplay_Item = TargetDisplay_Item->NextSiblingElement())
        {
            const char* Text = TargetDisplay_Item->GetText();
            if (!Text)
                continue;
            if (!strcmp(TargetDisplay_Item->Name(), "Name"))
                Name = Text;
            if (!strcmp(TargetDisplay_Item->Name(), "ID"))
                ID = Text;
            if (!strcmp(TargetDisplay_Item->Name(), "PeakBrightness"))
                PeakBrightness = atof(Text);
            if (!strcmp(TargetDisplay_Item->Name(), "MinimumBrightness"))
                MinimumBrightness = atof(Text);
        }
        int8u Index = ID ? (int8u)atoi(ID) : TargetDisplay_WithoutID--;
        if (find(DV.L8_target_display_index.begin(), DV.L8_target_display_index.end(), Index) != DV.L8_target_display_index.end())
            return;
        DV.L8_target_display_index.push_back(Index);
        if (Name)
            DV.TargetDisplay_Names.emplace_back(Index, Ztring().From_UTF8(Name));
        if (PeakBrightness)
            DV.L10.push_back({ Index, 0, DolbyVision_Nits_ToPQ12(PeakBrightness), DolbyVision_Nits_ToPQ12(MinimumBrightness) });
    };
    auto MasteringDisplay_Fill = [&](XMLElement* MasteringDisplay)
    {
        const char* Name = MasteringDisplay->Attribute("name");
        const char* ID = MasteringDisplay->Attribute("id");
        for (XMLElement* MasteringDisplay_Item = MasteringDisplay->FirstChildElement(); MasteringDisplay_Item; MasteringDisplay_Item = MasteringDisplay_Item->NextSiblingElement())
        {
            const char* Text = MasteringDisplay_Item->GetText();
            if (!Text)
                continue;
            if (!strcmp(MasteringDisplay_Item->Name(), "Name"))
                Name = Text;
            if (!strcmp(MasteringDisplay_Item->Name(), "ID"))
                ID = Text;
            if (!strcmp(MasteringDisplay_Item->Name(), "DiagonalSize"))
                DV.source_diagonal = (int16u)atoi(Text);
        }
        if (Name)
            DV.MasteringDisplay_Name.From_UTF8(Name);
        if (ID)
            DV.MasteringDisplay_ID.From_UTF8(ID);
    };
    auto MasteringDisplay_Values = [&](const mastering_metadata_2086& Mastering)
    {
        if (Mastering.Luminance[0] != (int32u)-1 && Mastering.Luminance[1] != (int32u)-1)
        {
            int32u Source = (((int32u)DolbyVision_Nits_ToPQ12(((float64)Mastering.Luminance[0]) / 10000)) << 16) | DolbyVision_Nits_ToPQ12(((float64)Mastering.Luminance[1]) / 10000);
            if (find(DV.Sources.begin(), DV.Sources.end(), Source) == DV.Sources.end())
                DV.Sources.push_back(Source);
        }
        DV_RPU::l9 L9{};
        static const int8u Order[8] = { 4, 5, 0, 1, 2, 3, 6, 7 }; // Green, blue, red, white to red, green, blue, white
        for (int8u i = 0; i < 8; i++)
        {
            int16u Value = Mastering.Primaries[Order[i]];
            if (Value == (int16u)-1)
                return;
            L9.Primaries[i] = (int16u)float64_int64s(((float64)Value) * 32767 / 50000);
        }
        L9.source_primary_index = 255;
        L9.Explicit = true;
        if (find(DV.L9.begin(), DV.L9.end(), L9) == DV.L9.end())
            DV.L9.push_back(L9);
    };
    auto Level_Parse = [&](XMLElement* Level_Item)
    {
        int Level = Level_Item->IntAttribute("level", -1);
        if (Level < 0)
            return;
        DV_RPU::l9 L9{};
        DV_RPU::l11 L11{ 0, 0, (int8u)-1 };
        bool L9_IsValid = false, L11_IsValid = false;
        for (XMLElement* Item = Level_Item->FirstChildElement(); Item; Item = Item->NextSiblingElement())
        {
            const char* Text = Item->GetText();
            if (!Text)
                continue;
            if (Level == 1 && !strcmp(Item->Name(), "ImageCharacter"))
            {
                auto Values = Values_Get(Text); // Minimum, average, maximum
                if (Values.size() == 3 && Values[1] >= 0 && Values[2] >= 0 && Values[1] <= 1 && Values[2] <= 1)
                {
                    auto avg_PQ = (int16u)float64_int64s(Values[1] * 4095);
                    auto max_PQ = (int16u)float64_int64s(Values[2] * 4095);
                    if (DV.L1_avg_PQ < avg_PQ)
                        DV.L1_avg_PQ = avg_PQ;
                    if (DV.L1_max_PQ < max_PQ)
                        DV.L1_max_PQ = max_PQ;
                }
            }
            if (Level == 5 && !strcmp(Item->Name(), "AspectRatios"))
            {
                auto Values = Values_Get(Text); // Canvas, image
                if (Values.size() == 2 && Values[1] > 0 && find(DV.Active_DisplayAspectRatios.begin(), DV.Active_DisplayAspectRatios.end(), Values[1]) == DV.Active_DisplayAspectRatios.end())
                    DV.Active_DisplayAspectRatios.push_back(Values[1]);
            }
            if (Level == 9 && !strcmp(Item->Name(), "SourceColorModel"))
            {
                L9.source_primary_index = (int8u)atoi(Text);
                L9_IsValid = true;
            }
            if (Level == 9 && !strcmp(Item->Name(), "SourceColorPrimary"))
            {
                auto Values = Values_Get(Text);
                if (Values.size() == 8)
                {
                    for (int8u i = 0; i < 8; i++)
                        L9.Primaries[i] = (int16u)(int16s)float64_int64s(Values[i] * 32767);
                    L9.Explicit = true;
                }
            }
            if (Level == 11 && !strcmp(Item->Name(), "ContentType"))
            {
                L11.content_type = (int8u)atoi(Text);
                L11_IsValid = true;
            }
            if (Level == 11 && !strcmp(Item->Name(), "IntendedWhitePoint"))
                L11.white_point = (int8u)atoi(Text);
        }
        if (L9_IsValid && (L9.Explicit || L9.source_primary_index != 255) && find(DV.L9.begin(), DV.L9.end(), L9) == DV.L9.end())
            DV.L9.push_back(L9);
        if (L11_IsValid && find(DV.L11.begin(), DV.L11.end(), L11) == DV.L11.end())
            DV.L11.push_back(L11);
    };
    auto DynamicData_Parse = [&](XMLElement* PluginNode)
    {
        // Version 2.x: DolbyEDR elements with a level attribute, version 4.x or more: LevelN elements in DVDynamicData
        for (XMLElement* PluginNode_Item = PluginNode->FirstChildElement(); PluginNode_Item; PluginNode_Item = PluginNode_Item->NextSiblingElement())
        {
            if (!strcmp(PluginNode_Item->Name(), "DVDynamicData"))
            {
                for (XMLElement* Level_Item = PluginNode_Item->FirstChildElement(); Level_Item; Level_Item = Level_Item->NextSiblingElement())
                    Level_Parse(Level_Item);
            }
            else
                Level_Parse(PluginNode_Item);
        }
    };

    if (MDF_Output)
    {
        if (!Version.empty())
            DV.CMVersion.From_UTF8(Version[0] == '2' ? "2.9" : "4.0");
        if (MDF_Revision_Last)
        {
            const char* CompanyName = nullptr;
            const char* Name = nullptr;
            const char* SoftwareVersion = nullptr;
            const char* DateTime = nullptr;
            for (XMLElement* Revision_Item = MDF_Revision_Last->FirstChildElement(); Revision_Item; Revision_Item = Revision_Item->NextSiblingElement())
            {
                if (!strcmp(Revision_Item->Name(), "Author"))
                    CompanyName = Revision_Item->GetText();
                if (!strcmp(Revision_Item->Name(), "Software"))
                    Name = Revision_Item->GetText();
                if (!strcmp(Revision_Item->Name(), "SoftwareVersion"))
                    SoftwareVersion = Revision_Item->GetText();
                if (!strcmp(Revision_Item->Name(), "DateTime"))
                    DateTime = Revision_Item->GetText();
            }
            if (CompanyName)
                Fill(Stream_General, 0, General_Encoded_Application_CompanyName, CompanyName);
            if (Name)
                Fill(Stream_General, 0, General_Encoded_Application_Name, Name);
            if (SoftwareVersion)
                Fill(Stream_General, 0, General_Encoded_Application_Version, SoftwareVersion);
            if (DateTime)
                Fill(Stream_General, 0, General_Encoded_Date, DateTime);
        }
        if (const char* Text = MDF_Output->Attribute("name"))
            Fill(Stream_General, 0, General_Title, Text);
        for (XMLElement* Output_Item = MDF_Output->FirstChildElement(); Output_Item; Output_Item = Output_Item->NextSiblingElement())
        {
            if (!strcmp(Output_Item->Name(), "CompositionName"))
            {
                if (const char* Text = Output_Item->GetText())
                    Fill(Stream_General, 0, General_Title, Text, Unlimited, true, true);
            }
            if (!strcmp(Output_Item->Name(), "UniqueID"))
            {
                if (const char* Text = Output_Item->GetText())
                    Fill(Stream_General, 0, General_UniqueID, Text);
            }
            if (!strcmp(Output_Item->Name(), "CanvasAspectRatio"))
            {
                if (const char* Text = Output_Item->GetText())
                    if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                        Fill(Stream_Video, 0, Video_DisplayAspectRatio, TextF);
            }
            if (!strcmp(Output_Item->Name(), "ImageAspectRatio"))
            {
                if (const char* Text = Output_Item->GetText())
                    if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                        Fill(Stream_Video, 0, Video_Active_DisplayAspectRatio, TextF);
            }
        }
        if (MasterElements[1])
            if (const char* Text = MasterElements[1]->Attribute("name"))
                Fill(Stream_Other, 0, Other_Title, Text);
    }

    int64u Shot_In = (int64u)-1, Shot_Out = 0;
    size_t Shot_Count = 0;
    for (auto MasterElement : MasterElements)
    {
    if (!MasterElement)
        continue;
    for (XMLElement* DolbyVisionGlobalData_Item=MasterElement->FirstChildElement(); DolbyVisionGlobalData_Item; DolbyVisionGlobalData_Item=DolbyVisionGlobalData_Item->NextSiblingElement())
    {
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "ColorEncoding"))
        {
            mastering_metadata_2086 Mastering;
            memset(&Mastering, 0xFF, sizeof(Mastering));
            for (XMLElement* ColorEncoding_Item=DolbyVisionGlobalData_Item->FirstChildElement(); ColorEncoding_Item; ColorEncoding_Item=ColorEncoding_Item->NextSiblingElement())
            {
                if (!strcmp(ColorEncoding_Item->Name(), "BitDepth"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        Fill(Stream_Video, 0, Video_BitDepth, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "ChromaFormat"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        if (strlen(Text) == 3 && Text[0] >= '0' && Text[0] <= '9' && Text[1] >= '0' && Text[1] <= '9' && Text[2] >= '0' && Text[2] <= '9')
                        {
                            const char ChromaSubsampling[] = { Text[0], ':', Text[1], ':', Text[2], '\0' };
                            Fill(Stream_Video, 0, Video_ChromaSubsampling, ChromaSubsampling);
                        }
                        else
                            Fill(Stream_Video, 0, Video_ChromaSubsampling, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "CanvasAspectRatio"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                        if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                            Fill(Stream_Video, 0, Video_DisplayAspectRatio, TextF);
                }
                if (false) // !strcmp(ColorEncoding_Item->Name(), "ColorSpace"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "rgb"))
                            Text = "RGB";
                        if (!strcmp(Text, "yuv"))
                            Text = "YUV";
                        Fill(Stream_Video, 0, Video_ColorSpace, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "Encoding"))
                {
                    if (const char* Text=ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "pq"))
                            Text="PQ";
                        Fill(Stream_Video, 0, Video_transfer_characteristics, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "Primaries"))
                {
                    for (XMLElement* Primaries_Item=ColorEncoding_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                    {
                        int8u i=(int8u)-1;
                        if (!strcmp(Primaries_Item->Name(), "Green"))
                            i=0;
                        if (!strcmp(Primaries_Item->Name(), "Blue"))
                            i=2;
                        if (!strcmp(Primaries_Item->Name(), "Red"))
                            i=4;
                        if (i!=(int8u)-1)
                        {
                            if (const char* Text=Primaries_Item->GetText())
                            {
                                ZtringList List;
                                List.Separator_Set(0, __T(" "));
                                List.Write(Ztring(Text));
                                if (List.size()==1)
                                {
                                    //Trying with space
                                    List.Separator_Set(0, __T(","));
                                    List.Write(Ztring(Text));
                                }
                                if (List.size()==2)
                                {
                                    Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                    Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                }
                            }
                        }
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "SignalRange"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "computer"))
                            Text = "Full";
                        Fill(Stream_Video, 0, Video_colour_range, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "WhitePoint"))
                {
                    if (const char* Text=ColorEncoding_Item->GetText())
                    {
                        ZtringList List;
                        List.Separator_Set(0, __T(" "));
                        List.Write(Ztring(Text));
                        if (List.size()==1)
                        {
                            //Trying with space
                            List.Separator_Set(0, __T(","));
                            List.Write(Ztring(Text));
                        }
                        if (List.size()==2)
                        {
                            Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                            Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                        }
                    }
                }
            }
            Ztring colour_primaries;
            Get_MasteringDisplayColorVolume(colour_primaries, colour_primaries, Mastering); // Second part is not used
            if (!colour_primaries.empty())
                Fill(Stream_Video, 0, Video_colour_primaries, colour_primaries);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "EditRate"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
            {
                float32 FrameRate = Ztring().From_UTF8(Text).To_float32();
                if (FrameRate)
                {
                    Text = strchr(Text, ' ');
                    if (Text)
                    {
                        Text++;
                        float32 FrameRateD = Ztring().From_UTF8(Text).To_float32();
                        if (FrameRateD)
                            FrameRate /= FrameRateD;
                    }
                }
                Fill(Stream_Video, 0, Video_FrameRate, FrameRate);
            }
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Level6"))
        {
            DV_RPU::l6 L6{};
            for (XMLElement* Level6_Item = DolbyVisionGlobalData_Item->FirstChildElement(); Level6_Item; Level6_Item = Level6_Item->NextSiblingElement())
            {
                if (!strcmp(Level6_Item->Name(), "MaxCLL"))
                {
                    if (const char* Text = Level6_Item->GetText())
                        if ((L6.max_content_light_level = Ztring().From_UTF8(Text).To_float64()))
                            Fill(Stream_Video, 0, Video_MaxCLL, Ztring(Text));
                }
                if (!strcmp(Level6_Item->Name(), "MaxFALL"))
                {
                    if (const char* Text = Level6_Item->GetText())
                        if ((L6.max_frame_average_light_level = Ztring().From_UTF8(Text).To_float64()))
                            Fill(Stream_Video, 0, Video_MaxFALL, Ztring(Text));
                }
            }
            if (L6.max_content_light_level || L6.max_frame_average_light_level)
                DV.L6.push_back(L6);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "PluginNode"))
        {
            for (XMLElement* PluginNode_Item=DolbyVisionGlobalData_Item->FirstChildElement(); PluginNode_Item; PluginNode_Item=PluginNode_Item->NextSiblingElement())
            {
                if (!strcmp(PluginNode_Item->Name(), "DolbyEDR") || !strcmp(PluginNode_Item->Name(), "DVGlobalData"))
                {
                    for (XMLElement* DolbyEDR_Item=PluginNode_Item->FirstChildElement(); DolbyEDR_Item; DolbyEDR_Item=DolbyEDR_Item->NextSiblingElement())
                    {
                        if (!strcmp(DolbyEDR_Item->Name(), "Characteristics"))
                        {
                            for (XMLElement* Characteristics_Item=DolbyEDR_Item->FirstChildElement(); Characteristics_Item; Characteristics_Item=Characteristics_Item->NextSiblingElement())
                            {
                                if (!strcmp(Characteristics_Item->Name(), "MasteringDisplay"))
                                {
                                    MasteringDisplay_Fill(Characteristics_Item);
                                    mastering_metadata_2086 Mastering;
                                    memset(&Mastering, 0xFF, sizeof(Mastering));
                                    for (XMLElement* MasteringDisplay_Item=Characteristics_Item->FirstChildElement(); MasteringDisplay_Item; MasteringDisplay_Item=MasteringDisplay_Item->NextSiblingElement())
                                    {
                                        if (!strcmp(MasteringDisplay_Item->Name(), "MinimumBrightness"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                                Mastering.Luminance[0]=float64_int64s(Ztring(Text).To_float64()*10000);
                                        }
                                        if (!strcmp(MasteringDisplay_Item->Name(), "PeakBrightness"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                                Mastering.Luminance[1]=float64_int64s(Ztring(Text).To_float64()*10000);
                                        }
                                        if (!strcmp(MasteringDisplay_Item->Name(), "Primaries"))
                                        {
                                            for (XMLElement* Primaries_Item=MasteringDisplay_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                                            {
                                                int8u i=(int8u)-1;
                                                if (!strcmp(Primaries_Item->Name(), "Green"))
                                                    i=0;
                                                if (!strcmp(Primaries_Item->Name(), "Blue"))
                                                    i=2;
                                                if (!strcmp(Primaries_Item->Name(), "Red"))
                                                    i=4;
                                                if (i!=(int8u)-1)
                                                {
                                                    if (const char* Text=Primaries_Item->GetText())
                                                    {
                                                        ZtringList List;
                                                        List.Separator_Set(0, __T(" "));
                                                        List.Write(Ztring(Text));
                                                        if (List.size()==1)
                                                        {
                                                            //Trying with space
                                                            List.Separator_Set(0, __T(","));
                                                            List.Write(Ztring(Text));
                                                        }
                                                        if (List.size()==2)
                                                        {
                                                            Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                                            Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (!strcmp(MasteringDisplay_Item->Name(), "WhitePoint"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                            {
                                                ZtringList List;
                                                List.Separator_Set(0, __T(" "));
                                                List.Write(Ztring(Text));
                                                if (List.size()==1)
                                                {
                                                    //Trying with space
                                                    List.Separator_Set(0, __T(","));
                                                    List.Write(Ztring(Text));
                                                }
                                                if (List.size()==2)
                                                {
                                                    Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                                                    Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                                                }
                                            }
                                        }
                                    }
                                    Ztring colour_primaries, luminance;
                                    MasteringDisplay_Values(Mastering);
                                    Get_MasteringDisplayColorVolume(colour_primaries, luminance, Mastering);
                                    if (!colour_primaries.empty())
                                        Fill(Stream_Video, 0, Video_MasteringDisplay_ColorPrimaries, colour_primaries);
                                    if (!luminance.empty())
                                        Fill(Stream_Video, 0, Video_MasteringDisplay_Luminance, luminance);
                                }
                                if (!strcmp(Characteristics_Item->Name(), "TargetDisplay"))
                                    TargetDisplay_Fill(Characteristics_Item);
                            }
                        }
                        if (!strcmp(DolbyEDR_Item->Name(), "TargetDisplay"))
                            TargetDisplay_Fill(DolbyEDR_Item);
                        if (!strcmp(DolbyEDR_Item->Name(), "MasteringDisplay"))
                        {
                            MasteringDisplay_Fill(DolbyEDR_Item);
                            mastering_metadata_2086 Mastering;
                            memset(&Mastering, 0xFF, sizeof(Mastering));
                            for (XMLElement* MasteringDisplay_Item=DolbyEDR_Item->FirstChildElement(); MasteringDisplay_Item; MasteringDisplay_Item=MasteringDisplay_Item->NextSiblingElement())
                            {
                                if (!strcmp(MasteringDisplay_Item->Name(), "MinimumBrightness"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                        Mastering.Luminance[0]=float64_int64s(Ztring(Text).To_float64()*10000);
                                }
                                if (!strcmp(MasteringDisplay_Item->Name(), "PeakBrightness"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                        Mastering.Luminance[1]=float64_int64s(Ztring(Text).To_float64()*10000);
                                }
                                if (!strcmp(MasteringDisplay_Item->Name(), "Primaries"))
                                {
                                    for (XMLElement* Primaries_Item=MasteringDisplay_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                                    {
                                        int8u i=(int8u)-1;
                                        if (!strcmp(Primaries_Item->Name(), "Green"))
                                            i=0;
                                        if (!strcmp(Primaries_Item->Name(), "Blue"))
                                            i=2;
                                        if (!strcmp(Primaries_Item->Name(), "Red"))
                                            i=4;
                                        if (i!=(int8u)-1)
                                        {
                                            if (const char* Text=Primaries_Item->GetText())
                                            {
                                                ZtringList List;
                                                List.Separator_Set(0, __T(" "));
                                                List.Write(Ztring(Text));
                                                if (List.size()==1)
                                                {
                                                    //Trying with space
                                                    List.Separator_Set(0, __T(","));
                                                    List.Write(Ztring(Text));
                                                }
                                                if (List.size()==2)
                                                {
                                                    Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                                    Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                                }
                                            }
                                        }
                                    }
                                }
                                if (!strcmp(MasteringDisplay_Item->Name(), "WhitePoint"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                    {
                                        ZtringList List;
                                        List.Separator_Set(0, __T(" "));
                                        List.Write(Ztring(Text));
                                        if (List.size()==1)
                                        {
                                            //Trying with space
                                            List.Separator_Set(0, __T(","));
                                            List.Write(Ztring(Text));
                                        }
                                        if (List.size()==2)
                                        {
                                            Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                                            Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                                        }
                                    }
                                }
                            }
                            Ztring colour_primaries, luminance;
                            MasteringDisplay_Values(Mastering);
                            Get_MasteringDisplayColorVolume(colour_primaries, luminance, Mastering);
                            if (!colour_primaries.empty())
                                Fill(Stream_Video, 0, Video_MasteringDisplay_ColorPrimaries, colour_primaries);
                            if (!luminance.empty())
                                Fill(Stream_Video, 0, Video_MasteringDisplay_Luminance, luminance);
                        }
                    }
                }
                if (!strcmp(PluginNode_Item->Name(), "Level254"))
                {
                    for (XMLElement* Level254_Item = PluginNode_Item->FirstChildElement(); Level254_Item; Level254_Item = Level254_Item->NextSiblingElement())
                    {
                        const char* Text = Level254_Item->GetText();
                        if (!Text)
                            continue;
                        if (!strcmp(Level254_Item->Name(), "CMVersion"))
                        {
                            // "4 1" is displayed as "4.1"
                            Ztring CMVersion;
                            CMVersion.From_UTF8(Text);
                            CMVersion.FindAndReplace(__T(" "), __T("."), 0, Ztring_Recursive);
                            CMVersion.FindAndReplace(__T(","), __T("."), 0, Ztring_Recursive);
                            DV.CMVersion = CMVersion;
                        }
                        if (!strcmp(Level254_Item->Name(), "DMVersion"))
                        {
                            DV.L254_Present = true;
                            DV.dm_version_index = (int8u)atoi(Text);
                        }
                        if (!strcmp(Level254_Item->Name(), "DMMode"))
                        {
                            DV.L254_Present = true;
                            DV.dm_mode = (int8u)atoi(Text);
                        }
                    }
                }
            }
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Rate"))
        {
            int64u n = 0, d = 0;
            for (XMLElement* Rate_Item = DolbyVisionGlobalData_Item->FirstChildElement(); Rate_Item; Rate_Item = Rate_Item->NextSiblingElement())
            {
                if (!strcmp(Rate_Item->Name(), "d"))
                    d = Ztring().From_UTF8(Rate_Item->GetText()).To_float32();
                if (!strcmp(Rate_Item->Name(), "n"))
                    n = Ztring().From_UTF8(Rate_Item->GetText()).To_float32();
            }
            if (n && d)
                Fill(Stream_Video, 0, Video_FrameRate, ((float32)n) / d);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Shot"))
        {
            if (XMLElement* Record = DolbyVisionGlobalData_Item->FirstChildElement("Record"))
            {
                XMLElement* In = Record->FirstChildElement("In");
                XMLElement* Duration = Record->FirstChildElement("Duration");
                const char* In_Text = In ? In->GetText() : nullptr;
                const char* Duration_Text = Duration ? Duration->GetText() : nullptr;
                if (In_Text && Duration_Text)
                {
                    int64u In_Value = Ztring().From_UTF8(In_Text).To_int64u();
                    int64u Duration_Value = Ztring().From_UTF8(Duration_Text).To_int64u();
                    if (Shot_In > In_Value)
                        Shot_In = In_Value;
                    if (Shot_Out < In_Value + Duration_Value)
                        Shot_Out = In_Value + Duration_Value;
                }
            }
            for (XMLElement* Shot_Item = DolbyVisionGlobalData_Item->FirstChildElement(); Shot_Item; Shot_Item = Shot_Item->NextSiblingElement())
            {
                if (!strcmp(Shot_Item->Name(), "PluginNode"))
                    DynamicData_Parse(Shot_Item);
                if (!strcmp(Shot_Item->Name(), "Frame"))
                    if (XMLElement* PluginNode = Shot_Item->FirstChildElement("PluginNode"))
                        DynamicData_Parse(PluginNode);
            }
            Shot_Count++;
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "TrackName"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Other, 0, Other_Title, Text, Unlimited, true, true);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "UniqueID"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Other, 0, Other_UniqueID, Text);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Version"))
        {
            if (const char* Text= DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Video, 0, Video_HDR_Format_Version, Text);
        }
    }
    }
    if (Shot_Count)
    {
        DV.Count_Shot = (int32u)Shot_Count;
        if (Shot_Out > Shot_In)
        {
            Fill(Stream_Video, 0, Video_FrameCount, Shot_Out - Shot_In);
            DV.FrameRange_Start = Shot_In;
            DV.FrameRange_End = Shot_Out - 1;
        }
    }
    DolbyVision_Streams_Fill(DV, DV_Source_XML);

    if (MDF_Output)
        Conformance_MDF(document.FirstChildElement(), MasterElements[1], Version);

    //All should be OK...
    Element_Offset=File_Size;
    return true;
}

//***************************************************************************
// Conformance
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyVisionMetadata::Conformance_MDF(XMLElement* Root, XMLElement* Track, const string& Version)
{
    // Version, attribute in version 2.x, element in version 4.x or more
    if (Version.empty())
    {
        Fill_Conformance("DolbyVision Version", "Version is missing", {}, Conformance_Error, Stream_Video, 0);
        return;
    }
    const int Version_Major = atoi(Version.c_str());
    const bool IsV2 = Version_Major < 4;
    const char* Version_Attribute = Root->Attribute("version");
    const char* Version_Element = nullptr;
    if (XMLElement* Version_Item = Root->FirstChildElement("Version"))
        Version_Element = Version_Item->GetText();
    if (Version_Attribute && Version_Element && strcmp(Version_Attribute, Version_Element))
        Fill_Conformance("DolbyVision Version", "version attribute (" + string(Version_Attribute) + ") and Version element (" + Version_Element + ") differ", {}, Conformance_Error, Stream_Video, 0);
    if (IsV2 ? Version_Element != nullptr : Version_Attribute != nullptr)
        Fill_Conformance("DolbyVision Version", IsV2 ? "Version element is not expected in version 2.x, version attribute is expected" : "version attribute is not expected in version 4.x or more, Version element is expected", {}, Conformance_Warning, Stream_Video, 0);

    // Namespace
    if (const char* Namespace = Root->Attribute("xmlns"))
    {
        static const char Namespace_Prefix[] = "http://www.dolby.com/schemas/dvmd/";
        if (!strncmp(Namespace, Namespace_Prefix, sizeof(Namespace_Prefix) - 1))
        {
            string Namespace_Expected(Version);
            for (auto& Character : Namespace_Expected)
                if (Character == '.')
                    Character = '_';
            if (Namespace_Expected != Namespace + sizeof(Namespace_Prefix) - 1)
                Fill_Conformance("DolbyVision Namespace", "Namespace " + string(Namespace) + " does not match version " + Version, {}, Conformance_Error, Stream_Video, 0);
        }
    }

    if (!Track)
        return;

    // Structure, element names and value separator depend on version
    const char* GlobalData_Unexpected = IsV2 ? "DVGlobalData" : "DolbyEDR";
    const char* DynamicData_Unexpected = IsV2 ? "DVDynamicData" : "DolbyEDR";
    auto Separator_Check = [&](XMLElement* Element)
    {
        const char* Text = Element ? Element->GetText() : nullptr;
        if (!Text)
            return;
        bool HasComma = strchr(Text, ',') != nullptr;
        if (IsV2 ? (!HasComma && strchr(Text, ' ')) : HasComma)
            Fill_Conformance("DolbyVision Structure", string(Element->Name()) + " values are separated by " + (IsV2 ? "space" : "comma") + ", " + (IsV2 ? "comma" : "space") + " is expected in version " + Version, {}, Conformance_Error, Stream_Video, 0);
    };
    if (XMLElement* ColorEncoding = Track->FirstChildElement("ColorEncoding"))
    {
        Separator_Check(ColorEncoding->FirstChildElement("WhitePoint"));
        if (XMLElement* Primaries = ColorEncoding->FirstChildElement("Primaries"))
            Separator_Check(Primaries->FirstChildElement("Red"));
    }

    // Global data
    std::set<string> TargetDisplay_IDs;
    const char* MasteringDisplay_Peak = nullptr;
    XMLElement* Level254 = nullptr;
    auto Display_Parse = [&](XMLElement* Parent)
    {
        for (XMLElement* Item = Parent->FirstChildElement(); Item; Item = Item->NextSiblingElement())
        {
            if (!strcmp(Item->Name(), "TargetDisplay"))
            {
                const char* ID = Item->Attribute("id");
                if (XMLElement* ID_Item = Item->FirstChildElement("ID"))
                    if (const char* Text = ID_Item->GetText())
                        ID = Text;
                if (ID)
                    TargetDisplay_IDs.insert(ID);
            }
            if (!strcmp(Item->Name(), "MasteringDisplay"))
            {
                if (XMLElement* PeakBrightness = Item->FirstChildElement("PeakBrightness"))
                    MasteringDisplay_Peak = PeakBrightness->GetText();
            }
        }
    };
    if (XMLElement* PluginNode = Track->FirstChildElement("PluginNode"))
    {
        for (XMLElement* PluginNode_Item = PluginNode->FirstChildElement(); PluginNode_Item; PluginNode_Item = PluginNode_Item->NextSiblingElement())
        {
            if (!strcmp(PluginNode_Item->Name(), GlobalData_Unexpected))
                Fill_Conformance("DolbyVision Structure", string(GlobalData_Unexpected) + " element is not expected in version " + Version, {}, Conformance_Error, Stream_Video, 0);
            if (!strcmp(PluginNode_Item->Name(), "DVGlobalData") || !strcmp(PluginNode_Item->Name(), "DolbyEDR"))
            {
                // Same places as for the display of the displays, also if the element name does not match the version
                Display_Parse(PluginNode_Item);
                if (XMLElement* Characteristics = PluginNode_Item->FirstChildElement("Characteristics"))
                    Display_Parse(Characteristics);
            }
            if (!strcmp(PluginNode_Item->Name(), "Level254"))
                Level254 = PluginNode_Item;
        }
    }

    // Level 254
    if (IsV2 && Level254)
        Fill_Conformance("DolbyVision Level254", "Level254 is not expected in version " + Version, {}, Conformance_Warning, Stream_Video, 0);
    if (!IsV2 && !Level254)
        Fill_Conformance("DolbyVision Level254", "Level254 is missing", {}, Conformance_Warning, Stream_Video, 0);
    if (!IsV2 && Level254)
    {
        if (XMLElement* CMVersion = Level254->FirstChildElement("CMVersion"))
        {
            if (const char* Text = CMVersion->GetText())
                if (atoi(Text) != 4)
                    Fill_Conformance("DolbyVision Level254", "CMVersion " + string(Text) + " does not match version " + Version + " (actual " + std::to_string(atoi(Text)) + ", expected 4)", {}, Conformance_Warning, Stream_Video, 0);
        }
        else
            Fill_Conformance("DolbyVision Level254", "CMVersion is missing", {}, Conformance_Warning, Stream_Video, 0);
    }

    // Level 6
    if (MasteringDisplay_Peak)
        if (XMLElement* Level6 = Track->FirstChildElement("Level6"))
            if (XMLElement* MaxCLL = Level6->FirstChildElement("MaxCLL"))
                if (const char* Text = MaxCLL->GetText())
                    if (Ztring().From_UTF8(Text).To_float64() > Ztring().From_UTF8(MasteringDisplay_Peak).To_float64())
                        Fill_Conformance("DolbyVision Level6", "MaxCLL is more than mastering display peak brightness (actual " + string(Text) + ", expected <= " + MasteringDisplay_Peak + ")", {}, Conformance_Warning, Stream_Video, 0);

    // Shots
    std::set<string> TIDs;
    int64u Previous_Out = (int64u)-1;
    int64u Gap_Count = 0, Gap_Frames = 0, Gap_First = 0;
    int64u Overlap_Count = 0, Overlap_Frames = 0, Overlap_First = 0;
    int64u Empty_Count = 0, Empty_First = 0;
    bool DynamicData_Unexpected_Found = false;
    for (XMLElement* Shot = Track->FirstChildElement("Shot"); Shot; Shot = Shot->NextSiblingElement("Shot"))
    {
        TID_Collect(Shot, TIDs);
        if (!DynamicData_Unexpected_Found)
            if (XMLElement* PluginNode = Shot->FirstChildElement("PluginNode"))
                if (PluginNode->FirstChildElement(DynamicData_Unexpected))
                {
                    Fill_Conformance("DolbyVision Structure", string(DynamicData_Unexpected) + " element in Shot is not expected in version " + Version, {}, Conformance_Error, Stream_Video, 0);
                    DynamicData_Unexpected_Found = true;
                }

        XMLElement* Record = Shot->FirstChildElement("Record");
        XMLElement* In = Record ? Record->FirstChildElement("In") : nullptr;
        XMLElement* Duration = Record ? Record->FirstChildElement("Duration") : nullptr;
        const char* In_Text = In ? In->GetText() : nullptr;
        const char* Duration_Text = Duration ? Duration->GetText() : nullptr;
        if (!In_Text || !Duration_Text)
        {
            Fill_Conformance("DolbyVision Shot", "Record In or Duration is missing", {}, Conformance_Error, Stream_Video, 0);
            continue;
        }
        int64u In_Value = Ztring().From_UTF8(In_Text).To_int64u();
        int64u Duration_Value = Ztring().From_UTF8(Duration_Text).To_int64u();
        if (!Duration_Value)
        {
            if (!Empty_Count)
                Empty_First = In_Value;
            Empty_Count++;
        }
        if (Previous_Out != (int64u)-1)
        {
            if (In_Value > Previous_Out)
            {
                if (!Gap_Count)
                    Gap_First = Previous_Out;
                Gap_Count++;
                Gap_Frames += In_Value - Previous_Out;
            }
            if (In_Value < Previous_Out)
            {
                if (!Overlap_Count)
                    Overlap_First = In_Value;
                Overlap_Count++;
                Overlap_Frames += Previous_Out - In_Value;
            }
        }
        Previous_Out = In_Value + Duration_Value;
    }
    if (Gap_Count)
        Fill_Conformance("DolbyVision Shot", std::to_string(Gap_Count) + " gap(s) between shots, " + std::to_string(Gap_Frames) + " frame(s) not covered, first gap at frame " + std::to_string(Gap_First), {}, Conformance_Error, Stream_Video, 0);
    if (Overlap_Count)
        Fill_Conformance("DolbyVision Shot", std::to_string(Overlap_Count) + " overlap(s) between shots, " + std::to_string(Overlap_Frames) + " frame(s) covered more than once, first overlap at frame " + std::to_string(Overlap_First), {}, Conformance_Error, Stream_Video, 0);
    if (Empty_Count)
        Fill_Conformance("DolbyVision Shot", std::to_string(Empty_Count) + " shot(s) with a duration of 0, first at frame " + std::to_string(Empty_First), {}, Conformance_Error, Stream_Video, 0);

    // Trims must reference a declared target display
    for (const auto& TID : TIDs)
        if (TargetDisplay_IDs.find(TID) == TargetDisplay_IDs.end())
            Fill_Conformance("DolbyVision TID", "TID " + TID + " does not match any TargetDisplay ID", {}, Conformance_Error, Stream_Video, 0);
}

} //NameSpace

#endif //defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)

