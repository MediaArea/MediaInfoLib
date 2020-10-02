/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// https://developer.dolby.com/globalassets/professional/dolby-e/dolby-e-high-level-frame-description.pdf
//---------------------------------------------------------------------------

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
#if defined(MEDIAINFO_DOLBYE_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_DolbyE.h"
#include <cmath>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
//CRC computing, with incomplete first and last bytes
//Inspired by http://zorc.breitbandkatze.de/crc.html
extern const int16u CRC_16_Table[256];
int16u CRC_16_Compute(const int8u* Buffer_Begin, size_t Buffer_Size, int8u SkipBits_Begin, int8u SkipBits_End)
{
    int16u CRC_16=0x0000;
    const int8u* Buffer=Buffer_Begin;
    const int8u* Buffer_End=Buffer+Buffer_Size;
    if (SkipBits_End)
        Buffer_End--; //Not handling completely the last byte

    //First partial byte
    if (SkipBits_Begin)
    {
        for (int8u Mask=(1<<(7-SkipBits_Begin)); Mask; Mask>>=1)
        {
            bool NewBit=(CRC_16&0x8000)?true:false;
            CRC_16<<=1;
            if ((*Buffer)&Mask)
                NewBit=!NewBit;
            if (NewBit)
                CRC_16^=0x8005;
        }

        Buffer++;
    }

    //Complete bytes
    while (Buffer<Buffer_End)
    {
        CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^(*Buffer)];
        Buffer++;
    }

    //Last partial byte
    if (SkipBits_End)
    {
        for (int8u Mask=0x80; Mask>(1<<(SkipBits_End-1)); Mask>>=1)
        {
            bool NewBit=(CRC_16&0x8000)?true:false;
            CRC_16<<=1;
            if ((*Buffer)&Mask)
                NewBit=!NewBit;
            if (NewBit)
                CRC_16^=0x8005;
        }

        Buffer++;
    }

    return CRC_16;
}

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const int8u DolbyE_Programs[64]=
{2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 8, 1, 2, 3, 3, 4, 5, 6, 1, 2, 3, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//---------------------------------------------------------------------------
static const int8u DolbyE_Channels[64]=
{8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//---------------------------------------------------------------------------
const int8u DolbyE_Channels_PerProgram(int8u program_config, int8u program)
{
    switch (program_config)
    {
        case  0 :   switch (program)
                    {
                        case  0 :   return 6;
                        default :   return 2;
                    }
        case  1 :   switch (program)
                    {
                        case  0 :   return 6;
                        default :   return 1;
                    }
        case  2 :
        case 18 :   return 4;
        case  3 :
        case 12 :   switch (program)
                    {
                        case  0 :   return 4;
                        default :   return 2;
                    }
        case  4 :   switch (program)
                    {
                        case  0 :   return 4;
                        case  1 :   return 2;
                        default :   return 1;
                    }
        case  5 :
        case 13 :   switch (program)
                    {
                        case  0 :   return 4;
                        default :   return 1;
                    }
        case  6 :
        case 14 :
        case 19 :   return 2;
        case  7 :   switch (program)
                    {
                        case  0 :
                        case  1 :
                        case  2 :   return 2;
                        default :   return 1;
                    }
        case  8 :
        case 15 :   switch (program)
                    {
                        case  0 :
                        case  1 :   return 2;
                        default :   return 1;
                    }
        case  9 :
        case 16 :
        case 20 :   switch (program)
                    {
                        case  0 :   return 2;
                        default :   return 1;
                    }
        case 10 :
        case 17 :
        case 21 :   return 1;
        case 11 :   return 6;
        case 22 :   return 8;
        case 23 :   return 8;
        default :   return 0;
    }
};

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelPositions[64]=
{
    "Front: L C R, Side: L R, LFE / Front: L R",
    "Front: L C R, Side: L R, LFE / Front: C / Front: C",
    "Front: L C R, LFE / Front: L C R, LFE",
    "Front: L C R, LFE / Front: L R / Front: L R",
    "Front: L C R, LFE / Front: L R / Front: C / Front: C",
    "Front: L C R, LFE / Front: C / Front: C / Front: C / Front: C",
    "Front: L R / Front: L R / Front: L R / Front: L R",
    "Front: L R / Front: L R / Front: L R / Front: C / Front: C",
    "Front: L R / Front: L R / Front: C / Front: C / Front: C / Front: C",
    "Front: L R / Front: C / Front: C / Front: C / Front: C / Front: C / Front: C",
    "Front: C / Front: C / Front: C / Front: C / Front: C / Front: C / Front: C / Front: C",
    "Front: L C R, Side: L R, LFE",
    "Front: L C R, LFE / Front: L R",
    "Front: L C R, LFE / Front: C / Front: C",
    "Front: L R / Front: L R / Front: L R",
    "Front: L R / Front: L R / Front: C / Front: C",
    "Front: L R / Front: C / Front: C / Front: C / Front: C",
    "Front: C / Front: C / Front: C / Front: C / Front: C / Front: C",
    "Front: L C R, LFE",
    "Front: L R / Front: L R",
    "Front: L R / Front: C / Front: C",
    "Front: C / Front: C / Front: C / Front: C",
    "Front: L C R, Side: L R, Rear: L R, LFE",
    "Front: L C C C R, Side: L R, LFE",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelPositions_PerProgram(int8u program_config, int8u program)
{
    switch (program_config)
    {
        case  0 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, Side: L R, LFE";
                        default :   return "Front: L R";
                    }
        case  1 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, Side: L R, LFE";
                        default :   return "Front: C";
                    }
        case  2 :
        case 18 :   return "Front: L C R, LFE";
        case  3 :
        case 12 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, LFE";
                        default :   return "Front: L R";
                    }
        case  4 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, LFE";
                        case  1 :   return "Front: L R";
                        default :   return "Front: C";
                    }
        case  5 :
        case 13 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, LFE";
                        default :   return "Front: C";
                    }
        case  6 :
        case 14 :
        case 19 :   return "Front: L R";
        case  7 :   switch (program)
                    {
                        case  0 :
                        case  1 :
                        case  2 :   return "Front: L R";
                        default :   return "Front: C";
                    }
        case  8 :
        case 15 :   switch (program)
                    {
                        case  0 :
                        case  1 :   return "Front: L R";
                        default :   return "Front: C";
                    }
        case  9 :
        case 16 :
        case 20 :   switch (program)
                    {
                        case  0 :   return "Front: L R";
                        default :   return "Front: C";
                    }
        case 10 :
        case 17 :
        case 21 :   return "Front: C";
        case 11 :   return "Front: L C R, Side: L R, LFE";
        case 22 :   return "Front: L C R, Side: L R, Rear: L R, LFE";
        case 23 :   return "Front: L C C C R, Side: L R, LFE";
        default :   return "";
    }
};

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelPositions2[64]=
{
    "3/2/0.1 / 2/0/0",
    "3/2/0.1 / 1/0/0 / 1/0/0",
    "3/0/0.1 / 3/0/0.1",
    "3/0/0.1 / 2/0/0 / 2/0/0",
    "3/0/0.1 / 2/0/0 / 1/0/0 / 1/0/0",
    "3/0/0.1 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "2/0/0 / 2/0/0 / 2/0/0 / 2/0/0",
    "2/0/0 / 2/0/0 / 2/0/0 / 1/0/0 / 1/0/0",
    "2/0/0 / 2/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "2/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "3/2/0.1",
    "3/0/0.1 / 2/0/0",
    "3/0/0.1 / 1/0/0 / 1/0/0",
    "2/0/0 / 2/0/0 / 2/0/0",
    "2/0/0 / 2/0/0 / 1/0/0 / 1/0/0",
    "2/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "3/0/0.1",
    "2/0/0 / 2/0/0",
    "2/0/0 / 1/0/0 / 1/0/0",
    "1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "3/2/2.1",
    "5/2/0.1",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelPositions2_PerProgram(int8u program_config, int8u program)
{
    switch (program_config)
    {
        case  0 :   switch (program)
                    {
                        case  0 :   return "3/2/0.1";
                        default :   return "2/0/0";
                    }
        case  1 :   switch (program)
                    {
                        case  0 :   return "3/2/0.1";
                        default :   return "1/0/0";
                    }
        case  2 :
        case 18 :   return "3/0/0.1";
        case  3 :
        case 12 :   switch (program)
                    {
                        case  0 :   return "3/0/0.1";
                        default :   return "2/0/0";
                    }
        case  4 :   switch (program)
                    {
                        case  0 :   return "3/0/0.1";
                        case  1 :   return "2/0/0";
                        default :   return "1/0/0";
                    }
        case  5 :
        case 13 :   switch (program)
                    {
                        case  0 :   return "3/0/0.1";
                        default :   return "1/0/0";
                    }
        case  6 :
        case 14 :
        case 19 :   return "Front: L R";
        case  7 :   switch (program)
                    {
                        case  0 :
                        case  1 :
                        case  2 :   return "2/0/0";
                        default :   return "1/0/0";
                    }
        case  8 :
        case 15 :   switch (program)
                    {
                        case  0 :
                        case  1 :   return "2/0/0";
                        default :   return "1/0/0";
                    }
        case  9 :
        case 16 :
        case 20 :   switch (program)
                    {
                        case  0 :   return "2/0/0";
                        default :   return "1/0/0";
                    }
        case 10 :
        case 17 :
        case 21 :   return "1/0/0";
        case 11 :   return "3/2/0.1";
        case 22 :   return "3/2/2.1";
        case 23 :   return "5/2/0.1";
        default :   return "";
    }
};

extern const char*  AC3_Surround[];

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelLayout_PerProgram(int8u program_config, int8u ProgramNumber)
{
    switch (program_config)
    {
        case  0 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C Ls X R LFE Rs X";
                        default :   return "X X X L X X X R";
                    }
        case  1 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C Ls X R LFE Rs X";
                        case  1 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  2 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X X R S X X";
                        default :   return "X X L C X X R S";
                    }
        case  3 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X X R S X X";
                        case  1 :   return "X X L X X X R X";
                        default :   return "X X X L X X X R";
                    }
        case  4 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X X R S X X";
                        case  1 :   return "X X L X X X R X";
                        case  2 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  5 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X X R S X X";
                        case  1 :   return "X X C X X X X X";
                        case  2 :   return "X X X X X X C X";
                        case  3 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  6 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X X R X X X";
                        case  1 :   return "X L X X X R X X";
                        case  2 :   return "X X L X X X R X";
                        default :   return "X X X L X X X R";
                    }
        case  7 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X X R X X X";
                        case  1 :   return "X L X X X R X X";
                        case  2 :   return "X X L X X X R X";
                        case  3 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  8 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X X R X X X";
                        case  1 :   return "X L X X X R X X";
                        case  2 :   return "X X C X X X X X";
                        case  3 :   return "X X X X X X C X";
                        case  4 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  9 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X X R X X X";
                        case  1 :   return "X C X X X X X X";
                        case  2 :   return "X X X X X C X X";
                        case  3 :   return "X X C X X X X X";
                        case  4 :   return "X X X X X X C X";
                        case  5 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case 10 :   switch (ProgramNumber)
                    {
                        case  0 :   return "C X X X X X X X";
                        case  1 :   return "X X X X C X X X";
                        case  2 :   return "X C X X X X X X";
                        case  3 :   return "X X X X X C X X";
                        case  4 :   return "X X C X X X X X";
                        case  5 :   return "X X X X X X C X";
                        case  6 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case 11 :   return "L C Ls R LFE Rs";
        case 12 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X R S X";
                        default :   return "X X L X X R";
                    }
        case 13 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X R S X";
                        case  1 :   return "X X C X X X";
                        default :   return "X X X X X C";
                    }
        case 14 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X R X X";
                        case  1 :   return "X L X X R X";
                        default :   return "X X L X X R";
                    }
        case 15 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X R X X";
                        case  1 :   return "X L X R X";
                        case  2 :   return "X X C X X X";
                        default :   return "X X X X X C";
                    }
        case 16 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X R X X";
                        case  1 :   return "X C X X X X";
                        case  2 :   return "X X X X C X";
                        case  3 :   return "X X C X X X";
                        default :   return "X X X X X C";
                    }
        case 17 :   switch (ProgramNumber)
                    {
                        case  0 :   return "C X X X X X";
                        case  1 :   return "X X X C X X";
                        case  2 :   return "X C X X X X";
                        case  3 :   return "X X X X C X";
                        case  4 :   return "X X C X X X";
                        default :   return "X X X X X C";
                    }
        case 18 :   return "L C R S";
        case 19 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X R X";
                        default :   return "X L X R";
                    }
        case 20 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X R X";
                        case  1 :   return "X C X X";
                        default :   return "X X X C";
                    }
        case 21 :   switch (ProgramNumber)
                    {
                        case  0 :   return "C X X X";
                        case  1 :   return "X X C X";
                        case  2 :   return "X C X X";
                        default :   return "X X X C";
                    }
        case 22 :   return "L C Ls Lrs R LFE Rs Rrs";
        case 23 :   return "L C Ls Lc R LFE Rs Rc";
        default :   return "";
    }
};

