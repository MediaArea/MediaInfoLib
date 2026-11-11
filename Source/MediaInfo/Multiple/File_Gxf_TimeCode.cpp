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
#if defined(MEDIAINFO_TIMECODE_YES) || defined(MEDIAINFO_GXF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Gxf_TimeCode.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_GXF_YES)
    extern double Gxf_FrameRate(int32u Content);
#else //defined(MEDIAINFO_GXF_YES)
    double Gxf_FrameRate(int32u Content) //TODO: remove any relationship with GXF
    {
        switch (Content)
        {
            case 1 : return 60.000;
            case 2 : return 59.940;
            case 3 : return 50.000;
            case 4 : return 30.000;
            case 5 : return 29.970;
            case 6 : return 25.000;
            case 7 : return 24.000;
            case 8 : return 23.976;
            default: return  0.000;
        }
    }
#endif //defined(MEDIAINFO_GXF_YES)

//---------------------------------------------------------------------------
static const char* Atc_PayloadType (int8u PayloadType)
{
    switch (PayloadType)
    {
        case   0 : return "ATC_LTC";
        case   1 : return "ATC_VITC1";
        case   2 : return "ATC_VITC2";
        default  : return "";
    }
}

//---------------------------------------------------------------------------
// Count of frames per second of a time code (24, 25 or 30) from the duration in nanoseconds of a frame of the content, 0 if unknown
static int8u TimeCode_FramesPerSecond(int64u FrameDuration)
{
    if (!FrameDuration || FrameDuration==(int64u)-1)
        return 0;
    const float64 FrameRate=1000000000.0/FrameDuration;
    static const int8u Values[]={24, 25, 30};
    for (const auto Value : Values)
        for (int8u FramesPerValue=1; FramesPerValue<=2; FramesPerValue++) //A time code value is for a pair of frames above 30 frames per second
            if (FrameRate>Value*FramesPerValue*0.985 && FrameRate<Value*FramesPerValue*1.015) //Including the frame rates divided by 1.001
                return Value;
    return 0;
}

//---------------------------------------------------------------------------
// Digits of a time address as they are, a digit out of the decimal range is in hexadecimal
static string TimeAddress_String(int8u Hours_Tens, int8u Hours_Units, int8u Minutes_Tens, int8u Minutes_Units, int8u Seconds_Tens, int8u Seconds_Units, bool DropFrame, int8u Frames_Tens, int8u Frames_Units)
{
    static const char Digits[]="0123456789ABCDEF";
    string Value;
    Value+=Digits[Hours_Tens&0xF];
    Value+=Digits[Hours_Units&0xF];
    Value+=':';
    Value+=Digits[Minutes_Tens&0xF];
    Value+=Digits[Minutes_Units&0xF];
    Value+=':';
    Value+=Digits[Seconds_Tens&0xF];
    Value+=Digits[Seconds_Units&0xF];
    Value+=DropFrame?';':':';
    Value+=Digits[Frames_Tens&0xF];
    Value+=Digits[Frames_Units&0xF];
    return Value;
}

//***************************************************************************
// Time code dump
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
// A value is a time address: drop frame flag (1 bit), hours (7 bits), minutes, seconds and frames (8 bits each)
// Several consecutive values may have the same time address, e.g. frame pairs above 30 frames per second
static const size_t TimeCode_Dump_Repeat_Max=16;
static const int8u TimeCode_Dump_FrameRates[]={30, 25, 24};

//---------------------------------------------------------------------------
static inline int32u TimeCode_Dump_Value(int32u Hours, int32u Minutes, int32u Seconds, int32u Frames, bool DropFrame=false)
{
    return ((int32u)DropFrame<<31)|(Hours<<24)|(Minutes<<16)|(Seconds<<8)|Frames;
}

//---------------------------------------------------------------------------
static inline bool TimeCode_Dump_IsValid(int32u Value)
{
    return ((Value>>16)&0xFF)<60 && ((Value>>8)&0xFF)<60;
}

