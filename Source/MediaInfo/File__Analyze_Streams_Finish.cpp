/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Init and Finalize part
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#include "ZenLib/Utils.h"
#if defined(MEDIAINFO_FILE_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_FILE_YES)
#include "ZenLib/FileName.h"
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/TimeCode.h"
#include "MediaInfo/ExternalCommandHelpers.h"
#if MEDIAINFO_IBI
    #include "MediaInfo/Multiple/File_Ibi.h"
#endif //MEDIAINFO_IBI
#if MEDIAINFO_FIXITY
    #ifndef WINDOWS
    //ZenLib has File::Copy only for Windows for the moment. //TODO: support correctly (including meta)
    #include <fstream>
    #endif //WINDOWS
#endif //MEDIAINFO_FIXITY
#include <algorithm>
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
static const char* CompanySuffixes[] = {
    "AG",
    "CAMERA",
    "CO.",
    "COMPANY",
    "CORP.",
    "CORPORATION",
    "ELECTRONICS",
    "IMAGING",
    "INC.",
    "LABORATORIES",
    "LTD",
    "LTD.",
    "OP",
    "TECHNOLOGIES",
    "TECHNOLOGY",
};

//---------------------------------------------------------------------------
static const char* CompanyNames[] = {
    "ASHAMPOO",
    "APPLE",
    "CANON",
    "EPSON",
    "FRAUNHOFER",
    "FUJIFILM",
    "KODAK",
    "LEICA",
    "HP",
    "HUAWEI",
    "MICROSOFT",
    "NIKON",
    "OLYMPUS",
    "MAMIYA",
    "NIKON",
    "PENTAX",
    "RICOH",
    "SAMSUNG",
    "SONY",
    "ZJMEDIA",
};

//---------------------------------------------------------------------------
struct FindReplace_struct {
    const char* Find;
    const char* ReplaceBy;
};
static const FindReplace_struct CompanyNames_Replace[] = {
    { "EASTMAN KODAK", "KODAK" },
    { "OC", "OpenCube" },
    { "SEIKO EPSON", "EPSON" },
    { "SAMSUNG DIGITAL IMA", "SAMSUNG" },
    { "Thomson Grass Valley", "Grass Valley" },
};

//---------------------------------------------------------------------------
static const FindReplace_struct Model_Replace_Canon[] = { // Based on https://en.wikipedia.org/wiki/Canon_EOS and https://wiki.magiclantern.fm/camera_models_map
    { "EOS 1500D", "EOS 2000D" },
    { "EOS 200D Mark II", "EOS 250D" },
    { "EOS 3000D", "EOS 4000D" },
    { "EOS 770D", "EOS 77D" },
    { "EOS 8000D", "EOS 760D" },
    { "EOS 9000D", "EOS 77D" },
    { "EOS DIGITAL REBEL XT", "EOS 350D" },
    { "EOS DIGITAL REBEL XTI", "EOS 400D" },
    { "EOS DIGITAL REBEL", "EOS 300D" },
    { "EOS HI", "EOS 1200D" },
    { "EOS KISS DIGITAL N", "EOS 350D" },
    { "EOS KISS DIGITAL X", "EOS 400D" },
    { "EOS KISS DIGITAL", "EOS 300D" },
    { "EOS KISS F", "EOS 1000D" },
    { "EOS KISS M", "EOS M50" },
    { "EOS KISS M2", "EOS M50 Mark II" },
    { "EOS KISS M50m2", "EOS M50 Mark II" },
    { "EOS KISS X10", "EOS 250D" },
    { "EOS KISS X10I", "EOS 850D" },
    { "EOS KISS X2", "EOS 450D" },
    { "EOS KISS X3", "EOS 500D" },
    { "EOS KISS X4", "EOS 550D" },
    { "EOS KISS X5", "EOS 600D" },
    { "EOS KISS X50", "EOS 1100D" },
    { "EOS KISS X6I", "EOS 650D" },
    { "EOS KISS X7", "EOS 100D" },
    { "EOS KISS X70", "EOS 1200D" },
    { "EOS KISS X7I", "EOS 700D" },
    { "EOS KISS X80", "EOS 1300D" },
    { "EOS KISS X8I", "EOS 750D" },
    { "EOS KISS X9", "EOS 200D" },
    { "EOS KISS X90", "EOS 2000D" },
    { "EOS KISS X9I", "EOS 800D" },
    { "EOS REBEL SL1", "EOS 100D" },
    { "EOS REBEL SL2", "EOS 200D" },
    { "EOS REBEL SL3", "EOS 250D" },
    { "EOS REBEL T100", "EOS 4000D" },
    { "EOS REBEL T1I", "EOS 500D" },
    { "EOS REBEL T2I", "EOS 550D" },
    { "EOS REBEL T3", "EOS 1100D" },
    { "EOS REBEL T3I", "EOS 600D" },
    { "EOS REBEL T4I", "EOS 650D" },
    { "EOS REBEL T5", "EOS 1200D" },
    { "EOS REBEL T5I", "EOS 700D" },
    { "EOS REBEL T6", "EOS 1300D" },
    { "EOS REBEL T6I", "EOS 750D" },
    { "EOS REBEL T6S", "EOS 760D" },
    { "EOS REBEL T7", "EOS 2000D" },
    { "EOS REBEL T7I", "EOS 800D" },
    { "EOS REBEL T8I", "EOS 850D" },
    { "EOS REBEL XS", "EOS 1000D" },
    { "EOS REBEL XSI", "EOS 450D" },
};
static const FindReplace_struct Model_Replace_OpenCube[] = {
    { "OCTk", "Toolkit" },
    { "Tk", "Toolkit" },
};
struct FindReplaceCompany_struct {
    const char* CompanyName;
    const FindReplace_struct* Find;
    size_t Size;
};
static const FindReplaceCompany_struct Model_Replace[] = {
    { "Canon", Model_Replace_Canon, sizeof(Model_Replace_Canon) / sizeof(Model_Replace_Canon[0])},
    { "OpenCube", Model_Replace_OpenCube, sizeof(Model_Replace_OpenCube) / sizeof(Model_Replace_OpenCube[0])},
};

// Sony Xperia model name mappings
static const FindReplace_struct Model_Name_Sony[] = {
    { "XQ-FS44", "Xperia 1 VII" },
    { "XQ-FS54", "Xperia 1 VII" },
    { "XQ-FS72", "Xperia 1 VII" },
    { "XQ-EC44", "Xperia 1 VI" },
    { "XQ-EC54", "Xperia 1 VI" },
    { "XQ-EC72", "Xperia 1 VI" },
    { "XQ-ES44", "Xperia 10 VI" },
    { "XQ-ES54", "Xperia 10 VI" },
    { "XQ-ES72", "Xperia 10 VI" },
    { "XQ-DE44", "Xperia 5 V" },
    { "XQ-DE54", "Xperia 5 V" },
    { "XQ-DE72", "Xperia 5 V" },
    { "XQ-DQ44", "Xperia 1 V" },
    { "XQ-DQ54", "Xperia 1 V" },
    { "XQ-DQ62", "Xperia 1 V" },
    { "XQ-DQ72", "Xperia 1 V" },
    { "XQ-DC44", "Xperia 10 V" },
    { "XQ-DC54", "Xperia 10 V" },
    { "XQ-DC72", "Xperia 10 V" },
    { "XQ-CQ44", "Xperia 5 IV" },
    { "XQ-CQ54", "Xperia 5 IV" },
    { "XQ-CQ62", "Xperia 5 IV" },
    { "XQ-CQ72", "Xperia 5 IV" },
    { "XQ-CT44", "Xperia 1 IV" },
    { "XQ-CT54", "Xperia 1 IV" },
    { "XQ-CT62", "Xperia 1 IV" },
    { "XQ-CT72", "Xperia 1 IV" },
    { "XQ-CC44", "Xperia 10 IV" },
    { "XQ-CC54", "Xperia 10 IV" },
    { "XQ-CC72", "Xperia 10 IV" },
    { "XQ-BE42", "Xperia PRO-I" },
    { "XQ-BE52", "Xperia PRO-I" },
    { "XQ-BE62", "Xperia PRO-I" },
    { "XQ-BE72", "Xperia PRO-I" },
    { "XQ-BT44", "Xperia 10 III Lite" },
    { "XQ-BC42", "Xperia 1 IV" },
    { "XQ-BC52", "Xperia 1 IV" },
    { "XQ-BC62", "Xperia 1 IV" },
    { "XQ-BC72", "Xperia 1 IV" },
    { "XQ-BQ42", "Xperia 5 III" },
    { "XQ-BQ52", "Xperia 5 III" },
    { "XQ-BQ62", "Xperia 5 III" },
    { "XQ-BQ72", "Xperia 5 III" },
    { "XQ-BT52", "Xperia 10 III" },
    { "XQ-AQ52", "Xperia PRO" },
    { "XQ-AQ62", "Xperia PRO" },
    { "XQ-AS42", "Xperia 5 II" },
    { "XQ-AS52", "Xperia 5 II" },
    { "XQ-AS62", "Xperia 5 II" },
    { "XQ-AS72", "Xperia 5 II" },
    { "XQ-AT42", "Xperia 1 II" },
    { "XQ-AT51", "Xperia 1 II" },
    { "XQ-AT52", "Xperia 1 II" },
    { "XQ-AU42", "Xperia 10 II" },
    { "XQ-AU51", "Xperia 10 II" },
    { "XQ-AU52", "Xperia 10 II" },
    { "J8210", "Xperia 5" },
    { "J8270", "Xperia 5" },
    { "J9210", "Xperia 5 Dual" },
    { "J9260", "Xperia 5 Dual" },
    { "J8110", "Xperia 1" },
    { "J8170", "Xperia 1" },
    { "J9180", "Xperia 1" },
    { "J9110", "Xperia 1 Dual" },
    { "H8416", "Xperia XZ3" },
    { "H9436", "Xperia XZ3" },
    { "H9493", "Xperia XZ3" },
    { "H8216", "Xperia XZ2" },
    { "H8276", "Xperia XZ2" },
    { "H8314", "Xperia XZ2 Compact" },
    { "H8324", "Xperia XZ2 Compact" },
    { "H8266", "Xperia XZ2 Dual" },
    { "H8296", "Xperia XZ2 Dual" },
    { "H8116", "Xperia XZ2 Premium" },
    { "H8166", "Xperia XZ2 Premium Dual" },
    { "G8341", "Xperia XZ1" },
    { "G8343", "Xperia XZ1" },
    { "G8441", "Xperia XZ1 Compact" },
    { "G8342", "Xperia XZ1 Dual" },
    { "F8331", "Xperia XZ" },
    { "F8332", "Xperia XZ" },
    { "G8141", "Xperia XZ Premium" },
    { "G8188", "Xperia XZ Premium" },
    { "G8142", "Xperia XZ Premium Dual" },
    { "G8231", "Xperia XZs" },
    { "G8232", "Xperia XZs Dual" },
    { "E6603", "Xperia Z5" },
    { "E6653", "Xperia Z5" },
    { "E5803", "Xperia Z5 Compact" },
    { "E5823", "Xperia Z5 Compact" },
    { "E6633", "Xperia Z5 Dual" },
    { "E6683", "Xperia Z5 Dual" },
    { "E6853", "Xperia Z5 Premium" },
    { "E6833", "Xperia Z5 Premium Dual" },
    { "E6883", "Xperia Z5 Premium Dual" },
    { "SGP712", "Xperia Z4 Tablet" },
    { "SGP771", "Xperia Z4 Tablet" },
    { "E6553", "Xperia Z3+" },
    { "E6533", "Xperia Z3+ Dual" },
    { "D6603", "Xperia Z3" },
    { "D6616", "Xperia Z3" },
    { "D6633", "Xperia Z3" },
    { "D6643", "Xperia Z3" },
    { "D6646", "Xperia Z3" },
    { "D6653", "Xperia Z3" },
    { "D5803", "Xperia Z3 Compact" },
    { "D5833", "Xperia Z3 Compact" },
    { "D6683", "Xperia Z3 Dual TD" },
    { "D6708", "Xperia Z3v" },
    { "SGP611", "Xperia Z3 Tablet Compact" },
    { "SGP612", "Xperia Z3 Tablet Compact" },
    { "SGP621", "Xperia Z3 Tablet Compact" },
    { "SGP641", "Xperia Z3 Tablet Compact" },
    { "D6502", "Xperia Z2" },
    { "D6503", "Xperia Z2" },
    { "D6543", "Xperia Z2" },
    { "D6563", "Xperia Z2a" },
    { "SGP511", "Xperia Z2 Tablet" },
    { "SGP512", "Xperia Z2 Tablet" },
    { "SGP521", "Xperia Z2 Tablet" },
    { "SGP541", "Xperia Z2 Tablet" },
    { "SGP551", "Xperia Z2 Tablet" },
    { "SGP561", "Xperia Z2 Tablet" },
    { "C6902", "Xperia Z1" },
    { "C6903", "Xperia Z1" },
    { "C6906", "Xperia Z1" },
    { "C6943", "Xperia Z1" },
    { "C6916", "Xperia Z1s" },
    { "D5503", "Xperia Z1 Compact" },
    { "C6802", "Xperia Z Ultra" },
    { "C6806", "Xperia Z Ultra" },
    { "C6833", "Xperia Z Ultra" },
    { "C6843", "Xperia Z Ultra" },
    { "SGP412", "Xperia Z Ultra" },
    { "C5502", "Xperia ZR" },
    { "C5503", "Xperia ZR" },
    { "C5302", "Xperia SP" },
    { "C5303", "Xperia SP" },
    { "C5306", "Xperia SP" },
    { "C6602", "Xperia Z" },
    { "C6603", "Xperia Z" },
    { "C6606", "Xperia Z" },
    { "C6616", "Xperia Z" },
    { "C6502", "Xperia ZL" },
    { "C6503", "Xperia ZL" },
    { "C6506", "Xperia ZL" },
};
static const FindReplace_struct Model_Name_Sony_Ericsson[] = {
    { "SGPT121", "Xperia Tablet S" },
    { "SGPT122", "Xperia Tablet S" },
    { "SGPT123", "Xperia Tablet S" },
    { "SGPT131", "Xperia Tablet S 3G" },
    { "SGPT132", "Xperia Tablet S 3G" },
    { "SGPT133", "Xperia Tablet S 3G" },
    { "LT26ii", "Xperia SL" },
    { "LT26w", "Xperia acro S" },
    { "LT26i", "Xperia S" },
};
static const FindReplaceCompany_struct Model_Name[] = {
    { "Sony", Model_Name_Sony, sizeof(Model_Name_Sony) / sizeof(Model_Name_Sony[0])},
    { "Sony Ericsson", Model_Name_Sony_Ericsson, sizeof(Model_Name_Sony_Ericsson) / sizeof(Model_Name_Sony_Ericsson[0])},
};

//---------------------------------------------------------------------------
static const char* VersionPrefixes[] = {
    ", VERSION: ",
    "FILE VERSION",
    "RELEASE",
    "V",
    "VER",
    "VERSION",
};
   
//---------------------------------------------------------------------------
extern MediaInfo_Config Config;
extern size_t DolbyVision_Compatibility_Pos(const Ztring& Value);
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
Ztring File__Analyze_Encoded_Library_String (const Ztring &CompanyName, const Ztring &Name, const Ztring &Version, const Ztring &Date, const Ztring &Encoded_Library)
{
    if (!Name.empty())
    {
        Ztring String;
        if (!CompanyName.empty())
        {
            String+=CompanyName;
            String+=__T(" ");
        }
        String+=Name;
        if (!Version.empty())
        {
            String+=__T(" ");
            String+=Version;
        }
        if (!Date.empty())
        {
            String+=__T(" (");
            String+=Date;
            String+=__T(")");
        }
        return String;
    }
    else
        return Encoded_Library;
}