extern const float64 Mpegv_frame_rate[16];

const bool Mpegv_frame_rate_type[16]=
{false, false, false, false, false, false, true, true, true, false, false, false, false, false, false, false};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DolbyE::File_DolbyE()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_DolbyE;
    #endif //MEDIAINFO_EVENTS

    //Configuration
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=32*1024;

    //In
    GuardBand_Before=0;

    //Out
    GuardBand_After=0;

    //Temp
    SMPTE_time_code_StartTimecode=(int64u)-1;
    FrameInfo.DTS=0;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyE::Streams_Fill()
{
    Fill(Stream_General, 0, General_Format, "Dolby E");
    int8u DolbyE_Audio_Pos=0;
    for (size_t i=0; i<8; i++)
        if (channel_subsegment_sizes[i].size()>1)
            DolbyE_Audio_Pos=(int8u)-1;
    for (int8u program=0; program<DolbyE_Programs[program_config]; program++)
    {
        Stream_Prepare(Stream_Audio);
        Fill(Stream_Audio, StreamPos_Last, Audio_Format, "Dolby E");
        if (DolbyE_Programs[program_config]>1)
            Fill(Stream_Audio, StreamPos_Last, Audio_ID, Count_Get(Stream_Audio));
        Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, DolbyE_Channels_PerProgram(program_config, program));
        Fill(Stream_Audio, StreamPos_Last, Audio_ChannelPositions, DolbyE_ChannelPositions_PerProgram(program_config, program));
        Fill(Stream_Audio, StreamPos_Last, Audio_ChannelPositions_String2, DolbyE_ChannelPositions2_PerProgram(program_config, program));
        Fill(Stream_Audio, StreamPos_Last, Audio_ChannelLayout, DolbyE_ChannelLayout_PerProgram(program_config, program));
        int32u Program_Size=0;
        if (DolbyE_Audio_Pos!=(int8u)-1)
            for (int8u Pos=0; Pos<DolbyE_Channels_PerProgram(program_config, program); Pos++)
                Program_Size+=channel_subsegment_size[DolbyE_Audio_Pos+Pos];
        if (!Mpegv_frame_rate_type[frame_rate_code])
            Program_Size*=2; //Low bit rate, 2 channel component per block
        Program_Size*=bit_depth;
        Fill(Stream_Audio, StreamPos_Last, Audio_BitRate, Program_Size*Mpegv_frame_rate[frame_rate_code], 0);
        if (DolbyE_Audio_Pos!=(int8u)-1)
            DolbyE_Audio_Pos+=DolbyE_Channels_PerProgram(program_config, program);
        Streams_Fill_PerProgram();

        if (program<description_text_Values.size())
        {
            Fill(Stream_Audio, StreamPos_Last, Audio_Title, description_text_Values[program].Previous);
            Fill(Stream_Audio, StreamPos_Last, "Title_FromStream", description_text_Values[program].Previous);
            Fill_SetOptions(Stream_Audio, StreamPos_Last, "Title_FromStream", "N NT");
        }
    }
}