//---------------------------------------------------------------------------
static inline bool TimeCode_Dump_IsSame(int32u A, int32u B)
{
    return !((A^B)&0x7FFFFFFF);
}

//---------------------------------------------------------------------------
// If the time address of B is the one after the time address of A
static bool TimeCode_Dump_IsNext(int32u A, int32u B, int8u FramesMax)
{
    const auto DropFrame=B>>31;
    A&=0x7FFFFFFF;
    B&=0x7FFFFFFF;
    const auto Frames=(int8u)A;

    //Same second
    if (Frames!=FramesMax)
        return Frames<FramesMax && B==A+1;

    //Next second, 24-hour clock
    auto Hours=A>>24;
    auto Minutes=(A>>16)&0xFF;
    auto Seconds=(A>>8)&0xFF;
    Seconds++;
    if (Seconds>=60)
    {
        Seconds=0;
        Minutes++;
        if (Minutes>=60)
        {
            Minutes=0;
            Hours++;
            if (Hours>=24)
                Hours=0;
        }
    }
    const int32u Frames_First=(DropFrame && FramesMax==29 && !Seconds && Minutes%10)?2:0; //Drop frame: frame numbers 00 and 01 are skipped at the start of each minute except each tenth minute
    return B==TimeCode_Dump_Value(Hours, Minutes, Seconds, Frames_First);
}

//---------------------------------------------------------------------------
// Frame number max and count of values per time address, from the count of values per second
static bool TimeCode_Dump_FromFrameRate(float64 FrameRate, int8u& FramesMax, size_t& Repeat)
{
    const auto FrameRate_Rounded=float64_int64s(FrameRate);
    if (FrameRate_Rounded<=0 || FrameRate<FrameRate_Rounded*0.9985 || FrameRate>FrameRate_Rounded*1.0005)
        return false; //Neither an integer nor an integer divided by 1.001
    for (Repeat=1; Repeat<=TimeCode_Dump_Repeat_Max; Repeat++)
        for (const auto FrameRate_Base : TimeCode_Dump_FrameRates)
            if ((int64u)FrameRate_Rounded==FrameRate_Base*Repeat)
            {
                FramesMax=(int8u)(FrameRate_Base-1);
                return true;
            }
    return false;
}

//---------------------------------------------------------------------------
// Frame number max and count of values per time address, from the values
static void TimeCode_Dump_FromValues(const std::vector<int32u>& Values, int8u& FramesMax, size_t& Repeat)
{
    //Frame number max: most seen frame number before the first time address of the next second
    size_t FramesMax_Counts[sizeof(TimeCode_Dump_FrameRates)]={};
    for (size_t i=1; i<Values.size(); i++)
        if (TimeCode_Dump_IsValid(Values[i-1]) && TimeCode_Dump_IsValid(Values[i]))
            for (size_t j=0; j<sizeof(TimeCode_Dump_FrameRates); j++)
            {
                const auto FramesMax_Candidate=(int8u)(TimeCode_Dump_FrameRates[j]-1);
                if ((int8u)Values[i-1]==FramesMax_Candidate && TimeCode_Dump_IsNext(Values[i-1], Values[i], FramesMax_Candidate))
                    FramesMax_Counts[j]++;
            }
    FramesMax=(int8u)-1; //Not needed if there is no such change of second
    size_t FramesMax_Count=0;
    for (size_t j=0; j<sizeof(TimeCode_Dump_FrameRates); j++)
        if (FramesMax_Counts[j]>FramesMax_Count)
        {
            FramesMax_Count=FramesMax_Counts[j];
            FramesMax=(int8u)(TimeCode_Dump_FrameRates[j]-1);
        }

    //Count of values per time address: most seen count between 2 changes to the next time address, else before such change
    size_t Repeat_Counts[2][TimeCode_Dump_Repeat_Max+1]={};
    size_t Run=0;
    bool Run_IsAfterNext=false;
    for (size_t i=0; i<Values.size(); i++)
    {
        if (!TimeCode_Dump_IsValid(Values[i]))
        {
            Run=0;
            continue;
        }
        if (Run && TimeCode_Dump_IsSame(Values[i-1], Values[i]))
        {
            Run++;
            continue;
        }
        const auto IsNext=Run && TimeCode_Dump_IsNext(Values[i-1], Values[i], FramesMax);
        if (IsNext && Run<=TimeCode_Dump_Repeat_Max)
        {
            if (Run_IsAfterNext)
                Repeat_Counts[0][Run]++;
            Repeat_Counts[1][Run]++;
        }
        Run=1;
        Run_IsAfterNext=IsNext;
    }
    Repeat=1;
    for (const auto& Repeat_Count : Repeat_Counts)
    {
        size_t Repeat_Count_Max=0;
        for (size_t j=1; j<=TimeCode_Dump_Repeat_Max; j++)
            if (Repeat_Count[j]>Repeat_Count_Max)
            {
                Repeat_Count_Max=Repeat_Count[j];
                Repeat=j;
            }
        if (Repeat_Count_Max)
            break;
    }
}

