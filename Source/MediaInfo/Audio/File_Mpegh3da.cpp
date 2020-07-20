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
#if defined(MEDIAINFO_MPEGH3DA_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Mpegh3da.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cmath>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

extern const size_t Aac_sampling_frequency_Size_Usac; // USAC expands Aac_sampling_frequency[]
extern const int32u Aac_sampling_frequency[];
struct coreSbrFrameLengthIndex_mapping
{
    int8u    sbrRatioIndex;
    int8u    outputFrameLengthDivided256;
};
extern const size_t coreSbrFrameLengthIndex_Mapping_Size;
extern coreSbrFrameLengthIndex_mapping coreSbrFrameLengthIndex_Mapping[];
extern int8u Aac_Channels_Get(int8u ChannelLayout);
extern string Aac_Channels_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration2_GetString(int8u ChannelLayout);
extern string Aac_ChannelLayout_GetString(const Aac_OutputChannel* const OutputChannels, size_t OutputChannels_Size);
extern string Aac_ChannelLayout_GetString(int8u ChannelLayout, bool IsMpegh3da=false);
extern string Aac_ChannelLayout_GetString(const vector<Aac_OutputChannel>& OutputChannels);
extern string Aac_ChannelMode_GetString(int8u ChannelLayout, bool IsMpegh3da=false);
extern string Aac_ChannelMode_GetString(const vector<Aac_OutputChannel>& OutputChannels);
extern string Aac_OutputChannelPosition_GetString(int8u OutputChannelPosition);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_Profile[3]=
{
    "Main",
    "High",
    "LC",
};
extern string Mpegh3da_Profile_Get(int8u mpegh3daProfileLevelIndication)
{
    if (!mpegh3daProfileLevelIndication)
        return string();
    if (mpegh3daProfileLevelIndication>=0x10)
        return Ztring::ToZtring(mpegh3daProfileLevelIndication).To_UTF8(); // Raw value
    return string(Mpegh3da_Profile[(mpegh3daProfileLevelIndication-1)/5])+"@L"+char('0'+((mpegh3daProfileLevelIndication-1)%5));
}

//---------------------------------------------------------------------------
static const size_t Mpegh3da_MHASPacketType_Size=19;
static char* Mpegh3da_MHASPacketType[Mpegh3da_MHASPacketType_Size]=
{
    "FILLDATA",
    "MPEGH3DACFG",
    "MPEGH3DAFRAME",
    "AUDIOSCENEINFO",
    "",
    "",
    "SYNC",
    "SYNCGAP",
    "MARKER",
    "CRC16",
    "CRC32",
    "DESCRIPTOR",
    "USERINTERACTION",
    "LOUDNESS_DRC",
    "BUFFERINFO",
    "GLOBAL_CRC16",
    "GLOBAL_CRC32",
    "AUDIOTRUNCATION",
    "GENDATA",
};