//---------------------------------------------------------------------------
void File_DolbyE::Streams_Fill_PerProgram()
{
    Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, 48000);
    Fill(Stream_Audio, StreamPos_Last, Audio_BitDepth, bit_depth);

    if (SMPTE_time_code_StartTimecode!=(int64u)-1)
    {
        Fill(StreamKind_Last, StreamPos_Last, Audio_Delay, SMPTE_time_code_StartTimecode);
        Fill(StreamKind_Last, StreamPos_Last, Audio_Delay_Source, "Stream");
    }

    Fill(Stream_Audio, StreamPos_Last, Audio_FrameRate, Mpegv_frame_rate[frame_rate_code]);
    if (FrameInfo.PTS!=(int64u)-1 && bit_depth)
    {
        float BitRate=(float)(96000*bit_depth);

        if (GuardBand_Before_Initial)
        {
            float GuardBand_Before_Initial_Duration=GuardBand_Before_Initial*8/BitRate;
            Fill(Stream_Audio, StreamPos_Last, "GuardBand_Before", GuardBand_Before_Initial_Duration, 9);
            Fill(Stream_Audio, StreamPos_Last, "GuardBand_Before/String", Ztring::ToZtring(GuardBand_Before_Initial_Duration*1000000, 0)+Ztring().From_UTF8(" \xC2xB5s")); //0xC2 0xB5 = micro sign
            Fill_SetOptions(Stream_Audio, StreamPos_Last, "GuardBand_Before", "N NT");
            Fill_SetOptions(Stream_Audio, StreamPos_Last, "GuardBand_Before/String", "N NT");

            float GuardBand_After_Initial_Duration=GuardBand_After_Initial*8/BitRate;
            Fill(Stream_Audio, StreamPos_Last, "GuardBand_After", GuardBand_After_Initial_Duration, 9);
            Fill(Stream_Audio, StreamPos_Last, "GuardBand_After/String", Ztring::ToZtring(GuardBand_After_Initial_Duration*1000000, 0)+Ztring().From_UTF8(" \xC2xB5s")); //0xC2 0xB5 = micro sign
            Fill_SetOptions(Stream_Audio, StreamPos_Last, "GuardBand_After", "N NT");
            Fill_SetOptions(Stream_Audio, StreamPos_Last, "GuardBand_After/String", "N NT");
        }
    }

    if (FrameSizes.size()==1)
    {
        if (StreamPos_Last)
            Fill(Stream_Audio, StreamPos_Last, Audio_BitRate_Encoded, 0, 0, true);
        else
        {
            Fill(Stream_General, 0, General_OverallBitRate, FrameSizes.begin()->first*8*Mpegv_frame_rate[frame_rate_code], 0);
            Fill(Stream_Audio, 0, Audio_BitRate_Encoded, FrameSizes.begin()->first*8*Mpegv_frame_rate[frame_rate_code], 0);
        }
    }
}