//---------------------------------------------------------------------------
static void TimeCode_Dump_List_Finish(MediaInfo_Config_MediaInfo::timecode_dump& TimeCode_Dump)
{
    //The frame rate provides the expected time addresses, they are guessed from the values if it is not known
    const auto& Values=TimeCode_Dump.Values;
    int8u FramesMax;
    size_t Repeat;
    if (!TimeCode_Dump_FromFrameRate(TimeCode_Dump.FrameRate, FramesMax, Repeat))
        TimeCode_Dump_FromValues(Values, FramesMax, Repeat);

    auto& List=TimeCode_Dump.List;
    List.clear();
    List.reserve(Values.size()*26);
    size_t Run=0; //Count of consecutive values with the current time address, 0 if the previous value is not valid
    bool Run_IsPartial=false; //Values with the current time address may be before the first value or after a discontinuity
    for (size_t i=0; i<Values.size(); i++)
    {
        const auto Value=Values[i];
        bool IsNotContinuous;
        List+="    <tc v=\"";
        if (!TimeCode_Dump_IsValid(Value))
        {
            IsNotContinuous=Run?true:false;
            Run=0;
        }
        else
        {
            const auto IsSame=Run && TimeCode_Dump_IsSame(Values[i-1], Value);
            if (!Run)
                IsNotContinuous=false;
            else if (IsSame)
                IsNotContinuous=Run>=Repeat; //Too many values with this time address
            else
                IsNotContinuous=(Run<Repeat && !Run_IsPartial) //Not enough values with the previous time address
                             || !TimeCode_Dump_IsNext(Values[i-1], Value, FramesMax);
            if (!IsSame || IsNotContinuous)
            {
                Run_IsPartial=!Run || IsNotContinuous;
                Run=0;
            }
            Run++;

            const auto Hours=(Value>>24)&0x7F;
            const auto Minutes=(Value>>16)&0xFF;
            const auto Seconds=(Value>>8)&0xFF;
            const auto Frames=Value&0xFF;
            List+=(char)('0'+Hours/10);
            List+=(char)('0'+Hours%10);
            List+=':';
            List+=(char)('0'+Minutes/10);
            List+=(char)('0'+Minutes%10);
            List+=':';
            List+=(char)('0'+Seconds/10);
            List+=(char)('0'+Seconds%10);
            List+=((Value>>31) && FramesMax>=29)?';':':'; //No drop frame flag with 24 or 25 frames per second
            List+=(char)('0'+Frames/10);
            List+=(char)('0'+Frames%10);
        }
        List+='\"';
        if (IsNotContinuous)
            List+=" nc=\"1\"";
        List+="/>\n";
    }
}
#endif //MEDIAINFO_ADVANCED

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Gxf_TimeCode::File_Gxf_TimeCode()
:File__Analyze()
{
    //Configuration
    ParserName="Time code";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Gxf;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_Level=2; //Container
    #endif //MEDIAINFO_DEMUX

    //In
    FrameRate_Code=(int32u)-1;
    FieldsPerFrame_Code=(int32u)-1;
    IsAtc=false;
    IsBigEndian=false;
    IsTimeCodeTrack=false;
    IsBlockAddition=false;

    //Out
    TimeCode_FirstFrame_ms=(int64u)-1;

    //Temp
    TimeCode_LastFrame_FrameNumber=(int64u)-1;
    TimeCode_LastFrame_PTS=(int64u)-1;
}