static const size_t Mpegh3da_SpeakerInfo_Size=43;
static const speaker_info Mpegh3da_SpeakerInfo[Mpegh3da_SpeakerInfo_Size]=
{
    {CH_M_L030,  30, false,  0, false, false},
    {CH_M_R030,  30, true ,  0, false, false},
    {CH_M_000 ,   0, false,  0, false, false},
    {CH_LFE   ,   0, false, 15, true ,  true},
    {CH_M_L110, 110, false,  0, false, false},
    {CH_M_R110, 110, true ,  0, false, false},
    {CH_M_L022,  22, false,  0, false, false},
    {CH_M_R022,  22, true ,  0, false, false},
    {CH_M_L135, 135, false,  0, false, false},
    {CH_M_R135, 135, true ,  0, false, false},
    {CH_M_180 , 180, false,  0, false, false},
    {CH_M_LSD , 135, false,  0, false, false},
    {CH_M_RSD , 135, true ,  0, false, false},
    {CH_M_L090,  90, false,  0, false, false},
    {CH_M_R090,  90, true ,  0, false, false},
    {CH_M_L060,  60, false,  0, false, false},
    {CH_M_R060,  60, true ,  0, false, false},
    {CH_U_L030,  30, false, 35, false, false},
    {CH_U_R030,  30, true , 35, false, false},
    {CH_U_000 ,   0, false, 35, false, false},
    {CH_U_L135, 135, false, 35, false, false},
    {CH_U_R135, 135, true , 35, false, false},
    {CH_U_180 , 180, false, 35, false, false},
    {CH_U_L090,  90, false, 35, false, false},
    {CH_U_R090,  90, true , 35, false, false},
    {CH_T_000 ,   0, false, 90, false, false},
    {CH_LFE2  ,  45, false, 15, true ,  true},
    {CH_L_L045,  45, false, 15, true , false},
    {CH_L_R045,  45, true , 15, true , false},
    {CH_L_000 ,   0, false, 15, true , false},
    {CH_U_L110, 110, false, 35, false, false},
    {CH_U_R110, 110, true , 35, false, false},
    {CH_U_L045,  45, false, 35, false, false},
    {CH_U_R045,  45, true , 35, false, false},
    {CH_M_L045,  45, false,  0, false, false},
    {CH_M_R045,  45, true ,  0, false, false},
    {CH_LFE3  ,  45, true , 15, true ,  true},
    {CH_M_LSCR,   2, false,  0, false, false},
    {CH_M_RSCR,   2, true ,  0, false, false},
    {CH_M_LSCH,   1, false,  0, false, false},
    {CH_M_RSCH,   1, true ,  0, false, false},
    {CH_M_L150, 150, false,  0, false, false},
    {CH_M_R150, 150, true ,  0, false, false},
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mpegh3da::File_Mpegh3da()
:File_Usac()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE

    //In
    MustParse_mhaC=false;
    MustParse_mpegh3daFrame=false;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Fill()
{
    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "MPEG-H 3D Audio");
    Fill(Stream_Audio, 0, Audio_Format_Profile, Mpegh3da_Profile_Get(mpegh3daProfileLevelIndication));
    Fill(Stream_Audio, 0, Audio_SamplingRate, usacSamplingFrequency);
    Fill(Stream_Audio, 0, Audio_SamplesPerFrame, coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex].outputFrameLengthDivided256 << 8);
    Streams_Fill_ChannelLayout(string(), referenceLayout);
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Finish()
{
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Read_Buffer_Continue()
{
    if (MustParse_mhaC)
    {
        mhaC();
        MustParse_mhaC=false;
        MustParse_mpegh3daFrame=true;
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");
        return;
    }
    if (MustParse_mpegh3daFrame)
    {
        mpegh3daFrame();
    }
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Header_Parse()
{
    //Parsing
    int32u MHASPacketType, MHASPacketLabel, MHASPacketLength;
    BS_Begin();
    escapedValue(MHASPacketType, 3, 8, 8,                       "MHASPacketType");
    escapedValue(MHASPacketLabel, 2, 8, 32,                     "MHASPacketLabel");
    escapedValue(MHASPacketLength, 11, 24, 24,                  "MHASPacketLength");
    BS_End();

    FILLING_BEGIN();
        Header_Fill_Code(MHASPacketType, MHASPacketType<Mpegh3da_MHASPacketType_Size?Ztring().From_UTF8(Mpegh3da_MHASPacketType[MHASPacketType]):Ztring().From_CC3(MHASPacketType));
        Header_Fill_Size(Element_Offset+MHASPacketLength);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Data_Parse()
{
    //Parsing
    switch (Element_Code)
    {
        case  1 : mpegh3daConfig(); break;
        case  2 : mpegh3daFrame(); break;
        case  6 : Sync(); break;
        default : Skip_XX(Element_Size-Element_Offset,          "Data");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daConfig()
{
    Element_Begin1("mpegh3daConfig");
    BS_Begin();
    int8u usacSamplingFrequencyIndex;
    Get_S1 (8, mpegh3daProfileLevelIndication,                  "mpegh3daProfileLevelIndication");
    Get_S1 (5, usacSamplingFrequencyIndex,                      "usacSamplingFrequencyIndex");
    if (usacSamplingFrequencyIndex==0x1f)
        Get_S3 (24, usacSamplingFrequency,                      "usacSamplingFrequency");
    else
    {
        if (usacSamplingFrequencyIndex<Aac_sampling_frequency_Size_Usac)
            usacSamplingFrequency=Aac_sampling_frequency[usacSamplingFrequencyIndex];
        else
            usacSamplingFrequency=0;
    }
    Get_S1 (3, coreSbrFrameLengthIndex,                         "coreSbrFrameLengthIndex");
    Skip_SB(                                                    "cfg_reserved");
    Skip_SB(                                                    "receiverDelayCompensation");
    SpeakerConfig3d(referenceLayout);
    /*TODO
    FrameworkConfig3d();
    mpegh3daDecoderConfig();
    TEST_SB_SKIP(                                               "usacConfigExtensionPresent");
        mpegh3daConfigExtension();
    TEST_SB_END();
    */
    BS_End();
    Element_End0();

    FILLING_BEGIN();
        //Filling
        if (!Status[IsAccepted])
            Accept("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::SpeakerConfig3d(speaker_layout& Layout)
{
    int8u speakerLayoutType;
    Element_Begin1("SpeakerConfig3d");
    Get_S1(2, speakerLayoutType,                                "speakerLayoutType");
    if (speakerLayoutType==0)
    {
        Get_S1 (6, Layout.ChannelLayout,                        "CICPspeakerLayoutIdx"); Param_Info2(Aac_Channels_Get(Layout.ChannelLayout), " channels");
    }
    else
    {
        int32u numSpeakers;
        escapedValue(numSpeakers, 5, 8, 16,                     "numSpeakers");
        numSpeakers++;
        Layout.numSpeakers=numSpeakers;

        if (speakerLayoutType==1)
        {
            Layout.CICPspeakerIdxs.resize(numSpeakers);
            for (size_t Pos=0; Pos<numSpeakers; Pos++)
            {
                int8u CICPspeakerIdx;
                Get_S1(7, CICPspeakerIdx,                       "CICPspeakerIdx");
                Layout.CICPspeakerIdxs[Pos]=(Aac_OutputChannel)CICPspeakerIdx;
            }
        }
        else if (speakerLayoutType==2)
        {
            mpegh3daFlexibleSpeakerConfig(Layout);
        }
    }
    Element_End0();

    FILLING_BEGIN();
        //Finish
        if (Status[IsAccepted])
            Finish("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daFlexibleSpeakerConfig(speaker_layout& Layout)
{
    bool angularPrecision;
    Element_Begin1("mpegh3daFlexibleSpeakerConfig");
    Get_SB(angularPrecision,                                    "angularPrecision");
    for (size_t Pos=0; Pos<Layout.numSpeakers; Pos++)
    {
        Layout.SpeakersInfo.push_back(speaker_info());
        speaker_info& SpeakerInfo=Layout.SpeakersInfo[Layout.SpeakersInfo.size()-1];
        mpegh3daSpeakerDescription(SpeakerInfo, angularPrecision);
        if (SpeakerInfo.AzimuthAngle && SpeakerInfo.AzimuthAngle!=180)
        {
            bool alsoAddSymmetricPair;
            Get_SB (alsoAddSymmetricPair,                       "alsoAddSymmetricPair");
            if (alsoAddSymmetricPair)
                Pos++;
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daSpeakerDescription(speaker_info& SpeakerInfo, bool angularPrecision)
{
    Element_Begin1("mpegh3daSpeakerDescription");
    TESTELSE_SB_SKIP(                                           "isCICPspeakerIdx");
    {
        int8u CICPspeakerIdx;
        Get_S1 (7, CICPspeakerIdx,                              "CICPspeakerIdx");
        if (CICPspeakerIdx<Mpegh3da_SpeakerInfo_Size)
            SpeakerInfo=Mpegh3da_SpeakerInfo[CICPspeakerIdx];
        else
            SpeakerInfo.CICPspeakerIdx=(Aac_OutputChannel)CICPspeakerIdx;
    }
    TESTELSE_SB_ELSE(                                           "isCICPspeakerIdx");
        int8u ElevationClass;
        Get_S1(2, ElevationClass,                               "ElevationClass");

        switch (ElevationClass)
        {
        case 0:
            SpeakerInfo.ElevationAngle=0;
            break;
        case 1:
            SpeakerInfo.ElevationAngle=35;
            SpeakerInfo.ElevationDirection=false;
            break;
        case 2:
            SpeakerInfo.ElevationAngle=15;
            SpeakerInfo.ElevationDirection=true;
            break;
        case 3:
            int8u ElevationAngleIdx;
            Get_S1(angularPrecision?7:5, ElevationAngleIdx, "ElevationAngleIdx");
            SpeakerInfo.ElevationAngle=ElevationAngleIdx*(angularPrecision?1:5);

            if (SpeakerInfo.ElevationAngle)
                Get_SB(SpeakerInfo.ElevationDirection,         "ElevationDirection");
            break;
        }

        int8u AzimuthAngleIdx;
        Get_S1(angularPrecision?8:6, AzimuthAngleIdx, "AzimuthAngleIdx");
        SpeakerInfo.AzimuthAngle=AzimuthAngleIdx*(angularPrecision?1:5);

        if (SpeakerInfo.AzimuthAngle && SpeakerInfo.AzimuthAngle!=180)
            Get_SB(SpeakerInfo.AzimuthDirection, "AzimuthDirection");

        Get_SB(SpeakerInfo.isLFE,                "isLFE");

        SpeakerInfo.CICPspeakerIdx=(Aac_OutputChannel)-1;
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daFrame()
{
    Skip_XX(Element_Size,                                       "mpegh3daFrame");

    FILLING_BEGIN();
        //Filling
        if (Status[IsAccepted])
            Finish("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Sync()
{
    //Parsing
    Skip_B1(                                                    "syncword");
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mhaC()
{
    Element_Begin1("MHADecoderConfigurationRecord");
    Skip_B1(                                                    "configurationVersion");
    Skip_B1(                                                    "mpegh3daProfileLevelIndication");
    Skip_B1(                                                    "referenceChannelLayout");
    Skip_B2(                                                    "mpegh3daConfigLength");
    mpegh3daConfig();
    Element_End0();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Fill_ChannelLayout(const string& Prefix, const speaker_layout& Layout, int8u speakerLayoutType)
{
    if (Aac_Channels_Get(Layout.ChannelLayout))
    {
        Fill(Stream_Audio, 0, (Prefix+"Channel(s)").c_str(), Aac_Channels_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix + "Channel(s)").c_str(), "N NTY");
        if (!Prefix.empty())
        {
            string ChannelString=MediaInfoLib::Config.Language_Get(Ztring().From_UTF8(Aac_Channels_GetString(Layout.ChannelLayout)), __T(" channel")).To_UTF8();
            string ChannelMode=Aac_ChannelMode_GetString(Layout.ChannelLayout, true);
            if (ChannelMode.size()>3 || (ChannelMode.size()==3 && ChannelMode[2]!='0'))
                ChannelString+=" ("+Aac_ChannelMode_GetString(Layout.ChannelLayout, true)+")";
            Fill(Stream_Audio, 0, (Prefix+"Channel(s)/String").c_str(), ChannelString);
            Fill_SetOptions(Stream_Audio, 0, (Prefix + "Channel(s)/String").c_str(), "Y NTN");
        }
        Fill(Stream_Audio, 0, (Prefix+"ChannelPositions").c_str(), Aac_ChannelConfiguration_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelPositions").c_str(), "N YTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelPositions/String2").c_str(), Aac_ChannelConfiguration2_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelPositions/String2").c_str(), "N YTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), Aac_ChannelMode_GetString(Layout.ChannelLayout, true));
        Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), "N NTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Aac_ChannelLayout_GetString(Layout.ChannelLayout, true));
    }
    else if (Layout.numSpeakers)
    {
        if (speakerLayoutType==1) // Objects
        {
            Fill(Stream_Audio, 0, (Prefix+"NumberOfObjects").c_str(), Layout.numSpeakers);
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"NumberOfObjects").c_str(), "N YTY");
            Fill(Stream_Audio, 0, (Prefix + "NumberOfObjects/String").c_str(), MediaInfoLib::Config.Language_Get(Ztring::ToZtring(Layout.numSpeakers), __T(" object")));
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"NumberOfObjects/String").c_str(), "Y YTN");
        }
        else
        {
            Fill(Stream_Audio, 0, (Prefix+"Channel(s)").c_str(), Layout.numSpeakers);
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"Channel(s)").c_str(), "N YTY");
            Fill(Stream_Audio, 0, (Prefix + "Channel(s)/String").c_str(), MediaInfoLib::Config.Language_Get(Ztring::ToZtring(Layout.numSpeakers), __T(" channel")));
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"Channel(s)/String").c_str(), "Y YTN");
        }
        if (!Layout.CICPspeakerIdxs.empty())
        {
            Fill(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), Aac_ChannelMode_GetString(Layout.CICPspeakerIdxs));
            Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Aac_ChannelLayout_GetString(Layout.CICPspeakerIdxs));
        }
        else
        {
            vector<Aac_OutputChannel> CICPspeakerIdxs;
            string ChannelLayout;
            for (size_t i=0; i<Layout.SpeakersInfo.size(); i++)
            {
                if (i)
                    ChannelLayout+=' ';
                if (Layout.SpeakersInfo[i].CICPspeakerIdx!=(Aac_OutputChannel)-1)
                {
                    ChannelLayout+=Aac_ChannelLayout_GetString(&Layout.SpeakersInfo[i].CICPspeakerIdx, 1);
                    CICPspeakerIdxs.push_back(Layout.SpeakersInfo[i].CICPspeakerIdx);
                }
                else
                {
                    if (Layout.SpeakersInfo[i].ElevationAngle==0)
                        ChannelLayout+='M';
                    else
                        ChannelLayout+=Layout.SpeakersInfo[i].ElevationDirection?'B':'U';
                    ChannelLayout+='_';
                    if (Layout.SpeakersInfo[i].AzimuthAngle!=0 && Layout.SpeakersInfo[i].AzimuthAngle!=180)
                        ChannelLayout+=Layout.SpeakersInfo[i].AzimuthDirection?'L':'R';
                    string AzimuthAngleString=Ztring::ToZtring(Layout.SpeakersInfo[i].AzimuthAngle).To_UTF8();
                    AzimuthAngleString.insert(0, 3-AzimuthAngleString.size(), '0');
                    ChannelLayout.append(AzimuthAngleString);
                }
            }
            if (CICPspeakerIdxs.size()==Layout.SpeakersInfo.size())
            {
                Fill(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), Aac_ChannelMode_GetString(CICPspeakerIdxs));
                Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Aac_ChannelLayout_GetString(CICPspeakerIdxs));
            }
            else
                Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), ChannelLayout);
        }
    }
    else if (Layout.ChannelLayout)
    {
        Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Layout.ChannelLayout);
    }
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_MPEGH3DA_YES