//---------------------------------------------------------------------------
void Merge_FillTimeCode(File__Analyze& In, const string& Prefix, const TimeCode& TC_Time, float FramesPerSecondF, bool DropFrame, TimeCode::rounding Rounding=TimeCode::Nearest, int32u Frequency=0)
{
    if (!TC_Time.IsSet())
        return;
    auto FramesPerSecondI=float32_int32s(FramesPerSecondF);
    TimeCode TC_Frames;
    string TC_WithExtraSamples_String;
    if (FramesPerSecondI)
    {
        TC_Frames=TC_Time.ToRescaled(FramesPerSecondI-1, TimeCode::DropFrame(DropFrame).FPS1001(FramesPerSecondI!=FramesPerSecondF), Rounding);
        bool IsRounded=false;
        if (Rounding==TimeCode::Floor)
        {
            //Handling TC_Time rounding issue
            TimeCode TC_Frames1=TC_Frames+1;
            TimeCode TC_Frames1_InTime=TC_Frames1.ToRescaled(TC_Time.GetFramesMax(), TimeCode::flags(), TimeCode::Floor);
            if (TC_Time==TC_Frames1_InTime)
            {
                TC_Frames=TC_Frames1;
                IsRounded=true;
            }
        }
        if (Rounding==TimeCode::Ceil)
        {
            //Handling TC_Time rounding issue
            TimeCode TC_Frames1=TC_Frames-1;
            TimeCode TC_Frames1_InTime=TC_Frames1.ToRescaled(TC_Time.GetFramesMax(), TimeCode::flags(), TimeCode::Ceil);
            if (TC_Time==TC_Frames1_InTime)
            {
                TC_Frames=TC_Frames1;
                IsRounded=true;
            }
        }
        TC_WithExtraSamples_String=TC_Frames.ToString();

        if (Frequency)
        {
            // With samples
            int64_t Samples;
            if (IsRounded)
                Samples=0;
            else
            {
                TimeCode TC_Frames_InSamples=TC_Frames.ToRescaled(Frequency-1, TimeCode::flags(), Rounding);
                TimeCode TC_Time_Samples=TC_Time.ToRescaled(Frequency-1, TimeCode::flags(), Rounding);
                TimeCode TC_ExtraSamples;
                if (Rounding==TimeCode::Ceil)
                    TC_ExtraSamples=TC_Frames_InSamples-TC_Time_Samples;
                else
                    TC_ExtraSamples=TC_Time_Samples-TC_Frames_InSamples;
                Samples=TC_ExtraSamples.ToFrames();
            }
            if (Samples)
            {
                if (Samples>=0)
                    TC_WithExtraSamples_String+=Rounding==TimeCode::Ceil?'-':'+';
                TC_WithExtraSamples_String+=std::to_string(Samples);
                TC_WithExtraSamples_String+="samples";
            }
        }
    }

    if (Prefix.find("TimeCode")!=string::npos)
    {
        In.Fill(Stream_Audio, 0, Prefix.c_str(), TC_WithExtraSamples_String, true, true);
        return;
    }

    string TC_WithExtraSubFrames_String;
    if (FramesPerSecondI)
    {
        // With subframes
        constexpr TimeCode::rounding TC_Frames_Sub_Rounding=TimeCode::Ceil;
        TimeCode TC_Frames_Sub=TC_Time.ToRescaled(FramesPerSecondI*100-1, TimeCode::DropFrame(DropFrame).FPS1001(FramesPerSecondI!=FramesPerSecondF), TC_Frames_Sub_Rounding);
        bool IsRounded=false;
        if (TC_Frames_Sub_Rounding==TimeCode::Floor)
        {
            //Handling TC_Time rounding issue
            TimeCode TC_Frames_Sub1=TC_Frames_Sub+1;
            TimeCode TC_Frames_Sub1_InTime=TC_Frames_Sub1.ToRescaled(TC_Time.GetFramesMax(), TimeCode::flags(), TimeCode::Floor);
            if (TC_Time==TC_Frames_Sub1_InTime)
            {
                TC_Frames_Sub=TC_Frames_Sub1;
                IsRounded=true;
            }
        }
        if (TC_Frames_Sub_Rounding==TimeCode::Ceil)
        {
            //Handling TC_Time rounding issue
            TimeCode TC_Frames_Sub1=TC_Frames_Sub-1;
            TimeCode TC_Frames_Sub1_InTime=TC_Frames_Sub1.ToRescaled(TC_Time.GetFramesMax(), TimeCode::flags(), TimeCode::Ceil);
            if (TC_Time==TC_Frames_Sub1_InTime)
            {
                TC_Frames_Sub=TC_Frames_Sub1;
                IsRounded=true;
            }
        }
        int64_t SubFrames=TC_Frames_Sub.ToFrames();
        int64_t SubFrames_Main=SubFrames/100;
        int64_t SubFrames_Part=SubFrames%100;
        TimeCode TC_Frames_Sub_Main=TimeCode(SubFrames_Main, FramesPerSecondI-1, TimeCode::DropFrame(DropFrame).FPS1001(FramesPerSecondI!=FramesPerSecondF));
        TC_WithExtraSubFrames_String=TC_Frames_Sub_Main.ToString();
        if (SubFrames_Part)
        {
            TC_WithExtraSubFrames_String+='.';
            auto Temp=std::to_string(SubFrames_Part);
            if (Temp.size()==1)
                Temp.insert(0, 1, '0');
            TC_WithExtraSubFrames_String+=Temp;
        }
    }
    
    In.Fill(Stream_Audio, 0, Prefix.c_str(), TC_Time.ToString(), true, true);
    In.Fill_SetOptions(Stream_Audio, 0, Prefix.c_str(), "N NTY");
    string ForDisplay;
    if (Prefix=="Dolby_Atmos_Metadata FirstFrameOfAction")
        ForDisplay=TC_Frames.ToString();
    else if (Prefix.find(" Start")+6==Prefix.size() || Prefix.find(" End")+4==Prefix.size())
        ForDisplay=TC_WithExtraSubFrames_String;
    else
        ForDisplay=TC_WithExtraSamples_String;
    In.Fill(Stream_Audio, 0, (Prefix+"/String").c_str(), TC_Time.ToString()+(ForDisplay.empty()?string():(" ("+ForDisplay+')')), true, true);
    In.Fill_SetOptions(Stream_Audio, 0, (Prefix+"/String").c_str(), "Y NTN");
    In.Fill(Stream_Audio, 0, (Prefix+"/TimeCode").c_str(), TC_Frames.ToString(), true, true);
    if (TC_Frames.IsValid())
        In.Fill_SetOptions(Stream_Audio, 0, (Prefix+"/TimeCode").c_str(), "N NTN");
    In.Fill(Stream_Audio, 0, (Prefix+"/TimeCodeSubFrames").c_str(), TC_WithExtraSubFrames_String, true, true);
    if (!TC_WithExtraSubFrames_String.empty())
        In.Fill_SetOptions(Stream_Audio, 0, (Prefix+"/TimeCodeSubFrames").c_str(), "N NTN");
    In.Fill(Stream_Audio, 0, (Prefix+"/TimeCodeSamples").c_str(), Frequency?TC_WithExtraSamples_String:string(), true, true);
    if (Frequency && !TC_WithExtraSamples_String.empty())
        In.Fill_SetOptions(Stream_Audio, 0, (Prefix+"/TimeCodeSamples").c_str(), "N NTN");
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_Global()
{
    if (IsSub)
        return;

    TestDirectory();

    #if MEDIAINFO_ADVANCED
        if (MediaInfoLib::Config.ExternalMetaDataConfig_Get().empty()) // ExternalMetadata is used directly only if there is no ExternalMetadata config (=another format)
        {
            Ztring ExternalMetadata=MediaInfoLib::Config.ExternalMetadata_Get();
            if (!ExternalMetadata.empty())
            {
                ZtringListList List;
                List.Separator_Set(0, MediaInfoLib::Config.LineSeparator_Get());
                List.Separator_Set(1, __T(";"));
                List.Write(ExternalMetadata);

                for (size_t i=0; i<List.size(); i++)
                {
                    // col 1&2 can be removed, conidered as "General;0"
                    // 1: stream kind (General, Video, Audio, Text...)
                    // 2: 0-based stream number
                    // 3: field name
                    // 4: field value
                    // 5 (optional): replace instead of ignoring if field is already present (metadata from the file)
                    if (List[i].size()<2 || List[i].size()>5)
                    {
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, "Invalid column size for external metadata");
                        continue;
                    }

                    Ztring StreamKindZ=Ztring(List[i][0]).MakeLowerCase();
                    stream_t StreamKind;
                    size_t   Offset;
                    if (List[i].size()<4)
                    {
                        StreamKind=Stream_General;
                        Offset=2;
                    }
                    else
                    {
                        Offset=0;
                             if (StreamKindZ==__T("general"))   StreamKind=Stream_General;
                        else if (StreamKindZ==__T("video"))     StreamKind=Stream_Video;
                        else if (StreamKindZ==__T("audio"))     StreamKind=Stream_Audio;
                        else if (StreamKindZ==__T("text"))      StreamKind=Stream_Text;
                        else if (StreamKindZ==__T("other"))     StreamKind=Stream_Other;
                        else if (StreamKindZ==__T("image"))     StreamKind=Stream_Image;
                        else if (StreamKindZ==__T("menu"))      StreamKind=Stream_Menu;
                        else
                        {
                            MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, "Invalid column 0 for external metadata");
                            continue;
                        }
                    }
                    size_t StreamPos=(size_t)List[i][1].To_int64u();
                    bool ShouldReplace=List[i].size()>4-Offset && List[i][4-Offset].To_int64u();
                    if (ShouldReplace || Retrieve_Const(StreamKind, StreamPos, List[i][2-Offset].To_UTF8().c_str()).empty())
                        Fill(StreamKind, StreamPos, List[i][2-Offset].To_UTF8().c_str(), List[i][3-Offset], ShouldReplace);
                }
            }
        }
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
        //Default frame rate
        if (Count_Get(Stream_Video)==1 && Retrieve(Stream_Video, 0, Video_FrameRate).empty() && Config->File_DefaultFrameRate_Get())
            Fill(Stream_Video, 0, Video_FrameRate, Config->File_DefaultFrameRate_Get());
    #endif //MEDIAINFO_ADVANCED

    //Video Frame count
    if (Count_Get(Stream_Video)==1 && Count_Get(Stream_Audio)==0 && Retrieve(Stream_Video, 0, Video_FrameCount).empty())
    {
        if (Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
            Fill(Stream_Video, 0, Video_FrameCount, Frame_Count_NotParsedIncluded);
        else if (Config->File_Names.size()>1 && StreamSource==IsStream)
            Fill(Stream_Video, 0, Video_FrameCount, Config->File_Names.size());
        #if MEDIAINFO_IBIUSAGE
        else
        {
            //External IBI
            std::string IbiFile=Config->Ibi_Get();
            if (!IbiFile.empty())
            {
                if (IbiStream)
                    IbiStream->Infos.clear(); //TODO: support IBI data from different inputs
                else
                    IbiStream=new ibi::stream;

                File_Ibi MI;
                Open_Buffer_Init(&MI, IbiFile.size());
                MI.Ibi=new ibi;
                MI.Open_Buffer_Continue((const int8u*)IbiFile.c_str(), IbiFile.size());
                if (!MI.Ibi->Streams.empty())
                    (*IbiStream)=(*MI.Ibi->Streams.begin()->second);
            }

            if (IbiStream && !IbiStream->Infos.empty() && IbiStream->Infos[IbiStream->Infos.size()-1].IsContinuous && IbiStream->Infos[IbiStream->Infos.size()-1].FrameNumber!=(int64u)-1)
                Fill(Stream_Video, 0, Video_FrameCount, IbiStream->Infos[IbiStream->Infos.size()-1].FrameNumber);
        }
        #endif //MEDIAINFO_IBIUSAGE
    }

    //Exception
    if (Retrieve(Stream_General, 0, General_Format)==__T("AC-3") && (Retrieve(Stream_General, 0, General_Format_Profile).find(__T("E-AC-3"))==0 || Retrieve(Stream_General, 0, General_Format_AdditionalFeatures).find(__T("Dep"))!=string::npos))
    {
        //Using AC-3 extensions + E-AC-3 extensions + "eb3" specific extension
        Ztring Extensions=Retrieve(Stream_General, 0, General_Format_Extensions);
        if (Extensions.find(__T(" eb3"))==string::npos)
        {
            Extensions+=__T(' ');
            Extensions+=MediaInfoLib::Config.Format_Get(__T("E-AC-3"), InfoFormat_Extensions);
            Extensions+=__T(" eb3");
            Fill(Stream_General, 0, General_Format_Extensions, Extensions, true);
            if (MediaInfoLib::Config.Legacy_Get())
                Fill(Stream_General, 0, General_Codec_Extensions, Extensions, true);
        }
    }

    #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
        // Cropped
        if (Count_Get(Stream_Video)+Count_Get(Stream_Image) && MediaInfoLib::Config.Enable_FFmpeg_Get())
        {
            Ztring Command=External_Command_Exists(ffmpeg_PossibleNames);;
            if (!Command.empty())
            {
                auto StreamKind=Count_Get(Stream_Video)?Stream_Video:Stream_Image;
                {
                    const auto StreamCount=Count_Get(StreamKind);
                    for (size_t StreamPos=0; StreamPos<StreamCount; StreamPos++)
                    {
                        ZtringList Arguments;
                        if (StreamKind==Stream_Video)
                        {
                            Arguments.push_back(__T("-noaccurate_seek"));
                            Arguments.push_back(__T("-ss"));
                            Arguments.push_back(Ztring::ToZtring(Retrieve_Const(Stream_Video, 0, Video_Duration).To_int64u()/2000));
                        }
                        Arguments.push_back(__T("-i"));
                        Arguments.push_back(Retrieve_Const(Stream_General, 0, General_CompleteName));
                        if (StreamKind==Stream_Video)
                        {
                            Arguments.push_back(__T("-map"));
                            Arguments.push_back(__T("v:")+Ztring::ToZtring(StreamPos));
                        }
                        Arguments.push_back(__T("-vf"));
                        Arguments.push_back(__T("cropdetect=skip=0:round=1"));
                        if (StreamKind==Stream_Video)
                        {
                            Arguments.back().insert(0, __T("select='eq(pict_type,I)',"));
                            Arguments.push_back(__T("-copyts"));
                            Arguments.push_back(__T("-vframes"));
                            Arguments.push_back(__T("4"));
                        }
                        Arguments.push_back(__T("-f"));
                        Arguments.push_back(__T("null"));
                        Arguments.push_back(__T("-"));
                        Ztring Err;
                        External_Command_Run(Command, Arguments, nullptr, &Err);
                        auto Pos_Start=Err.rfind(__T("[Parsed_cropdetect_"));
                        if (Pos_Start!=string::npos)
                        {
                            auto Pos_End=Err.find(__T('\n'), Pos_Start);
                            if (Pos_End==string::npos)
                                Pos_End=Err.size();
                            Ztring Crop_Line=Err.substr(Pos_Start, Pos_End-Pos_Start);
                            ZtringList Crop;
                            Crop.Separator_Set(0, __T(" "));
                            Crop.Write(Crop_Line);
                            int32u Values[6];
                            memset(Values, -1, sizeof(Values));
                            int32u Width=Retrieve_Const(StreamKind, StreamPos, "Width").To_int32u();
                            int32u Height=Retrieve_Const(StreamKind, StreamPos, "Height").To_int32u();
                            for (const auto& Item : Crop)
                            {
                                if (Item.size()<=3
                                 || Item[0]<__T('x') || Item[0]>__T('y')
                                 || Item[1]<__T('1') || Item[1]>__T('2')
                                 || Item[2]!=__T(':')
                                 || Item[3]<__T('0') || Item[3]>__T('9'))
                                    continue;
                                Values[((Item[0]-__T('x'))<<1)|(Item[1]-__T('1'))]=Ztring(Item.substr(3)).To_int32u();
                            }
                            if (Values[0]!=(int32u)-1 && Values[1]!=(int32u)-1)
                            {
                                Values[4]=Values[1]-Values[0];
                                if (((int32s)Values[4])>=0)
                                {
                                    Values[4]++;
                                    if (Values[4]!=Width)
                                        Fill(StreamKind, StreamPos, "Active_Width", Values[4]);
                                }
                            }
                            if (Values[2]!=(int32u)-1 && Values[3]!=(int32u)-1)
                            {
                                Values[5]=Values[3]-=Values[2];
                                if (((int32s)Values[5])>=0)
                                {
                                    Values[5]++;
                                    if (Values[5]!=Height)
                                        Fill(StreamKind, StreamPos, "Active_Height", Values[5]);
                                }
                            }
                            if (((int32s)Values[4])>=0 && ((int32s)Values[5])>=0 && (Values[4]!=Width || Values[5]!=Height))
                            {
                                float32 PAR=Retrieve_Const(StreamKind, 0, "PixelAspectRatio").To_float32();
                                if (PAR)
                                    Fill(StreamKind, StreamPos, "Active_DisplayAspectRatio", ((float32)Values[4])/Values[5]*PAR, 2);
                            }
                        }
                    }
                }
            }
        }
    #endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)

    Streams_Finish_StreamOnly();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();
    Streams_Finish_StreamOnly_General_Curate(0);

    Config->File_ExpandSubs_Update((void**)(&Stream_More));

    if (!IsSub && !Config->File_IsReferenced_Get() && MediaInfoLib::Config.ReadByHuman_Get())
        Streams_Finish_HumanReadable();
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_FILE_YES)
void File__Analyze::TestContinuousFileNames(size_t CountOfFiles, Ztring FileExtension, bool SkipComputeDelay)
{
    if (IsSub || !Config->File_TestContinuousFileNames_Get())
        return;

    size_t Pos=Config->File_Names.size();
    if (!Pos)
        return;

    //Trying to detect continuous file names (e.g. video stream as an image or HLS)
    size_t Pos_Base = (size_t)-1;
    bool AlreadyPresent=Config->File_Names.size()==1?true:false;
    FileName FileToTest(Config->File_Names.Read(Config->File_Names.size()-1));
    #ifdef WIN32
        FileToTest.FindAndReplace(__T("/"), __T("\\"), 0, Ztring_Recursive); // "/" is sometimes used on Windows and it is considered as valid
    #endif //WIN32
    Ztring FileToTest_Name=FileToTest.Name_Get();
    Ztring FileToTest_Name_After=FileToTest_Name;
    size_t FileNameToTest_End=FileToTest_Name.size();
    while (FileNameToTest_End && !(FileToTest_Name[FileNameToTest_End-1]>=__T('0') && FileToTest_Name[FileNameToTest_End-1]<=__T('9')))
        FileNameToTest_End--;
    size_t FileNameToTest_Pos=FileNameToTest_End;
    while (FileNameToTest_Pos && FileToTest_Name[FileNameToTest_Pos-1]>=__T('0') && FileToTest_Name[FileNameToTest_Pos-1]<=__T('9'))
        FileNameToTest_Pos--;
    if (FileNameToTest_Pos!=FileToTest_Name.size() && FileNameToTest_Pos!=FileNameToTest_End)
    {
        size_t Numbers_Size=FileNameToTest_End-FileNameToTest_Pos;
        int64u Pos=Ztring(FileToTest_Name.substr(FileNameToTest_Pos)).To_int64u();
        FileToTest_Name.resize(FileNameToTest_Pos);
        FileToTest_Name_After.erase(0, FileToTest_Name.size()+Numbers_Size);

        /*
        for (;;)
        {
            Pos++;
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest.Path_Get()+PathSeparator+FileToTest_Name+Pos_Ztring+__T('.')+(FileExtension.empty()?FileToTest.Extension_Get():FileExtension);
            if (!File::Exists(Next))
                break;
            Config->File_Names.push_back(Next);
        }
        */

        //Detecting with a smarter algo (but missing frames are not detected)
        Ztring FileToTest_Name_Begin=FileToTest.Path_Get()+PathSeparator+FileToTest_Name;
        Ztring FileToTest_Name_End=FileToTest_Name_After+__T('.')+(FileExtension.empty()?FileToTest.Extension_Get():FileExtension);
        Pos_Base = (size_t)Pos;
        size_t Pos_Add_Max = 1;
        #if MEDIAINFO_ADVANCED
            bool File_IgnoreSequenceFileSize=Config->File_IgnoreSequenceFilesCount_Get(); //TODO: double check if it is expected

            size_t SequenceFileSkipFrames=Config->File_SequenceFilesSkipFrames_Get();
            if (SequenceFileSkipFrames)
            {
                for (;;)
                {
                    size_t Pos_Add_Max_Old=Pos_Add_Max;
                    for (size_t TempPos=Pos_Add_Max; TempPos<=Pos_Add_Max+SequenceFileSkipFrames; TempPos++)
                    {
                        Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+TempPos);
                        if (Numbers_Size>Pos_Ztring.size())
                            Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
                        Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
                        if (File::Exists(Next))
                        {
                            Pos_Add_Max=TempPos+1;
                            break;
                        }
                    }
                    if (Pos_Add_Max==Pos_Add_Max_Old)
                        break;
                }
            }
            else
            {
        #endif //MEDIAINFO_ADVANCED
        for (;;)
        {
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+Pos_Add_Max);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
            if (!File::Exists(Next))
                break;
            Pos_Add_Max<<=1;
            #if MEDIAINFO_ADVANCED
                if (File_IgnoreSequenceFileSize && Pos_Add_Max>=CountOfFiles)
                    break;
            #endif //MEDIAINFO_ADVANCED
        }
        size_t Pos_Add_Min = Pos_Add_Max >> 1;
        while (Pos_Add_Min+1<Pos_Add_Max)
        {
            size_t Pos_Add_Middle = Pos_Add_Min + ((Pos_Add_Max - Pos_Add_Min) >> 1);
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+Pos_Add_Middle);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
            if (File::Exists(Next))
                Pos_Add_Min=Pos_Add_Middle;
            else
                Pos_Add_Max=Pos_Add_Middle;
        }

        #if MEDIAINFO_ADVANCED
            } //SequenceFileSkipFrames
        #endif //MEDIAINFO_ADVANCED

        size_t Pos_Max = Pos_Base + Pos_Add_Max;
        Config->File_Names.reserve(Pos_Add_Max);
        for (Pos=Pos_Base+1; Pos<Pos_Max; ++Pos)
        {
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Config->File_Names.push_back(FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End);
        }

        if (!Config->File_IsReferenced_Get() && Config->File_Names.size()<CountOfFiles && AlreadyPresent)
            Config->File_Names.resize(1); //Removing files, wrong detection
    }

    if (Config->File_Names.size()==Pos)
        return;

    Config->File_IsImageSequence=true;
    if (StreamSource==IsStream)
        Frame_Count_NotParsedIncluded=Pos_Base;
    #if MEDIAINFO_DEMUX
        float64 Demux_Rate=Config->Demux_Rate_Get();
        if (!Demux_Rate)
            Demux_Rate=24;
        if (!SkipComputeDelay && Frame_Count_NotParsedIncluded!=(int64u)-1)
            Fill(Stream_Video, 0, Video_Delay, float64_int64s(Frame_Count_NotParsedIncluded*1000/Demux_Rate));
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFileSize_Get() || Config->File_Names.size()<=1)
    #endif //MEDIAINFO_ADVANCED
    {
        for (; Pos<Config->File_Names.size(); Pos++)
        {
            int64u Size=File::Size_Get(Config->File_Names[Pos]);
            Config->File_Sizes.push_back(Size);
            Config->File_Size+=Size;
        }
    }
    #if MEDIAINFO_ADVANCED
        else
        {
            Config->File_Size=(int64u)-1;
            File_Size=(int64u)-1;
            Clear(Stream_General, 0, General_FileSize);
        }
    #endif //MEDIAINFO_ADVANCED

    File_Size=Config->File_Size;
    Element[0].Next=File_Size;
    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFileSize_Get() || Config->File_Names.size()<=1)
    #endif //MEDIAINFO_ADVANCED
        Fill (Stream_General, 0, General_FileSize, File_Size, 10, true);
    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFilesCount_Get())
    #endif //MEDIAINFO_ADVANCED
    {
        Fill (Stream_General, 0, General_CompleteName_Last, Config->File_Names[Config->File_Names.size()-1], true);
        Fill (Stream_General, 0, General_FolderName_Last, FileName::Path_Get(Config->File_Names[Config->File_Names.size()-1]), true);
        Fill (Stream_General, 0, General_FileName_Last, FileName::Name_Get(Config->File_Names[Config->File_Names.size()-1]), true);
        Fill (Stream_General, 0, General_FileExtension_Last, FileName::Extension_Get(Config->File_Names[Config->File_Names.size()-1]), true);
        if (Retrieve(Stream_General, 0, General_FileExtension_Last).empty())
            Fill(Stream_General, 0, General_FileNameExtension_Last, Retrieve(Stream_General, 0, General_FileName_Last));
        else
            Fill(Stream_General, 0, General_FileNameExtension_Last, Retrieve(Stream_General, 0, General_FileName_Last)+__T('.')+Retrieve(Stream_General, 0, General_FileExtension_Last));
    }

    #if MEDIAINFO_ADVANCED
        if (Config->File_Source_List_Get())
        {
            Ztring SourcePath=FileName::Path_Get(Retrieve(Stream_General, 0, General_CompleteName));
            size_t SourcePath_Size=SourcePath.size()+1; //Path size + path separator size
            for (size_t Pos=0; Pos<Config->File_Names.size(); Pos++)
            {
                Ztring Temp=Config->File_Names[Pos];
                Temp.erase(0, SourcePath_Size);
                Fill(Stream_General, 0, "Source_List", Temp);
            }
            Fill_SetOptions(Stream_General, 0, "Source_List", "N NT");
        }
    #endif //MEDIAINFO_ADVANCED
}
#endif //defined(MEDIAINFO_FILE_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_FILE_YES)
// title-of-work/
//     title-of-work.wav
//     dpx/
//         title-of-work_0086880.dpx
//         title-of-work_0086881.dpx
//         ... etc ...
static void PotentialAudioNames_Scenario1(const Ztring& DpxName, Ztring& ContainerDirName, ZtringList& List)
{
    if (DpxName.size()<4)
        return;

    if (DpxName.substr(DpxName.size()-4)!=__T(".dpx"))
        return;

    size_t PathSeparator_Pos1=DpxName.find_last_of(__T("\\/"));
    if (PathSeparator_Pos1==string::npos)
        return;

    size_t PathSeparator_Pos2=DpxName.find_last_of(__T("\\/"), PathSeparator_Pos1-1);
    if (PathSeparator_Pos2==string::npos)
        return;

    size_t PathSeparator_Pos3=DpxName.find_last_of(__T("\\/"), PathSeparator_Pos2-1); //string::npos is accepted (relative path) 

    size_t TitleSeparator_Pos=DpxName.find_last_of(__T('_'));
    if (TitleSeparator_Pos==string::npos || TitleSeparator_Pos<=PathSeparator_Pos1)
        return;

    Ztring DirDpx=DpxName.substr(PathSeparator_Pos2+1, PathSeparator_Pos1-(PathSeparator_Pos2+1));
    if (DirDpx!=__T("dpx"))
        return;

    Ztring TitleDpx=DpxName.substr(PathSeparator_Pos1+1, TitleSeparator_Pos-(PathSeparator_Pos1+1));
    Ztring TitleDir=DpxName.substr(PathSeparator_Pos3+1, PathSeparator_Pos2-(PathSeparator_Pos3+1));
    if (TitleDpx!=TitleDir)
        return;

    ContainerDirName=DpxName.substr(0, PathSeparator_Pos2+1);
    List.push_back(ContainerDirName+TitleDpx+__T(".wav"));
}
void File__Analyze::TestDirectory()
{
    if (IsSub || !Config->File_TestDirectory_Get())
        return;

    if (Config->File_Names.size()<=1)
        return;

    Ztring ContainerDirName;
    ZtringList List;
    PotentialAudioNames_Scenario1(Config->File_Names[0], ContainerDirName, List);
    bool IsModified=false;
    for (size_t i=0; i<List.size(); i++)
    {
        MediaInfo_Internal MI;
        if (MI.Open(List[i]))
        {
            IsModified=true;
            Ztring AudioFileName=MI.Get(Stream_General, 0, General_CompleteName);
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<MI.Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Stream_Prepare((stream_t)StreamKind);
                    Merge(MI, (stream_t)StreamKind, StreamPos_Last, StreamPos);
                    if (AudioFileName.size()>ContainerDirName.size())
                        Fill((stream_t)StreamKind, StreamPos_Last, "Source", AudioFileName.substr(ContainerDirName.size()));
                    Fill((stream_t)StreamKind, StreamPos_Last, "MuxingMode", MI.Get(Stream_General, 0, General_Format));
                    if (Retrieve_Const((stream_t)StreamKind, StreamPos_Last, "Encoded_Application").empty())
                        Fill((stream_t)StreamKind, StreamPos_Last, "Encoded_Application", MI.Get(Stream_General, 0, General_Encoded_Application));
                    if (Retrieve_Const((stream_t)StreamKind, StreamPos_Last, "Encoded_Library").empty())
                        Fill((stream_t)StreamKind, StreamPos_Last, "Encoded_Library", MI.Get(Stream_General, 0, General_Encoded_Library));
                }
            #if MEDIAINFO_ADVANCED
                if (!Config->File_IgnoreSequenceFileSize_Get())
            #endif //MEDIAINFO_ADVANCED
            {
                File_Size+=MI.Get(Stream_General, 0, General_FileSize).To_int64u();
            }
        }
    }
    if (IsModified)
    {
        Ztring VideoFileName=Retrieve(Stream_General, 0, General_CompleteName);
        Ztring VideoFileName_Last=Retrieve(Stream_General, 0, General_CompleteName_Last);
        Ztring VideoMuxingMode=Retrieve_Const(Stream_General, 0, General_Format);
        if (VideoFileName.size()>ContainerDirName.size())
            Fill(Stream_Video, 0, "Source", VideoFileName.substr(ContainerDirName.size()));
        if (VideoFileName_Last.size()>ContainerDirName.size())
            Fill(Stream_Video, 0, "Source_Last", VideoFileName_Last.substr(ContainerDirName.size()));
        Fill(Stream_Video, 0, Video_MuxingMode, VideoMuxingMode);

        Fill(Stream_General, 0, General_CompleteName, ContainerDirName, true);
        Fill(Stream_General, 0, General_FileSize, File_Size, 10, true);
        Fill(Stream_General, 0, General_Format, "Directory", Unlimited, true, true);

        Clear(Stream_General, 0, General_CompleteName_Last);
        Clear(Stream_General, 0, General_FolderName_Last);
        Clear(Stream_General, 0, General_FileName_Last);
        Clear(Stream_General, 0, General_FileNameExtension_Last);
        Clear(Stream_General, 0, General_FileExtension_Last);
        Clear(Stream_General, 0, General_Format_String);
        Clear(Stream_General, 0, General_Format_Info);
        Clear(Stream_General, 0, General_Format_Url);
        Clear(Stream_General, 0, General_Format_Commercial);
        Clear(Stream_General, 0, General_Format_Commercial_IfAny);
        Clear(Stream_General, 0, General_Format_Version);
        Clear(Stream_General, 0, General_Format_Profile);
        Clear(Stream_General, 0, General_Format_Level);
        Clear(Stream_General, 0, General_Format_Compression);
        Clear(Stream_General, 0, General_Format_Settings);
        Clear(Stream_General, 0, General_Format_AdditionalFeatures);
        Clear(Stream_General, 0, General_InternetMediaType);
        Clear(Stream_General, 0, General_Duration);
        Clear(Stream_General, 0, General_Encoded_Application);
        Clear(Stream_General, 0, General_Encoded_Application_String);
        Clear(Stream_General, 0, General_Encoded_Application_CompanyName);
        Clear(Stream_General, 0, General_Encoded_Application_Name);
        Clear(Stream_General, 0, General_Encoded_Application_Version);
        Clear(Stream_General, 0, General_Encoded_Application_Url);
        Clear(Stream_General, 0, General_Encoded_Library);
        Clear(Stream_General, 0, General_Encoded_Library_String);
        Clear(Stream_General, 0, General_Encoded_Library_CompanyName);
        Clear(Stream_General, 0, General_Encoded_Library_Name);
        Clear(Stream_General, 0, General_Encoded_Library_Version);
        Clear(Stream_General, 0, General_Encoded_Library_Date);
        Clear(Stream_General, 0, General_Encoded_Library_Settings);
        Clear(Stream_General, 0, General_Encoded_OperatingSystem);
        Clear(Stream_General, 0, General_FrameCount);
        Clear(Stream_General, 0, General_FrameRate);
    }
}
#endif //defined(MEDIAINFO_FILE_YES)