//---------------------------------------------------------------------------
File_Gxf_TimeCode::~File_Gxf_TimeCode()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Gxf_TimeCode::Streams_Fill()
{
    Stream_Prepare(Stream_Other);
    if (IsBlockAddition)
        Fill(Stream_Other, 0, Other_Type, "Time code");
    Fill(Stream_Other, 0, Other_Format, Format);
    Fill(Stream_Other, 0, Other_TimeCode_FirstFrame, TimeCode_FirstFrame);
    if (IsTimeCodeTrack)
        return;


    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Delay, TimeCode_FirstFrame_ms);
    if (TimeCode_FirstFrame.size()==11)
        Fill(Stream_Video, StreamPos_Last, Video_Delay_DropFrame, TimeCode_FirstFrame[8]==';'?"Yes":"No");
    Fill(Stream_Video, 0, Video_Delay_Source, "Container");
    Fill(Stream_Video, 0, Video_TimeCode_FirstFrame, TimeCode_FirstFrame.c_str());
    //Fill(Stream_Video, 0, Video_TimeCode_Source, "Time code track");

    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Delay, TimeCode_FirstFrame_ms);
    if (TimeCode_FirstFrame.size()==11)
        Fill(Stream_Audio, StreamPos_Last, Audio_Delay_DropFrame, TimeCode_FirstFrame[8]==';'?"Yes":"No");
    Fill(Stream_Audio, 0, Audio_Delay_Source, "Container");
    Fill(Stream_Audio, 0, Video_TimeCode_FirstFrame, TimeCode_FirstFrame.c_str());
    //Fill(Stream_Audio, 0, Video_TimeCode_Source, "Time code track");
}