//---------------------------------------------------------------------------
void File_DolbyE::Streams_Finish()
{
    if (FrameInfo.PTS!=(int64u)-1 && FrameInfo.PTS>PTS_Begin)
    {
        int64s Duration=float64_int64s(((float64)(FrameInfo.PTS-PTS_Begin))/1000000);
        int64s FrameCount;
        if (Mpegv_frame_rate[frame_rate_code])
            FrameCount=float64_int64s(((float64)(FrameInfo.PTS-PTS_Begin))/1000000000*Mpegv_frame_rate[frame_rate_code]);
        else
            FrameCount=0;

        for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
        {
            Fill(Stream_Audio, Pos, Audio_Duration, Duration);
            if (FrameCount)
                Fill(Stream_Audio, Pos, Audio_FrameCount, FrameCount);
        }
    }
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DolbyE::Synchronize()
{
    //Synchronizing
    while (Buffer_Offset+3<=Buffer_Size)
    {
        if ((CC2(Buffer+Buffer_Offset_Temp)&0xFFFE)==0x078E) //16-bit
        {
            bit_depth=16;
            key_present=(CC2(Buffer+Buffer_Offset)&0x0001)?true:false;
            break; //while()
        }
        if ((CC3(Buffer+Buffer_Offset)&0xFFFFE0)==0x0788E0) //20-bit
        {
            bit_depth=20;
            key_present=(CC3(Buffer+Buffer_Offset)&0x000010)?true:false;
            break; //while()
        }
        if ((CC3(Buffer+Buffer_Offset)&0xFFFFFE)==0x07888E) //24-bit
        {
            bit_depth=24;
            key_present=(CC3(Buffer+Buffer_Offset)&0x000001)?true:false;
            break; //while()
        }
        Buffer_Offset++;
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+3>Buffer_Size)
        return false;

    //Synched
    return true;
}

//---------------------------------------------------------------------------
bool File_DolbyE::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+3>Buffer_Size)
        return false;

    //Quick test of synchro
    switch (bit_depth)
    {
        case 16 : if ((CC2(Buffer+Buffer_Offset)&0xFFFE  )!=0x078E  ) {Synched=false; return true;} break;
        case 20 : if ((CC3(Buffer+Buffer_Offset)&0xFFFFE0)!=0x0788E0) {Synched=false; return true;} break;
        case 24 : if ((CC3(Buffer+Buffer_Offset)&0xFFFFFE)!=0x07888E) {Synched=false; return true;} break;
        default : ;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyE::Read_Buffer_Unsynched()
{
    description_text_Values.clear();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyE::Header_Parse()
{
    //Filling
    if (IsSub)
        Header_Fill_Size(Buffer_Size-Buffer_Offset);
    else
    {
        //Looking for synchro
        //Synchronizing
        Buffer_Offset_Temp=Buffer_Offset+3;
        if (bit_depth==16)
            while (Buffer_Offset_Temp+2<=Buffer_Size)
            {
                if ((CC2(Buffer+Buffer_Offset_Temp)&0xFFFE)==0x078E) //16-bit
                    break; //while()
                Buffer_Offset_Temp++;
            }
        if (bit_depth==20)
            while (Buffer_Offset_Temp+3<=Buffer_Size)
            {
                if ((CC3(Buffer+Buffer_Offset_Temp)&0xFFFFE0)==0x0788E0) //20-bit
                    break; //while()
                Buffer_Offset_Temp++;
            }
        if (bit_depth==24)
            while (Buffer_Offset_Temp+3<=Buffer_Size)
            {
                if ((CC3(Buffer+Buffer_Offset_Temp)&0xFFFFFE)==0x07888E) //24-bit
                    break; //while()
                Buffer_Offset_Temp++;
            }

        if (Buffer_Offset_Temp+(bit_depth>16?3:2)>Buffer_Size)
        {
            if (File_Offset+Buffer_Size==File_Size)
                Buffer_Offset_Temp=Buffer_Size;
            else
            {
                Element_WaitForMoreData();
                return;
            }
        }

        Header_Fill_Size(Buffer_Offset_Temp-Buffer_Offset);
    }
    Header_Fill_Code(0, "Dolby_E_frame");
}

//---------------------------------------------------------------------------
void File_DolbyE::Data_Parse()
{
    FrameSizes[Element_Size]++;

    //In case of scrambling
    const int8u*    Save_Buffer=NULL;
    size_t          Save_Buffer_Offset=0;
    int64u          Save_File_Offset=0;
    if (key_present)
    {
        //We must change the buffer,
        Save_Buffer=Buffer;
        Save_Buffer_Offset=Buffer_Offset;
        Save_File_Offset=File_Offset;
        File_Offset+=Buffer_Offset;
        Buffer_Offset=0;
        Descrambled_Buffer=new int8u[(size_t)Element_Size];
        std::memcpy(Descrambled_Buffer, Save_Buffer+Save_Buffer_Offset, (size_t)Element_Size);
        Buffer=Descrambled_Buffer;
    }

    //Parsing
    BS_Begin();
    sync_segment();
    metadata_segment();
    audio_segment();
    metadata_extension_segment();
    audio_extension_segment();
    meter_segment();
    BS_End();

    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");

    //In case of scrambling
    if (key_present)
    {
        delete[] Buffer; Buffer=Save_Buffer;
        Buffer_Offset=Save_Buffer_Offset;
        File_Offset=Save_File_Offset;
    }

    FILLING_BEGIN();
        {
            //Guard band
            if (Mpegv_frame_rate[frame_rate_code])
            {
            int64u BytesPerSecond=96000*bit_depth/8;
            float64 BytesPerFrame=BytesPerSecond/Mpegv_frame_rate[frame_rate_code];
            int64u BytesUpToLastFrame;
            int64u BytesUpToNextFrame;
            for (;;)
            {
                BytesUpToLastFrame=(int64u)(BytesPerFrame*Frame_Count);
                BytesUpToLastFrame/=bit_depth/4;
                BytesUpToLastFrame*=bit_depth/4;
                BytesUpToNextFrame=(int64u)(BytesPerFrame*(Frame_Count+1));
                BytesUpToNextFrame/=bit_depth/4;
                BytesUpToNextFrame*=bit_depth/4;

                if (BytesUpToLastFrame+GuardBand_Before<BytesUpToNextFrame)
                    break;

                // In case previous frame was PCM
                Frame_Count++;
                GuardBand_Before-=BytesUpToNextFrame-BytesUpToLastFrame;
            }
            GuardBand_After=BytesUpToNextFrame-BytesUpToLastFrame;
            int64u ToRemove=GuardBand_Before+(bit_depth>>1)+Element_Size; // Guardband + AES3 header + Dolby E frame
            if (ToRemove<(int64u)GuardBand_After)
                GuardBand_After-=ToRemove;
            else
                GuardBand_After=0;
            GuardBand_After/=bit_depth/4;
            GuardBand_After*=bit_depth/4;

            Element_Info1(GuardBand_Before);
            float64 GuardBand_Before_Duration=((float64)GuardBand_Before)/BytesPerSecond;
            Ztring GuardBand_Before_String=__T("GuardBand_Begin ")+Ztring::ToZtring(GuardBand_Before)+__T(" (")+Ztring::ToZtring(GuardBand_Before_Duration*1000000, 0)+Ztring().From_UTF8(" \0xC20xB5s"); //0xC20xB5 = micro sign
            Element_Info1(GuardBand_Before_String);
            }
        }

        if (!Status[IsAccepted])
        {
            Accept("Dolby E");
            PTS_Begin=FrameInfo.PTS;

            //Guard band
            GuardBand_Before_Initial=GuardBand_Before;
            GuardBand_After_Initial=GuardBand_After;
        }
        Frame_Count++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
        if (Mpegv_frame_rate[frame_rate_code])
            FrameInfo.DUR=float64_int64s(1000000000/Mpegv_frame_rate[frame_rate_code]);
        else
            FrameInfo.DUR=(int64u)-1;
        if (FrameInfo.DTS!=(int64u)-1)
            FrameInfo.DTS+=FrameInfo.DUR;
        if (FrameInfo.PTS!=(int64u)-1)
            FrameInfo.PTS+=FrameInfo.DUR;
        if (!Status[IsFilled] && (description_text_Values.empty() || Frame_Count>=32+1+1+32+1)) // max 32 chars (discarded) + ETX (discarded) + STX + max 32 chars + ETX
            Fill("Dolby E");
    FILLING_END();
    if (Frame_Count==0 && Buffer_TotalBytes>Buffer_TotalBytes_FirstSynched_Max)
        Reject("Dolby E");
}

//---------------------------------------------------------------------------
void File_DolbyE::sync_segment()
{
    //Parsing
    Element_Begin1("sync_segment");
    Skip_S3(bit_depth,                                      "sync_word");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::metadata_segment()
{
    //Parsing
    Element_Begin1("metadata_segment");
    if (key_present)
    {
        //We must change the buffer
        switch (bit_depth)
        {
            case 16 :
                        {
                        int16u metadata_key;
                        Get_S2 (16, metadata_key, "metadata_key");
                        int16u metadata_segment_size=((BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Size-Data_BS_Remain()/8)^metadata_key)>>2)&0x3FF;

                        if (Data_BS_Remain()<((size_t)metadata_segment_size+1)*(size_t)bit_depth) //+1 for CRC
                            return; //There is a problem

                        int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                        for (int16u Pos=0; Pos<metadata_segment_size+1; Pos++)
                            int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^metadata_key);
                        }
                        break;
            case 20 :
                        {
                        int32u metadata_key;
                        Get_S3 (20, metadata_key, "metadata_key");
                        int16u metadata_segment_size=((BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Size-Data_BS_Remain()/8)^(metadata_key>>4))>>2)&0x3FF;

                        if (Data_BS_Remain()<((size_t)metadata_segment_size+1)*(size_t)bit_depth) //+1 for CRC
                            return; //There is a problem

                        Descramble_20bit(metadata_key, metadata_segment_size);
                        }
                        break;
            case 24 :
                        {
                        int32u metadata_key;
                        Get_S3 (24, metadata_key, "metadata_key");
                        int32u metadata_segment_size=((BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Size-Data_BS_Remain()/8)^metadata_key)>>2)&0x3FF;

                        if (Data_BS_Remain()<((size_t)metadata_segment_size+1)*bit_depth) //+1 for CRC
                            return; //There is a problem

                        int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                        for (int16u Pos=0; Pos<metadata_segment_size+1; Pos++)
                            int24u2BigEndian(Temp+Pos*2, BigEndian2int24u(Temp+Pos*2)^metadata_key);
                        }
                        break;
            default :   ;
        }
    }
    size_t  metadata_segment_BitCountAfter=Data_BS_Remain();
    int16u  metadata_segment_size;
    Skip_S1( 4,                                                 "metadata_revision_id");
    Get_S2 (10, metadata_segment_size,                          "metadata_segment_size");
    metadata_segment_BitCountAfter-=metadata_segment_size*bit_depth;
    Get_S1 ( 6, program_config,                                 "program_config"); Param_Info1(DolbyE_ChannelPositions[program_config]);
    Get_S1 ( 4, frame_rate_code,                                "frame_rate_code"); Param_Info3(Mpegv_frame_rate[frame_rate_code], " fps", 3);
    Info_S1( 4, original_frame_rate_code,                       "original_frame_rate_code"); Param_Info3(Mpegv_frame_rate[original_frame_rate_code], " fps", 3);
    Skip_S2(16,                                                 "frame_count");
    Element_Begin1("SMPTE_time_code");
    int8u Frames_Units, Frames_Tens, Seconds_Units, Seconds_Tens, Minutes_Units, Minutes_Tens, Hours_Units, Hours_Tens;
    bool  DropFrame;

    Skip_S1(4,                                                  "BG8");
    Skip_S1(4,                                                  "BG7");

    Skip_SB(                                                    "BGF2 / Field Phase");
    Skip_SB(                                                    "BGF1");
    Get_S1 (2, Hours_Tens,                                      "Hours (Tens)");
    Get_S1 (4, Hours_Units,                                     "Hours (Units)");

    Skip_S1(4,                                                  "BG6");
    Skip_S1(4,                                                  "BG5");

    Skip_SB(                                                    "BGF0 / BGF2");
    Get_S1 (3, Minutes_Tens,                                    "Minutes (Tens)");
    Get_S1 (4, Minutes_Units,                                   "Minutes (Units)");

    Skip_S1(4,                                                  "BG4");
    Skip_S1(4,                                                  "BG3");

    Skip_SB(                                                    "FP - Field Phase / BGF0");
    Get_S1 (3, Seconds_Tens,                                    "Seconds (Tens)");
    Get_S1 (4, Seconds_Units,                                   "Seconds (Units)");

    Skip_S1(4,                                                  "BG2");
    Skip_S1(4,                                                  "BG1");

    Skip_SB(                                                    "CF - Color fame");
    Get_SB (   DropFrame,                                       "DP - Drop frame");
    Get_S1 (2, Frames_Tens,                                     "Frames (Tens)");
    Get_S1 (4, Frames_Units,                                    "Frames (Units)");

    if (Hours_Tens<3)
    {
        int64u TimeCode=(int64u)(Hours_Tens     *10*60*60*1000
                               + Hours_Units       *60*60*1000
                               + Minutes_Tens      *10*60*1000
                               + Minutes_Units        *60*1000
                               + Seconds_Tens         *10*1000
                               + Seconds_Units           *1000
                               + (Mpegv_frame_rate[frame_rate_code]?float64_int32s((Frames_Tens*10+Frames_Units)*1000/Mpegv_frame_rate[frame_rate_code]):0));

        Element_Info1(Ztring().Duration_From_Milliseconds(TimeCode));

        //TimeCode
        if (SMPTE_time_code_StartTimecode==(int64u)-1)
            SMPTE_time_code_StartTimecode=TimeCode;
    }
    Element_End0();
    Skip_S1( 8,                                                 "metadata_reserved_bits");
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
    {
        Get_S2 (10, channel_subsegment_size[Channel],           "channel_subsegment_size");
        channel_subsegment_sizes[Channel][channel_subsegment_size[Channel]]++;
    }
    if (!Mpegv_frame_rate_type[frame_rate_code])
        Get_S1 ( 8, metadata_extension_segment_size,            "metadata_extension_segment_size");
    else
        metadata_extension_segment_size=0;
    Get_S1 ( 8, meter_segment_size,                             "meter_segment_size");
    for (int8u Program=0; Program<DolbyE_Programs[program_config]; Program++)
    {
        Element_Begin1("per program");
        int8u description_text;
        Get_S1 ( 8, description_text,                           "description_text"); Element_Info1(description_text);
        Info_S1( 2, bandwidth_id,                               "bandwidth_id"); Element_Info1(bandwidth_id);
        if (description_text && Program>=description_text_Values.size())
            description_text_Values.resize(Program+1);
        switch (description_text)
        {
            case 0x00: // No text
                    if (Program<description_text_Values.size())
                    {
                        description_text_Values[Program].Previous.clear();
                        description_text_Values[Program].Current.clear();
                    }
                    break;
            case 0x02: // STX
                    description_text_Values[Program].Current.clear();
                    description_text_Values[Program].Current.push_back(description_text);
                    break;
            case 0x03: // ETX
                    if (!description_text_Values[Program].Current.empty() && description_text_Values[Program].Current[0]==0x02)
                        description_text_Values[Program].Previous= description_text_Values[Program].Current.substr(1);
                    else
                        description_text_Values[Program].Previous.clear();
                    description_text_Values[Program].Current.clear();
                    break;
            default: if (description_text>=0x20 && description_text<=0x7E)
                        description_text_Values[Program].Current.push_back(description_text);
        }
        Element_End0();
    }
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
    {
        Element_Begin1("per channel");
        Info_S1( 4, revision_id,                                "revision_id"); Element_Info1(revision_id);
        Info_SB(    bitpool_type,                               "bitpool_type");
        Info_S2(10, begin_gain,                                 "begin_gain"); Element_Info1(begin_gain);
        Info_S2(10, end_gain,                                   "end_gain"); Element_Info1(end_gain);
        Element_End0();
    }
    for(;;)
    {
        Element_Begin1("metadata_subsegment");
        int16u  metadata_subsegment_length;
        int8u   metadata_subsegment_id;
        Get_S1 ( 4, metadata_subsegment_id,                     "metadata_subsegment_id");
        if (metadata_subsegment_id==0)
        {
            Element_End0();
            break;
        }
        Get_S2 (12, metadata_subsegment_length,                 "metadata_subsegment_length");
        size_t End=Data_BS_Remain()-metadata_subsegment_length;
        switch (metadata_subsegment_id)
        {
            case 1 : ac3_metadata_subsegment(true); break;
            case 2 : ac3_metadata_subsegment(false); break;
            default: Skip_BS(metadata_subsegment_length,        "metadata_subsegment (unknown)");
        }
        if (Data_BS_Remain()>End)
            Skip_BS(Data_BS_Remain()-End,                       "unknown");
        Element_End0();
    }
    if (Data_BS_Remain()>metadata_segment_BitCountAfter)
        Skip_BS(Data_BS_Remain()-metadata_segment_BitCountAfter,"reserved_metadata_bits");
    Skip_S3(bit_depth,                                          "metadata_crc");

    {
        //CRC test
        size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        size_t Pos_Begin=Pos_End-(metadata_segment_size+1)*bit_depth; //+1 for CRC
        int8u BitSkip_Begin=Pos_Begin%8;
        Pos_Begin/=8;
        int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
        Pos_End/=8;
        if (BitSkip_End)
            Pos_End++;

        int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
        if (CRC)
        {
            // CRC is wrong
            Param_Info1("metadata_crc NOK");
        }
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::audio_segment()
{
    //Parsing
    Element_Begin1("audio_segment");
    #if MEDIAINFO_TRACE
        //CRC test
        size_t Pos_Begin=0;
    #endif //MEDIAINFO_TRACE
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
    {
        if ((Channel%(DolbyE_Channels[program_config]/2))==0 && key_present)
        {
            int16u audio_subsegment_size=0;
            for (int8u ChannelForSize=0; ChannelForSize<DolbyE_Channels[program_config]/2; ChannelForSize++)
                audio_subsegment_size+=channel_subsegment_size[((Channel<DolbyE_Channels[program_config]/2)?0:(DolbyE_Channels[program_config]/2))+ChannelForSize];

            if (Data_BS_Remain()<(audio_subsegment_size+1)*(size_t)bit_depth)
                return; //There is a problem

            //We must change the buffer
            switch (bit_depth)
            {
                case 16 :
                            {
                            int16u audio_subsegment_key;
                            Get_S2 (16, audio_subsegment_key, (Channel+1==DolbyE_Channels[program_config])?"audio_subsegment1_key":"audio_subsegment0_key");

                            int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                            for (int16u Pos=0; Pos<audio_subsegment_size+1; Pos++)
                                int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^audio_subsegment_key);
                            }
                            break;
                case 20 :
                            {
                            int32u audio_subsegment_key;
                            Get_S3 (20, audio_subsegment_key, (Channel+1==DolbyE_Channels[program_config])?"audio_subsegment1_key":"audio_subsegment0_key");

                            Descramble_20bit(audio_subsegment_key, audio_subsegment_size);
                            }
                            break;
                default :   ;
            }
        }

        #if MEDIAINFO_TRACE
            //CRC test
            if ((Channel%(DolbyE_Channels[program_config]/2))==0)
                Pos_Begin=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        #endif //MEDIAINFO_TRACE

        Element_Begin1(__T("Channel ")+Ztring::ToZtring(Channel));
        Element_Info1(Ztring::ToZtring(channel_subsegment_size[Channel])+__T(" words"));
        Skip_BS(channel_subsegment_size[Channel]*bit_depth,     "channel_subsegment");
        Element_End0();
        if ((Channel%(DolbyE_Channels[program_config]/2))==DolbyE_Channels[program_config]/2-1)
        {
            Skip_S3(bit_depth,                                  (Channel+1==DolbyE_Channels[program_config])?"audio_subsegment1_crc":"audio_subsegment0_crc");

            #if MEDIAINFO_TRACE
                //CRC test
                size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
                int8u BitSkip_Begin=Pos_Begin%8;
                Pos_Begin/=8;
                int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
                Pos_End/=8;
                if (BitSkip_End)
                    Pos_End++;

                int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
                if (CRC)
                {
                    //CRC is wrong
                    Param_Info1("NOK");
                }
            #endif //MEDIAINFO_TRACE
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::metadata_extension_segment()
{
    //Parsing
    Element_Begin1("metadata_extension_segment");
    if (key_present)
    {
        if (Data_BS_Remain()<((size_t)metadata_extension_segment_size+1)*(size_t)bit_depth) //+1 for CRC
            return; //There is a problem

        //We must change the buffer
        switch (bit_depth)
        {
            case 16 :
                        {
                        int16u metadata_extension_segment_key;
                        Get_S2 (16, metadata_extension_segment_key, "metadata_extension_segment_key");

                        int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                        for (int16u Pos=0; Pos<metadata_extension_segment_size+1; Pos++)
                            int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^metadata_extension_segment_key);
                        }
                        break;
            case 20 :
                        {
                        int32u metadata_extension_segment_key;
                        Get_S3 (20, metadata_extension_segment_key, "metadata_extension_segment_key");

                        Descramble_20bit(metadata_extension_segment_key, metadata_extension_segment_size);
                        }
                        break;
            default :   ;
        }
    }

    #if MEDIAINFO_TRACE
        //CRC test
        size_t Pos_Begin=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
    #endif //MEDIAINFO_TRACE

    size_t  metadata_extension_segment_BitCountAfter=Data_BS_Remain();
    metadata_extension_segment_BitCountAfter-=metadata_extension_segment_size*bit_depth;
    if (metadata_extension_segment_size)
    {
        for(;;)
        {
            Element_Begin1("metadata_extension_subsegment");
            int16u  metadata_extension_subsegment_length;
            int8u   metadata_extension_subsegment_id;
            Get_S1 ( 4, metadata_extension_subsegment_id,       "metadata_extension_subsegment_id");
            if (metadata_extension_subsegment_id==0)
            {
                Element_End0();
                break;
            }
            Get_S2 (12, metadata_extension_subsegment_length,   "metadata_extension_subsegment_length");
            switch (metadata_extension_subsegment_id)
            {
                default: Skip_BS(metadata_extension_subsegment_length,"metadata_extension_subsegment (unknown)");
            }
            Element_End0();
        }
        Param_Info1(metadata_extension_segment_BitCountAfter);
        Param_Info1(Data_BS_Remain());
        Param_Info1(Data_BS_Remain()-metadata_extension_segment_BitCountAfter);
        if (Data_BS_Remain()>metadata_extension_segment_BitCountAfter)
            Skip_BS(Data_BS_Remain()-metadata_extension_segment_BitCountAfter,"reserved_metadata_extension_bits");
    }
    Skip_S3(bit_depth,                                          "metadata_extension_crc");

    #if MEDIAINFO_TRACE
        //CRC test
        size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        int8u BitSkip_Begin=Pos_Begin%8;
        Pos_Begin/=8;
        int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
        Pos_End/=8;
        if (BitSkip_End)
            Pos_End++;

        int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
        if (CRC)
        {
            //CRC is wrong
            Param_Info1("NOK");
        }
    #endif //MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::audio_extension_segment()
{
    //Parsing
    Element_Begin1("audio_extension_segment");
    #if MEDIAINFO_TRACE
        //CRC test
        size_t Pos_Begin=0;
    #endif //MEDIAINFO_TRACE
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
    {
        if ((Channel%(DolbyE_Channels[program_config]/2))==0 && key_present)
        {
            int16u audio_extension_subsegment_size=0;
            for (int8u ChannelForSize=0; ChannelForSize<DolbyE_Channels[program_config]/2; ChannelForSize++)
                audio_extension_subsegment_size+=channel_subsegment_size[((Channel<DolbyE_Channels[program_config]/2)?0:(DolbyE_Channels[program_config]/2))+ChannelForSize];

            if (Data_BS_Remain()<((size_t)audio_extension_subsegment_size+1)*(size_t)bit_depth)
                return; //There is a problem

            //We must change the buffer
            switch (bit_depth)
            {
                case 16 :
                            {
                            int16u audio_extension_subsegment_key;
                            Get_S2 (16, audio_extension_subsegment_key, (Channel+1==DolbyE_Channels[program_config])?"audio_extension_subsegment1_key":"audio_extension_subsegment0_key");

                            int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                            for (int16u Pos=0; Pos<audio_extension_subsegment_size+1; Pos++)
                                int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^audio_extension_subsegment_key);
                            }
                            break;
                case 20 :
                            {
                            int32u audio_extension_subsegment_key;
                            Get_S3 (20, audio_extension_subsegment_key, (Channel+1==DolbyE_Channels[program_config])?"audio_extension_subsegment1_key":"audio_extension_subsegment0_key");

                            Descramble_20bit(audio_extension_subsegment_key, audio_extension_subsegment_size);
                            }
                            break;
                default :   ;
            }
        }

        #if MEDIAINFO_TRACE
            //CRC test
            if ((Channel%(DolbyE_Channels[program_config]/2))==0)
                Pos_Begin=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        #endif //MEDIAINFO_TRACE

        Element_Begin1(__T("Channel ")+Ztring::ToZtring(Channel));
        Element_Info1(Ztring::ToZtring(channel_subsegment_size[Channel])+__T(" words"));
        Skip_BS(channel_subsegment_size[Channel]*bit_depth,     "channel_subsegment");
        Element_End0();
        if ((Channel%(DolbyE_Channels[program_config]/2))==DolbyE_Channels[program_config]/2-1)
        {
            Skip_S3(bit_depth,                                  (Channel+1==DolbyE_Channels[program_config])?"audio_extension_subsegment1_crc":"audio_extension_subsegment0_crc");

            #if MEDIAINFO_TRACE
                //CRC test
                size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
                int8u BitSkip_Begin=Pos_Begin%8;
                Pos_Begin/=8;
                int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
                Pos_End/=8;
                if (BitSkip_End)
                    Pos_End++;

                int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
                if (CRC)
                {
                    //CRC is wrong
                    Param_Info1("NOK");
                }
            #endif //MEDIAINFO_TRACE
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::meter_segment()
{
    //Parsing
    Element_Begin1("meter_segment");
    if (key_present)
    {
        if (Data_BS_Remain()<((size_t)meter_segment_size+1)*(size_t)bit_depth) //+1 for CRC
            return; //There is a problem

        //We must change the buffer
        switch (bit_depth)
        {
            case 16 :
                        {
                        int16u meter_segment_key;
                        Get_S2 (16, meter_segment_key, "meter_segment_key");

                        int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                        for (int16u Pos=0; Pos<meter_segment_size+1; Pos++)
                            int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^meter_segment_key);
                        }
                        break;
            case 20 :
                        {
                        int32u meter_segment_key;
                        Get_S3 (20, meter_segment_key, "meter_segment_key");

                        Descramble_20bit(meter_segment_key, meter_segment_size);
                        }
                        break;
            default :   ;
        }
    }
    size_t  meter_segment_BitCountAfter=Data_BS_Remain();
    meter_segment_BitCountAfter-=meter_segment_size*bit_depth;
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
        Skip_S2(10,                                             "peak_meter");
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
        Skip_S2(10,                                             "rms_meter");
    if (Data_BS_Remain()>meter_segment_BitCountAfter)
        Skip_BS(Data_BS_Remain()>meter_segment_BitCountAfter,   "reserved_meter_bits");
    Skip_S3(bit_depth,                                          "meter_crc");

    #if MEDIAINFO_TRACE
        //CRC test
        size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        size_t Pos_Begin=Pos_End-(meter_segment_size+1)*bit_depth; //+1 for CRC
        int8u BitSkip_Begin=Pos_Begin%8;
        Pos_Begin/=8;
        int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
        Pos_End/=8;
        if (BitSkip_End)
            Pos_End++;

        int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
        if (CRC)
        {
            //CRC is wrong
            Param_Info1("NOK");
        }
    #endif //MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::ac3_metadata_subsegment(bool xbsi)
{
    for (int8u program=0; program<DolbyE_Programs[program_config]; program++)
    {
        Element_Begin1("per program");
        Skip_S1(5,                                          "ac3_datarate");
        Skip_S1(3,                                          "ac3_bsmod");
        Skip_S1(3,                                          "ac3_acmod");
        Skip_S1(2,                                          "ac3_cmixlev");
        Skip_S1(2,                                          "ac3_surmixlev");
        Skip_S1(2,                                          "ac3_dsurmod");
        Skip_S1(1,                                          "ac3_lfeon");
        Skip_S1(5,                                          "ac3_dialnorm");
        Skip_S1(1,                                          "ac3_langcode");
        Skip_S1(8,                                          "ac3_langcod");
        Skip_S1(1,                                          "ac3_audprodie");
        Skip_S1(5,                                          "ac3_mixlevel");
        Skip_S1(2,                                          "ac3_roomtyp");
        Skip_S1(1,                                          "ac3_copyrightb");
        Skip_S1(1,                                          "ac3_origbs");
        if (xbsi)
        {
            Skip_S1(1,                                      "ac3_xbsi1e");
            Skip_S1(2,                                      "ac3_dmixmod");
            Skip_S1(3,                                      "ac3_ltrtcmixlev");
            Skip_S1(3,                                      "ac3_ltrtsurmixlev");
            Skip_S1(3,                                      "ac3_lorocmixlev"); 
            Skip_S1(3,                                      "ac3_lorosurmixlev");
            Skip_S1(1,                                      "ac3_xbsi2e");
            Skip_S1(2,                                      "ac3_dsurexmod");
            Skip_S1(2,                                      "ac3_dheadphonmod");
            Skip_S1(1,                                      "ac3_adconvtyp");
            Skip_S1(8,                                      "ac3_xbsi2");
            Skip_S1(1,                                      "ac3_encinfo");
        }
        else
        {
            Skip_S1(1,                                      "ac3_timecode1e");
            Skip_S2(14,                                     "ac3_timecode1");
            Skip_S1(1,                                      "ac3_timecode2e");
            Skip_S2(14,                                     "ac3_timecode2");
        }
        Skip_S1(1,                                          "ac3_hpfon");
        Skip_S1(1,                                          "ac3_bwlpfon");
        Skip_S1(1,                                          "ac3_lfelpfon");
        Skip_S1(1,                                          "ac3_sur90on");
        Skip_S1(1,                                          "ac3_suratton");
        Skip_S1(1,                                          "ac3_rfpremphon");
        Skip_S1(1,                                          "ac3_compre");
        Skip_S1(8,                                          "ac3_compr1");
        Skip_S1(1,                                          "ac3_dynrnge");
        Skip_S1(8,                                          "ac3_dynrng1");
        Skip_S1(8,                                          "ac3_dynrng2");
        Skip_S1(8,                                          "ac3_dynrng3");
        Skip_S1(8,                                          "ac3_dynrng4");
        Element_End0();
    }
    for (int8u program=0; program<DolbyE_Programs[program_config]; program++)
    {
        Element_Begin1("per program");
        bool ac3_addbsie;
        Get_SB (   ac3_addbsie,                             "ac3_addbsie");
        if (ac3_addbsie)
        {
            int8u ac3_addbsil;
            Get_S1 (6, ac3_addbsil,                         "ac3_addbsil");
            for (int8u Pos=0; Pos<ac3_addbsil+1; Pos++)
                Skip_S1(8,                                  "ac3_addbsi[x]");
        }
        Element_End0();
    }
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyE::Descramble_20bit (int32u key, int16u size)
{
    int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
    int64u keys=(((int64u)key)<<20)|key;
    bool Half;
    if (Data_BS_Remain()%8)
    {
        Temp--;
        int24u2BigEndian(Temp, BigEndian2int24u(Temp)^(key));
        Half=true;
    }
    else
        Half=false;
    for (int16u Pos=0; Pos<size-(Half?1:0); Pos+=2)
        int40u2BigEndian(Temp+(Half?3:0)+Pos*5/2, BigEndian2int40u(Temp+(Half?3:0)+Pos*5/2)^keys);
    if ((size-((size && Half)?1:0))%2==0)
        int24u2BigEndian(Temp+(Half?3:0)+(size-((size && Half)?1:0))*5/2, BigEndian2int24u(Temp+(Half?3:0)+(size-((size && Half)?1:0))*5/2)^(key<<4));
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_DOLBYE_YES