//---------------------------------------------------------------------------
#if MEDIAINFO_FIXITY
bool File__Analyze::FixFile(int64u FileOffsetForWriting, const int8u* ToWrite, const size_t ToWrite_Size)
{
    if (Config->File_Names.empty())
        return false; //Streams without file names are not supported
        
    #ifdef WINDOWS
    File::Copy(Config->File_Names[0], Config->File_Names[0]+__T(".Fixed"));
    #else //WINDOWS
    //ZenLib has File::Copy only for Windows for the moment. //TODO: support correctly (including meta)
    if (!File::Exists(Config->File_Names[0]+__T(".Fixed")))
    {
        std::ofstream  Dest(Ztring(Config->File_Names[0]+__T(".Fixed")).To_Local().c_str(), std::ios::binary);
        if (Dest.fail())
            return false;
        std::ifstream  Source(Config->File_Names[0].To_Local().c_str(), std::ios::binary);
        if (Source.fail())
            return false;
        Dest << Source.rdbuf();
        if (Dest.fail())
            return false;
    }
    #endif //WINDOWS

    File F;
    if (!F.Open(Config->File_Names[0]+__T(".Fixed"), File::Access_Write))
        return false;

    if (!F.GoTo(FileOffsetForWriting))
        return false;

    F.Write(ToWrite, ToWrite_Size);

    return true;
}
#endif //MEDIAINFO_FIXITY

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly()
{
    //Generic
    for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            Streams_Finish_StreamOnly((stream_t)StreamKind, StreamPos);

    //For each kind of (*Stream)
    for (size_t Pos=0; Pos<Count_Get(Stream_General);  Pos++) Streams_Finish_StreamOnly_General(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Video);    Pos++) Streams_Finish_StreamOnly_Video(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Audio);    Pos++) Streams_Finish_StreamOnly_Audio(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Text);     Pos++) Streams_Finish_StreamOnly_Text(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Other);    Pos++) Streams_Finish_StreamOnly_Other(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Image);    Pos++) Streams_Finish_StreamOnly_Image(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Menu);     Pos++) Streams_Finish_StreamOnly_Menu(Pos);
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly(stream_t StreamKind, size_t Pos)
{
    //Format
    if (Retrieve_Const(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Format)).empty())
        Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Format), Retrieve_Const(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_CodecID)));

    //BitRate from Duration and StreamSize
    if (StreamKind!=Stream_General && StreamKind!=Stream_Other && StreamKind!=Stream_Menu && Retrieve(StreamKind, Pos, "BitRate").empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty())
    {
        float64 Duration=0;
        if (StreamKind==Stream_Video && !Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && !Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_float64();
        int64u StreamSize=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).To_int64u();
        if (Duration>0 && StreamSize>0)
            Fill(StreamKind, Pos, "BitRate", StreamSize*8*1000/Duration, 0);
    }

    //BitRate_Encoded from Duration and StreamSize_Encoded
    if (StreamKind!=Stream_General && StreamKind!=Stream_Other && StreamKind!=Stream_Menu && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Encoded)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize_Encoded)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty())
    {
        float64 Duration=0;
        if (StreamKind==Stream_Video && !Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && !Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_float64();
        int64u StreamSize_Encoded=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize_Encoded)).To_int64u();
        if (Duration>0)
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Encoded), StreamSize_Encoded*8*1000/Duration, 0);
    }

    //Duration from Bitrate and StreamSize
    if (StreamKind!=Stream_Other && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, "BitRate").empty())
    {
        int64u BitRate=Retrieve(StreamKind, Pos, "BitRate").To_int64u();
        int64u StreamSize=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).To_int64u();
        if (BitRate>0 && StreamSize>0)
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration), ((float64)StreamSize)*8*1000/BitRate, 0);
    }

    //StreamSize from BitRate and Duration
    if (StreamKind!=Stream_Other && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, "BitRate").empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty() && Retrieve(StreamKind, Pos, "BitRate").find(__T(" / "))==std::string::npos) //If not done the first time or by other routine
    {
        float64 BitRate=Retrieve(StreamKind, Pos, "BitRate").To_float64();
        float64 Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_float64();
        if (BitRate>0 && Duration>0)
        {
            float64 StreamSize=BitRate*Duration/8/1000;
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize), StreamSize, 0);
        }
    }

    //Bit rate and maximum bit rate
    if (!Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate)).empty() && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate))==Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Maximum)))
    {
        Clear(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Maximum));
        if (Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Mode)).empty())
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Mode), "CBR");
    }

    //ServiceKind
    auto ServiceKind = Retrieve(StreamKind, Pos, "ServiceKind");
    if (!ServiceKind.empty())
    {
        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        List.Write(ServiceKind);
        if (List.size()>1)
        {
            size_t HI_ME_Pos=(size_t)-1;
            size_t HI_D_Pos=(size_t)-1;
            static const auto HI_ME_Text=__T("HI-ME");
            static const auto HI_D_Text=__T("HI-D");
            static const auto VI_ME_Text=__T("VI-ME");
            static const auto VI_D_Text=__T("VI-D");
            for (size_t i=0; i<List.size(); i++)
            {
                const auto& Item=List[i];
                if (HI_ME_Pos==(size_t)-1 && (Item==HI_ME_Text || Item==VI_ME_Text))
                    HI_ME_Pos=i;
                if (HI_D_Pos==(size_t)-1 && (Item==HI_D_Text || Item==VI_D_Text))
                    HI_D_Pos=i;
            }
            if (HI_ME_Pos!=(size_t)-1 && HI_D_Pos!=(size_t)-1)
            {
                if (HI_ME_Pos>HI_D_Pos)
                    std::swap(HI_ME_Pos, HI_D_Pos);
                List[HI_ME_Pos]=__T("HI");
                List.erase(List.begin()+HI_D_Pos);
                Fill(StreamKind, Pos, "ServiceKind", List.Read(), true);
                List.Write(Retrieve(StreamKind, Pos, "ServiceKind/String"));
                List[HI_ME_Pos].From_UTF8("Hearing Impaired");
                List.erase(List.begin()+HI_D_Pos);
                Fill(StreamKind, Pos, "ServiceKind/String", List.Read(), true);
            }
        }
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_General(size_t StreamPos)
{
    //File extension test
    if (Retrieve(Stream_General, StreamPos, "FileExtension_Invalid").empty())
    {
        const Ztring& Name=Retrieve(Stream_General, StreamPos, General_FileName);
        const Ztring& Extension=Retrieve(Stream_General, StreamPos, General_FileExtension);
        const Ztring& FormatName=Retrieve(Stream_General, StreamPos, General_Format);
        auto IsMachOAndEmptyExtension = Extension.empty() && FormatName.rfind(__T("Mach-O"), 0)==0;
        if ((!Name.empty() && !IsMachOAndEmptyExtension) || !Extension.empty())
        {
            InfoMap &FormatList=MediaInfoLib::Config.Format_Get();
            InfoMap::iterator Format=FormatList.find(FormatName);
            if (Format!=FormatList.end())
            {
                ZtringList ValidExtensions;
                ValidExtensions.Separator_Set(0, __T(" "));
                ValidExtensions.Write(Retrieve(Stream_General, StreamPos, General_Format_Extensions));
                if (!ValidExtensions.empty() && ValidExtensions.Find(Extension)==string::npos)
                    Fill(Stream_General, StreamPos, "FileExtension_Invalid", ValidExtensions.Read());
            }
        }
    }

    //Audio_Channels_Total
    if (Retrieve_Const(Stream_General, StreamPos, General_Audio_Channels_Total).empty())
    {
        auto Audio_Count = Count_Get(Stream_Audio);
        int64u Channels_Total=0;
        for (size_t i=0; i<Audio_Count; i++)
        {
            int64u Channels=Retrieve_Const(Stream_Audio, i, Audio_Channel_s_).To_int64u();
            if (!Channels)
            {
                Channels_Total=0;
                break;
            }
            Channels_Total+=Channels;
        }
        if (Channels_Total)
            Fill(Stream_General, StreamPos, General_Audio_Channels_Total, Channels_Total);
    }

    //Exceptions (empiric)
    {
        const auto& Application_Name=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name);
        if (Application_Name.size()>=5 && Application_Name.find(__T(", LLC"))==Application_Name.size()-5)
        {
            Fill(Stream_General, 0, General_Encoded_Application_CompanyName, Application_Name.substr(0, Application_Name.size()-5));
            Clear(Stream_General, StreamPos, General_Encoded_Application_Name);
        }
    }
    {
        const auto& Application_Name=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name);
        if (Application_Name.size()>=5 && Application_Name.rfind(__T("Mac OS X "), 0)==0)
        {
            Fill(Stream_General, 0, General_Encoded_Application_Version, Application_Name.substr(9), true);
            const auto& Application_CompanyName=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName);
            if (Application_CompanyName.empty())
                Fill(Stream_General, 0, General_Encoded_Application_CompanyName, "Apple");
            Fill(Stream_General, StreamPos, General_Encoded_Application_Name, "Mac OS X", Unlimited, true, true);
        }
        if (Application_Name.size()>=5 && Application_Name.rfind(__T("Sorenson "), 0)==0)
        {
            auto Application_Name_Max=Application_Name.find(__T(" / "));
            if (Application_Name_Max!=(size_t)-1)
                Application_Name_Max-=9;
            Fill(Stream_General, 0, General_Encoded_Application_Name, Application_Name.substr(9, Application_Name_Max), true);
            const auto& Application_CompanyName=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName);
            if (Application_CompanyName.empty())
                Fill(Stream_General, 0, General_Encoded_Application_CompanyName, "Sorenson");
        }
    }
    {
        const auto& Application_Name=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name);
        const auto& OperatingSystem_Version=Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Version);
        const auto& Hardware_Name=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Name);
        if (OperatingSystem_Version.empty() && !Application_Name.empty() && Application_Name.find_first_not_of(__T("0123456789."))==string::npos && Hardware_Name.rfind(__T("iPhone "), 0)==0)
        {
            Fill(Stream_General, 0, General_Encoded_OperatingSystem_Version, Application_Name);
            Fill(Stream_General, 0, General_Encoded_OperatingSystem_Name, "iOS", Unlimited, true, true);
            const auto& OperatingSystem_CompanyName=Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_CompanyName);
            if (OperatingSystem_CompanyName.empty())
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_CompanyName, "Apple");
            Clear(Stream_General, StreamPos, General_Encoded_Application_Name);
        }
    }
    {
        if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Name).empty()) {
            auto Application = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application).To_UTF8();
            auto pos = Application.rfind(" (Android)");
            if (Application.length() >= 12) {
            if (pos == Application.length() - 10) {
                Application.erase(pos, 10);
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_Name, "Android");
            }
            pos = Application.rfind(" (Macintosh)");
            if (pos == Application.length() - 12) {
                Application.erase(pos, 12);
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_Name, "macOS");
            }
            pos = Application.rfind(" (Windows)");
            if (pos == Application.length() - 10) {
                Application.erase(pos, 10);
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_Name, "Windows");
            }
            }
            if (Application != Retrieve_Const(Stream_General, 0, General_Encoded_Application).To_UTF8())
                Fill(Stream_General, 0, General_Encoded_Application, Application, true, true);
        }
        if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_CompanyName).empty()) {
            const auto OperatingSystem = Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Name).To_UTF8();
            if (OperatingSystem == "Android")
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_CompanyName, "Google");
            if (OperatingSystem == "macOS")
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_CompanyName, "Apple");
            if (OperatingSystem == "Windows")
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_CompanyName, "Microsoft");
        }
    }
    {
        const auto& Hardware_CompanyName = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName);
        const auto& Hardware_Name = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Name);
        if (Hardware_Name.rfind(Hardware_CompanyName + __T(' '), 0) == 0)
            Fill(Stream_General, StreamPos, General_Encoded_Hardware_Name, Hardware_Name.substr(Hardware_CompanyName.size() + 1), true);
    }
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Name).empty())
    {
        const auto& Performer = Retrieve_Const(Stream_General, StreamPos, General_Performer);
        const auto& Hardware_CompanyName = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName);
        ZtringList PerformerList;
        PerformerList.Separator_Set(0, __T(" / "));
        PerformerList.Write(Performer);
        set<Ztring> HardwareName_List;
        for (size_t i = 0; i < PerformerList.size(); i++)
        {
            const auto& PerformerItem = PerformerList[i];
            auto ShortAndContainsHardwareCompanyName = PerformerItem.size() - Hardware_CompanyName.size() <= 16 && PerformerItem.rfind(Hardware_CompanyName + __T(' '), 0) == 0;
            if (ShortAndContainsHardwareCompanyName || Hardware_CompanyName == __T("Samsung") && PerformerItem.size() <= 32 && PerformerItem.rfind(__T("Galaxy "), 0) == 0)
            {
                ZtringList Items;
                Items.Separator_Set(0, __T(" "));
                Items.Write(PerformerItem);
                if (Items.size() < 6)
                {
                    auto IsLikelyName = false;
                    auto LastHasOnlyDigits = false;
                    for (const auto& Item : Items)
                    {
                        size_t HasUpper = 0;
                        size_t HasDigit = 0;
                        for (const auto& Value : Item)
                        {
                            HasUpper += IsAsciiUpper(Value);
                            HasDigit += IsAsciiDigit(Value);
                        }
                        LastHasOnlyDigits = HasDigit == Item.size();
                        if (Item.size() == 1 || HasUpper >= 2 || (HasDigit && HasDigit < Item.size()))
                            IsLikelyName = true;
                    }
                    if (IsLikelyName || LastHasOnlyDigits)
                    {
                        HardwareName_List.insert(PerformerItem.substr(ShortAndContainsHardwareCompanyName ? (Hardware_CompanyName.size() + 1) : 0));
                        PerformerList.erase(PerformerList.begin() + i);
                        continue;
                    }
                }
            }
        }
        if (HardwareName_List.size() == 1)
        {
            //Performer is likely the actual performer
            Fill(Stream_General, StreamPos, General_Encoded_Hardware_Name, *HardwareName_List.begin());
            Fill(Stream_General, StreamPos, General_Performer, PerformerList.Read(), true);
        }
    }
    {
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Name);
        const auto& Model = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Model);
        if (Name == Model)
        {
            //Name is actually the model (technical name), keeping only model
            Clear(Stream_General, StreamPos, General_Encoded_Hardware_Name);
        }
    }

    //OperatingSystem
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_String).empty())
    {
        //Filling
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_CompanyName);
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Name);
        const auto& Version = Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Version);
        Ztring OperatingSystem = CompanyName;
        if (!Name.empty())
        {
            if (!OperatingSystem.empty())
                OperatingSystem += ' ';
            OperatingSystem += Name;
            if (!Version.empty())
            {
                OperatingSystem += ' ';
                OperatingSystem += Version;
            }
        }
        Fill(Stream_General, StreamPos, General_Encoded_OperatingSystem_String, OperatingSystem);
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_General_Curate(size_t StreamPos)
{
    // Remove redundant content
    if (Retrieve_Const(Stream_General, StreamPos, General_Copyright) == Retrieve_Const(Stream_General, StreamPos, General_Encoded_Library_Name)) {
        Clear(Stream_General, StreamPos, General_Copyright);
    }
    
    // Remove useless characters
    auto RemoveUseless = [&](size_t Parameter) {
        for (;;) {
            const auto& Value = Retrieve_Const(Stream_General, StreamPos, Parameter);
            if (Value.size() < 2) {
                return;
            }
            if (Value.front() == '[' && Value.back() == ']') {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(1, Value.size() - 2), true);
                continue;
            }
            if (Value.rfind(__T("Digital Camera "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(15), true);
                continue;
            }
            if (Value.rfind(__T("encoded by "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(11), true);
                continue;
            }
            if (Value.rfind(__T("This file was made by "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(22), true);
                continue;
            }
            if (Value.rfind(__T("KODAK EASYSHARE "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, __T("KODAK EasyShare") + Value.substr(15), true);
                continue;
            }
            if (Value.size() >= 15 && Value.find(__T(" DIGITAL CAMERA"), 15) != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Value.size() - 15), true);
                continue;
            }
            if (Value.size() >= 15 && Value.find(__T(" Digital Camera"), 0) != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Value.size() - 15), true);
                continue;
            }
            if (Value.size() >= 5 && Value.find(__T(" ZOOM"), 0) != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Value.size() - 5) + __T(" Zoom"), true);
                continue;
            }
            break;
        }
    };
    RemoveUseless(General_Encoded_Hardware_Model);
    RemoveUseless(General_Encoded_Application);
    RemoveUseless(General_Encoded_Application_Name);
    RemoveUseless(General_Encoded_Library);
    RemoveUseless(General_Encoded_Library_Name);

    // Remove company legal suffixes and rename some company trademarks
    auto RemoveLegal = [&](size_t Parameter) {
        bool DoAgain;
        do {
            DoAgain = false;
            const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter);
            if (CompanyName.empty()) {
                return;
            }
            auto CompanyNameU = CompanyName;
            CompanyNameU.MakeUpperCase();
            for (const auto& CompanySuffix : CompanySuffixes) {
                auto len = strlen(CompanySuffix);
                if (len < CompanyNameU.size() - 1
                    && (CompanyNameU[CompanyNameU.size() - (len + 1)] == ' '
                        || CompanyNameU[CompanyNameU.size() - (len + 1)] == ','
                        || CompanyNameU[CompanyNameU.size() - (len + 1)] == '-' )
                    && CompanyNameU.find(Ztring().From_UTF8(CompanySuffix), CompanyNameU.size() - len) != string::npos) {
                    len++;
                    if (len < CompanyName.size() && CompanyName[CompanyName.size() - (len + 1)] == ',') {
                        len++;
                    }
                    Fill(Stream_General, StreamPos, Parameter, CompanyName.substr(0, CompanyName.size() - len), true);
                    DoAgain = true;
                    break;
                }
            }
        } while (DoAgain);

        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter);
        auto CompanyNameU = CompanyName;
        CompanyNameU.MakeUpperCase();
        auto CompanyNameUS = CompanyNameU.To_UTF8();
        for (const auto& ToSearch : CompanyNames_Replace) {
            if (CompanyNameUS == ToSearch.Find) {
                Fill(Stream_General, StreamPos, Parameter, ToSearch.ReplaceBy, Unlimited, true, true);
            }
        }
    };
    RemoveLegal(General_Encoded_Hardware_CompanyName);
    RemoveLegal(General_Encoded_Library_CompanyName);
    RemoveLegal(General_Encoded_Application_CompanyName);
    auto General_StreamPos_Count = Count_Get(Stream_General, StreamPos);
    for (size_t i = 0; i < General_StreamPos_Count; i++) {
        const auto Name = Retrieve_Const(Stream_General, StreamPos, i, Info_Name).To_UTF8();
        if (Name == "LensMake") {
            RemoveLegal(i);
        }
    }

    // Move versions found in name field
    auto MoveVersion = [&](size_t Parameter_Source, size_t Parameter_Version, size_t Parameter_Name = 0) {
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, Parameter_Source);
        if (Name.empty()) {
            return;
        }
        const auto& Version = Retrieve_Const(Stream_General, StreamPos, Parameter_Version);
        size_t Version_Pos = Name.size() - 1;
        for (;;) {
            const auto Letter = Name[Version_Pos];
            if (((Letter < '0' || Letter > '9') && Letter != '.')
             && (!Version_Pos || Name[Version_Pos - 1] != '.')) { // We accept e.g. one letter if it is after a dot
                auto Dot_Pos = Name.find('.');
                if (Dot_Pos != string::npos) {
                    Dot_Pos = Name.find('.', Dot_Pos + 1);
                }
                auto Space_Pos = Name.find(' ');
                if (Version_Pos || Dot_Pos == string::npos || Space_Pos != string::npos) {
                    break;
                }
                Version_Pos = 0; // We consider no space and at least 2 dots as a version in full
            }
            if (!Version_Pos) {
                if (Name != Version) {
                    Fill(Stream_General, StreamPos, Parameter_Version, Name);
                }
                Clear(Stream_General, StreamPos, Parameter_Source);
                return;
            }
            Version_Pos--;
        }
        Version_Pos++;
        if (Version_Pos == Name.size()) {
            return; // No version found
        }
        auto Extra_Pos = Version_Pos;
        auto IsLikelyVersion = false;

        if (Name[Version_Pos] == '.') {
            Version_Pos++;
        }

        switch (Name[Extra_Pos - 1]) {
        case '-':
        {
            auto Space_Pos = Name.find(' ');
            if (Space_Pos != string::npos) {
                Version_Pos = Space_Pos + 1;
                Extra_Pos = Version_Pos;
            }
        }
        [[fallthrough]];
        case ' ':
            if (Name.find('.', Version_Pos) != string::npos) { // catching version with a dot only for avoiding a digit in the name
                IsLikelyVersion = true;
            }
            Extra_Pos--;
            break;
        }
        
        // Version string
        Ztring NameU = Name;
        NameU.MakeUpperCase();
        for (const auto& VersionPrefix : VersionPrefixes) {
            Ztring Prefix;
            Prefix.From_UTF8(VersionPrefix);
            if (Extra_Pos >= Prefix.size()
             && !NameU.compare(Extra_Pos - Prefix.size(), Prefix.size(), Prefix)
             && (Extra_Pos == Prefix.size()
              || NameU[Extra_Pos - (Prefix.size() + 1)] == ' '
              || Prefix.front() == ',')) {
                Extra_Pos -= Prefix.size() + (Prefix.front() != ',' && Extra_Pos != Prefix.size());
                IsLikelyVersion = true;
                break;
            }
        }

        if (!IsLikelyVersion) {
            return;
        }

        auto Plus_Pos = Name.rfind(__T(" + "), Extra_Pos);
        auto And_Pos = Name.rfind(__T(" & "), Extra_Pos);
        auto Comma_Pos = Extra_Pos ? Name.rfind(__T(", "), Extra_Pos - 1) : string::npos;
        if (Plus_Pos != string::npos || And_Pos != string::npos || Comma_Pos != string::npos) {
            return; // TODO: handle complex string e.g. with 2 versions
        }

        auto VersionFromName = Name.substr(Version_Pos);
        if (VersionFromName != Version) {
            Fill(Stream_General, StreamPos, Parameter_Version, VersionFromName);
        }
        if (!Extra_Pos) {
            Clear(Stream_General, StreamPos, Parameter_Source);
            return; // No name found
        }
        Fill(Stream_General, StreamPos, Parameter_Name ? Parameter_Name : Parameter_Source, Name.substr(0, Extra_Pos), true);
    };
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Version) == __T("Unknown")) {
        Clear(Stream_General, StreamPos, General_Encoded_Application_Version);
    }
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name).empty() && Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName) == __T("Google")) {
        const auto& Application = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application);
        if (Application.rfind(__T("HDR+ "), 0) == 0) {
            if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Version).empty()) {
                Fill(Stream_General, StreamPos, General_Encoded_Application_Version, Application.substr(5));
            }
            Fill(Stream_General, StreamPos, General_Encoded_Application_Name, Application.substr(0, 4));
            if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName).empty()) {
                Fill(Stream_General, StreamPos, General_Encoded_Application_CompanyName, Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName));
            }
        }
    }
    MoveVersion(General_Encoded_Hardware_Model, General_Encoded_Hardware_Version);
    MoveVersion(General_Encoded_Hardware_Name, General_Encoded_Hardware_Version);
    MoveVersion(General_Encoded_Hardware, General_Encoded_Hardware_Version, General_Encoded_Hardware_Name);
    MoveVersion(General_Encoded_Library_Name, General_Encoded_Library_Version);
    MoveVersion(General_Encoded_Library, General_Encoded_Library_Version, General_Encoded_Library_Name);
    MoveVersion(General_Encoded_Application_Name, General_Encoded_Application_Version);
    MoveVersion(General_Encoded_Application, General_Encoded_Application_Version, General_Encoded_Application_Name);

    // Move company name found in name field
    auto MoveCompanyName = [&](size_t Parameter_Search, size_t Parameter_CompanyName) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName);
        const auto& Search = Retrieve_Const(Stream_General, StreamPos, Parameter_Search);

        auto CompanyNameU = CompanyName;
        auto SearchU = Search;
        CompanyNameU.MakeUpperCase();
        SearchU.MakeUpperCase();
        if (!CompanyNameU.empty() && SearchU.size() > CompanyNameU .size() && SearchU[CompanyNameU.size()] == ' ' && SearchU.rfind(CompanyNameU, 0) == 0) {
            Fill(Stream_General, StreamPos, Parameter_Search, Search.substr(CompanyName.size() + 1), true);
        }
        else {
            auto SearchUS = SearchU.To_UTF8();
            for (const auto& ToSearch : CompanyNames) {
                if (SearchUS.rfind(ToSearch, 0) == 0) {
                    const auto ToSearch_Len = strlen(ToSearch);
                    if (SearchUS.size() > ToSearch_Len && SearchUS[ToSearch_Len] == ' ') {
                        Fill(Stream_General, StreamPos, Parameter_CompanyName, Search.substr(0, ToSearch_Len));
                        Fill(Stream_General, StreamPos, Parameter_Search, Search.substr(ToSearch_Len), true);
                        break;
                    }
                }
            }
        }
    };
    MoveCompanyName(General_Encoded_Hardware_Model, General_Encoded_Hardware_CompanyName);
    MoveCompanyName(General_Encoded_Library_Name, General_Encoded_Library_CompanyName);
    MoveCompanyName(General_Encoded_Application_Name, General_Encoded_Application_CompanyName);

    // Remove capitalization
    auto RemoveCapitalization = [&](size_t Parameter) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter);
        if (CompanyName.size() < 2) {
            return;
        }
        auto CompanyNameU = CompanyName;
        CompanyNameU.MakeUpperCase();
        auto CompanyNameUS = CompanyNameU.To_UTF8();
        for (const auto& ToSearch : CompanyNames) {
            if (CompanyNameUS.size() > 2 && CompanyNameUS == ToSearch) {
                for (size_t i = 1; i < CompanyName.size(); i++) {
                    auto& Letter = CompanyNameUS[i];
                    if (Letter >= 'A' && Letter <= 'Z') {
                        CompanyNameUS[i] += 'a' - 'A';
                    }
                }
                Fill(Stream_General, StreamPos, Parameter, CompanyNameUS, true, true);
            }
        }
    };
    RemoveCapitalization(General_Encoded_Hardware_CompanyName);
    RemoveCapitalization(General_Encoded_Library_CompanyName);
    RemoveCapitalization(General_Encoded_Application_CompanyName);

    // Model name
    auto FillModelName = [&](size_t Parameter_CompanyName, size_t Parameter_Model, size_t Parameter_Name) {
        if (!Retrieve_Const(Stream_General, StreamPos, Parameter_Name).empty())
            return;
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName).To_UTF8();
        for (const auto& ToSearch : Model_Name) {
            if (CompanyName == ToSearch.CompanyName) {
                const auto& Model = Retrieve_Const(Stream_General, StreamPos, Parameter_Model).To_UTF8();
                for (size_t i = 0; i < ToSearch.Size; i++) {
                    const auto& ToSearch2 = ToSearch.Find[i];
                    if (Model == ToSearch2.Find) {
                        Fill(Stream_General, StreamPos, Parameter_Name, ToSearch2.ReplaceBy);
                        break;
                    }
                }
                break;
            }
        }
    };
    FillModelName(General_Encoded_Hardware_CompanyName, General_Encoded_Hardware_Model, General_Encoded_Hardware_Name);

    // Crosscheck
    auto Crosscheck = [&](size_t Parameter_CompanyName_Source, size_t Parameter_Start, bool CheckName) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName_Source);
        if (!CompanyName.empty()) {
            const auto Parameter = Parameter_Start;
            const auto Parameter_String = ++Parameter_Start;
            const auto Parameter_CompanyName = ++Parameter_Start;
            const auto Parameter_Name = ++Parameter_Start;
            const auto& Application_CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName);
            if (Application_CompanyName.empty()) {
                const auto& Application = Retrieve_Const(Stream_General, StreamPos, CheckName ? Parameter_Name : Parameter);
                auto CompanyNameU = CompanyName;
                CompanyNameU.MakeUpperCase();
                auto ApplicationU = Application;
                ApplicationU.MakeUpperCase();
                if (ApplicationU.size() > CompanyNameU.size() && ApplicationU[CompanyNameU.size()] == ' ' && ApplicationU.rfind(CompanyNameU, 0) == 0) {
                    Fill(Stream_General, StreamPos, Parameter_CompanyName, CompanyName);
                    Fill(Stream_General, StreamPos, Parameter_Name, Application.substr(CompanyName.size() + 1), true);
                    if (!CheckName) {
                        Clear(Stream_General, StreamPos, Parameter);
                    }
                }
            }
        }
    };
    Crosscheck(General_Encoded_Hardware_CompanyName, General_Encoded_Application, false);
    Crosscheck(General_Encoded_Hardware_CompanyName, General_Encoded_Application, true);
    Crosscheck(General_Encoded_Hardware_CompanyName, General_Encoded_Library, false);
    Crosscheck(General_Encoded_Hardware_CompanyName, General_Encoded_Library, true);
    Crosscheck(General_Encoded_Library_CompanyName, General_Encoded_Application, false);
    Crosscheck(General_Encoded_Library_CompanyName, General_Encoded_Application, true);

    // Copy name from other sources
    auto CopyName = [&](size_t IfParameter, size_t Parameter_Name, size_t Parameter_Source) {
        const auto& If = Retrieve_Const(Stream_General, StreamPos, IfParameter);
        if (If.empty()) {
            return;
        }
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, Parameter_Name);
        if (!Name.empty()) {
            return;
        }
        const auto& Source = Retrieve_Const(Stream_General, StreamPos, Parameter_Source);
        if (Source.empty()) {
            return;
        }
        Fill(Stream_General, StreamPos, Parameter_Name, Source);
    };
    CopyName(General_Encoded_Library_CompanyName, General_Encoded_Library_Name, General_Encoded_Hardware_Name);
    CopyName(General_Encoded_Application_CompanyName, General_Encoded_Application_Name, General_Encoded_Library_Name);
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name).empty() && !Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Version).empty()) {
        const auto& Model = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Model);
        if (Model.rfind(__T("iPhone "), 0) == 0) {
            if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName).empty()) {
                Fill(Stream_General, StreamPos, General_Encoded_Application_CompanyName, Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName));
            }
            Fill(Stream_General, StreamPos, General_Encoded_Application_Name, "iOS");
        }
        CopyName(General_Encoded_Hardware_Name, General_Encoded_Application_Name, General_Encoded_Hardware_Name);
        CopyName(General_Encoded_Hardware_Model, General_Encoded_Application_Name, General_Encoded_Hardware_Model);
    }

    // Copy company name from other sources
    auto CopyCompanyName = [&](size_t Parameter_CompanyName_Source, size_t Parameter_CompanyName_Dest, size_t Parameter_Name_Source, size_t Parameter_Name_Dest) {
        const auto& CompanyName_Dest = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName_Dest);
        const auto& CompanyName_Source = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName_Source);
        const auto& Name_Dest = Retrieve_Const(Stream_General, StreamPos, Parameter_Name_Dest);
        if (!CompanyName_Dest.empty() || CompanyName_Source.empty() || Name_Dest.empty()) {
            return;
        }
        const auto& Name_Source = Retrieve_Const(Stream_General, StreamPos, Parameter_Name_Source);
        if (Name_Source != Name_Dest) {
            return;
        }
        Fill(Stream_General, StreamPos, Parameter_CompanyName_Dest, CompanyName_Source);
    };
    CopyCompanyName(General_Encoded_Library_CompanyName, General_Encoded_Application_CompanyName, General_Encoded_Library_Name, General_Encoded_Application_Name);
    CopyCompanyName(General_Encoded_Application_CompanyName, General_Encoded_Library_CompanyName, General_Encoded_Application_Name, General_Encoded_Library_Name);
    CopyCompanyName(General_Encoded_Hardware_CompanyName, General_Encoded_Application_CompanyName, General_Encoded_Hardware_Name, General_Encoded_Application_Name);
    CopyCompanyName(General_Encoded_Hardware_CompanyName, General_Encoded_Application_CompanyName, General_Encoded_Hardware_Model, General_Encoded_Application_Name);

    // Check if it is really a library
    {
        /*
        const auto& Application_Name = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name);
        if (!Application_Name.empty()
         && Application_Name == Retrieve_Const(Stream_General, StreamPos, General_Encoded_Library_Name)
         && (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Library_Version).empty() || Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Version) == Retrieve_Const(Stream_General, StreamPos, General_Encoded_Library_Version))) {
            Clear(Stream_General, StreamPos, General_Encoded_Library);
            Clear(Stream_General, StreamPos, General_Encoded_Library_CompanyName);
            Clear(Stream_General, StreamPos, General_Encoded_Library_Name);
            Clear(Stream_General, StreamPos, General_Encoded_Library_Version);
        }
        */
    }

    // Redundancy
    if (Retrieve_Const(Stream_General, 0, General_Encoded_Application_Name).empty() && Retrieve_Const(Stream_General, 0, General_Encoded_Application).empty())
    {
        if (Retrieve_Const(Stream_General, 0, General_Comment) == __T("Created with GIMP") || Retrieve_Const(Stream_General, 0, General_Description) == __T("Created with GIMP"))
            Fill(Stream_General, 0, General_Encoded_Application, "GIMP");
    }
    if (Retrieve_Const(Stream_General, 0, General_Encoded_Application_Name) == __T("GIMP") || Retrieve_Const(Stream_General, 0, General_Encoded_Application) == __T("GIMP") || !Retrieve_Const(Stream_General, 0, General_Encoded_Application).rfind(__T("GIMP ")))
    {
        if (Retrieve_Const(Stream_General, 0, General_Comment) == __T("Created with GIMP"))
            Clear(Stream_General, StreamPos, General_Comment);
        if (Retrieve_Const(Stream_General, 0, General_Description) == __T("Created with GIMP"))
            Clear(Stream_General, StreamPos, General_Description);
    }

    // Remove synonyms
    auto RemoveSynonyms = [&](size_t Parameter_CompanyName, size_t Parameter_Model) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName).To_UTF8();
        for (const auto& ToSearch : Model_Replace) {
            if (CompanyName == ToSearch.CompanyName) {
                const auto& Model = Retrieve_Const(Stream_General, StreamPos, Parameter_Model).To_UTF8();
                for (size_t i = 0; i < ToSearch.Size; i++) {
                    const auto& ToSearch2 = ToSearch.Find[i];
                    if (Model == ToSearch2.Find) {
                        Fill(Stream_General, StreamPos, Parameter_Model, ToSearch2.ReplaceBy, Unlimited, true, true);
                        break;
                    }
                }
                break;
            }
        }
        };
    RemoveSynonyms(General_Encoded_Hardware_CompanyName, General_Encoded_Hardware_Model);
    RemoveSynonyms(General_Encoded_Application_CompanyName, General_Encoded_Application_Name);

    // Create displayed string
    auto CreateString = [&](size_t Parameter_Start, bool HasModel = false) {
        const auto Parameter = Parameter_Start;
        const auto Parameter_String = ++Parameter_Start;
        if (!Retrieve_Const(Stream_General, StreamPos, Parameter_String).empty()) {
            return;
        }
        const auto Parameter_CompanyName = ++Parameter_Start;
        const auto Parameter_Name = ++Parameter_Start;
        const auto Parameter_Model = ++Parameter_Start;
        const auto Parameter_Version = Parameter_Start + HasModel;

        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName);
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, Parameter_Name);
        const auto& Model = Retrieve_Const(Stream_General, StreamPos, Parameter_Model);
        const auto& Version = Retrieve_Const(Stream_General, StreamPos, Parameter_Version);

        auto Value = CompanyName;
        if (!Name.empty())
        {
            if (!Value.empty())
                Value += ' ';
            Value += Name;
        }
        if (HasModel && !Model.empty())
        {
            if (!Value.empty())
                Value += ' ';
            if (!Name.empty())
                Value += '(';
            Value += Model;
            if (!Name.empty())
                Value += ')';
        }
        if (!Value.empty() && !Version.empty()) {
            Value += ' ';
            Value += Version;
        }
        Fill(Stream_General, StreamPos, Parameter_String, Value, true);
    };
    CreateString(General_Encoded_Hardware, true);
    CreateString(General_Encoded_Library);
    CreateString(General_Encoded_Application);
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Video(size_t Pos)
{
    //Frame count
    if (Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
    {
        if (Count_Get(Stream_Video)==1 && Count_Get(Stream_Audio)==0)
            Fill(Stream_Video, 0, Video_FrameCount, Frame_Count_NotParsedIncluded);
    }

    //FrameCount from Duration and FrameRate
    if (Retrieve(Stream_Video, Pos, Video_FrameCount).empty())
    {
        int64s Duration=Retrieve(Stream_Video, Pos, Video_Duration).To_int64s();
        bool DurationFromGeneral;
        if (Duration==0)
        {
            Duration=Retrieve(Stream_General, 0, General_Duration).To_int64s();
            DurationFromGeneral=Retrieve(Stream_General, 0, General_Format)!=Retrieve(Stream_Video, Pos, Audio_Format);
        }
        else
            DurationFromGeneral=false;
        float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
        if (Duration && FrameRate)
        {
            Fill(Stream_Video, Pos, Video_FrameCount, Duration*FrameRate/1000, 0);
            if (DurationFromGeneral && Retrieve_Const(Stream_Audio, Pos, Audio_Format)!=Retrieve_Const(Stream_General, 0, General_Format))
            {
                Fill(Stream_Video, Pos, "FrameCount_Source", "General_Duration");
                Fill_SetOptions(Stream_Video, Pos, "FrameCount_Source", "N NTN");
            }
        }
    }

    //Duration from FrameCount and FrameRate
    if (Retrieve(Stream_Video, Pos, Video_Duration).empty())
    {
        int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
        float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
        if (FrameCount && FrameRate)
        {
            Fill(Stream_Video, Pos, Video_Duration, FrameCount/FrameRate*1000, 0);
            Ztring Source=Retrieve(Stream_Video, Pos, "FrameCount_Source");
            if (!Source.empty())
            {
                Fill(Stream_Video, Pos, "Duration_Source", Source);
                Fill_SetOptions(Stream_Video, Pos, "Duration_Source", "N NTN");
            }
        }
    }

    //FrameRate from FrameCount and Duration
    if (Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
    {
        int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
        float64 Duration=Retrieve(Stream_Video, Pos, Video_Duration).To_float64()/1000;
        if (FrameCount && Duration)
           Fill(Stream_Video, Pos, Video_FrameRate, FrameCount/Duration, 3);
    }

    //Pixel Aspect Ratio forced from picture pixel size and Display Aspect Ratio
    if (Retrieve(Stream_Video, Pos, Video_PixelAspectRatio).empty())
    {
        const Ztring& DAR_S=Retrieve_Const(Stream_Video, Pos, Video_DisplayAspectRatio);
        float DAR=DAR_S.To_float32();
        float Width=Retrieve(Stream_Video, Pos, Video_Width).To_float32();
        float Height=Retrieve(Stream_Video, Pos, Video_Height).To_float32();
        if (DAR && Height && Width)
        {
            if (DAR_S==__T("1.778"))
                DAR=((float)16)/9; //More exact value
            if (DAR_S==__T("1.333"))
                DAR=((float)4)/3; //More exact value
            Fill(Stream_Video, Pos, Video_PixelAspectRatio, DAR/(((float32)Width)/Height));
        }
    }

    //Pixel Aspect Ratio forced to 1.000 if none
    if (Retrieve(Stream_Video, Pos, Video_PixelAspectRatio).empty() && Retrieve(Stream_Video, Pos, Video_DisplayAspectRatio).empty())
        Fill(Stream_Video, Pos, Video_PixelAspectRatio, 1.000);

    //Standard
    if (Retrieve(Stream_Video, Pos, Video_Standard).empty() && (Retrieve(Stream_Video, Pos, Video_Width)==__T("720") || Retrieve(Stream_Video, Pos, Video_Width)==__T("704")))
    {
             if (Retrieve(Stream_Video, Pos, Video_Height)==__T("576") && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("25.000"))
            Fill(Stream_Video, Pos, Video_Standard, "PAL");
        else if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("486") || Retrieve(Stream_Video, Pos, Video_Height)==__T("480")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("29.970"))
            Fill(Stream_Video, Pos, Video_Standard, "NTSC");
    }
    if (Retrieve(Stream_Video, Pos, Video_Standard).empty() && Retrieve(Stream_Video, Pos, Video_Width)==__T("352"))
    {
             if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("576") || Retrieve(Stream_Video, Pos, Video_Height)==__T("288")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("25.000"))
            Fill(Stream_Video, Pos, Video_Standard, "PAL");
        else if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("486") || Retrieve(Stream_Video, Pos, Video_Height)==__T("480") || Retrieve(Stream_Video, Pos, Video_Height)==__T("243") || Retrieve(Stream_Video, Pos, Video_Height)==__T("240")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("29.970"))
            Fill(Stream_Video, Pos, Video_Standard, "NTSC");
    }

    //Known ScanTypes
    if (Retrieve(Stream_Video, Pos, Video_ScanType).empty()
     && (Retrieve(Stream_Video, Pos, Video_Format)==__T("RED")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("CineForm")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("DPX")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("EXR")))
            Fill(Stream_Video, Pos, Video_ScanType, "Progressive");

    //Useless chroma subsampling
    if (Retrieve(Stream_Video, Pos, Video_ColorSpace)==__T("RGB")
     && Retrieve(Stream_Video, Pos, Video_ChromaSubsampling)==__T("4:4:4"))
        Clear(Stream_Video, Pos, Video_ChromaSubsampling);

    //Chroma subsampling position
    if (Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_String).empty() && !Retrieve(Stream_Video, Pos, Video_ChromaSubsampling).empty())
    {
        if (Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_Position).empty())
            Fill(Stream_Video, Pos, Video_ChromaSubsampling_String, Retrieve(Stream_Video, Pos, Video_ChromaSubsampling));
        else
            Fill(Stream_Video, Pos, Video_ChromaSubsampling_String, Retrieve(Stream_Video, Pos, Video_ChromaSubsampling)+__T(" (")+ Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_Position)+__T(')'));
    }

    //Commercial name
    if (Retrieve(Stream_Video, Pos, Video_HDR_Format_Compatibility).rfind(__T("HDR10"), 0)==0
     && ((!Retrieve(Stream_Video, Pos, Video_BitDepth).empty() && Retrieve(Stream_Video, Pos, Video_BitDepth).To_int64u()<10) //e.g. ProRes has not bitdepth info
     || Retrieve(Stream_Video, Pos, Video_colour_primaries)!=__T("BT.2020")
     || Retrieve(Stream_Video, Pos, Video_transfer_characteristics)!=__T("PQ")
     || Retrieve(Stream_Video, Pos, Video_MasteringDisplay_ColorPrimaries).empty()
        ))
    {
        //We actually fill HDR10/HDR10+ by default, so it will be removed below if not fitting in the color related rules
        Clear(Stream_Video, Pos, Video_HDR_Format_Compatibility);
    }
    if (Retrieve(Stream_Video, Pos, Video_HDR_Format_String).empty())
    {
        ZtringList Summary;
        Summary.Separator_Set(0, __T(" / "));
        Summary.Write(Retrieve(Stream_Video, Pos, Video_HDR_Format));
        ZtringList Commercial=Summary;
        size_t DolbyVision_Pos=(size_t)-1;
        for (size_t j=0; j<Summary.size(); j++)
            if (Summary[j]==__T("Dolby Vision"))
                DolbyVision_Pos=j;
        if (!Summary.empty())
        {
            ZtringList HDR_Format_Compatibility;
            HDR_Format_Compatibility.Separator_Set(0, __T(" / "));
            HDR_Format_Compatibility.Write(Retrieve(Stream_Video, Pos, Video_HDR_Format_Compatibility));
            HDR_Format_Compatibility.resize(Summary.size());
            ZtringList ToAdd;
            ToAdd.Separator_Set(0, __T(" / "));
            for (size_t i=Video_HDR_Format_String+1; i<=Video_HDR_Format_Compression; i++)
            {
                ToAdd.Write(Retrieve(Stream_Video, Pos, i));
                ToAdd.resize(Summary.size());
                for (size_t j=0; j<Summary.size(); j++)
                {
                    if (!ToAdd[j].empty())
                    {
                        switch (i)
                        {
                            case Video_HDR_Format_Version: Summary[j]+=__T(", Version "); break;
                            case Video_HDR_Format_Level: Summary[j]+=__T('.'); break;
                            case Video_HDR_Format_Compression: ToAdd[j][0]+=0x20; if (ToAdd[j].size()==4) ToAdd[j].resize(2); ToAdd[j]+=__T(" metadata compression"); [[fallthrough]];
                            default: Summary[j] += __T(", ");
                        }
                        Summary[j]+=ToAdd[j];
                        if (i==Video_HDR_Format_Version && j==DolbyVision_Pos)
                        {
                            ToAdd.Write(Retrieve(Stream_Video, Pos, Video_HDR_Format_Profile));
                            if (j<ToAdd.size())
                            {
                                const Ztring& Profile=ToAdd[j];
                                size_t Profile_Dot=Profile.find(__T('.'));
                                if (Profile_Dot!=string::npos)
                                {
                                    Profile_Dot++;
                                    if (Profile_Dot<Profile.size() && Profile[Profile_Dot]==__T('0'))
                                        Profile_Dot++;
                                    Summary[j]+=__T(", Profile ");
                                    Summary[j]+=Profile.substr(Profile_Dot);
                                    ToAdd.Write(Retrieve(Stream_Video, Pos, Video_HDR_Format_Compatibility));
                                    if (j<ToAdd.size())
                                    {
                                        const Ztring& Compatibility=ToAdd[j];
                                        size_t Compatibility_Pos=DolbyVision_Compatibility_Pos(Compatibility);
                                        if (Compatibility_Pos!=size_t()-1)
                                        {
                                            Summary[j]+=__T('.');
                                            Summary[j]+=Ztring::ToZtring(Compatibility_Pos, 16);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            for (size_t j=0; j<Summary.size(); j++)
                if (!HDR_Format_Compatibility[j].empty())
                {
                    Summary[j]+=__T(", ")+HDR_Format_Compatibility[j]+__T(" compatible");
                    Commercial[j]=HDR_Format_Compatibility[j];
                    if (!Commercial[j].empty())
                    {
                        auto Commercial_Reduce=Commercial[j].find(__T(' '));
                        if (Commercial_Reduce<Commercial[j].size()-1 && Commercial[j][Commercial_Reduce+1]>='0' && Commercial[j][Commercial_Reduce+1]<='9')
                            Commercial_Reduce=Commercial[j].find(__T(' '), Commercial_Reduce+1);
                        if (Commercial_Reduce!=string::npos)
                            Commercial[j].resize(Commercial_Reduce);
                    }
                }
            Fill(Stream_Video, Pos, Video_HDR_Format_String, Summary.Read());
            Fill(Stream_Video, Pos, Video_HDR_Format_Commercial, Commercial.Read());
        }
    }
    #if defined(MEDIAINFO_VC3_YES)
        if (Retrieve(Stream_Video, Pos, Video_Format_Commercial_IfAny).empty() && Retrieve(Stream_Video, Pos, Video_Format)==__T("VC-3") && Retrieve(Stream_Video, Pos, Video_Format_Profile).find(__T("HD"))==0)
        {
            //http://www.avid.com/static/resources/US/documents/dnxhd.pdf
            int64u Height=Retrieve(Stream_Video, Pos, Video_Height).To_int64u();
            int64u BitRate=float64_int64s(Retrieve(Stream_Video, Pos, Video_BitRate).To_float64()/1000000);
            int64u FrameRate=float64_int64s(Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64());
            int64u BitRate_Final=0;
            if (Height>=900 && Height<=1300)
            {
                if (FrameRate==60)
                {
                    if (BitRate>=420 && BitRate<440) //440
                        BitRate_Final=440;
                    if (BitRate>=271 && BitRate<311) //291
                        BitRate_Final=290;
                    if (BitRate>=80 && BitRate<100) //90
                        BitRate_Final=90;
                }
                if (FrameRate==50)
                {
                    if (BitRate>=347 && BitRate<387) //367
                        BitRate_Final=365;
                    if (BitRate>=222 && BitRate<262) //242
                        BitRate_Final=240;
                    if (BitRate>=65 && BitRate<85) //75
                        BitRate_Final=75;
                }
                if (FrameRate==30)
                {
                    if (BitRate>=420 && BitRate<440) //440
                        BitRate_Final=440;
                    if (BitRate>=200 && BitRate<240) //220
                        BitRate_Final=220;
                    if (BitRate>=130 && BitRate<160) //145
                        BitRate_Final=145;
                    if (BitRate>=90 && BitRate<110) //100
                        BitRate_Final=100;
                    if (BitRate>=40 && BitRate<50) //45
                        BitRate_Final=45;
                }
                if (FrameRate==25)
                {
                    if (BitRate>=347 && BitRate<387) //367
                        BitRate_Final=365;
                    if (BitRate>=164 && BitRate<204) //184
                        BitRate_Final=185;
                    if (BitRate>=111 && BitRate<131) //121
                        BitRate_Final=120;
                    if (BitRate>=74 && BitRate<94) //84
                        BitRate_Final=85;
                    if (BitRate>=31 && BitRate<41) //36
                        BitRate_Final=36;
                }
                if (FrameRate==24)
                {
                    if (BitRate>=332 && BitRate<372) //352
                        BitRate_Final=350;
                    if (BitRate>=156 && BitRate<196) //176
                        BitRate_Final=175;
                    if (BitRate>=105 && BitRate<125) //116
                        BitRate_Final=116;
                    if (BitRate>=70 && BitRate<90) //80
                        BitRate_Final=80;
                    if (BitRate>=31 && BitRate<41) //36
                        BitRate_Final=36;
                }
            }
            if (Height>=600 && Height<=800)
            {
                if (FrameRate==60)
                {
                    if (BitRate>=200 && BitRate<240) //220
                        BitRate_Final=220;
                    if (BitRate>=130 && BitRate<160) //145
                        BitRate_Final=145;
                    if (BitRate>=90 && BitRate<110) //110
                        BitRate_Final=100;
                }
                if (FrameRate==50)
                {
                    if (BitRate>=155 && BitRate<195) //175
                        BitRate_Final=175;
                    if (BitRate>=105 && BitRate<125) //115
                        BitRate_Final=115;
                    if (BitRate>=75 && BitRate<95) //85
                        BitRate_Final=85;
                }
                if (FrameRate==30)
                {
                    if (BitRate>=100 && BitRate<120) //110
                        BitRate_Final=110;
                    if (BitRate>=62 && BitRate<82) //72
                        BitRate_Final=75;
                    if (BitRate>=44 && BitRate<56) //51
                        BitRate_Final=50;
                }
                if (FrameRate==25)
                {
                    if (BitRate>=82 && BitRate<102) //92
                        BitRate_Final=90;
                    if (BitRate>=55 && BitRate<65) //60
                        BitRate_Final=60;
                    if (BitRate>=38 && BitRate<48) //43
                        BitRate_Final=45;
                }
                if (FrameRate==24)
                {
                    if (BitRate>=78 && BitRate<98) //88
                        BitRate_Final=90;
                    if (BitRate>=53 && BitRate<63) //58
                        BitRate_Final=60;
                    if (BitRate>=36 && BitRate<46) //41
                        BitRate_Final=41;
                }
            }

            if (BitRate_Final)
            {
                int64u BitDepth=Retrieve(Stream_Video, Pos, Video_BitDepth).To_int64u();
                if (BitDepth==8 || BitDepth==10)
                    Fill(Stream_Video, Pos, Video_Format_Commercial_IfAny, __T("DNxHD ")+Ztring::ToZtring(BitRate_Final)+(BitDepth==10?__T("x"):__T(""))); //"x"=10-bit
            }
        }
        if (Retrieve(Stream_Video, Pos, Video_Format_Commercial_IfAny).empty() && Retrieve(Stream_Video, Pos, Video_Format)==__T("VC-3") && Retrieve(Stream_Video, Pos, Video_Format_Profile).find(__T("RI@"))==0)
        {
            Fill(Stream_Video, Pos, Video_Format_Commercial_IfAny, __T("DNxHR ")+Retrieve(Stream_Video, Pos, Video_Format_Profile).substr(3));
        }
    #endif //defined(MEDIAINFO_VC3_YES)
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Audio(size_t Pos)
{
    // 
    if (Retrieve(Stream_Audio, Pos, Audio_StreamSize_Encoded)==Retrieve(Stream_Audio, Pos, Audio_StreamSize))
        Clear(Stream_Audio, Pos, Audio_StreamSize_Encoded);
    if (Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded)==Retrieve(Stream_Audio, Pos, Audio_BitRate))
        Clear(Stream_Audio, Pos, Audio_BitRate_Encoded);

    //Dolby ED2 merge
    if (Retrieve(Stream_Audio, Pos, Audio_Format)==__T("Dolby ED2"))
    {
        int64u BitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_int64u();
        int64u BitRate_Encoded=Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).To_int64u();
        int64u StreamSize=Retrieve(Stream_Audio, Pos, Audio_StreamSize).To_int64u();
        int64u StreamSize_Encoded=Retrieve(Stream_Audio, Pos, Audio_StreamSize_Encoded).To_int64u();
        for (size_t i=Pos+1; i<Count_Get(Stream_Audio);)
        {
            size_t OtherID_Count;
            Ztring OtherStreamOrder;
            Ztring OtherID;
            Ztring OtherID_String;
            if (Retrieve_Const(Stream_Audio, i, Audio_Format)==__T("Dolby ED2"))
            {
                //if (Retrieve_Const(Stream_Audio, i, Audio_Channel_s_).To_int64u())
                if (!Retrieve_Const(Stream_Audio, i, "Presentation0").empty())
                    break; // It is the next ED2
                OtherID_Count=0;
                OtherStreamOrder=Retrieve(Stream_Audio, i, Audio_StreamOrder);
                OtherID=Retrieve(Stream_Audio, i, Audio_ID);
                OtherID_String =Retrieve(Stream_Audio, i, Audio_ID_String);
            }
            if (i+7<Count_Get(Stream_Audio) // 8 tracks Dolby E
             && Retrieve_Const(Stream_Audio, i  , Audio_Format)==__T("Dolby E")
             && Retrieve_Const(Stream_Audio, i+7, Audio_Format)==__T("Dolby E"))
            {
                Ztring NextStreamOrder=Retrieve_Const(Stream_Audio, i, Audio_StreamOrder);
                Ztring NextID=Retrieve_Const(Stream_Audio, i, Audio_ID);
                size_t NextID_DashPos=NextID.rfind(__T('-'));
                if (NextID_DashPos!=(size_t)-1)
                    NextID.erase(NextID_DashPos);
                if (Retrieve_Const(Stream_Audio, i+7, Audio_ID)==NextID+__T("-8"))
                {
                    OtherID_Count=7;
                    OtherStreamOrder=NextStreamOrder;
                    OtherID=NextID;
                }
                NextID=Retrieve_Const(Stream_Audio, i, Audio_ID_String);
                NextID_DashPos=NextID.rfind(__T('-'));
                if (NextID_DashPos!=(size_t)-1)
                    NextID.erase(NextID_DashPos);
                if (Retrieve_Const(Stream_Audio, i+7, Audio_ID_String)==NextID+__T("-8"))
                {
                    OtherID_String=NextID;
                }
            }
            if (OtherID.empty())
                break;

            size_t OtherID_DashPos=OtherID.rfind(__T('-'));
            if (OtherID_DashPos!=(size_t)-1)
                OtherID.erase(0, OtherID_DashPos+1);
            if (!OtherID.empty() && OtherID[0]==__T('(') && OtherID[OtherID.size()-1]==__T(')'))
            {
                OtherID.resize(OtherID.size()-1);
                OtherID.erase(0, 1);
            }
            Ztring ID=Retrieve(Stream_Audio, Pos, Audio_ID);
            if (!ID.empty() && ID[ID.size()-1]==__T(')'))
            {
                ID.resize(ID.size()-1);
                ID+=__T(" / ");
                ID+=OtherID;
                ID+=__T(')');
                Fill(Stream_Audio, Pos, Audio_ID, ID, true);
            }
            else
            {
                Ztring CurrentID_String=Retrieve(Stream_Audio, Pos, Audio_ID_String);
                if (Retrieve_Const(Stream_General, 0, General_Format)==__T("MPEG-TS"))
                {
                    auto ProgramSeparator=OtherStreamOrder.find(__T('-'));
                    if (ProgramSeparator!=string::npos)
                        OtherStreamOrder.erase(0, ProgramSeparator+1);
                }
                Fill(Stream_Audio, Pos, Audio_StreamOrder, OtherStreamOrder);
                Fill(Stream_Audio, Pos, Audio_ID, OtherID);
                Fill(Stream_Audio, Pos, Audio_ID_String, CurrentID_String+__T(" / ")+OtherID_String, true);
            }
            for (size_t j=i+OtherID_Count; j>=i; j--)
            {
                BitRate+=Retrieve(Stream_Audio, j, Audio_BitRate).To_int64u();
                BitRate_Encoded+=Retrieve(Stream_Audio, j, Audio_BitRate_Encoded).To_int64u();
                StreamSize+=Retrieve(Stream_Audio, j, Audio_StreamSize).To_int64u();
                StreamSize_Encoded+=Retrieve(Stream_Audio, j, Audio_StreamSize_Encoded).To_int64u();
                Stream_Erase(Stream_Audio, j);
            }

            ZtringList List[6];
            for (size_t j=0; j<6; j++)
                List[j].Separator_Set(0, __T(" / "));
            List[0].Write(Get(Stream_Menu, 0, __T("Format")));
            List[1].Write(Get(Stream_Menu, 0, __T("Format/String")));
            List[2].Write(Get(Stream_Menu, 0, __T("List_StreamKind")));
            List[3].Write(Get(Stream_Menu, 0, __T("List_StreamPos")));
            List[4].Write(Get(Stream_Menu, 0, __T("List")));
            List[5].Write(Get(Stream_Menu, 0, __T("List/String")));
            bool IsNok=false;
            for (size_t j=0; j<6; j++)
                if (!List[j].empty() && List[j].size()!=List[3].size())
                    IsNok=true;
            if (!IsNok && !List[2].empty() && List[2].size()==List[3].size())
            {
                size_t Audio_Begin;
                for (Audio_Begin=0; Audio_Begin <List[2].size(); Audio_Begin++)
                    if (List[2][Audio_Begin]==__T("2"))
                        break;
                if (Audio_Begin!=List[2].size())
                {
                    for (size_t j=0; j<6; j++)
                        if (!List[j].empty() && Audio_Begin+i<List[j].size())
                            List[j].erase(List[j].begin()+Audio_Begin+i);
                    size_t Audio_End;
                    for (Audio_End=Audio_Begin+1; Audio_End<List[2].size(); Audio_End++)
                        if (List[2][Audio_End]!=__T("2"))
                            break;
                    for (size_t j=Audio_Begin+i; j<Audio_End; j++)
                        List[3][j].From_Number(List[3][j].To_int32u()-1-OtherID_Count);
                    Fill(Stream_Menu, 0, "Format", List[0].Read(), true);
                    Fill(Stream_Menu, 0, "Format/String", List[1].Read(), true);
                    Fill(Stream_Menu, 0, "List_StreamKind", List[2].Read(), true);
                    Fill(Stream_Menu, 0, "List_StreamPos", List[3].Read(), true);
                    Fill(Stream_Menu, 0, "List", List[4].Read(), true);
                    Fill(Stream_Menu, 0, "List/String", List[5].Read(), true);
                }
            }
        }
        if (BitRate)
            Fill(Stream_Audio, Pos, Audio_BitRate, BitRate, 10, true);
        if (BitRate_Encoded)
            Fill(Stream_Audio, Pos, Audio_BitRate_Encoded, BitRate_Encoded, 10, true);
        if (StreamSize)
            Fill(Stream_Audio, Pos, Audio_StreamSize, StreamSize, 10, true);
        if (StreamSize_Encoded)
            Fill(Stream_Audio, Pos, Audio_StreamSize_Encoded, StreamSize_Encoded, 10, true);
    }

    //Channels
    if (Retrieve(Stream_Audio, Pos, Audio_Channel_s_).empty())
    {
        const Ztring& CodecID=Retrieve(Stream_Audio, Pos, Audio_CodecID);
        if (CodecID==__T("samr")
         || CodecID==__T("sawb")
         || CodecID==__T("7A21")
         || CodecID==__T("7A22"))
        Fill(Stream_Audio, Pos, Audio_Channel_s_, 1); //AMR is always with 1 channel
    }

    //SamplingCount
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingCount).empty())
    {
        float64 Duration=Retrieve(Stream_Audio, Pos, Audio_Duration).To_float64();
        bool DurationFromGeneral; 
        if (Duration==0)
        {
            Duration=Retrieve(Stream_General, 0, General_Duration).To_float64();
            DurationFromGeneral=Retrieve(Stream_General, 0, General_Format)!=Retrieve(Stream_Audio, Pos, Audio_Format);
        }
        else
            DurationFromGeneral=false;
        float64 SamplingRate=Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_float64();
        if (Duration && SamplingRate)
        {
            Fill(Stream_Audio, Pos, Audio_SamplingCount, Duration/1000*SamplingRate, 0);
            if (DurationFromGeneral && Retrieve_Const(Stream_Audio, Pos, Audio_Format)!=Retrieve_Const(Stream_General, 0, General_Format))
            {
                Fill(Stream_Audio, Pos, "SamplingCount_Source", "General_Duration");
                Fill_SetOptions(Stream_Audio, Pos, "SamplingCount_Source", "N NTN");
            }
        }
    }

    //Frame count
    if (Retrieve(Stream_Audio, Pos, Audio_FrameCount).empty() && Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
    {
        if (Count_Get(Stream_Video)==0 && Count_Get(Stream_Audio)==1)
            Fill(Stream_Audio, 0, Audio_FrameCount, Frame_Count_NotParsedIncluded);
    }

    //FrameRate same as SampleRate
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_float64() == Retrieve(Stream_Audio, Pos, Audio_FrameRate).To_float64())
        Clear(Stream_Audio, Pos, Audio_FrameRate);

    //SamplingRate
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingRate).empty())
    {
        float64 BitDepth=Retrieve(Stream_Audio, Pos, Audio_BitDepth).To_float64();
        float64 Channels=Retrieve(Stream_Audio, Pos, Audio_Channel_s_).To_float64();
        float64 BitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_float64();
        if (BitDepth && Channels && BitRate)
            Fill(Stream_Audio, Pos, Audio_SamplingRate, BitRate/Channels/BitDepth, 0);
    }

    //SamplesPerFrames
    if (Retrieve(Stream_Audio, Pos, Audio_SamplesPerFrame).empty())
    {
        float64 FrameRate=Retrieve(Stream_Audio, Pos, Audio_FrameRate).To_float64();
        float64 SamplingRate=0;
        ZtringList SamplingRates;
        SamplingRates.Separator_Set(0, " / ");
        SamplingRates.Write(Retrieve(Stream_Audio, Pos, Audio_SamplingRate));
        for (size_t i=0; i<SamplingRates.size(); ++i)
        {
            SamplingRate = SamplingRates[i].To_float64();
            if (SamplingRate)
                break; // Using the first valid one
        }
        if (FrameRate && SamplingRate && FrameRate!=SamplingRate)
        {
            float64 SamplesPerFrameF=SamplingRate/FrameRate;
            Ztring SamplesPerFrame;
            if (SamplesPerFrameF>1601 && SamplesPerFrameF<1602)
                SamplesPerFrame = __T("1601.6"); // Usually this is 29.970 fps PCM. TODO: check if it is OK in all cases
            else if (SamplesPerFrameF>800 && SamplesPerFrameF<801)
                SamplesPerFrame = __T("800.8"); // Usually this is 59.940 fps PCM. TODO: check if it is OK in all cases
            else
                SamplesPerFrame.From_Number(SamplesPerFrameF, 0);
            Fill(Stream_Audio, Pos, Audio_SamplesPerFrame, SamplesPerFrame);
        }
    }

    //ChannelLayout
    if (Retrieve_Const(Stream_Audio, Pos, Audio_ChannelLayout).empty())
    {
        ZtringList ChannelLayout_List;
        ChannelLayout_List.Separator_Set(0, __T(" "));
        ChannelLayout_List.Write(Retrieve_Const(Stream_Audio, Pos, Audio_ChannelLayout));
        size_t ChannelLayout_List_SizeBefore=ChannelLayout_List.size();
        
        size_t NumberOfSubstreams=(size_t)Retrieve_Const(Stream_Audio, Pos, "NumberOfSubstreams").To_int64u();
        for (size_t i=0; i<NumberOfSubstreams; i++)
        {
            static const char* const Places[]={ "ChannelLayout", "BedChannelConfiguration" };
            static constexpr size_t Places_Size=sizeof(Places)/sizeof(decltype(*Places));
            for (const auto Place : Places)
            {
                ZtringList AdditionaChannelLayout_List;
                AdditionaChannelLayout_List.Separator_Set(0, __T(" "));
                AdditionaChannelLayout_List.Write(Retrieve_Const(Stream_Audio, Pos, ("Substream"+std::to_string(i)+' '+Place).c_str()));
                for (auto& AdditionaChannelLayout_Item: AdditionaChannelLayout_List)
                {
                    if (std::find(ChannelLayout_List.cbegin(), ChannelLayout_List.cend(), AdditionaChannelLayout_Item)==ChannelLayout_List.cend())
                        ChannelLayout_List.push_back(std::move(AdditionaChannelLayout_Item));
                }
            }
        }
        if (ChannelLayout_List.size()!=ChannelLayout_List_SizeBefore)
        {
            Fill(Stream_Audio, Pos, Audio_Channel_s_, ChannelLayout_List.size(), 10, true);
            Clear(Stream_Audio, Pos, Audio_ChannelPositions);
            Fill(Stream_Audio, Pos, Audio_ChannelLayout, ChannelLayout_List.Read(), true);
        }
    }

    //Channel(s)
    if (Retrieve_Const(Stream_Audio, Pos, Audio_Channel_s_).empty())
    {
        size_t NumberOfSubstreams=(size_t)Retrieve_Const(Stream_Audio, Pos, "NumberOfSubstreams").To_int64u();
        if (NumberOfSubstreams==1)
        {
            auto Channels=Retrieve_Const(Stream_Audio, Pos, "Substream0 Channel(s)").To_int32u();
            if (Channels)
                Fill(Stream_Audio, Pos, Audio_Channel_s_, Channels);
        }
    }

    //Duration
    if (Retrieve(Stream_Audio, Pos, Audio_Duration).empty())
    {
        float64 SamplingRate=Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_float64();
        if (SamplingRate)
        {
            float64 Duration=Retrieve(Stream_Audio, Pos, Audio_SamplingCount).To_float64()*1000/SamplingRate;
            if (Duration)
            {
                Fill(Stream_Audio, Pos, Audio_Duration, Duration, 0);
                Ztring Source=Retrieve(Stream_Audio, Pos, "SamplingCount_Source");
                if (!Source.empty())
                {
                    Fill(Stream_Audio, Pos, "Duration_Source", Source);
                    Fill_SetOptions(Stream_Audio, Pos, "Duration_Source", "N NTN");
                }
            }
        }
    }

    //Stream size
    if (Retrieve(Stream_Audio, Pos, Audio_StreamSize).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate_Mode)==__T("CBR"))
    {
        float64 Duration=Retrieve(Stream_Audio, Pos, Audio_Duration).To_float64();
        float64 BitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_float64();
        if (Duration && BitRate)
            Fill(Stream_Audio, Pos, Audio_StreamSize, Duration*BitRate/8/1000, 0, true);
    }
    if (Retrieve(Stream_Audio, Pos, Audio_StreamSize_Encoded).empty() && !Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate_Mode)==__T("CBR"))
    {
        float64 Duration=Retrieve(Stream_Audio, Pos, Audio_Duration).To_float64();
        float64 BitRate_Encoded=Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).To_float64();
        if (Duration)
            Fill(Stream_Audio, Pos, Audio_StreamSize_Encoded, Duration*BitRate_Encoded/8/1000, 0, true);
    }

    //CBR/VBR
    if (Retrieve(Stream_Audio, Pos, Audio_BitRate_Mode).empty() && !Retrieve(Stream_Audio, Pos, Audio_Codec).empty())
    {
        Ztring Z1=MediaInfoLib::Config.Codec_Get(Retrieve(Stream_Audio, Pos, Audio_Codec), InfoCodec_BitRate_Mode, Stream_Audio);
        if (!Z1.empty())
            Fill(Stream_Audio, Pos, Audio_BitRate_Mode, Z1);
    }

    //Commercial name
    if (Retrieve(Stream_Audio, Pos, Audio_Format_Commercial_IfAny).empty() && Retrieve(Stream_Audio, Pos, Audio_Format)==__T("USAC") && Retrieve(Stream_Audio, Pos, Audio_Format_Profile).rfind(__T("Extended HE AAC@"), 0)==0)
    {
        Fill(Stream_Audio, Pos, Audio_Format_Commercial_IfAny, "xHE-AAC");
    }

    //Timestamp to timecode
    auto FramesPerSecondF=Retrieve_Const(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate").To_float32();
    auto DropFrame=Retrieve_Const(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate_DropFrame")==__T("Yes");
    auto SamplingRate=Retrieve_Const(Stream_Audio, 0, Audio_SamplingRate).To_int32u();
    const auto& FirstFrameOfAction=Retrieve_Const(Stream_Audio, 0, "Dolby_Atmos_Metadata FirstFrameOfAction");
    if (!FirstFrameOfAction.empty())
    {
        auto TC_FirstFrameOfAction=TimeCode(FirstFrameOfAction.To_UTF8());
        Merge_FillTimeCode(*this, "Dolby_Atmos_Metadata FirstFrameOfAction", TC_FirstFrameOfAction, FramesPerSecondF, DropFrame, TimeCode::Ceil, SamplingRate);
    }
    const auto& Start=Retrieve_Const(Stream_Audio, 0, "Programme0 Start");
    if (!Start.empty())
    {
        auto TC_End=TimeCode(Start.To_UTF8());
        Merge_FillTimeCode(*this, "Programme0 Start", TC_End, FramesPerSecondF, DropFrame, TimeCode::Floor, SamplingRate);
    }
    const auto& End=Retrieve_Const(Stream_Audio, 0, "Programme0 End");
    if (!End.empty())
    {
        auto TC_End=TimeCode(End.To_UTF8());
        Merge_FillTimeCode(*this, "Programme0 End", TC_End, FramesPerSecondF, DropFrame, TimeCode::Ceil, SamplingRate);
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Text(size_t Pos)
{
    //FrameRate from FrameCount and Duration
    if (Retrieve(Stream_Text, Pos, Text_FrameRate).empty())
    {
        int64u FrameCount=Retrieve(Stream_Text, Pos, Text_FrameCount).To_int64u();
        float64 Duration=Retrieve(Stream_Text, Pos, Text_Duration).To_float64()/1000;
        if (FrameCount && Duration)
           Fill(Stream_Text, Pos, Text_FrameRate, FrameCount/Duration, 3);
    }

    //FrameCount from Duration and FrameRate
    if (Retrieve(Stream_Text, Pos, Text_FrameCount).empty())
    {
        float64 Duration=Retrieve(Stream_Text, Pos, Text_Duration).To_float64()/1000;
        float64 FrameRate=Retrieve(Stream_Text, Pos, Text_FrameRate).To_float64();
        if (Duration && FrameRate)
           Fill(Stream_Text, Pos, Text_FrameCount, float64_int64s(Duration*FrameRate));
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Other(size_t UNUSED(StreamPos))
{
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Image(size_t Pos)
{
    //Commercial name
    if (Retrieve(Stream_Image, Pos, Image_HDR_Format_Compatibility).rfind(__T("HDR10"), 0)==0
     && ((!Retrieve(Stream_Image, Pos, Image_BitDepth).empty() && Retrieve(Stream_Image, Pos, Image_BitDepth).To_int64u()<10) //e.g. ProRes has not bitdepth info
      || Retrieve(Stream_Image, Pos, Image_colour_primaries)!=__T("BT.2020")
      || Retrieve(Stream_Image, Pos, Image_transfer_characteristics)!=__T("PQ")
      || Retrieve(Stream_Image, Pos, Image_MasteringDisplay_ColorPrimaries).empty()
        ))
    {
        //We actually fill HDR10/HDR10+ by default, so it will be removed below if not fitting in the color related rules
        Clear(Stream_Image, Pos, Image_HDR_Format_Compatibility);
    }
    if (Retrieve(Stream_Image, Pos, Image_HDR_Format_String).empty())
    {
        ZtringList Summary;
        Summary.Separator_Set(0, __T(" / "));
        Summary.Write(Retrieve(Stream_Image, Pos, Image_HDR_Format));
        ZtringList Commercial=Summary;
        if (!Summary.empty())
        {
            ZtringList HDR_Format_Compatibility;
            HDR_Format_Compatibility.Separator_Set(0, __T(" / "));
            HDR_Format_Compatibility.Write(Retrieve(Stream_Image, Pos, Image_HDR_Format_Compatibility));
            HDR_Format_Compatibility.resize(Summary.size());
            ZtringList ToAdd;
            ToAdd.Separator_Set(0, __T(" / "));
            for (size_t i=Image_HDR_Format_String+1; i<=Image_HDR_Format_Settings; i++)
            {
                ToAdd.Write(Retrieve(Stream_Image, Pos, i));
                ToAdd.resize(Summary.size());
                for (size_t j=0; j<Summary.size(); j++)
                {
                    if (!ToAdd[j].empty())
                    {
                        switch (i)
                        {
                            case Image_HDR_Format_Version: Summary[j]+=__T(", Version "); break;
                            case Image_HDR_Format_Level: Summary[j]+=__T('.'); break;
                            default: Summary[j] += __T(", ");
                        }
                        Summary[j]+=ToAdd[j];
                    }
                }
            }
            for (size_t j=0; j<Summary.size(); j++)
                if (!HDR_Format_Compatibility[j].empty())
                {
                    Summary[j]+=__T(", ")+HDR_Format_Compatibility[j]+__T(" compatible");
                    Commercial[j]=HDR_Format_Compatibility[j];
                    if (!Commercial[j].empty())
                    {
                        auto Commercial_Reduce=Commercial[j].find(__T(' '));
                        if (Commercial_Reduce<Commercial[j].size()-1 && Commercial[j][Commercial_Reduce+1]>='0' && Commercial[j][Commercial_Reduce+1]<='9')
                            Commercial_Reduce=Commercial[j].find(__T(' '), Commercial_Reduce+1);
                        if (Commercial_Reduce!=string::npos)
                            Commercial[j].resize(Commercial_Reduce);
                    }
                }
            Fill(Stream_Image, Pos, Image_HDR_Format_String, Summary.Read());
            Fill(Stream_Image, Pos, Image_HDR_Format_Commercial, Commercial.Read());
        }
    }

    if (Retrieve(Stream_Image, Pos, Image_Type_String).empty())
    {
        const auto& Type=Retrieve_Const(Stream_Image, Pos, Image_Type);
        if (!Type.empty())
        {
            auto Type_String=__T("Type_")+Type;
            auto Type_String2=MediaInfoLib::Config.Language_Get(Type_String);
            if (Type_String2==Type_String)
                Type_String2=Type;
            Fill(Stream_Image, Pos, Image_Type_String, Type_String2);
        }
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Menu(size_t UNUSED(StreamPos))
{
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_InterStreams()
{
    //Duration if General not filled
    if (Retrieve(Stream_General, 0, General_Duration).empty())
    {
        int64u Duration=0;
        //For all streams (Generic)
        for (size_t StreamKind=Stream_Video; StreamKind<Stream_Max; StreamKind++)
            for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind); Pos++)
            {
                if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).empty())
                {
                    int64u Duration_Stream=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).To_int64u();
                    if (Duration_Stream>Duration)
                        Duration=Duration_Stream;
                }
            }

        //Filling
        if (Duration>0)
            Fill(Stream_General, 0, General_Duration, Duration);
    }

    //(*Stream) size if all stream sizes are OK
    if (Retrieve(Stream_General, 0, General_StreamSize).empty())
    {
        int64u StreamSize_Total=0;
        bool IsOK=true;
        //For all streams (Generic)
        for (size_t StreamKind=Stream_Video; StreamKind<Stream_Max; StreamKind++)
        {
                for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind); Pos++)
                {
                    if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize_Encoded)).empty())
                        StreamSize_Total+=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize_Encoded)).To_int64u();
                    else if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Source_StreamSize)).empty())
                        StreamSize_Total+=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Source_StreamSize)).To_int64u();
                    else if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize)).empty())
                        StreamSize_Total+=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize)).To_int64u();
                    else if (StreamKind!=Stream_Other && StreamKind!=Stream_Menu) //They have no big size, we never calculate them
                        IsOK=false; //StreamSize not available for 1 stream, we can't calculate
                }
        }

        //Filling
        auto StreamSize=File_Size-StreamSize_Total;
        if (IsOK && StreamSize_Total>0 && StreamSize_Total<File_Size && (File_Size==StreamSize_Total || !StreamSize || StreamSize>=4)) //to avoid strange behavior due to rounding, TODO: avoid rounding
            Fill(Stream_General, 0, General_StreamSize, StreamSize);
    }

    //OverallBitRate if we have one Audio stream with bitrate
    if (Retrieve(Stream_General, 0, General_Duration).empty() && Retrieve(Stream_General, 0, General_OverallBitRate).empty() && Count_Get(Stream_Video) == 0 && Count_Get(Stream_Audio) == 1 && Retrieve(Stream_Audio, 0, Audio_BitRate).To_int64u() != 0 && (Retrieve(Stream_General, 0, General_Format) == Retrieve(Stream_Audio, 0, Audio_Format) || !Retrieve(Stream_General, 0, General_HeaderSize).empty()))
    {
        const Ztring& EncodedBitRate=Retrieve_Const(Stream_Audio, 0, Audio_BitRate_Encoded);
        Fill(Stream_General, 0, General_OverallBitRate, EncodedBitRate.empty()?Retrieve_Const(Stream_Audio, 0, Audio_BitRate):EncodedBitRate);
    }

    //OverallBitRate if Duration
    if (Retrieve(Stream_General, 0, General_OverallBitRate).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()!=0 && !Retrieve(Stream_General, 0, General_FileSize).empty())
    {
        float64 Duration=0;
        if (Count_Get(Stream_Video)==1 && Retrieve(Stream_General, 0, General_Duration)==Retrieve(Stream_Video, 0, General_Duration) && !Retrieve(Stream_Video, 0, Video_FrameCount).empty() && !Retrieve(Stream_Video, 0, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(Stream_General, 0, General_Duration).To_float64();
        Fill(Stream_General, 0, General_OverallBitRate, Retrieve(Stream_General, 0, General_FileSize).To_int64u()*8*1000/Duration, 0);
    }

    //Duration if OverallBitRate
    if (Retrieve(Stream_General, 0, General_Duration).empty() && Retrieve(Stream_General, 0, General_OverallBitRate).To_int64u()!=0)
        Fill(Stream_General, 0, General_Duration, Retrieve(Stream_General, 0, General_FileSize).To_float64()*8*1000/Retrieve(Stream_General, 0, General_OverallBitRate).To_float64(), 0);

    //Video bitrate can be the nominal one if <4s (bitrate estimation is not enough precise
    if (Count_Get(Stream_Video)==1 && Retrieve(Stream_Video, 0, Video_BitRate).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()<4000)
    {
        Fill(Stream_Video, 0, Video_BitRate, Retrieve(Stream_Video, 0, Video_BitRate_Nominal));
        Clear(Stream_Video, 0, Video_BitRate_Nominal);
    }

    //Video bitrate if we have all audio bitrates and overal bitrate
    if (Count_Get(Stream_Video)==1 && Retrieve(Stream_General, 0, General_OverallBitRate).size()>4 && Retrieve(Stream_Video, 0, Video_BitRate).empty() && Retrieve(Stream_Video, 0, Video_BitRate_Encoded).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()>=1000) //BitRate is > 10 000 and Duration>10s, to avoid strange behavior
    {
        double GeneralBitRate_Ratio=0.98;  //Default container overhead=2%
        int32u GeneralBitRate_Minus=5000;  //5000 bps because of a "classic" stream overhead
        double VideoBitRate_Ratio  =0.98;  //Default container overhead=2%
        int32u VideoBitRate_Minus  =2000;  //2000 bps because of a "classic" stream overhead
        double AudioBitRate_Ratio  =0.98;  //Default container overhead=2%
        int32u AudioBitRate_Minus  =2000;  //2000 bps because of a "classic" stream overhead
        double TextBitRate_Ratio   =0.98;  //Default container overhead=2%
        int32u TextBitRate_Minus   =2000;  //2000 bps because of a "classic" stream overhead
        //Specific value depends of Container
        if (StreamSource==IsStream)
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =1;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =1;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =1;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MPEG-TS"))
        {
            GeneralBitRate_Ratio=0.98;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.97;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.96;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.96;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MPEG-PS"))
        {
            GeneralBitRate_Ratio=0.99;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.99;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.99;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.99;
            TextBitRate_Minus   =0;
        }
        if (MediaInfoLib::Config.Format_Get(Retrieve(Stream_General, 0, General_Format), InfoFormat_KindofFormat)==__T("MPEG-4"))
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =1;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =1;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =1;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("Matroska"))
        {
            GeneralBitRate_Ratio=0.99;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.99;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.99;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.99;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MXF"))
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=1000;
            VideoBitRate_Ratio  =1.00;
            VideoBitRate_Minus  =1000;
            AudioBitRate_Ratio  =1.00;
            AudioBitRate_Minus  =1000;
            TextBitRate_Ratio   =1.00;
            TextBitRate_Minus   =1000;
        }

        //Testing
        float64 VideoBitRate=Retrieve(Stream_General, 0, General_OverallBitRate).To_float64()*GeneralBitRate_Ratio-GeneralBitRate_Minus;
        bool VideobitRateIsValid=true;
        for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
        {
            float64 AudioBitRate=0;
            if (!Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded)[0]<=__T('9')) //Note: quick test if it is a number
                AudioBitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).To_float64();
            else if (!Retrieve(Stream_Audio, Pos, Audio_BitRate).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate)[0]<=__T('9')) //Note: quick test if it is a number
                AudioBitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_float64();
            else
                VideobitRateIsValid=false;
            if (VideobitRateIsValid && AudioBitRate_Ratio)
                VideoBitRate-=AudioBitRate/AudioBitRate_Ratio+AudioBitRate_Minus;
        }
        for (size_t Pos=0; Pos<Count_Get(Stream_Text); Pos++)
        {
            float64 TextBitRate;
            if (Retrieve(Stream_Text, Pos, Text_BitRate_Encoded).empty())
                TextBitRate=Retrieve(Stream_Text, Pos, Text_BitRate).To_float64();
            else
                TextBitRate=Retrieve(Stream_Text, Pos, Text_BitRate_Encoded).To_float64();
            if (TextBitRate_Ratio)
                VideoBitRate-=TextBitRate/TextBitRate_Ratio+TextBitRate_Minus;
            else
                VideoBitRate-=1000; //Estimation: Text stream are not often big
        }
        if (VideobitRateIsValid && VideoBitRate>=10000) //to avoid strange behavior
        {
            VideoBitRate=VideoBitRate*VideoBitRate_Ratio-VideoBitRate_Minus;
            Fill(Stream_Video, 0, Video_BitRate, VideoBitRate, 0); //Default container overhead=2%
            if (Retrieve(Stream_Video, 0, Video_StreamSize).empty() && !Retrieve(Stream_Video, 0, Video_Duration).empty())
            {
                float64 Duration=0;
                if (!Retrieve(Stream_Video, 0, Video_FrameCount).empty() && !Retrieve(Stream_Video, 0, Video_FrameRate).empty())
                {
                    int64u FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount).To_int64u();
                    float64 FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate).To_float64();
                    if (FrameCount && FrameRate)
                        Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
                }
                if (Duration==0)
                    Duration=Retrieve(Stream_Video, 0, Video_Duration).To_float64();
                if (Duration)
                {
                    int64u StreamSize=float64_int64s(VideoBitRate/8*Duration/1000);
                    if (StreamSource==IsStream && File_Size!=(int64u)-1 && StreamSize>=File_Size*0.99)
                        StreamSize=File_Size;
                    Fill(Stream_Video, 0, Video_StreamSize, StreamSize);
                }
            }
        }
    }

    //General stream size if we have all streamsize
    if (File_Size!=(int64u)-1 && Retrieve(Stream_General, 0, General_StreamSize).empty())
    {
        //Testing
        int64s StreamSize=File_Size;
        bool StreamSizeIsValid=true;
        for (size_t StreamKind_Pos=Stream_General+1; StreamKind_Pos<Stream_Menu; StreamKind_Pos++)
            for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind_Pos); Pos++)
            {
                int64u StreamXX_StreamSize=0;
                if (!Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize_Encoded)).empty())
                    StreamXX_StreamSize+=Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize_Encoded)).To_int64u();
                else if (!Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize)).empty())
                    StreamXX_StreamSize+=Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize)).To_int64u();
                if (StreamXX_StreamSize>0 || StreamKind_Pos==Stream_Text)
                    StreamSize-=StreamXX_StreamSize;
                else
                    StreamSizeIsValid=false;
            }
        if (StreamSizeIsValid && (!StreamSize || StreamSize>=4)) //to avoid strange behavior due to rounding, TODO: avoid rounding
            Fill(Stream_General, 0, General_StreamSize, StreamSize);
    }

    //General_OverallBitRate_Mode
    if (Retrieve(Stream_General, 0, General_OverallBitRate_Mode).empty())
    {
        bool IsValid=false;
        bool IsCBR=true;
        bool IsVBR=false;
        for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Menu; StreamKind++)
        {
            if (StreamKind==Stream_Image)
                continue;
            for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            {
                if (!IsValid)
                    IsValid=true;
                if (Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_BitRate_Mode))!=__T("CBR"))
                    IsCBR=false;
                if (Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_BitRate_Mode))==__T("VBR"))
                    IsVBR=true;
            }
        }
        if (IsValid)
        {
            if (IsCBR)
                Fill(Stream_General, 0, General_OverallBitRate_Mode, "CBR");
            if (IsVBR)
                Fill(Stream_General, 0, General_OverallBitRate_Mode, "VBR");
        }
    }

    //FrameRate if General not filled
    if (Retrieve(Stream_General, 0, General_FrameRate).empty() && Count_Get(Stream_Video))
    {
        Ztring FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate);
        bool IsOk=true;
        if (FrameRate.empty())
        {
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Ztring FrameRate2=Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_FrameRate));
                    if (!FrameRate2.empty() && FrameRate2!=FrameRate)
                        IsOk=false;
                }
        }
        if (IsOk)
            Fill(Stream_General, 0, General_FrameRate, FrameRate);
    }

    //FrameCount if General not filled
    if (Retrieve(Stream_General, 0, General_FrameCount).empty() && Count_Get(Stream_Video) && Retrieve(Stream_General, 0, "IsTruncated").empty())
    {
        Ztring FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount);
        bool IsOk=true;
        if (FrameCount.empty())
        {
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Ztring FrameCount2=Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_FrameCount));
                    if (!FrameCount2.empty() && FrameCount2!=FrameCount)
                        IsOk=false;
                }
        }
        if (IsOk)
            Fill(Stream_General, 0, General_FrameCount, FrameCount);
    }

    //Tags
    Tags();
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_HumanReadable()
{
    //Generic
    for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            for (size_t Parameter=0; Parameter<Count_Get((stream_t)StreamKind, StreamPos); Parameter++)
                Streams_Finish_HumanReadable_PerStream((stream_t)StreamKind, StreamPos, Parameter);
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_HumanReadable_PerStream(stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    const Ztring ParameterName=Retrieve(StreamKind, StreamPos, Parameter, Info_Name);
    const Ztring Value=Retrieve(StreamKind, StreamPos, Parameter, Info_Text);

    //Strings
    const Ztring &List_Measure_Value=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure);
            if (List_Measure_Value==__T(" byte"))
        FileSize_FileSize123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T(" bps") || List_Measure_Value==__T(" Hz"))
        Kilo_Kilo123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T(" ms"))
        Duration_Duration123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T("Yes"))
        YesNo_YesNo(StreamKind, StreamPos, Parameter);
    else
        Value_Value123(StreamKind, StreamPos, Parameter);

    //BitRate_Mode / OverallBitRate_Mode
    if (ParameterName==(StreamKind==Stream_General?__T("OverallBitRate_Mode"):__T("BitRate_Mode")) && MediaInfoLib::Config.ReadByHuman_Get())
    {
        Clear(StreamKind, StreamPos, StreamKind==Stream_General?"OverallBitRate_Mode/String":"BitRate_Mode/String");

        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        List.Write(Retrieve(StreamKind, StreamPos, Parameter));

        //Per value
        for (size_t Pos=0; Pos<List.size(); Pos++)
            List[Pos]=MediaInfoLib::Config.Language_Get(Ztring(__T("BitRate_Mode_"))+List[Pos]);

        const Ztring Translated=List.Read();
        Fill(StreamKind, StreamPos, StreamKind==Stream_General?"OverallBitRate_Mode/String":"BitRate_Mode/String", Translated.find(__T("BitRate_Mode_"))?Translated:Value);
    }

    //Encoded_Application
    if ((   ParameterName==__T("Encoded_Application")
         || ParameterName==__T("Encoded_Application_CompanyName")
         || ParameterName==__T("Encoded_Application_Name")
         || ParameterName==__T("Encoded_Application_Version")
         || ParameterName==__T("Encoded_Application_Date"))
        && Retrieve(StreamKind, StreamPos, "Encoded_Application/String").empty())
    {
        Ztring CompanyName=Retrieve(StreamKind, StreamPos, "Encoded_Application_CompanyName");
        Ztring Name=Retrieve(StreamKind, StreamPos, "Encoded_Application_Name");
        Ztring Version=Retrieve(StreamKind, StreamPos, "Encoded_Application_Version");
        Ztring Date=Retrieve(StreamKind, StreamPos, "Encoded_Application_Date");
        if (!CompanyName.empty() || !Name.empty())
        {
            Ztring String=CompanyName;
            if (!CompanyName.empty() && !Name.empty())
                String+=' ';
            String+=Name;
            if (!Version.empty())
            {
                String+=__T(" ");
                String+=Version;
            }
            if (!Date.empty())
            {
                String+=__T(" (");
                String+=Date;
                String+=__T(")");
            }
            Fill(StreamKind, StreamPos, "Encoded_Application/String", String, true);
        }
        else
            Fill(StreamKind, StreamPos, "Encoded_Application/String", Retrieve(StreamKind, StreamPos, "Encoded_Application"), true);
    }

    //Encoded_Library
    if ((   ParameterName==__T("Encoded_Library")
         || ParameterName==__T("Encoded_Library_CompanyName")
         || ParameterName==__T("Encoded_Library_Name")
         || ParameterName==__T("Encoded_Library_Version")
         || ParameterName==__T("Encoded_Library_Date"))
        && Retrieve(StreamKind, StreamPos, "Encoded_Library/String").empty())
    {
        Ztring CompanyName=Retrieve(StreamKind, StreamPos, "Encoded_Library_CompanyName");
        Ztring Name=Retrieve(StreamKind, StreamPos, "Encoded_Library_Name");
        Ztring Version=Retrieve(StreamKind, StreamPos, "Encoded_Library_Version");
        Ztring Date=Retrieve(StreamKind, StreamPos, "Encoded_Library_Date");
        Ztring Encoded_Library=Retrieve(StreamKind, StreamPos, "Encoded_Library");
        Fill(StreamKind, StreamPos, "Encoded_Library/String", File__Analyze_Encoded_Library_String(CompanyName, Name, Version, Date, Encoded_Library), true);
    }

    //Format_Settings_Matrix
    if (StreamKind==Stream_Video && Parameter==Video_Format_Settings_Matrix)
    {
        Fill(Stream_Video, StreamPos, Video_Format_Settings_Matrix_String, MediaInfoLib::Config.Language_Get_Translate(__T("Format_Settings_Matrix_"), Value), true);
    }

    //Scan type
    if (StreamKind==Stream_Video && Parameter==Video_ScanType)
    {
        Fill(Stream_Video, StreamPos, Video_ScanType_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanType_Original)
    {
        Fill(Stream_Video, StreamPos, Video_ScanType_Original_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanType_StoreMethod)
    {
        Ztring ToTranslate=Ztring(__T("StoreMethod_"))+Value;
        if (!Retrieve(Stream_Video, StreamPos, Video_ScanType_StoreMethod_FieldsPerBlock).empty())
            ToTranslate+=__T('_')+Retrieve(Stream_Video, StreamPos, Video_ScanType_StoreMethod_FieldsPerBlock);
        Ztring Translated=MediaInfoLib::Config.Language_Get(ToTranslate);
        Fill(Stream_Video, StreamPos, Video_ScanType_StoreMethod_String, Translated.find(__T("StoreMethod_"))?Translated:Value, true);
    }

    //Scan order
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder)
    {
        Fill(Stream_Video, StreamPos, Video_ScanOrder_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder_Stored)
    {
        Fill(Stream_Video, StreamPos, Video_ScanOrder_Stored_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder_Original)
    {
        Fill(Stream_Video, StreamPos, Video_ScanOrder_Original_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }

    //Interlacement
    if (StreamKind==Stream_Video && Parameter==Video_Interlacement)
    {
        const Ztring &Z1=Retrieve(Stream_Video, StreamPos, Video_Interlacement);
        if (Z1.size()==3)
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, MediaInfoLib::Config.Language_Get(Ztring(__T("Interlaced_"))+Z1));
        else
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, MediaInfoLib::Config.Language_Get(Z1));
        if (Retrieve(Stream_Video, StreamPos, Video_Interlacement_String).empty())
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, Z1, true);
    }

    //FrameRate_Mode
    if (StreamKind==Stream_Video && Parameter==Video_FrameRate_Mode)
    {
        Fill(Stream_Video, StreamPos, Video_FrameRate_Mode_String, MediaInfoLib::Config.Language_Get_Translate(__T("FrameRate_Mode_"), Value), true);
    }

    //Compression_Mode
    if (Parameter==Fill_Parameter(StreamKind, Generic_Compression_Mode))
    {
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Compression_Mode_String), MediaInfoLib::Config.Language_Get_Translate(__T("Compression_Mode_"), Value), true);
    }

    //Delay_Source
    if (Parameter==Fill_Parameter(StreamKind, Generic_Delay_Source))
    {
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Delay_Source_String), MediaInfoLib::Config.Language_Get_Translate(__T("Delay_Source_"), Value), true);
    }

    //Gop_OpenClosed
    if (StreamKind==Stream_Video && (Parameter==Video_Gop_OpenClosed || Parameter==Video_Gop_OpenClosed_FirstFrame))
    {
        Fill(Stream_Video, StreamPos, Parameter+1, MediaInfoLib::Config.Language_Get_Translate(__T("Gop_OpenClosed_"), Value), true);
    }
}

} //NameSpace