//---------------------------------------------------------------------------
void File_Gxf_TimeCode::Streams_Finish()
{
    if (!IsBlockAddition)
        return;

    //Time code of the last frame, if all the frames are parsed and the last frame of the track has a valid time code
    //When known, the container provides before the finish the count of frames of the track (Frame_Count_NotParsedIncluded) and the time stamp of its last frame (FrameInfo.PTS)
    if (TimeCode_LastFrame.empty() || Config->ParseSpeed<1.0)
        return;
    if (TimeCode_LastFrame_PTS!=(int64u)-1 && FrameInfo.PTS!=(int64u)-1)
    {
        if (TimeCode_LastFrame_PTS!=FrameInfo.PTS)
            return;
    }
    else if (TimeCode_LastFrame_FrameNumber==(int64u)-1 || TimeCode_LastFrame_FrameNumber+1!=Frame_Count_NotParsedIncluded)
        return;
    Fill(Stream_Other, 0, Other_TimeCode_LastFrame, TimeCode_LastFrame);
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Gxf_TimeCode::Read_Buffer_Continue()
{
    int8u Validity[504];

    //Matroska BlockAdditional: a buffer is the time code of 64 bits of a frame of the track, the stream is present also when no time code is valid
    //When known, the container provides before each buffer the number of the frame in the track (Frame_Count_NotParsedIncluded), its time stamp (FrameInfo.PTS) and the duration of a frame (FrameInfo.DUR)
    auto BlockAddition_End=[&]()
    {
        if (Element_Offset<Element_Size)
            Skip_XX(Element_Size-Element_Offset,                "Unknown");
        if (!Status[IsFilled])
        {
            Accept();
            Fill();
        }
        Merge_Conformance();
        Frame_Count++;
    };
    if (IsBlockAddition && Element_Size!=8)
    {
        Fill_Conformance("BlockAdditional", "Size is not the size of a SMPTE ST 12-1 time code (actual "+std::to_string(Element_Size)+", expected 8)");
        if (Element_Size<8)
        {
            BlockAddition_End();
            return;
        }
    }

    if (!IsAtc && !IsBigEndian) // TODO: enum of timecode "formats"
    {
        if (Element_Size!=4096)
        {
            Skip_XX(Element_Size,                                   "Data");
            return;
        }

        //Reading bitmap first (validity of first byte is at the end)
        Element_Offset=504*8;
        Element_Begin1("Validity");
        BS_Begin_LE(); //is Little Endian
        for (size_t Pos=0; Pos<504; Pos++)
        {
            bool Validity_Bit;
            Get_TB (Validity_Bit,                                   "Bit");
            Validity[Pos]=Validity_Bit?1:0;
        }
        BS_End_LE();
        Skip_B1(                                                    "Pad");
        Element_End0();
    }

    //Parsing
    Element_Offset=0;
    for (size_t Pos=0; Pos<((IsAtc || IsBigEndian)?(size_t)1:(size_t)504); Pos++)
    {
        if (IsAtc || IsBigEndian || Validity[Pos])
        {
            Element_Begin1("TimeCode");
            int8u Frames_Units, Frames_Tens, Seconds_Units, Seconds_Tens, Minutes_Units, Minutes_Tens, Hours_Units, Hours_Tens;
            bool  DropFrame;
            int8u DBB1=0, DBB2=0;
            if (IsAtc)
            {
                bool Temp;
                BS_Begin();

                Get_S1 (4, Frames_Units,                            "Frames (Units)");
                Get_SB (   Temp,                                    "DBB1_0"); if (Temp) DBB1|=(1<<0);
                Skip_S1(3,                                          "Zero");

                Skip_S1(4,                                          "BG1");
                Get_SB (   Temp,                                    "DBB1_1"); if (Temp) DBB1|=(1<<1);
                Skip_S1(3,                                          "Zero");

                Skip_SB(                                            "CF - Color fame");
                Get_SB (   DropFrame,                               "DP - Drop frame");
                Get_S1 (2, Frames_Tens,                             "Frames (Tens)");
                Get_SB (   Temp,                                    "DBB1_2"); if (Temp) DBB1|=(1<<2);
                Skip_S1(3,                                          "Zero");

                Skip_S1(4,                                          "BG2");
                Get_SB (   Temp,                                    "DBB1_3"); if (Temp) DBB1|=(1<<3);
                Skip_S1(3,                                          "Zero");

                Get_S1 (4, Seconds_Units,                           "Seconds (Units)");
                Get_SB (   Temp,                                    "DBB1_4"); if (Temp) DBB1|=(1<<4);
                Skip_S1(3,                                          "Zero");

                Skip_S1(4,                                          "BG3");
                Get_SB (   Temp,                                    "DBB1_5"); if (Temp) DBB1|=(1<<5);
                Skip_S1(3,                                          "Zero");

                Skip_SB(                                            "FM - Frame Mark");
                Get_S1 (3, Seconds_Tens,                            "Seconds (Tens)");
                Get_SB (   Temp,                                    "DBB1_6"); if (Temp) DBB1|=(1<<6);
                Skip_S1(3,                                          "Zero");

                Skip_S1(4,                                          "BG4");
                Get_SB (   Temp,                                    "DBB1_7"); if (Temp) DBB1|=(1<<7);
                Skip_S1(3,                                          "Zero");

                Get_S1 (4, Minutes_Units,                           "Minutes (Units)");
                Get_SB (   Temp,                                    "DBB2_0"); if (Temp) DBB2|=(1<<0);
                Skip_S1(3,                                          "Zero");

                Skip_S1(4,                                          "BG5");
                Get_SB (   Temp,                                    "DBB2_1"); if (Temp) DBB2|=(1<<1);
                Skip_S1(3,                                          "Zero");

                Skip_SB(                                            "BGF0");
                Get_S1 (3, Minutes_Tens,                            "Minutes (Tens)");
                Get_SB (   Temp,                                    "DBB2_2"); if (Temp) DBB2|=(1<<2);
                Skip_S1(3,                                          "Zero");

                Skip_S1(4,                                          "BG6");
                Get_SB (   Temp,                                    "DBB2_3"); if (Temp) DBB2|=(1<<3);
                Skip_S1(3,                                          "Zero");

                Get_S1 (4, Hours_Units,                             "Hours (Units)");
                Get_SB (   Temp,                                    "DBB2_4"); if (Temp) DBB2|=(1<<4);
                Skip_S1(3,                                          "Zero");

                Skip_S1(4,                                          "BG7");
                Get_SB (   Temp,                                    "DBB2_5"); if (Temp) DBB2|=(1<<5);
                Skip_S1(3,                                          "Zero");

                Skip_SB(                                            "BGF2");
                Skip_SB(                                            "BGF1");
                Get_S1 (2, Hours_Tens,                              "Hours (Tens)");
                Get_SB (   Temp,                                    "DBB2_6"); if (Temp) DBB2|=(1<<6);
                Skip_S1(3,                                          "Zero");

                Skip_S1(4,                                          "BG8");
                Get_SB (   Temp,                                    "DBB2_7"); if (Temp) DBB2|=(1<<7);
                Skip_S1(3,                                          "Zero");

                BS_End();
            }
            else if (IsBigEndian)
            {
                BS_Begin();

                Skip_S1(4,                                          "BG8");
                Skip_SB(                                            "BGF2");
                Skip_SB(                                            "BGF1");
                Get_S1 (2, Hours_Tens,                              "Hours (Tens)");

                Skip_S1(4,                                          "BG7");
                Get_S1 (4, Hours_Units,                             "Hours (Units)");

                Skip_S1(4,                                          "BG6");
                Skip_SB(                                            "BGF0");
                Get_S1 (3, Minutes_Tens,                            "Minutes (Tens)");

                Skip_S1(4,                                          "BG5");
                Get_S1 (4, Minutes_Units,                           "Minutes (Units)");

                Skip_S1(4,                                          "BG4");
                Skip_SB(                                            "FM - Frame Mark");
                Get_S1 (3, Seconds_Tens,                            "Seconds (Tens)");

                Skip_S1(4,                                          "BG3");
                Get_S1 (4, Seconds_Units,                           "Seconds (Units)");

                Skip_S1(4,                                          "BG2");
                Skip_SB(                                            "CF - Color fame");
                Get_SB (   DropFrame,                               "DP - Drop frame");
                Get_S1 (2, Frames_Tens,                             "Frames (Tens)");

                Skip_S1(4,                                          "BG1");
                Get_S1 (4, Frames_Units,                            "Frames (Units)");

                BS_End();
            }
            else
            {
                BS_Begin();

                Skip_S1(4,                                          "BG1");
                Get_S1 (4, Frames_Units,                            "Frames (Units)");

                Skip_S1(4,                                          "BG2");
                Skip_SB(                                            "CF - Color fame");
                Get_SB (   DropFrame,                               "DP - Drop frame");
                Get_S1 (2, Frames_Tens,                             "Frames (Tens)");

                Skip_S1(4,                                          "BG3");
                Get_S1 (4, Seconds_Units,                           "Seconds (Units)");

                Skip_S1(4,                                          "BG4");
                Skip_SB(                                            "FM - Frame Mark");
                Get_S1 (3, Seconds_Tens,                            "Seconds (Tens)");

                Skip_S1(4,                                          "BG5");
                Get_S1 (4, Minutes_Units,                           "Minutes (Units)");

                Skip_S1(4,                                          "BG6");
                Skip_SB(                                            "BGF0");
                Get_S1 (3, Minutes_Tens,                            "Minutes (Tens)");

                Skip_S1(4,                                          "BG7");
                Get_S1 (4, Hours_Units,                             "Hours (Units)");

                Skip_S1(4,                                          "BG8");
                Skip_SB(                                            "BGF2");
                Skip_SB(                                            "BGF1");
                Get_S1 (2, Hours_Tens,                              "Hours (Tens)");

                BS_End();
            }

            int64u TimeCode_Ms=(int64u)(Hours_Tens     *10*60*60*1000
                                      + Hours_Units       *60*60*1000
                                      + Minutes_Tens      *10*60*1000
                                      + Minutes_Units        *60*1000
                                      + Seconds_Tens         *10*1000
                                      + Seconds_Units           *1000
                                      + (Gxf_FrameRate(FrameRate_Code)==0?0:((Frames_Tens*10+Frames_Units)*1000/float64_int32s(Gxf_FrameRate(FrameRate_Code)/(Gxf_FrameRate(FrameRate_Code)>30?2:1)))));

            //Matroska BlockAdditional, check of the flag bits and of the time address
            bool DropFrame_IsUsed=DropFrame;
            bool TimeAddress_IsValid=true;
            if (IsBlockAddition)
            {
                const auto FramesPerSecond=TimeCode_FramesPerSecond(FrameInfo.DUR);
                const auto Frames=Frames_Tens*10+Frames_Units;
                if (FramesPerSecond && FramesPerSecond!=30)
                    DropFrame_IsUsed=false; //Unused flag bits are set to 0 by original sources and ignored by receiving equipment
                Element_Offset-=8; //The conformance items are about the time code just parsed
                if (Hours_Units>9 || Hours_Tens*10+Hours_Units>23 || Minutes_Units>9 || Minutes_Tens>5 || Seconds_Units>9 || Seconds_Tens>5 || Frames_Units>9 || Frames>29
                 || (DropFrame_IsUsed && !Seconds_Tens && !Seconds_Units && Frames<2 && Minutes_Units)) //With drop frame, frames 00 and 01 are not in the count at the start of a minute, except for minutes 00, 10, 20, 30, 40 and 50
                {
                    Fill_Conformance("TimeAddress", "Time address is not valid (actual "+TimeAddress_String(Hours_Tens, Hours_Units, Minutes_Tens, Minutes_Units, Seconds_Tens, Seconds_Units, DropFrame_IsUsed, Frames_Tens, Frames_Units)+')');
                    TimeAddress_IsValid=false;
                }
                else if (FramesPerSecond && Frames>=FramesPerSecond)
                {
                    Fill_Conformance("TimeAddress", "Frames of the time address are out of range (actual "+std::to_string(Frames)+", expected <="+std::to_string(FramesPerSecond-1)+')');
                    TimeAddress_IsValid=false;
                }
                else if (DropFrame && !DropFrame_IsUsed)
                    Fill_Conformance("FlagBits", "Drop frame flag is unused at "+std::to_string(FramesPerSecond)+" frames per second (actual 1, expected 0)", {}, Conformance_Warning);
                Element_Offset+=8;

                //Time code of the last frame, used only if all the frames are parsed
                if (TimeAddress_IsValid && Config->ParseSpeed>=1.0 && (TimeCode_LastFrame.empty() || FrameInfo.PTS==(int64u)-1 || TimeCode_LastFrame_PTS==(int64u)-1 || FrameInfo.PTS>=TimeCode_LastFrame_PTS))
                {
                    TimeCode_LastFrame=TimeAddress_String(Hours_Tens, Hours_Units, Minutes_Tens, Minutes_Units, Seconds_Tens, Seconds_Units, DropFrame_IsUsed, Frames_Tens, Frames_Units);
                    TimeCode_LastFrame_FrameNumber=Frame_Count_NotParsedIncluded;
                    TimeCode_LastFrame_PTS=FrameInfo.PTS;
                }
            }

            if (TimeCode_FirstFrame.empty() && (!IsBlockAddition || (TimeAddress_IsValid && !Frame_Count_NotParsedIncluded))) //Matroska BlockAdditional: only the valid time code of the first frame of the track
            {
                TimeCode_FirstFrame+=('0'+Hours_Tens);
                TimeCode_FirstFrame+=('0'+Hours_Units);
                TimeCode_FirstFrame+=':';
                TimeCode_FirstFrame+=('0'+Minutes_Tens);
                TimeCode_FirstFrame+=('0'+Minutes_Units);
                TimeCode_FirstFrame+=':';
                TimeCode_FirstFrame+=('0'+Seconds_Tens);
                TimeCode_FirstFrame+=('0'+Seconds_Units);
                TimeCode_FirstFrame+=DropFrame_IsUsed?';':':';
                TimeCode_FirstFrame+=('0'+Frames_Tens);
                TimeCode_FirstFrame+=('0'+Frames_Units);
            }

            #if MEDIAINFO_TRACE
                string TimeCodeTrace;
                TimeCodeTrace+=('0'+Hours_Tens);
                TimeCodeTrace+=('0'+Hours_Units);
                TimeCodeTrace+=':';
                TimeCodeTrace+=('0'+Minutes_Tens);
                TimeCodeTrace+=('0'+Minutes_Units);
                TimeCodeTrace+=':';
                TimeCodeTrace+=('0'+Seconds_Tens);
                TimeCodeTrace+=('0'+Seconds_Units);
                TimeCodeTrace+=DropFrame?';':':';
                TimeCodeTrace+=('0'+Frames_Tens);
                TimeCodeTrace+=('0'+Frames_Units);
                Element_Info1(TimeCodeTrace.c_str());
            #endif //MEDIAINFO_TRACE
            if (IsAtc)
            {
                Settings=Atc_PayloadType(DBB1);
                Element_Info1(__T("PayloadType=")+Ztring().From_UTF8(Settings.c_str()));
                Element_Info1(__T("VitcLineSelect=")+Ztring::ToZtring(DBB2&0x1F));
            }
            Element_End0();

            FILLING_BEGIN();
                if (TimeCode_FirstFrame_ms==(int64u)-1)
                    TimeCode_FirstFrame_ms=TimeCode_Ms;

                #if MEDIAINFO_ADVANCED
                    if (Config->TimeCode_Dumps)
                    {
                        if (id.empty())
                            id = std::to_string(((*Config->TimeCode_Dumps).size() + 1));
                        auto& TimeCode_Dump = (*Config->TimeCode_Dumps)[id];
                        if (TimeCode_Dump.Attributes_First.empty())
                        {
                            TimeCode_Dump.Attributes_First += " id=\"" + id + "\" format=\"SMPTE ST12-1\"";
                            TimeCode_Dump.List_Finish = TimeCode_Dump_List_Finish;
                        }
                        if (FieldsPerFrame_Code == 1 || FieldsPerFrame_Code == 2)
                            TimeCode_Dump.FrameRate = Gxf_FrameRate(FrameRate_Code) * FieldsPerFrame_Code; //1 value per field
                        TimeCode_Dump.Values.push_back(TimeCode_Dump_Value(Hours_Tens * 10 + Hours_Units, Minutes_Tens * 10 + Minutes_Units, Seconds_Tens * 10 + Seconds_Units, Frames_Tens * 10 + Frames_Units, DropFrame));
                        TimeCode_Dump.FrameCount++;
                    }
                #endif //MEDIAINFO_ADVANCED
            FILLING_END();
        }
        else
            Skip_XX(8,                                              "Junk");
    }

    if (!IsAtc && !IsBigEndian)
        Element_Offset+=64; //bitmap, already parsed
    if (IsBlockAddition)
        BlockAddition_End();

    FILLING_BEGIN();
    if (!Status[IsFilled] && TimeCode_FirstFrame_ms!=(int64u)-1)
    {
        Accept();
        Fill();
    }

    FILLING_END();
}

} //NameSpace

#endif //defined(MEDIAINFO_TIMECODE_YES) || defined(MEDIAINFO_GXF_YES)
