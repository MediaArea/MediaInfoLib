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

//***************************************************************************
// Infos (Common)
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AC3_YES) || defined(MEDIAINFO_DOLBYE_YES) || defined(MEDIAINFO_DVDV_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MPEGPS_YES) || defined(MEDIAINFO_MPEGTS_YES) || defined(MEDIAINFO_MIXML_YES)
//---------------------------------------------------------------------------

#include "ZenLib/Conf.h"
#include <string>
using namespace ZenLib;

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern const float64 Mpegv_frame_rate[16];

//---------------------------------------------------------------------------
extern const int32u AC3_SamplingRate[]=
{ 48000,  44100,  32000,      0,};

//---------------------------------------------------------------------------
const char*  AC3_Mode[] =
{
    "CM",
    "ME",
    "VI",
    "HI",
    "D",
    "C",
    "E",
    "VO",
};

//---------------------------------------------------------------------------
const char*  AC3_Mode_String[] =
{
    "Complete Main",
    "Music and Effects",
    "Visually Impaired",
    "Hearing Impaired",
    "Dialogue",
    "Commentary",
    "Emergency",
    "Voice Over",
};

//---------------------------------------------------------------------------
const char*  AC3_Surround[]=
{
    "",
    "Not Dolby Surround encoded",
    "Dolby Surround encoded",
    "",
};

//---------------------------------------------------------------------------
extern const int16u AC3_BitRate[]=
{

     32,
     40,
     48,
     56,
     64,
     80,
     96,
    112,
    128,
    160,
    192,
    224,
    256,
    320,
    384,
    448,
    512,
    576,
    640,
};

//---------------------------------------------------------------------------
extern const int8u AC3_Channels[]=
{2, 1, 2, 3, 3, 4, 4, 5};

//---------------------------------------------------------------------------
static const char* AC3_dynrngprof[]=
{
    "Film Standard",
    "Film Light",
    "Music Standard",
    "Music Light",
    "Speech",
};
extern std::string AC3_dynrngprof_Get(int8u Value)
{
    if (!Value)
        return {};
    if (Value>sizeof(AC3_dynrngprof)/sizeof(AC3_dynrngprof[0]))
        return std::to_string(Value);
    return AC3_dynrngprof[Value - 1];
}

//---------------------------------------------------------------------------
} //NameSpace

//---------------------------------------------------------------------------
#endif //...
//---------------------------------------------------------------------------

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AC3_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Ac3.h"
#include <vector>
#include <cmath>
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Config_MediaInfo.h"
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
#include "MediaInfo/MediaInfo_Internal.h"
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
const int32u AC3_SamplingRate2[]=
{ 24000,  22050,  16000,      0,};

//---------------------------------------------------------------------------
const char*  AC3_ChannelPositions[]=
{
    "Dual mono",
    "Front: C",
    "Front: L R",
    "Front: L C R",
    "Front: L R,   Back: C",
    "Front: L C R, Back: C",
    "Front: L R,   Side: L R",
    "Front: L C R, Side: L R",
};

//---------------------------------------------------------------------------
const char*  AC3_ChannelPositions2[]=
{
    "1+1",
    "1/0/0",
    "2/0/0",
    "3/0/0",
    "2/1/0",
    "3/1/0",
    "2/2/0",
    "3/2/0",
};

//---------------------------------------------------------------------------
const char*  AC3_ChannelLayout_lfeoff[]=
{
    "M M",
    "M",
    "L R",
    "L R C",
    "L R S",
    "L R C Cs",
    "L R Ls Rs",
    "L R C Ls Rs",
};

//---------------------------------------------------------------------------
const char*  AC3_ChannelLayout_lfeon[]=
{
    "1+1 LFE",
    "C LFE",
    "L R LFE",
    "L R C LFE",
    "L R S LFE",
    "L R C LFE Cs",
    "L R LFE Ls Rs",
    "L R C LFE Ls Rs",
};

//---------------------------------------------------------------------------
const char* AC3_roomtyp[]=
{
    "Large",
    "Small",
    "3",
};

//---------------------------------------------------------------------------
const char* AC3_dmixmod[]=
{
    "Lt/Rt",
    "Lo/Ro",
    "3",
};

//---------------------------------------------------------------------------
extern string AC3_Level_Value(int8u Index, float Start, float Multiplier)
{
    return Index == 7 ? string("-inf") : Ztring::ToZtring((Start - (int)Index * Multiplier), 1).To_UTF8();
}

//---------------------------------------------------------------------------
extern void AC3_Level_Fill(File__Analyze* A, size_t StreamPos, int8u Index, float Start, float Multiplier, const char* Name)
{
    string Value = AC3_Level_Value(Index, Start, Multiplier);
    A->Fill(Stream_Audio, StreamPos, Name, Value);
    A->Fill_SetOptions(Stream_Audio, StreamPos, Name, "N NT");
    string Name_String = string(Name) + "/String";
    A->Fill(Stream_Audio, StreamPos, Name_String.c_str(), Value + " dB");
    A->Fill_SetOptions(Stream_Audio, StreamPos, Name_String.c_str(), "Y NTN");
}

//---------------------------------------------------------------------------
int16u AC3_acmod2chanmap[]=
{
    0xA000,
    0x4000,
    0xA000,
    0xE000,
    0xA100,
    0xE100,
    0xB800,
    0xF800,
};

//---------------------------------------------------------------------------
Ztring AC3_chanmap_ChannelPositions (int16u chanmap)
{
    Ztring Front;
    Ztring Side;
    Ztring Back;
    Ztring More;

    for (int8u Pos=0; Pos<16; Pos++)
    {
        if (chanmap&(1<<(15-Pos)))
        {
            switch (Pos)
            {
                case  0 :   Front+=__T(" L"); break;
                case  1 :   Front+=__T(" C"); break;
                case  2 :   Front+=__T(" R"); break;
                case  3 :   Side+=__T(" L"); break;
                case  4 :   Side+=__T(" R"); break;
                case  5 :   {
                            bool HasR=false;
                            if (Front.find(__T(" R"))!=string::npos)
                            {
                                Front.resize(Front.size()-2);
                                HasR=true;
                            }
                            Front+=__T(" C C");
                            if (HasR)
                                Front+=__T(" R");
                            }
                            break;
                case  6 :   Back+=__T(" L R"); break;
                case  7 :   if (Back.empty())
                                Back=__T(" C");
                            else
                                Back=__T(" L C R");
                            break;
                case 15 :   More+=__T(", LFE");
                            break;
                default: ;
            }
        }
    }

    Ztring ToReturn;
    if (!Front.empty())
    {
        ToReturn+=__T("Front:")+Front;
    }
    if (!Side.empty())
    {
        if (!ToReturn.empty())
            ToReturn+=__T(", ");
        ToReturn+=__T("Side:")+Side;
    }
    if (!Back.empty())
    {
        if (!ToReturn.empty())
            ToReturn+=__T(", ");
        ToReturn+=__T("Back:")+Back;
    }
    ToReturn+=More;

    return ToReturn;
}

//---------------------------------------------------------------------------
int8u AC3_chanmap_Channels (int16u chanmap)
{
    int8u Channels=0;

    for (int8u Pos=0; Pos<16; Pos++)
    {
        if (chanmap&(1<<(15-Pos)))
        {
            switch (Pos)
            {
                case  5 :
                case  6 :
                case  9 :
                case 10 :
                case 11 :
                case 13 :
                            Channels+=2; break;
                default:
                            Channels++; break;
            }
        }
    }

    return Channels;
}

//---------------------------------------------------------------------------
static const char* AC3_chanmap_ChannelLayout_List[16] =
{
    "L",
    "C",
    "R",
    "Ls",
    "Rs",
    "Lc Rc",
    "Lrs Rrs",
    "Cs",
    "Ts",
    "Lsd Rsd",
    "Lw Rw",
    "Lvh Rvh",
    "Vhc", // Cvh is a typo in specs 
    "Lts Rts",
    "LFE2",
    "LFE",
};
Ztring AC3_chanmap_ChannelLayout (int16u chanmap, const Ztring &ChannelLayout0)
{
    Ztring ToReturn=ChannelLayout0;

    for (int8u Pos=5; Pos<15; Pos++) // Custom ones only
    {
        if (chanmap&(1<<(15-Pos)))
        {
            if (!ChannelLayout0.empty())
                ToReturn+=__T(' ');
            ToReturn+=Ztring().From_UTF8(AC3_chanmap_ChannelLayout_List[Pos]);
        }
    }

    return ToReturn;
}

//---------------------------------------------------------------------------
static const char* AC3_nonstd_bed_channel_assignment_mask_ChannelLayout_List[17] =
{
    "L",
    "R",
    "C",
    "LFE",
    "Ls",
    "Rs",
    "Lrs",
    "Rrs",
    "Lvh", // Lfh in spec but same as Lvh from E-AC-3
    "Rvh", // Rfh in spec but same as Rvh from E-AC-3
    "Lts", // Ltm in spec but same as Lts from E-AC-3
    "Rts", // Rtm in spec but same as Rts from E-AC-3
    "Lrh",
    "Rrh",
    "Lw",
    "Rw",
    "LFE2",
};
static int8s AC3_nonstd_bed_channel_assignment_mask_ChannelLayout_Reordering[17] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    6, // Wide channels before top layer
    6, // Wide channels before top layer
    -2,
    -2,
    -2,
    -2,
    -2,
    -2,
    0,
};
Ztring AC3_nonstd_bed_channel_assignment_mask_ChannelLayout(int32u nonstd_bed_channel_assignment_mask)
{
    Ztring ToReturn;

    for (int8u i=0; i<17; i++)
    {
        int8u i2=i+AC3_nonstd_bed_channel_assignment_mask_ChannelLayout_Reordering[i];
        if (nonstd_bed_channel_assignment_mask&(1<<i2))
        {
            ToReturn+=Ztring().From_UTF8(AC3_nonstd_bed_channel_assignment_mask_ChannelLayout_List[i2]);
            ToReturn+=__T(' ');
        }
    }

    if (!ToReturn.empty())
        ToReturn.resize(ToReturn.size()-1);

    return ToReturn;
}
//---------------------------------------------------------------------------
static const int8u AC3_bed_channel_assignment_mask_ChannelLayout_Mapping[10] =
{
    2,
    1,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
};
extern int32u AC3_bed_channel_assignment_mask_2_nonstd(int16u bed_channel_assignment_mask)
{
    int32u ToReturn=0;

    int8u j=0;
    for (int8u i=0; i<10; i++)
    {
        if (bed_channel_assignment_mask&(1<<i))
        {
            ToReturn|=(1<<(j++));
            if (AC3_bed_channel_assignment_mask_ChannelLayout_Mapping[i]>1)
                ToReturn|=(1<<(j++));
        }
        else
            j+=AC3_bed_channel_assignment_mask_ChannelLayout_Mapping[i];
    }

    return ToReturn;
}


//---------------------------------------------------------------------------
const int16u AC3_FrameSize[27][4]=
{
    { 128,  138,  192,    0},
    { 160,  174,  240,    0},
    { 192,  208,  288,    0},
    { 224,  242,  336,    0},
    { 256,  278,  384,    0},
    { 320,  348,  480,    0},
    { 384,  416,  576,    0},
    { 448,  486,  672,    0},
    { 512,  556,  768,    0},
    { 640,  696,  960,    0},
    { 768,  834, 1152,    0},
    { 896,  974, 1344,    0},
    {1024, 1114, 1536,    0},
    {1280, 1392, 1920,    0},
    {1536, 1670, 2304,    0},
    {1792, 1950, 2688,    0},
    {2048, 2228, 3072,    0},
    {2304, 2506, 3456,    0},
    {2560, 2786, 3840,    0},
    {   0,    0,    0,    0},
    {   0,    0,    0,    0},
    {   0,    0,    0,    0},
    {   0,    0,    0,    0},
    {   0,    0,    0,    0},
    {   0,    0,    0,    0},
    {   0,    0,    0,    0},
    { 768,    0,    0,    0},
};

//---------------------------------------------------------------------------
int16u AC3_FrameSize_Get(int8u frmsizecod, int8u fscod)
{
    bool Padding=(frmsizecod%2)?true:false;
    int16u frame_size_id=frmsizecod/2;

    if (frame_size_id>26 || fscod>3)
        return 0;

    int16u FrameSize=AC3_FrameSize[frame_size_id][fscod];
    if (fscod==1 && Padding)
        FrameSize+=2; // frame lengths are padded by 1 word (16 bits) at 44100 Hz
    return FrameSize;
}

//---------------------------------------------------------------------------
// CRC_16_Table
// A CRC is computed like this:
// Init: int32u CRC_16 = 0x0000;
// for each data byte do
//     CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^(data_byte)];
extern const int16u CRC_16_Table[256] =
{
    0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011,
    0x8033, 0x0036, 0x003c, 0x8039, 0x0028, 0x802d, 0x8027, 0x0022,
    0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
    0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041,
    0x80c3, 0x00c6, 0x00cc, 0x80c9, 0x00d8, 0x80dd, 0x80d7, 0x00d2,
    0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
    0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1,
    0x8093, 0x0096, 0x009c, 0x8099, 0x0088, 0x808d, 0x8087, 0x0082,
    0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
    0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1,
    0x01e0, 0x81e5, 0x81ef, 0x01ea, 0x81fb, 0x01fe, 0x01f4, 0x81f1,
    0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
    0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151,
    0x8173, 0x0176, 0x017c, 0x8179, 0x0168, 0x816d, 0x8167, 0x0162,
    0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
    0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101,
    0x8303, 0x0306, 0x030c, 0x8309, 0x0318, 0x831d, 0x8317, 0x0312,
    0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
    0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371,
    0x8353, 0x0356, 0x035c, 0x8359, 0x0348, 0x834d, 0x8347, 0x0342,
    0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
    0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2,
    0x83a3, 0x03a6, 0x03ac, 0x83a9, 0x03b8, 0x83bd, 0x83b7, 0x03b2,
    0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,
    0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291,
    0x82b3, 0x02b6, 0x02bc, 0x82b9, 0x02a8, 0x82ad, 0x82a7, 0x02a2,
    0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
    0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1,
    0x8243, 0x0246, 0x024c, 0x8249, 0x0258, 0x825d, 0x8257, 0x0252,
    0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
    0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231,
    0x8213, 0x0216, 0x021c, 0x8219, 0x0208, 0x820d, 0x8207, 0x0202
};

int CRC16_Init(int16u *Table, int16u Polynomial)
{
    for (size_t Pos=0; Pos<256; Pos++)
    {
        Table[Pos]=(int16u)Pos<<8;

        for(int8u bit=0; bit<8; bit++)
        {
            if (Table[Pos]&0x8000)
                Table[Pos]=(Table[Pos]<<1)^Polynomial;
            else
                Table[Pos]=Table[Pos]<<1;
        }
    }
    return 0;
}

static void CRC8_Init(int8u *Table, int8u Polynomial)
{
    for (size_t Pos = 0; Pos < 256; ++Pos)
    {
        Table[Pos] = static_cast<int8u>(Pos);

        for (int8u bit = 0; bit < 8; ++bit)
        {
            if (Table[Pos] & 0x80)
                Table[Pos] = (Table[Pos] << 1) ^ Polynomial;
            else
                Table[Pos] = Table[Pos] << 1;
        }
    }
}

//---------------------------------------------------------------------------
extern const float64 AC3_dynrng[]=
{
      6.02,
     12.04,
     18.06,
     24.08,
    -18.06,
    -12.04,
    - 6.02,
      0.00,
};

//---------------------------------------------------------------------------
extern const float64 AC3_compr[]=
{
      6.02,
     12.04,
     18.06,
     24.08,
     30.10,
     36.12,
     42.14,
     48.16,
    -42.14,
    -36.12,
    -30.10,
    -24.08,
    -18.06,
    -12.04,
    - 6.02,
      0.00,
};

//---------------------------------------------------------------------------
const char* AC3_HD_StreamType(int8u StreamType)
{
    switch (StreamType)
    {
        case 0xBA : return "TrueHD";
        case 0xBB : return "MLP";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
int32u AC3_HD_SamplingRate(int8u SamplingRate)
{
    if (SamplingRate==0xF)
        return 0;

    return ((SamplingRate&8)?44100:48000)<<(SamplingRate&7) ;
}

//---------------------------------------------------------------------------
static const int8u AC3_TrueHD_ChannelCountPerBit[13]=
{
    2, //LR
    1, //C
    1, //LFE
    2, //LRs
    2, //LRvh
    2, //LRc
    2, //LRrs
    1, //Cs
    1, //Ts
    2, //LRsd
    2, //LRw
    1, //Cvh
    1, //LFE2
};

//---------------------------------------------------------------------------
int8u AC3_TrueHD_Channels(int16u ChannelsMap)
{
    int8u Channels=0;

    for (int8u Pos=0; Pos<13; Pos++)
        Channels+=AC3_TrueHD_ChannelCountPerBit[Pos]*((ChannelsMap>>Pos)&0x1);

    return Channels;
}

//---------------------------------------------------------------------------
std::string AC3_TrueHD_Channels_Positions(int16u ChannelsMap, bool Bit11=false)
{
    std::string Text;
    if ((ChannelsMap&0x0003)==0x0003)
        Text+="Front: L C R";
    else
    {
        if (ChannelsMap&0x0001)
            Text+="Front: C";
        if (ChannelsMap&0x0002)
            Text+="Front: L, R";
    }

    if (ChannelsMap&0x08)
        Text+=", Side: L R";

    if (ChannelsMap&0x80)
        Text+=", Back: C";

    if ((ChannelsMap&0x0810)==0x0810 && Bit11)
        Text+=", vh: L C R";
    else
    {
        if (ChannelsMap&0x0010 && !Bit11)
            Text+=", vh: L R";
        if (ChannelsMap&0x0800)
            Text+=", vh: C";
    }

    if (ChannelsMap&0x0020)
        Text+=", c: L R";
    if (ChannelsMap&0x0040)
        Text+=", Back: L R";
    if (ChannelsMap&0x0100)
        Text+=", s: T";
    if (ChannelsMap&0x0200)
        Text+=", sd: L R";
    if (ChannelsMap&0x0400)
        Text+=", w: L R";

    if (ChannelsMap&0x0004)
        Text+=", LFE";
    if (ChannelsMap&0x1000)
        Text+=", LFE2";

    return Text;
}

//---------------------------------------------------------------------------
Ztring AC3_TrueHD_Channels_Positions2(int16u ChannelsMap, bool Bit11=false)
{
    int8u Front=0, Surround=0, Rear=0, LFE=0;

    if (ChannelsMap&0x0001)
        Front++;
    if (ChannelsMap&0x0002)
        Front+=2;

    if (ChannelsMap&0x08)
        Surround+=2;
    if (ChannelsMap&0x80)
        Surround++;

    if (ChannelsMap & 0x0010)
        Rear+=2; //vh
    if (!Bit11)
    {
    if (ChannelsMap&0x0800)
        Rear++;  //vh


    if (ChannelsMap&0x0020)
        Rear+=2; //c
    if (ChannelsMap&0x0040)
        Rear+=2; //rs
    if (ChannelsMap&0x0100)
        Rear+=2; //s
    if (ChannelsMap&0x0200)
        Rear+=2; //sd
    if (ChannelsMap&0x0400)
        Rear+=2; //w

    if (ChannelsMap&0x0004)
        LFE++;
    if (ChannelsMap&0x1000)
        LFE++;
    }

    Ztring Text;
    Text+=Ztring::ToZtring(Front);
    Text+=__T('/')+Ztring::ToZtring(Surround);
    Text+=__T('/')+Ztring::ToZtring(Rear);
    Text+=__T('.')+Ztring::ToZtring(LFE);
    return Text;
}

//---------------------------------------------------------------------------
static const size_t AC3_TrueHD_ChannelLayoutNames_Size=13;
const char* AC3_TrueHD_ChannelLayoutNames[AC3_TrueHD_ChannelLayoutNames_Size]=
{
    "L R",
    "C",
    "LFE",
    "Ls Rs",
    "Tfl Tfr",
    "Lsc Rsc",
    "Lb Rb",
    "Cb",
    "Tc",
    "Lsd Rsd",
    "Lw Rw",
    "Tfc",
    "LFE2",
};
static const size_t AC3_TrueHD_ChannelLayoutNames2_Size=1;
const char* AC3_TrueHD_ChannelLayoutNames2[AC3_TrueHD_ChannelLayoutNames2_Size]=
{
    "Tsl Tsr",
};
std::string AC3_TrueHD_Channels_ChannelLayout(int16u ChannelsMap, bool Bit11=false)
{
    if (ChannelsMap==2)
        return "M";

    std::string Text;

    for (size_t i=0; i<16; i++)
        if (ChannelsMap&(1<<i))
        {
            if (!Text.empty())
                Text+=' ';

            if ((!Bit11 && i>=AC3_TrueHD_ChannelLayoutNames_Size) || (Bit11 && i>=4 && i-4>=AC3_TrueHD_ChannelLayoutNames2_Size))
            {
                Text+='+'; //Unknown layout
                return Text;
            }

            Text+=(Bit11 && i>=4)?AC3_TrueHD_ChannelLayoutNames2[i-4]:AC3_TrueHD_ChannelLayoutNames[i];
        }

    return Text;
}

//---------------------------------------------------------------------------
static const char* AC3_TrueHD_ExtendedSubstreamInfo(int8u extended_substream_info)
{
    switch (extended_substream_info) {
    case 0b00: return "16-channel presentation is carried in substream 3";
    case 0b01: return "16-channel presentation is carried in substreams 2 and 3";
    case 0b10: return "16-channel presentation is carried in substreams 1, 2 and 3";
    case 0b11: return "16-channel presentation is carried in substreams 0, 1, 2 and 3";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* AC3_TrueHD_SubstreamInfo6ch(int8u presentation_6ch)
{
    switch (presentation_6ch) {
    case 0b00: return "Illegal";
    case 0b01: return "6-ch presentation is a copy of the 2-ch presentation carried in substream 0";
    case 0b10: return "6-ch presentation is carried in substream 1";
    case 0b11: return "6-ch presentation is carried in substreams 0 and 1";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* AC3_TrueHD_SubstreamInfo8ch(int8u presentation_8ch)
{
    switch (presentation_8ch) {
    case 0b000: return "Illegal";
    case 0b001: return "8-ch presentation is a copy of the 2-ch presentation carried in substream 0";
    case 0b010: return "8-ch presentation is a copy of the 6-ch presentation carried in substream 1";
    case 0b011: return "8-ch presentation is a copy of the 6-ch presentation carried in substreams 0 and 1";
    case 0b100: return "8-ch presentation is carried in substream 2";
    case 0b101: return "8-ch presentation is carried in substreams 0 and 2";
    case 0b110: return "8-ch presentation is carried in substreams 1 and 2";
    case 0b111: return "8-ch presentation is carried in substreams 0, 1 and 2";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* AC3_TrueHD_ExtendedSubstreamInfoNum(int8u extended_substream_info)
{
    switch (extended_substream_info) {
    case 0b00: return "3";
    case 0b01: return "2 + 3";
    case 0b10: return "1 + 2 + 3";
    case 0b11: return "0 + 1 + 2 + 3";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* AC3_TrueHD_SubstreamInfoNum(int8u presentation)
{
    switch (presentation) {
    case 0b001: return "0";
    case 0b010: return "1";
    case 0b011: return "0 + 1";
    case 0b100: return "2";
    case 0b101: return "0 + 2";
    case 0b110: return "1 + 2";
    case 0b111: return "0 + 1 + 2";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const int32u AC3_MLP_Channels[32]=
{
    1,
    2,
    3,
    4,
    3,
    4,
    5,
    3,
    4,
    5,
    4,
    5,
    6,
    4,
    5,
    4,
    5,
    6,
    5,
    5,
    6,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

//---------------------------------------------------------------------------
static const int32u AC3_MLP_Resolution[16]=
{
    16,
    20,
    24,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

//---------------------------------------------------------------------------
const char* Ac3_emdf_payload_id[16]=
{
    "Container End",
    "Programme loudness data",
    "Programme information",
    "E-AC-3 substream structure",
    "Dynamic range compression data for portable devices",
    "Programme Language",
    "External Data",
    "Headphone rendering data",
    "",
    "",
    "",
    "OAMD",
    "",
    "Personalized Audio Mix-Graph",
    "JOC",
    "",
};

//---------------------------------------------------------------------------
int32u Ac3_variable_bits(BitStream_Fast &Search2, int8u Bits)
{
    int32u Info = 0;

    for (;;)
    {
        Info += Search2.Get4(Bits);
        if (!Search2.GetB())
            break;
        Info <<= Bits;
        Info += (1 << Bits);
    }

    return Info;
}

//---------------------------------------------------------------------------
bool Ac3_EMDF_Test(const BitStream_Fast &Search)
{
    BitStream_Fast Search2(Search);
    Search2.Skip(16); //syncword
    size_t Size=((size_t)Search2.Get2(16))*8+17; //emdf_container_length
    if (Size>Search2.Remain())
        return false;
    size_t End=Search2.Remain()-Size;
    if (Search2.Get1(2)) //emdf_version
        return false;
    if (Search2.Get1(3) == 0x7) //key_id
        Ac3_variable_bits(Search2, 3);
    for (;;)
    {
        int8u emdf_payload_id=Search2.Get1(5);
        if (!emdf_payload_id)
            break;
        if (emdf_payload_id == 0x1F)
            Ac3_variable_bits(Search2, 5);
        bool smploffste=Search2.GetB();
        if (smploffste)
            Search2.Skip(12);
        if (Search2.GetB()) //duratione
            Ac3_variable_bits(Search2, 11); //duration
        if (Search2.GetB()) //groupide
            Ac3_variable_bits(Search2, 2); //groupid
        if (Search2.GetB()) //codecdatae
            return false; //must be 0
        if (!Search2.GetB()) //discard_unknown_payload
        {
            bool payload_frame_aligned=false;
            if (!smploffste)
            {
                payload_frame_aligned=Search2.GetB();
                if (payload_frame_aligned)
                    Search2.Skip(2);
            }
            if (smploffste || payload_frame_aligned)
                Search2.Skip(7);
        }
        size_t emdf_payload_size=((size_t)Ac3_variable_bits(Search2, 8))*8;
        Search2.Skip(emdf_payload_size);
    }
    int8u protection_length_primary=Search2.Get1(2);
    switch (protection_length_primary)
    {
        case 0: return false;
        case 1: protection_length_primary=8; break;
        case 2: protection_length_primary=32; break;
        case 3: protection_length_primary=128; break;
        default: ;
    }
    int8u protection_bits_secondary=Search2.Get1(2);
    switch (protection_bits_secondary)
    {
        case 0: protection_bits_secondary=0; break;
        case 1: protection_bits_secondary=8; break;
        case 2: protection_bits_secondary=32; break;
        case 3: protection_bits_secondary=128; break;
        default: ;
    }
    Search2.Skip(protection_length_primary);
    Search2.Skip(protection_bits_secondary);
    return Search2.Remain()>=17?true:false;
}

//---------------------------------------------------------------------------
/*
static const int8u ecplsubbndtab[]=
{
    13,
    19,
    25,
    31,
    37,
    49,
    61,
    73,
    85,
    97,
    109,
    121,
    133,
    145,
    157,
    169,
    181,
    193,
    205,
    217,
    229,
    241,
    253,
};
*/

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ac3::File_Ac3()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Ac3;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=64*1024;
    Buffer_TotalBytes_Fill_Max=1024*1024;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;
    Frame_Count_NotParsedIncluded=0;

    //In
    Frame_Count_Valid=0;
    MustParse_dac3=false;
    MustParse_dec3=false;
    MustParse_dmlp=false;
    CalculateDelay=false;

    //Buffer
    Save_Buffer=NULL;

    //Temp
    Frame_Count_HD=0;
    addbsi_Buffer=NULL;
    addbsi_Buffer_Size=0;
    fscod=0;
    fscod2=0;
    frmsizecod=0;
    bsid_Max=(int8u)-1;
    Formats[0]=0;
    Formats[1]=0;
    for (int8u Pos=0; Pos<8; Pos++)
        for (int8u Pos2=0; Pos2<9; Pos2++)
        {
            frmsizplus1_Max[Pos][Pos2]=0;
            acmod_Max[Pos][Pos2]=(int8u)-1;
            lfeon_Max[Pos][Pos2]=false;
            bsmod_Max[Pos][Pos2]=0;
            cmixlev_Max[Pos][Pos2]=(int8u)-1;
            surmixlev_Max[Pos][Pos2]=(int8u)-1;
            roomtyp_Max[Pos][Pos2]=(int8u)-1;
            dmixmod_Max[Pos][Pos2]=(int8u)-1;
            dsurmod_Max[Pos][Pos2]=0;
            chanmape_Max[Pos][Pos2]=false;
            chanmap_Max[Pos][Pos2]=0;
        }
    numblkscod=(int8u)-1;
    dsurexmod=0;
    dheadphonmod=0;
    substreamid_Independant_Current=0;
    substreams_Count=0;
    joc_complexity_index_Container=(int8u)-1;
    joc_complexity_index_Stream=(int8u)-1;
    num_dynamic_objects=(int8u)-1;
    nonstd_bed_channel_assignment_mask=(int32u)-1;
    dxc3_Parsed=false;
    HD_MajorSync_Parsed=false;
    Core_IsPresent=false;
    HD_IsPresent=false;
    HD_HasAtmos=false;
    dynrnge_Exists=false;
    TimeStamp_IsPresent=false;
    TimeStamp_IsParsing=false;
    TimeStamp_Parsed=false;
    TimeStamp_Count=0;
    BigEndian=true;
    IgnoreCrc_Done=false;
}

//---------------------------------------------------------------------------
File_Ac3::~File_Ac3()
{
    delete[] addbsi_Buffer;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac3::Streams_Fill()
{
    if (dxc3_Parsed)
    {
        if (Count_Get(Stream_Audio)==0)
            Stream_Prepare(Stream_Audio);
    }

    if (HD_MajorSync_Parsed)
    {
        if (Count_Get(Stream_Audio)==0)
            Stream_Prepare(Stream_Audio);
        if (HD_BitRate_Max)
            Fill(Stream_Audio, 0, Audio_BitRate_Maximum, (HD_BitRate_Max*AC3_HD_SamplingRate(HD_SamplingRate2)+8)>>4);

        if (HD_StreamType==0xBA) //TrueHD
        {
            {
                Fill(Stream_Audio, 0, Audio_Format, "MLP FBA");
                Fill(Stream_Audio, 0, Audio_Codec, "MLP FBA");
            }
            if (HD_HasAtmos)
            {
                Fill(Stream_Audio, 0, Audio_Format_Profile, "MLP FBA 16-ch / MLP FBA");
                Fill(Stream_Audio, 0, Audio_Codec_Profile, "MLP FBA 16-ch / MLP FBA");
            }
            else
            {
                Fill(Stream_Audio, 0, Audio_Format_Profile, "MLP FBA");
                Fill(Stream_Audio, 0, Audio_Codec_Profile, "MLP FBA");
            }
            Fill(Stream_Audio, 0, Audio_BitRate_Mode, "VBR");
            Fill(Stream_Audio, 0, Audio_Compression_Mode, "Lossless");
            Ztring Sampling;
            Sampling.From_Number(AC3_HD_SamplingRate(HD_SamplingRate1));
            Fill(Stream_Audio, 0, Audio_SamplingRate, Sampling);
            Fill(Stream_Audio, 0, Audio_Channel_s_, AC3_TrueHD_Channels(HD_Channels2));
            Fill(Stream_Audio, 0, Audio_ChannelPositions, AC3_TrueHD_Channels_Positions(HD_Channels2, HD_flags&(1<<11)));
            Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, AC3_TrueHD_Channels_Positions2(HD_Channels2, HD_flags&(1<<11)));
            Fill(Stream_Audio, 0, Audio_ChannelLayout, AC3_TrueHD_Channels_ChannelLayout(HD_Channels2, HD_flags&(1<<11)));
            Fill(Stream_Audio, 0, Audio_BitDepth, AC3_MLP_Resolution[HD_Resolution2]);
            Fill(Stream_Audio, 0, "NumberOfPresentations", HD_HasAtmos ? 4 : 3);
            Fill(Stream_Audio, 0, "NumberOfSubstreams", HD_SubStreams_Count);
        }

        if (HD_StreamType==0xBB) //MLP
        {
            {
                Fill(Stream_Audio, 0, Audio_Format, "MLP");
                Fill(Stream_Audio, 0, Audio_Codec,  "MLP");
            }
            Fill(Stream_Audio, 0, Audio_BitRate_Mode, "VBR");
            Fill(Stream_Audio, 0, Audio_Compression_Mode, "Lossless");
            Fill(Stream_Audio, 0, Audio_SamplingRate, AC3_HD_SamplingRate(HD_SamplingRate2));
            if (HD_SamplingRate1!=HD_SamplingRate2)
                Fill(Stream_Audio, 0, Audio_SamplingRate, AC3_HD_SamplingRate(HD_SamplingRate2));
            Fill(Stream_Audio, 0, Audio_Channel_s_, AC3_MLP_Channels[HD_Channels1]);
            if (MediaInfoLib::Config.LegacyStreamDisplay_Get() && HD_Channels1!=HD_Channels2)
                Fill(Stream_Audio, 0, Audio_Channel_s_, AC3_MLP_Channels[HD_Channels1]);
            Fill(Stream_Audio, 0, Audio_BitDepth, AC3_MLP_Resolution[HD_Resolution2]);
            if (MediaInfoLib::Config.LegacyStreamDisplay_Get() && HD_Resolution1!=HD_Resolution2)
                Fill(Stream_Audio, 0, Audio_BitDepth, AC3_MLP_Resolution[HD_Resolution1]);
        }
    }

    bool HasJOC = joc_num_objects_map.size()==1 && (joc_num_objects_map.begin()->second >= Frame_Count_Valid / 8 || joc_num_objects_map.begin()->second >= Frame_Count / 8); //Accepting that some frames do not contain JOC
    if (HasJOC || HD_HasAtmos)
    {
        if (Count_Get(Stream_Audio)==0)
            Stream_Prepare(Stream_Audio);
        if (HasJOC)
        {
            Fill(Stream_Audio, 0, Audio_Format_Profile, bsid_Max<=0x09?"AC-3 JOC":"E-AC-3 JOC");
            Fill(Stream_Audio, 0, Audio_Codec_Profile, bsid_Max<=0x09?"AC-3 JOC":"E-AC-3 JOC");
        }
        if (dxc3_Parsed && joc_complexity_index_Container!=(int8u)-1)
            Fill(Stream_Audio, 0, "ComplexityIndex", joc_complexity_index_Container);
        if (dxc3_Parsed && joc_complexity_index_Container==(int8u)-1 && joc_complexity_index_Stream!=(int8u)-1)
            Fill(Stream_Audio, 0, "ComplexityIndex", "Not present");
        if (joc_complexity_index_Stream!=(int8u)-1 && joc_complexity_index_Stream!=joc_complexity_index_Container)
            Fill(Stream_Audio, 0, "ComplexityIndex", joc_complexity_index_Stream);
        if (dxc3_Parsed && joc_complexity_index_Container!=(int8u)-1 && joc_complexity_index_Stream==(int8u)-1)
            Fill(Stream_Audio, 0, "ComplexityIndex", "Not present");
        if (num_dynamic_objects!=(int8u)-1)
            Fill(Stream_Audio, 0, "NumberOfDynamicObjects", num_dynamic_objects);
        if (nonstd_bed_channel_assignment_mask !=(int32u)-1)
        {
            Ztring BedChannelConfiguration=AC3_nonstd_bed_channel_assignment_mask_ChannelLayout(nonstd_bed_channel_assignment_mask);
            size_t BedChannelCount=0;
            if (!BedChannelConfiguration.empty())
                for (size_t i=0; i<BedChannelConfiguration.size();)
                {
                    BedChannelCount++;
                    i=BedChannelConfiguration.find(__T(' '), i+1);
                }
            Fill(Stream_Audio, 0, "BedChannelCount", BedChannelCount);
            Fill_SetOptions(Stream_Audio, 0, "BedChannelCount", "N NIY");
            Fill(Stream_Audio, 0, "BedChannelCount/String", MediaInfoLib::Config.Language_Get(Ztring::ToZtring(BedChannelCount), __T(" channel")));
            Fill_SetOptions(Stream_Audio, 0, "BedChannelCount/String", "Y NIN");
            Fill(Stream_Audio, 0, "BedChannelConfiguration", BedChannelConfiguration);
        }
    }

    //Surround
    if (dsurmod_Max[0][0]==2)
    {
        Fill(Stream_Audio, 0, Audio_Format_Settings, "Dolby Surround");
        Fill(Stream_Audio, 0, Audio_Format_Settings_Mode, "Dolby Surround");
    }
    if (dsurexmod==2)
    {
        Fill(Stream_Audio, 0, Audio_Format_Settings, "Dolby Surround EX");
        Fill(Stream_Audio, 0, Audio_Format_Settings_Mode, "Dolby Surround EX");
    }
    if (dsurexmod==3)
    {
        Fill(Stream_Audio, 0, Audio_Format_Settings, "Dolby Pro Logic IIz");
        Fill(Stream_Audio, 0, Audio_Format_Settings_Mode, "Dolby Pro Logic IIz");
    }
    if (dheadphonmod==2)
    {
        Fill(Stream_Audio, 0, Audio_Format_Settings, "Dolby Headphone");
        Fill(Stream_Audio, 0, Audio_Format_Settings_Mode, "Dolby Headphone");
    }

    //AC-3
    if (bsid_Max<=0x09)
    {
        if (Count_Get(Stream_Audio)==0)
            Stream_Prepare(Stream_Audio);
        if (Retrieve(Stream_Audio, 0, Audio_Format).empty() || MediaInfoLib::Config.LegacyStreamDisplay_Get())
        {
            Fill(Stream_Audio, 0, Audio_Format, "AC-3");
            Fill(Stream_Audio, 0, Audio_Codec, "AC3");
        }

        int32u Divider=bsid_Max==9?2:1; // Unofficial hack for low sample rate (e.g. 22.05 kHz)
        if (Ztring::ToZtring(AC3_SamplingRate[fscod]/Divider)!=Retrieve(Stream_Audio, 0, Audio_SamplingRate))
            Fill(Stream_Audio, 0, Audio_SamplingRate, AC3_SamplingRate[fscod]/Divider);
        if (frmsizecod/2<19)
        {
            if (Frame_Count_HD)
                Fill(Stream_Audio, 0, Audio_BitRate, "Unknown");
            int32u BitRate=AC3_BitRate[frmsizecod/2]*1000;
            int32u TimeStamp_BitRate=0;
            if (TimeStamp_Count==Frame_Count || TimeStamp_Count>Frame_Count/2) // In case of corrupted stream, check that there is a minimal count of timestamps 
                TimeStamp_BitRate+=float32_int32s(AC3_SamplingRate[fscod]/Divider/12.0f); // 12 = 1536 samples per frame / 128 bits per timestamp frame
            Fill(Stream_Audio, 0, Audio_BitRate, BitRate/Divider);
            if (TimeStamp_BitRate)
                Fill(Stream_Audio, 0, Audio_BitRate_Encoded, BitRate/Divider+TimeStamp_BitRate);
            if (CalculateDelay && Buffer_TotalBytes_FirstSynched>100 && BitRate>0)
            {
                Fill(Stream_Audio, 0, Audio_Delay, (float)Buffer_TotalBytes_FirstSynched*8*1000/BitRate, 0);
                Fill(Stream_Audio, 0, Audio_Delay_Source, "Stream");
            }
        }

        Fill(Stream_Audio, 0, Audio_ServiceKind, AC3_Mode[bsmod_Max[0][0]]);
        Fill(Stream_Audio, 0, Audio_ServiceKind_String, AC3_Mode_String[bsmod_Max[0][0]]);
        if ((MediaInfoLib::Config.LegacyStreamDisplay_Get() || Retrieve(Stream_Audio, 0, Audio_ChannelLayout).empty()) && acmod_Max[0][0]!=(int8u)-1)
        {
            int8u Channels=AC3_Channels[acmod_Max[0][0]];
            Ztring ChannelPositions; ChannelPositions.From_UTF8(AC3_ChannelPositions[acmod_Max[0][0]]);
            Ztring ChannelPositions2; ChannelPositions2.From_UTF8(AC3_ChannelPositions2[acmod_Max[0][0]]);
            Ztring ChannelLayout; ChannelLayout.From_UTF8(lfeon_Max[0][0]?AC3_ChannelLayout_lfeon[acmod_Max[0][0]]:AC3_ChannelLayout_lfeoff[acmod_Max[0][0]]);
            if (lfeon_Max[0][0])
            {
                Channels+=1;
                ChannelPositions+=__T(", LFE");
                ChannelPositions2+=__T(".1");
            }
            if (Ztring::ToZtring(Channels)!=Retrieve(Stream_Audio, 0, Audio_Channel_s_))
                Fill(Stream_Audio, 0, Audio_Channel_s_, Channels);
            if (ChannelPositions!=Retrieve(Stream_Audio, 0, Audio_ChannelPositions))
                Fill(Stream_Audio, 0, Audio_ChannelPositions, ChannelPositions);
            if (ChannelPositions2!=Retrieve(Stream_Audio, 0, Audio_ChannelPositions_String2))
                Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, ChannelPositions2);
            if (Retrieve(Stream_Audio, 0, Audio_ChannelLayout).empty())
                Fill(Stream_Audio, 0, Audio_ChannelLayout, ChannelLayout);
        }
        if (!Retrieve(Stream_Audio, 0, Audio_Format_Profile).empty())
            Fill(Stream_Audio, 0, Audio_Format_Profile, "AC-3");
        if (!Retrieve(Stream_Audio, 0, Audio_Codec_Profile).empty())
            Fill(Stream_Audio, 0, Audio_Codec_Profile, "AC-3");
        if ((MediaInfoLib::Config.LegacyStreamDisplay_Get() && __T("CBR")!=Retrieve(Stream_Audio, 0, Audio_BitRate_Mode)) || Retrieve(Stream_Audio, 0, Audio_BitRate_Mode).empty())
            Fill(Stream_Audio, 0, Audio_BitRate_Mode, "CBR");
    }

    //E-AC-3
    else if (bsid_Max<=0x10)
    {
        for (size_t Pos=0; Pos<8; Pos++)
            if (acmod_Max[Pos][0]!=(int8u)-1)
            {
                if (Count_Get(Stream_Audio)==0)
                    Stream_Prepare(Stream_Audio);
                if (Retrieve(Stream_Audio, 0, Audio_Format).empty())
                {
                    Fill(Stream_Audio, 0, Audio_Format, Formats[0]?"AC-3":"E-AC-3");
                    Fill(Stream_Audio, 0, Audio_Codec, Formats[0] ?"AC-3":"AC3+");
                }

                if (acmod_Max[1][0]!=(int8u)-1)
                    Fill(Stream_Audio, 0, Audio_ID, 1+Pos);

                Fill(Stream_Audio, 0, Audio_BitRate_Mode, "CBR");
                int32u SamplingRate;
                if (fscod!=3)
                    SamplingRate=AC3_SamplingRate[fscod];
                else
                    SamplingRate=AC3_SamplingRate2[fscod2];
                Fill(Stream_Audio, 0, Audio_SamplingRate, SamplingRate);
                if (numblkscod!=(int8u)-1)
                {
                    int8u numblks=numblkscod==3?6:numblkscod+1;
                    int32u frmsiz_Total=0;
                    for (size_t Pos2=0; Pos2<8; Pos2++)
                        frmsiz_Total+=frmsizplus1_Max[Pos][Pos2];
                    int32u TimeStamp_Size=0;
                    if (TimeStamp_Count==Frame_Count || TimeStamp_Count>Frame_Count/2) // In case of corrupted stream, check that there is a minimal count of timestamps 
                        TimeStamp_Size=16;
                    if (frmsiz_Total)
                        Fill(Stream_Audio, 0, Audio_BitRate, ((int64u)frmsiz_Total)*SamplingRate/32/numblks);
                    if (TimeStamp_Size)
                        Fill(Stream_Audio, 0, Audio_BitRate_Encoded, ((int64u)frmsiz_Total+TimeStamp_Size)*SamplingRate/32/numblks);
                }

                if (acmod_Max[Pos][1]!=(int8u)-1)
                {
                    int16u chanmap_Final=0;
                    for (int8u Pos2=0; Pos2<9; Pos2++)
                        if (acmod_Max[Pos][Pos2]!=(int8u)-1)
                        {
                            if (chanmape_Max[Pos][Pos2])
                                chanmap_Final|=chanmap_Max[Pos][Pos2];
                            else
                            {
                                chanmap_Final|=AC3_acmod2chanmap[acmod_Max[Pos][Pos2]];
                                if (lfeon_Max[Pos][Pos2])
                                    chanmap_Final|=1; // LFE position in chanmap is bit 0
                            }
                        }

                    Fill(Stream_Audio, 0, Audio_Format_Profile, "E-AC-3+Dep");
                    Fill(Stream_Audio, 0, Audio_Codec_Profile, "E-AC-3+Dep");
                    Fill(Stream_Audio, 0, Audio_Channel_s_, AC3_chanmap_Channels(chanmap_Final));
                    Fill(Stream_Audio, 0, Audio_ChannelPositions, AC3_chanmap_ChannelPositions(chanmap_Final));
                    Ztring ChannelPositions2; ChannelPositions2.From_UTF8(AC3_ChannelPositions2[acmod_Max[0][0]]); //TODO: handle the dependancy
                    if (lfeon_Max[Pos][0])
                    {
                        ChannelPositions2+=__T(".1");
                    }
                    Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, ChannelPositions2);
                    if (MediaInfoLib::Config.LegacyStreamDisplay_Get() || Retrieve(Stream_Audio, 0, Audio_ChannelLayout).empty())
                    {
                        Ztring ChannelLayout; ChannelLayout.From_UTF8(lfeon_Max[0][0]?AC3_ChannelLayout_lfeon[acmod_Max[0][0]]:AC3_ChannelLayout_lfeoff[acmod_Max[0][0]]);
                        Fill(Stream_Audio, 0, Audio_ChannelLayout, AC3_chanmap_ChannelLayout(chanmap_Final, ChannelLayout));
                    }
                }
                if (!Retrieve(Stream_Audio, 0, Audio_Format_Profile).empty())
                    Fill(Stream_Audio, 0, Audio_Format_Profile, Retrieve(Stream_Audio, 0, Audio_Format));
                if (!Retrieve(Stream_Audio, 0, Audio_Codec_Profile).empty())
                    Fill(Stream_Audio, 0, Audio_Codec_Profile, Retrieve(Stream_Audio, 0, Audio_Format));
                Fill(Stream_Audio, 0, Audio_ServiceKind, AC3_Mode[bsmod_Max[0][0]]);
                Fill(Stream_Audio, 0, Audio_ServiceKind_String, AC3_Mode_String[bsmod_Max[0][0]]);
                if ((MediaInfoLib::Config.LegacyStreamDisplay_Get() || Retrieve(Stream_Audio, 0, Audio_Channel_s_).empty()) && acmod_Max[Pos][0]!=(int8u)-1)
                {
                    int8u Channels=AC3_Channels[acmod_Max[Pos][0]];
                    Ztring ChannelPositions; ChannelPositions.From_UTF8(AC3_ChannelPositions[acmod_Max[Pos][0]]);
                    Ztring ChannelPositions2; ChannelPositions2.From_UTF8(AC3_ChannelPositions2[acmod_Max[0][0]]);
                    if (lfeon_Max[Pos][0])
                    {
                        Channels+=1;
                        ChannelPositions+=__T(", LFE");
                        ChannelPositions2+=__T(".1");
                    }
                    Fill(Stream_Audio, 0, Audio_Channel_s_, Channels);
                    Fill(Stream_Audio, 0, Audio_ChannelPositions, ChannelPositions);
                    Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, ChannelPositions2);
                    Fill(Stream_Audio, 0, Audio_ChannelLayout, lfeon_Max[0][0]?AC3_ChannelLayout_lfeon[acmod_Max[0][0]]:AC3_ChannelLayout_lfeoff[acmod_Max[0][0]]);
                }
            }
    }

    if (HD_MajorSync_Parsed)
    {
        //Filling Maximum bitrate with the constant core bitrate for better coherancy
        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        List.Write(Retrieve(Stream_Audio, 0, Audio_BitRate));
        if (List.size()>=2)
            Fill(Stream_Audio, 0, Audio_BitRate_Maximum, List[1]);
    }

    //TrueHD presentations
    if (HD_MajorSync_Parsed && HD_StreamType == 0xBA) {
        for (int p = 0; p < p_max - !HD_HasAtmos; ++p) {
            const Presentation& Presentation_Current = presentations[p];
            string P = Ztring(__T("Presentation") + Ztring::ToZtring(p)).To_UTF8();
            if (p == p_16ch) {
                Fill(Stream_Audio, 0, P.c_str(), "16-ch presentation");
                Fill(Stream_Audio, 0, (P + " LinkedTo_Substream_Pos").c_str(), AC3_TrueHD_ExtendedSubstreamInfoNum(Presentation_Current.presentation));
                Fill(Stream_Audio, 0, (P + " Elements").c_str(), Presentation_Current.elements);
            }
            else {
                Fill(Stream_Audio, 0, P.c_str(), (to_string(AC3_TrueHD_Channels(Presentation_Current.ch_assign)) + "-ch presentation"));
                Fill(Stream_Audio, 0, (P + " LinkedTo_Substream_Pos").c_str(), AC3_TrueHD_SubstreamInfoNum(Presentation_Current.presentation));
                Fill(Stream_Audio, 0, (P + " Channel(s)").c_str(), AC3_TrueHD_Channels(Presentation_Current.ch_assign));
                Fill(Stream_Audio, 0, (P + " ChannelLayout").c_str(), AC3_TrueHD_Channels_ChannelLayout(Presentation_Current.ch_assign, HD_flags & (1 << 11)));
            }
            if (p == p_2ch) {
                string type;
                switch (Presentation_Current.modifier) {
                case 0b00: type = "Stereo"; break;
                case 0b01: type = "Lt/Rt"; break;
                case 0b10: type = "Lbin/Rbin"; break;
                case 0b11: type = "Mono"; break;
                default: break;
                }
                Fill(Stream_Audio, 0, (P + " Type").c_str(), type);
            }
            else {
                if (Presentation_Current.modifier == 0b10)
                    Fill(Stream_Audio, 0, (P + " Format_Settings_Mode").c_str(), "Dolby Surround EX / Dolby Pro Logic IIx");
                if (Presentation_Current.modifier == 0b11)
                    Fill(Stream_Audio, 0, (P + " Format_Settings_Mode").c_str(), "Dolby Pro Logic IIz");
            }
            Fill(Stream_Audio, 0, (P + " control_enabled").c_str(), Presentation_Current.control_enabled ? "Yes" : "No");
            Fill(Stream_Audio, 0, (P + " dialogue_norm").c_str(), Presentation_Current.dialogue_norm == 0 ? -31 : -Presentation_Current.dialogue_norm);
            Fill_SetOptions(Stream_Audio, 0, (P + " dialogue_norm").c_str(), "N NT");
            Fill(Stream_Audio, 0, (P + " dialogue_norm/String").c_str(), Ztring::ToZtring(Presentation_Current.dialogue_norm == 0 ? -31 : -Presentation_Current.dialogue_norm) + __T(" LKFS"));
            Fill_SetOptions(Stream_Audio, 0, (P + " dialogue_norm/String").c_str(), "Y NTN");
            Fill(Stream_Audio, 0, (P + " mix_level").c_str(), 70 + Presentation_Current.mix_level);
            Fill_SetOptions(Stream_Audio, 0, (P + " mix_level").c_str(), "N NT");
            Fill(Stream_Audio, 0, (P + " mix_level/String").c_str(), Ztring::ToZtring(70 + Presentation_Current.mix_level) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, (P + " mix_level/String").c_str(), "Y NTN");
        }
    }

    //Dolby Metadata
    if (Core_IsPresent)
    {
        //Endianess
        Fill(Stream_Audio, 0, Audio_Format_Settings_Endianness, BigEndian?"Big":"Little");
        Fill(Stream_Audio, 0, "bsid", bsid_Max);

        Fill(Stream_Audio, 0, "dialnorm", FirstFrame_Dolby.dialnorm==0?-31:-FirstFrame_Dolby.dialnorm);
        Fill_SetOptions(Stream_Audio, 0, "dialnorm", "N NT");
        Fill(Stream_Audio, 0, "dialnorm/String", Ztring::ToZtring(FirstFrame_Dolby.dialnorm==0?-31:-FirstFrame_Dolby.dialnorm)+__T(" dB"));
        Fill_SetOptions(Stream_Audio, 0, "dialnorm/String", "Y NTN");
        if (FirstFrame_Dolby.compre)
        {
            float64 Value;
            if (FirstFrame_Dolby.compr ==0)
                Value=0; //Special case in the formula
            else
                Value=AC3_compr[FirstFrame_Dolby.compr>>4]+20*std::log10(((float)(0x10+(FirstFrame_Dolby.compr&0x0F)))/32);
            Fill(Stream_Audio, 0, "compr", Value, 2);
            Fill_SetOptions(Stream_Audio, 0, "compr", "N NT");
            Fill(Stream_Audio, 0, "compr/String", Ztring::ToZtring(Value, 2)+__T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "compr/String", "Y NTN");
        }
        if (FirstFrame_Dolby.dynrnge)
        {
            float64 Value;
            if (FirstFrame_Dolby.dynrng==0)
                Value=0; //Special case in the formula
            else
                Value=AC3_dynrng[FirstFrame_Dolby.dynrng>>5]+20*std::log10(((float)(0x20+(FirstFrame_Dolby.dynrng&0x1F)))/64);
            Fill(Stream_Audio, 0, "dynrng", Value, 2);
            Fill_SetOptions(Stream_Audio, 0, "dynrng", "N NT");
            Fill(Stream_Audio, 0, "dynrng/String", Ztring::ToZtring(Value, 2)+__T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "dynrng/String", "Y NTN");
        }

        for (int8u Pos=0; Pos<8; Pos++)
            for (int8u Pos2=0; Pos2<9; Pos2++)
            {
                if (acmod_Max[Pos][Pos2]==(int8u)-1)
                    break;
                else
                {
                    if (acmod_Max[Pos][Pos2]==2)
                    {
                        Fill(Stream_Audio, 0, "dsurmod", dsurmod_Max[Pos][Pos2]);
                        Fill_SetOptions(Stream_Audio, 0, "dsurmod", "N NT");
                        Fill(Stream_Audio, 0, "dsurmod/String", AC3_Surround[dsurmod_Max[Pos][Pos2]]);
                        Fill_SetOptions(Stream_Audio, 0, "dsurmod/String", "Y NTN");
                    }
                    Fill_SetOptions(Stream_Audio, 0, "bsid", "N NT");
                    Fill(Stream_Audio, 0, "acmod", acmod_Max[Pos][Pos2]);
                    Fill_SetOptions(Stream_Audio, 0, "acmod", "N NT");
                    Fill(Stream_Audio, 0, "lfeon", (lfeon_Max[Pos][Pos2])?1:0);
                    Fill_SetOptions(Stream_Audio, 0, "lfeon", "N NT");
                }
            }

        if (dynrngprof)
            Fill(Stream_Audio, 0, "dynrngprof", AC3_dynrngprof_Get(dynrngprof));
        if (comprprof)
            Fill(Stream_Audio, 0, "comprprof", AC3_dynrngprof_Get(comprprof));
    }

    //TimeStamp
    if (TimeStamp_IsPresent)
    {
        Ztring TimeCode_FrameRate=Ztring::ToZtring(TimeStamp_FirstFrame.GetFrameRate(), 3);
        auto TimeStamp_FirstFrame_String=TimeStamp_FirstFrame.ToString();
        int64s TimeStamp_FirstFrame_Milliseconds=TimeStamp_FirstFrame.ToMilliseconds();
        if (TimeStamp_FirstFrame_SampleNumber)
        {
            TimeStamp_FirstFrame_String+='-'+to_string(TimeStamp_FirstFrame_SampleNumber);
            auto SamplingRate=Retrieve(Stream_Audio, 0, Audio_SamplingRate).To_int32u();
            if (SamplingRate)
            {
                TimeStamp_FirstFrame_String+='S';
                TimeStamp_FirstFrame_String+=to_string(SamplingRate);
                TimeStamp_FirstFrame_Milliseconds+=((int32u)TimeStamp_FirstFrame_SampleNumber)*1000/SamplingRate;
            }
        }
        Fill(Stream_Audio, 0, "TimeCode_FirstFrame", TimeStamp_FirstFrame_String);
        Fill_SetOptions(Stream_Audio, 0, "TimeCode_FirstFrame", "N YCY");
        Fill(Stream_Audio, 0, "TimeCode_FirstFrame/String", TimeStamp_FirstFrame_String+" ("+TimeCode_FrameRate.To_UTF8()+" fps), embedded in stream");
        Fill_SetOptions(Stream_Audio, 0, "TimeCode_FirstFrame/String", "Y NTN");
        Fill(Stream_Audio, 0, "TimeCode_FirstFrame_FrameRate", TimeCode_FrameRate);
        Fill_SetOptions(Stream_Audio, 0, "TimeCode_FirstFrame_FrameRate", "N YFY");
        Fill(Stream_Audio, 0, "TimeCode_Source", "Stream");
        Fill_SetOptions(Stream_Audio, 0, "TimeCode_Source", "N YTY");
        Fill(Stream_Audio, 0, Audio_Delay, TimeStamp_FirstFrame_Milliseconds);
        Fill(Stream_Audio, 0, Audio_Delay_Source, "Stream");
        Fill(Stream_Audio, 0, Audio_Delay_Settings, TimeStamp_FirstFrame.IsDropFrame()?"drop_frame_flag=1":"drop_frame_flag=0");
    }

    //Samples per frame
    int16u SamplesPerFrame;
    if (bsid_Max<=0x08)
        SamplesPerFrame=1536;
    else if (bsid_Max<=0x09)
        SamplesPerFrame=768; // Unofficial hack for low sample rate (e.g. 22.05 kHz)
    else if (bsid_Max>0x0A && bsid_Max<=0x10 && numblkscod!=(int8u)-1)
        SamplesPerFrame=256*(numblkscod==3?6:(numblkscod+1));
    else if (HD_MajorSync_Parsed && (HD_StreamType==0xBA || HD_StreamType==0xBB)) // TrueHD or MLP
    {
        int64u HD_SamplingRate=Retrieve_Const(Stream_Audio, 0, Audio_SamplingRate).To_int64u();
        if (HD_SamplingRate<44100)
            SamplesPerFrame=0; //Unknown
        else if (HD_SamplingRate<=48000)
            SamplesPerFrame=40;
        else if (HD_SamplingRate<=96000)
            SamplesPerFrame=80;
        else if (HD_SamplingRate<=192000)
            SamplesPerFrame=160;
        else
            SamplesPerFrame=0; //Unknown
    }
    else
        SamplesPerFrame=0;
    if (SamplesPerFrame)
        Fill(Stream_Audio, 0, Audio_SamplesPerFrame, SamplesPerFrame);

    // Commercial name
    if (Retrieve(Stream_Audio, 0, Audio_Format)==__T("E-AC-3") || Retrieve(Stream_Audio, 0, Audio_Format_Profile).find(__T("E-AC-3"))==0)
    {
        if (HasJOC)
            Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby Digital Plus with Dolby Atmos");
        else
            Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby Digital Plus");
    }
    else if (Retrieve(Stream_Audio, 0, Audio_Format)==__T("MLP FBA") || Retrieve(Stream_Audio, 0, Audio_Format_Profile).find(__T("MLP FBA"))==0)
    {
        if (HasJOC || HD_HasAtmos)
            Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby TrueHD with Dolby Atmos");
        else
            Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby TrueHD");
    }
    else if (Retrieve(Stream_Audio, 0, Audio_Format)==__T("MLP") || Retrieve(Stream_Audio, 0, Audio_Format_Profile).find(__T("MLP"))==0)
    {
        Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "MLP Lossless");
    }
    else if (Retrieve(Stream_Audio, 0, Audio_Format)==__T("AC-3"))
        Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby Digital");
    Fill(Stream_General, 0, General_Format, Retrieve(Stream_Audio, 0, Audio_Format), true);
    Fill(Stream_General, 0, General_Format_Profile, Retrieve(Stream_Audio, 0, Audio_Format_Profile));
    Fill(Stream_General, 0, General_Format_Commercial_IfAny, Retrieve(Stream_Audio, 0, Audio_Format_Commercial_IfAny));

    // Other metadata
    if (cmixlev_Max[0][0]<=2)
    {
        string Value = Ztring::ToZtring(-3 - ((float)cmixlev_Max[0][0]) * 1.5, 1).To_UTF8();
        Fill(Stream_Audio, 0, "cmixlev", Value);
        Fill_SetOptions(Stream_Audio, 0, "cmixlev", "N NT");
        Fill(Stream_Audio, 0, "cmixlev/String", Value + " dB");
        Fill_SetOptions(Stream_Audio, 0, "cmixlev/String", "Y NTN");
    }
    if (surmixlev_Max[0][0]<=2)
    {
        string Value = (surmixlev_Max[0][0] == 2 ? string("-inf") : to_string(-3 - (int)surmixlev_Max[0][0] * 3));
        Fill(Stream_Audio, 0, "surmixlev", Value + " dB");
        Fill_SetOptions(Stream_Audio, 0, "surmixlev", "N NT");
        Fill(Stream_Audio, 0, "surmixlev/String", Value + " dB");
        Fill_SetOptions(Stream_Audio, 0, "surmixlev/String", "Y NTN");
    }
    if (roomtyp_Max[0][0]!=(int8u)-1)
    {
        string Value = to_string(80 + mixlevel_Max[0][0]);
        Fill(Stream_Audio, 0, "mixlevel", Value);
        Fill_SetOptions(Stream_Audio, 0, "mixlevel", "N NT");
        Fill(Stream_Audio, 0, "mixlevel/String", Value + " dB");
        Fill_SetOptions(Stream_Audio, 0, "mixlevel/String", "Y NTN");
        if (roomtyp_Max[0][0]) {
            Fill(Stream_Audio, 0, "roomtyp", AC3_roomtyp[roomtyp_Max[0][0] - 1]);
            Fill_SetOptions(Stream_Audio, 0, "roomtyp", "Y NTY");
        }
    }
    if (dmixmod_Max[0][0]!=(int8u)-1)
    {
        if (dmixmod_Max[0][0])
        {
            Fill(Stream_Audio, 0, "dmixmod", AC3_dmixmod[dmixmod_Max[0][0] - 1]);
            Fill_SetOptions(Stream_Audio, 0, "dmixmod", "Y NTY");
        }
        AC3_Level_Fill(this, 0, ltrtcmixlev_Max[0][0], 3, 1.5, "ltrtcmixlev");
        AC3_Level_Fill(this, 0, ltrtsurmixlev_Max[0][0], 3, 1.5, "ltrtsurmixlev");
        AC3_Level_Fill(this, 0, lorocmixlev_Max[0][0], 3, 1.5, "lorocmixlev");
        AC3_Level_Fill(this, 0, lorosurmixlev_Max[0][0], 3, 1.5, "lorosurmixlev");
        if (adconvtyp_Max[0][0])
        {
            Fill(Stream_Audio, 0, "adconvtyp", "HDCD");
            Fill_SetOptions(Stream_Audio, 0, "adconvtyp", "Y NTY");
        }
    }
}

//---------------------------------------------------------------------------
void File_Ac3::Streams_Finish()
{
    //Stats
    if (!dialnorms.empty())
    {
        int8u Minimum_Raw=1;
        int8u Maximum_Raw=31;
        float64 Sum_Intensity=0;
        int64u Count=0;
        for (int8u Pos=0; (size_t)Pos<dialnorms.size(); Pos++)
            if (dialnorms[Pos])
            {
                if (Minimum_Raw<(Pos==0?31:Pos))
                    Minimum_Raw=(Pos==0?31:Pos);
                if (Maximum_Raw>(Pos==0?31:Pos))
                    Maximum_Raw=(Pos==0?31:Pos);
                Sum_Intensity+=dialnorms[Pos]*pow(10, -((float64)Pos)/10);
                Count+=dialnorms[Pos];
            }
        if (Count)
        {
            float64 Average_dB = log10(Sum_Intensity / Count) * 10;
            Fill(Stream_Audio, 0, "dialnorm_Average", Average_dB, 0);
            Fill_SetOptions(Stream_Audio, 0, "dialnorm_Average", "N NT");
            Fill(Stream_Audio, 0, "dialnorm_Average/String", Ztring::ToZtring(Average_dB, 0) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "dialnorm_Average/String", "Y NTN");
            Fill(Stream_Audio, 0, "dialnorm_Minimum", -Minimum_Raw);
            Fill_SetOptions(Stream_Audio, 0, "dialnorm_Minimum", "N NT");
            Fill(Stream_Audio, 0, "dialnorm_Minimum/String", Ztring::ToZtring(-Minimum_Raw) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "dialnorm_Minimum/String", "Y NTN");
            Fill(Stream_Audio, 0, "dialnorm_Maximum", -Maximum_Raw);
            Fill_SetOptions(Stream_Audio, 0, "dialnorm_Maximum", "N NTN");
            Fill(Stream_Audio, 0, "dialnorm_Maximum/String", Ztring::ToZtring(-Maximum_Raw) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "dialnorm_Maximum/String", "Y NTN");
            Fill(Stream_Audio, 0, "dialnorm_Count", Count);
            Fill_SetOptions(Stream_Audio, 0, "dialnorm_Count", "N NTN");
        }
    }
    if (!comprs.empty())
    {
        float64 Minimum_dB=47.89;
        float64 Maximum_dB=-48.16;
        float64 Sum_Intensity=0;
        int64u Count=0;
        for (size_t Pos=0; Pos<comprs.size(); Pos++)
            if (comprs[Pos])
            {
                float64 Value=AC3_compr[Pos>>4]+20*std::log10(((float)(0x10+(Pos&0x0F)))/32);
                if (Minimum_dB>Value)
                    Minimum_dB=Value;
                if (Maximum_dB<Value)
                    Maximum_dB=Value;
                Sum_Intensity+=comprs[Pos]*pow(10, Value/10);
                Count+=comprs[Pos];
            }
        if (Count)
        {
            float64 Average_dB = log10(Sum_Intensity / Count) * 10;
            Fill(Stream_Audio, 0, "compr_Average", Average_dB, 2);
            Fill_SetOptions(Stream_Audio, 0, "compr_Average", "N NT");
            Fill(Stream_Audio, 0, "compr_Average/String", Ztring::ToZtring(Average_dB, 2) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "compr_Average/String", "N NTN");
            Fill(Stream_Audio, 0, "compr_Minimum", Minimum_dB, 2);
            Fill_SetOptions(Stream_Audio, 0, "compr_Minimum", "N NT");
            Fill(Stream_Audio, 0, "compr_Minimum/String", Ztring::ToZtring(Minimum_dB, 2) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "compr_Minimum/String", "N NTN");
            Fill(Stream_Audio, 0, "compr_Maximum", Maximum_dB, 2);
            Fill_SetOptions(Stream_Audio, 0, "compr_Maximum", "N NT");
            Fill(Stream_Audio, 0, "compr_Maximum/String", Ztring::ToZtring(Maximum_dB, 2) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "compr_Maximum/String", "N NTN");
            Fill(Stream_Audio, 0, "compr_Count", Count);
            Fill_SetOptions(Stream_Audio, 0, "compr_Count", "N NT");
        }
    }
    if (dynrnge_Exists && !dynrngs.empty())
    {
        float64 Minimum_dB=23.95;
        float64 Maximum_dB=-24.08;
        float64 Sum_Intensity=0;
        int64u Count=0;
        for (size_t Pos=0; Pos<dynrngs.size(); Pos++)
            if (dynrngs[Pos])
            {
                float64 Value;
                if (Pos==0)
                    Value=0; //Special case in the formula
                else
                    Value=AC3_dynrng[Pos>>5]+20*std::log10(((float)(0x20+(Pos&0x1F)))/64);
                if (Minimum_dB>Value)
                    Minimum_dB=Value;
                if (Maximum_dB<Value)
                    Maximum_dB=Value;
                Sum_Intensity+=dynrngs[Pos]*pow(10, Value/10);
                Count+=dynrngs[Pos];
            }
        if (Count)
        {
            float64 Average_dB = log10(Sum_Intensity / Count) * 10;
            Fill(Stream_Audio, 0, "dynrng_Average", Average_dB, 2);
            Fill_SetOptions(Stream_Audio, 0, "dynrng_Average", "N NT");
            Fill(Stream_Audio, 0, "dynrng_Average/String", Ztring::ToZtring(Average_dB, 2) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "dynrng_Average/String", "N NTN");
            Fill(Stream_Audio, 0, "dynrng_Minimum", Minimum_dB, 2);
            Fill_SetOptions(Stream_Audio, 0, "dynrng_Minimum", "N NT");
            Fill(Stream_Audio, 0, "dynrng_Minimum/String", Ztring::ToZtring(Minimum_dB, 2) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "dynrng_Minimum/String", "N NTN");
            Fill(Stream_Audio, 0, "dynrng_Maximum", Maximum_dB, 2);
            Fill_SetOptions(Stream_Audio, 0, "dynrng_Maximum", "N NT");
            Fill(Stream_Audio, 0, "dynrng_Maximum/String", Ztring::ToZtring(Maximum_dB, 2) + __T(" dB"));
            Fill_SetOptions(Stream_Audio, 0, "dynrng_Maximum/String", "N NTN");
            Fill(Stream_Audio, 0, "dynrng_Count", Count);
            Fill_SetOptions(Stream_Audio, 0, "dynrng_Count", "N NT");
        }
    }

    //Duration
    if (!IsSub)
    {
        int64u Frame_Count_ForDuration=0;
        if (Config->ParseSpeed>=1)
        {
            Frame_Count_ForDuration=Frame_Count; //We have the exact count of frames
            Fill(Stream_Audio, 0, Audio_StreamSize, File_Offset+Buffer_Offset+Element_Size-File_Offset_FirstSynched);
        }
        else if (bsid_Max<=9 && frmsizecods.size()==1 && fscods.size()==1 && Frame_Count_HD==0)
        {
            int16u Size=AC3_FrameSize_Get(frmsizecods.begin()->first, fscods.begin()->first);
            if (Size)
            {
                if (TimeStamp_IsPresent)
                    Size+=16;
                Frame_Count_ForDuration=(File_Size-File_Offset_FirstSynched)/Size; //Only complete frames
                Fill(Stream_Audio, 0, Audio_StreamSize, Frame_Count_ForDuration*Size);
            }
        }
        if (Frame_Count_ForDuration)
        {
            Clear(Stream_Audio, 0, Audio_BitRate);

            //HD part
            if (Frame_Count_HD)
            {
                int32u HD_SamplingRate=AC3_HD_SamplingRate(HD_SamplingRate1);
                if (HD_SamplingRate)
                {
                    int8u FrameDuration; //In samples
                    if (HD_SamplingRate<44100)
                        FrameDuration=0; //Unknown
                    else if (HD_SamplingRate<=48000)
                        FrameDuration=40;
                    else if (HD_SamplingRate<=96000)
                        FrameDuration=80;
                    else if (HD_SamplingRate<=192000)
                        FrameDuration=160;
                    else
                        FrameDuration=0; //Unknown
                    if (FrameDuration)
                    {
                        int64u SamplingCount=Frame_Count_HD*FrameDuration;
                        Fill(Stream_Audio, 0, Audio_Duration, SamplingCount/(((float64)HD_SamplingRate)/1000), 0);
                        Fill(Stream_Audio, 0, Audio_SamplingCount, SamplingCount);
                        Fill(Stream_Audio, 0, Audio_BitRate, (File_Size-File_Offset_FirstSynched)/(SamplingCount/(((float64)HD_SamplingRate)/1000))*8, 0);
                    }
                    Fill(Stream_Audio, 0, Audio_FrameCount, Frame_Count_HD);
                }
            }
            if (Core_IsPresent)
            {
                Fill(Stream_Audio, 0, Audio_FrameCount, Frame_Count_ForDuration);
                if (AC3_SamplingRate[fscod])
                {
                    float64 FrameDuration;
                    if (bsid_Max<=0x08)
                        FrameDuration=32;
                    else if (bsid_Max<=0x09)
                        FrameDuration=16; // Unofficial hack for low sample rate (e.g. 22.05 kHz)
                    else
                        FrameDuration=0;
                    if (FrameDuration)
                    {
                        FrameDuration*=((float64)48000)/AC3_SamplingRate[fscod]; //32 ms for 48 KHz, else proportional (34.83 for 44.1 KHz, 48 ms for 32 KHz)
                        Fill(Stream_Audio, 0, Audio_SamplingCount, Frame_Count_ForDuration*1536);
                        Fill(Stream_Audio, 0, Audio_Duration, Frame_Count_ForDuration*FrameDuration, 0);
                        if (frmsizecod/2<19)
                        {
                            int32u BitRate=AC3_BitRate[frmsizecod/2]*1000;
                            int32u Divider=bsid_Max == 9?2:1; // Unofficial hack for low sample rate (e.g. 22.05 kHz)
                            Fill(Stream_Audio, 0, Audio_BitRate, BitRate/Divider);
                        }
                    }
                }
            }
        }
    }
    else if (FrameInfo.PTS!=(int64u)-1 && FrameInfo.PTS>PTS_Begin)
    {
        Fill(Stream_Audio, 0, Audio_Duration, float64_int64s(((float64)(FrameInfo.PTS-PTS_Begin))/1000000));
        float64 FrameDuration;
        if (bsid_Max<=0x08)
            FrameDuration=32;
        else if (bsid_Max<=0x09)
            FrameDuration=16; // Unofficial hack for low sample rate (e.g. 22.05 kHz)
        else if (bsid_Max>0x0A && bsid_Max<=0x10)
        {
            int8u numblks=numblkscod==3?6:numblkscod+1;
            FrameDuration=((float64)32)/6*numblks;
        }
        else
            FrameDuration=0;
        if (FrameDuration)
            Fill(Stream_Audio, 0, Audio_FrameCount, float64_int64s(((float64)(FrameInfo.PTS-PTS_Begin))/1000000/FrameDuration));
    }
}

//---------------------------------------------------------------------------
void File_Ac3::Read_Buffer_Unsynched()
{
    delete[] Save_Buffer; Save_Buffer=NULL;

    if (File_GoTo==0)
        Synched_Init();
}

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_Ac3::Read_Buffer_Seek (size_t Method, int64u Value, int64u /*ID*/)
{
    GoTo(0);
    Open_Buffer_Unsynch();
    return 1;
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ac3::FileHeader_Begin()
{
    //Specific cases
    if (MustParse_dac3 || MustParse_dec3)
        return true;

    //Must have enough buffer for having header
    if (Buffer_Size<4)
        return false; //Must wait for more data

    //False positives detection: detect Matroska files, AC-3 parser is not smart enough
    if (!FileHeader_Begin_0x000001())
    {
        Finish("AC-3");
        return false;
    }

    //All should be OK...
    return true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ac3::Synchronize()
{
    //Specific cases
    if (MustParse_dac3 || MustParse_dec3)
        return true;

    //Synchronizing
    while (Buffer_Offset+8<=Buffer_Size)
    {
        if (!FrameSynchPoint_Test())
            return false; //Need more data
        if (Synched)
            break;
        Buffer_Offset++;
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+8>Buffer_Size)
    {
        //We must keep more bytes in order to detect TimeStamp
        if (Frame_Count==0)
        {
            if (Buffer_Offset>=16)
                Buffer_Offset-=16;
            else
                Buffer_Offset=0;
            return false;
        }

        if (Buffer_Offset+7==Buffer_Size && CC3(Buffer+Buffer_Offset+4)!=0xF8726F && CC2(Buffer+Buffer_Offset)!=0x0B77 && CC2(Buffer+Buffer_Offset)!=0x770B)
            Buffer_Offset++;
        if (Buffer_Offset+6==Buffer_Size && CC2(Buffer+Buffer_Offset+4)!=0xF872   && CC2(Buffer+Buffer_Offset)!=0x0B77 && CC2(Buffer+Buffer_Offset)!=0x770B)
            Buffer_Offset++;
        if (Buffer_Offset+5==Buffer_Size && CC1(Buffer+Buffer_Offset+4)!=0xF8     && CC2(Buffer+Buffer_Offset)!=0x0B77 && CC2(Buffer+Buffer_Offset)!=0x770B)
            Buffer_Offset++;
        if (Buffer_Offset+4==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x0B77 && CC2(Buffer+Buffer_Offset)!=0x770B)
            Buffer_Offset++;
        if (Buffer_Offset+3==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x0B77 && CC2(Buffer+Buffer_Offset)!=0x770B)
            Buffer_Offset++;
        if (Buffer_Offset+2==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x0B77 && CC2(Buffer+Buffer_Offset)!=0x770B)
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && CC1(Buffer+Buffer_Offset)!=0x0B && CC1(Buffer+Buffer_Offset)!=0x77)
            Buffer_Offset++;
        return false;
    }

    //Testing if we have TimeStamp
    if (Buffer_Offset>=16)
    {
        if ( Buffer[Buffer_Offset-0x10+0x00]==0x01      //Magic value? Always 0x01
         &&  Buffer[Buffer_Offset-0x10+0x01]==0x10      //Size? Always 0x10
         &&  Buffer[Buffer_Offset-0x10+0x02]==0x00      //First  byte of HH? Always 0x00
         && (Buffer[Buffer_Offset-0x10+0x03]>>4 )<0x6   //Second byte of HH? First  4 bits must be <0x6
         && (Buffer[Buffer_Offset-0x10+0x03]&0xF)<0xA   //Second byte of HH? Second 4 bits must be <0xA
         &&  Buffer[Buffer_Offset-0x10+0x04]==0x00      //First  byte of MM? Always 0x00
         && (Buffer[Buffer_Offset-0x10+0x05]>>4 )<0x6   //Second byte of MM? First  4 bits must be <0x6
         && (Buffer[Buffer_Offset-0x10+0x05]&0xF)<0xA   //Second byte of MM? Second 4 bits must be <0xA
         &&  Buffer[Buffer_Offset-0x10+0x06]==0x00      //First  byte of SS? Always 0x00
         && (Buffer[Buffer_Offset-0x10+0x07]>>4 )<0x6   //Second byte of SS? First  4 bits must be <0x6
         && (Buffer[Buffer_Offset-0x10+0x07]&0xF)<0xA   //Second byte of SS? Second 4 bits must be <0xA
         &&  Buffer[Buffer_Offset-0x10+0x08]==0x00      //First  byte of FF? Always 0x00
         && (Buffer[Buffer_Offset-0x10+0x09]>>4 )<0x4   //Second byte of FF? First  4 bits must be <0x4
         && (Buffer[Buffer_Offset-0x10+0x09]&0xF)<0xA   //Second byte of FF? Second 4 bits must be <0xA
         && !(Buffer[Buffer_Offset-0x10+0x00]==0x00     //We want at least a byte not zero, in order to differentiate TimeStamp from padding
           && Buffer[Buffer_Offset-0x10+0x01]==0x00
           && Buffer[Buffer_Offset-0x10+0x0C]==0x00
           && Buffer[Buffer_Offset-0x10+0x0D]==0x00
           && Buffer[Buffer_Offset-0x10+0x0E]==0x00
           && Buffer[Buffer_Offset-0x10+0x0F]==0x00))
        {
            TimeStamp_IsPresent=true;
            Buffer_Offset-=16;
        }
    }

    //Synched
    return true;
}

//---------------------------------------------------------------------------
void File_Ac3::Synched_Init()
{
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?32:(IsSub?1:2);

    //FrameInfo
    PTS_End=0;
    if (!IsSub)
    {
        FrameInfo.DTS=0; //No DTS in container
        FrameInfo.PTS=0; //No PTS in container
    }
    DTS_Begin=FrameInfo.DTS;
    DTS_End=FrameInfo.DTS;
    if (Frame_Count_NotParsedIncluded==(int64u)-1)
        Frame_Count_NotParsedIncluded=0; //No Frame_Count_NotParsedIncluded in the container
}

//---------------------------------------------------------------------------
bool File_Ac3::Synched_Test()
{
    //Specific cases
    if (MustParse_dac3 || MustParse_dec3)
        return true;

    //Must have enough buffer for having header
    if (Buffer_Offset+(TimeStamp_IsPresent?16:0)+6>Buffer_Size)
        return false;

    //TimeStamp
    if (TimeStamp_IsPresent && !TimeStamp_Parsed)
    {
        if (!( Buffer[Buffer_Offset+0x00]==0x01         //Magic value? Always 0x01
           &&  Buffer[Buffer_Offset+0x01]==0x10         //Size? Always 0x10
           &&  Buffer[Buffer_Offset+0x02]==0x00         //First  byte of HH? Always 0x00
           && (Buffer[Buffer_Offset+0x03]>>4 )<0x6      //Second byte of HH? First  4 bits must be <0x6
           && (Buffer[Buffer_Offset+0x03]&0xF)<0xA      //Second byte of HH? Second 4 bits must be <0xA
           &&  Buffer[Buffer_Offset+0x04]==0x00         //First  byte of MM? Always 0x00
           && (Buffer[Buffer_Offset+0x05]>>4 )<0x6      //Second byte of MM? First  4 bits must be <0x6
           && (Buffer[Buffer_Offset+0x05]&0xF)<0xA      //Second byte of MM? Second 4 bits must be <0xA
           &&  Buffer[Buffer_Offset+0x06]==0x00         //First  byte of SS? Always 0x00
           && (Buffer[Buffer_Offset+0x07]>>4 )<0x6      //Second byte of SS? First  4 bits must be <0x6
           && (Buffer[Buffer_Offset+0x07]&0xF)<0xA      //Second byte of SS? Second 4 bits must be <0xA
           &&  Buffer[Buffer_Offset+0x08]==0x00         //First  byte of FF? Always 0x00
           && (Buffer[Buffer_Offset+0x09]>>4 )<0x4      //Second byte of FF? First  4 bits must be <0x4
           && (Buffer[Buffer_Offset+0x09]&0xF)<0xA))    //Second byte of FF? Second 4 bits must be <0xA
            TimeStamp_IsPresent=false;
    }
    if (TimeStamp_IsPresent && !TimeStamp_Parsed)
        Buffer_Offset+=16;

    //Quick test of synchro
    if (!FrameSynchPoint_Test())
    {
        if (TimeStamp_IsPresent && !TimeStamp_Parsed && Buffer_Offset>=16)
            Buffer_Offset-=16;
        return false; //Need more data
    }
    if (!Synched)
        return true;

    //TimeStamp
    if (TimeStamp_IsPresent && !TimeStamp_Parsed)
    {
        Buffer_Offset-=16;
        TimeStamp_IsParsing=true;
        TimeStamp_Parsed=false;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Demux
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
bool File_Ac3::Demux_UnpacketizeContainer_Test()
{
    if (TimeStamp_IsPresent)
        Buffer_Offset+=16;

    if (!HD_IsPresent && Frame_Count==0 && Save_Buffer==NULL)
    {
        //Searching HD part
        size_t Buffer_Offset_Save=Buffer_Offset;
        Buffer_Offset++;
        Synched=false;
        while (Buffer_Offset+8<=Buffer_Size)
        {
            if (!FrameSynchPoint_Test())
            {
                Buffer_Offset=Buffer_Offset_Save;
                return false; //Need more data
            }
            if (Synched)
                break;
            Buffer_Offset++;
        }
        Buffer_Offset=Buffer_Offset_Save;
        if (!Synched)
        {
            Synched=true;
            if (TimeStamp_IsPresent)
                Buffer_Offset-=16;
            return false; //Need more data
        }
    }

    if (Save_Buffer)
    {
        Demux_TotalBytes-=Buffer_Offset;
        Demux_Offset-=Buffer_Offset;
        File_Offset+=Buffer_Offset;
        swap(Buffer, Save_Buffer);
        swap(Buffer_Offset, Save_Buffer_Offset);
        swap(Buffer_Size, Save_Buffer_Size);
    }

    if (Buffer[Buffer_Offset]==0x0B && Buffer[Buffer_Offset+1]==0x77)
    {
        int8u bsid=Buffer[Buffer_Offset+5]>>3;
        if (bsid<=0x08)
            FrameInfo.DUR=32000000;
        else if (bsid<=0x09)
            FrameInfo.DUR=16000000; // Unofficial hack for low sample rate (e.g. 22.05 kHz)
        else if (bsid>0x0A && bsid<=0x10)
        {
            numblkscod=(Buffer[Buffer_Offset+4]>>4)&0x3;
            int64u numblks=numblkscod==3?6:numblkscod+1;
            FrameInfo.DUR=32000000*numblks/6;
        }

        Demux_Offset=Buffer_Offset+Core_Size_Get();

        //Core part
        if (HD_IsPresent)
        {
            if (TimeStamp_IsPresent)
                Buffer_Offset-=16;

            if (Save_Buffer)
            {
                swap(Buffer, Save_Buffer);
                swap(Buffer_Offset, Save_Buffer_Offset);
                swap(Buffer_Size, Save_Buffer_Size);
                Demux_TotalBytes+=Buffer_Offset;
                Demux_Offset+=Buffer_Offset;
                File_Offset-=Buffer_Offset;
            }

            return true; //No AC-3 demux
        }
    }
    else
    {
        Demux_Offset=Buffer_Offset+HD_Size_Get();
    }

    if (Demux_Offset>Buffer_Size && File_Offset+Buffer_Size!=File_Size)
    {
        if (TimeStamp_IsPresent)
            Buffer_Offset-=16;

        if (Save_Buffer)
        {
            swap(Buffer, Save_Buffer);
            swap(Buffer_Offset, Save_Buffer_Offset);
            swap(Buffer_Size, Save_Buffer_Size);
            Demux_TotalBytes+=Buffer_Offset;
            Demux_Offset+=Buffer_Offset;
            File_Offset-=Buffer_Offset;
        }

        return false; //No complete frame
    }

    Demux_UnpacketizeContainer_Demux();

    if (Save_Buffer)
    {
        swap(Buffer, Save_Buffer);
        swap(Buffer_Offset, Save_Buffer_Offset);
        swap(Buffer_Size, Save_Buffer_Size);
        Demux_TotalBytes+=Buffer_Offset;
        Demux_Offset+=Buffer_Offset;
        File_Offset-=Buffer_Offset;
    }

    if (TimeStamp_IsPresent)
        Buffer_Offset-=16;

    return true;
}
#endif //MEDIAINFO_DEMUX

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac3::Read_Buffer_OutOfBand()
{
    if (MustParse_dmlp)
    {
        dmlp();
        return;
    }
}

//---------------------------------------------------------------------------
void File_Ac3::Read_Buffer_Continue()
{
    if (MustParse_dac3)
    {
        dac3();
        return;
    }
    if (MustParse_dec3)
    {
        dec3();
        return;
    }
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac3::Header_Parse()
{
    //TimeStamp
    if (TimeStamp_IsParsing)
    {
        Header_Fill_Size(16);
        Header_Fill_Code(2, "TimeStamp");
        return;
    }

    if (Save_Buffer)
    {
        File_Offset+=Buffer_Offset;
        swap(Buffer, Save_Buffer);
        swap(Buffer_Offset, Save_Buffer_Offset);
        swap(Buffer_Size, Save_Buffer_Size);
    }

    //Filling
    if ((Buffer[Buffer_Offset]==0x0B && Buffer[Buffer_Offset+1]==0x77)
     || (Buffer[Buffer_Offset]==0x77 && Buffer[Buffer_Offset+1]==0x0B))
    {
        Header_Fill_Size(Core_Size_Get());
        Header_Fill_Code(0, "syncframe");

        //Little Endian management
        if (Save_Buffer)
        {
            swap(Buffer, Save_Buffer);
            swap(Buffer_Offset, Save_Buffer_Offset);
            swap(Buffer_Size, Save_Buffer_Size);
            File_Offset-=Buffer_Offset;
        }

        return;
    }

    //MLP or TrueHD specific
    int16u Size;
    BS_Begin();
    Skip_S1( 4,                                                 "check_nibble");
    Get_S2 (12, Size,                                           "access_unit_length");
    BS_End();
    Skip_B2(                                                    "input_timing");

    //Little Endian management
    if (Save_Buffer)
    {
        swap(Buffer, Save_Buffer);
        swap(Buffer_Offset, Save_Buffer_Offset);
        swap(Buffer_Size, Save_Buffer_Size);
    }

    //Filling
    if (Size<2)
    {
        Synched=false;
        Size=2;
    }

    Size*=2;
    Header_Fill_Size(Size);
    Header_Fill_Code(1, "HD");
}

//---------------------------------------------------------------------------
void File_Ac3::Data_Parse()
{
    if (Element_Code != 2) // Not time stamp
        TimeStamp_Parsed=false; //Currently, only one kind of intermediate element is detected (no TimeStamp and HD part together), and we don't know the precise specification of MLP nor TimeStamp, so we consider next eleemnt is TimeStamp

    if (Save_Buffer)
    {
        File_Offset+=Buffer_Offset;
        swap(Buffer, Save_Buffer);
        swap(Buffer_Offset, Save_Buffer_Offset);
        swap(Buffer_Size, Save_Buffer_Size);
    }

    //Parsing
    switch (Element_Code)
    {
        case 0 :
                    Core();
                    break;
        case 1 :
                    Element_Info1C((FrameInfo.PTS!=(int64u)-1), __T("PTS ")+Ztring().Duration_From_Milliseconds(float64_int64s(((float64)FrameInfo.PTS)/1000000)));
                    Element_Info1(Frame_Count);
                    HD();
                    break;
        case 2 : TimeStamp();   break;
        default: ;
    }

    //Little Endian management
    if (Save_Buffer)
    {
        delete[] Buffer;
        Buffer=Save_Buffer; Save_Buffer=NULL;
        Buffer_Offset=Save_Buffer_Offset;
        Buffer_Size=Save_Buffer_Size;
        File_Offset-=Buffer_Offset;
    }
}

//---------------------------------------------------------------------------
void File_Ac3::Core()
{
    while (Element_Offset<Element_Size)
    {
        if (substreams_Count)
        {
            Element_Name("Block");
            Element_Begin1("syncframe");
        }
        Core_Frame();
        if (substreams_Count)
            Element_End0();
    }

    if (acmod_Max[0][0]==(int8u)-1)
        return; //Waiting for the first sync frame

    FILLING_BEGIN();
        if (bsid>0x10)
            return; //Not supported

        //Counting
        if (Frame_Count==0)
        {
            Core_IsPresent=true;
            PTS_Begin=FrameInfo.PTS;
        }
        if (bsid==0x09)
            Frequency_b=AC3_SamplingRate2[fscod]; // Unofficial hack for low sample rate (e.g. 22.05 kHz)
        else
        {
            if (fscod!=3)
                Frequency_b=AC3_SamplingRate[fscod];
            else
                Frequency_b=AC3_SamplingRate2[fscod2];
        }
        if (bsid>0x0A)
        {
            int64u numblks = numblkscod == 3 ? 6 : numblkscod + 1;
            TS_Add(numblks*256);
        }
        else
            TS_Add(1536);

        if (File_Offset+Buffer_Offset+Element_Size==File_Size)
            Frame_Count_Valid=Frame_Count; //Finish frames in case of there are less than Frame_Count_Valid frames

        //Filling
        if (!Status[IsAccepted])
            Accept("AC-3");
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
        {
            Fill("AC-3");

            //No more need data
            if (!IsSub && Config->ParseSpeed<1.0)
                Finish("AC-3");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ac3::Core_Frame()
{
    //Save true Element_Size (if Core+substreams, Element_Size is for all elements, and we want to limit to core)
    int64u Element_Size_Save=Element_Size;
    bsid=CC1(Buffer+Buffer_Offset+Element_Offset+5)>>3;
    if (bsid<=0x09)
    {
        int8u fscod     =(Buffer[(size_t)(Buffer_Offset+4)]&0xC0)>>6;
        int8u frmsizecod= Buffer[(size_t)(Buffer_Offset+4)]&0x3F;

        //Filling
        fscods[fscod]++;
        frmsizecods[frmsizecod]++;
        Element_Size=Element_Offset+AC3_FrameSize_Get(frmsizecod, fscod);
        if (Element_Size>Element_Size_Save)
            Element_Size=Element_Size_Save; // Not expected, but trying to parse the begining
    }
    else if (bsid>0x0A && bsid<=0x10) //E-AC-3 only
    {
        int16u frmsiz=CC2(Buffer+Buffer_Offset+(size_t)Element_Offset+2)&0x07FF;
        Element_Size=Element_Offset+2+frmsiz*2;
        if (Element_Size>Element_Size_Save)
            Element_Size=Element_Size_Save; // Not expected, but trying to parse the begining
    }
    if (Element_Offset==Element_Size)
        Element_Size=Element_Size_Save; // Something went wrong, using the whole packet

    //Pre-parsing, finding some elements presence
    int16u auxdatal;
    if (Buffer[Buffer_Offset+(Element_Size)-3]&0x02) //auxdatae
        auxdatal=(((int16u)Buffer[Buffer_Offset+(Element_Size)-4])<<6)
                |(         Buffer[Buffer_Offset+(Element_Size)-3] >>2);
    else
        auxdatal=(int16u)-1; //auxdata is empty
    BitStream_Fast Search(Buffer+Buffer_Offset+Element_Offset, Element_Size-Element_Offset);
    while(Search.Remain()>18)
    {
        if (Search.Peek2(16)==0x5838 && Ac3_EMDF_Test(Search))
            break;
        Search.Skip(1);
    }
    if (Search.Remain()>18)
        EMDF_RemainPos=Search.Remain();
    else
        EMDF_RemainPos=(size_t)-1;

    //Parsing
    int16u frmsiz=0, chanmap=0;
    int8u  dialnorm=(int8u)-1, dialnorm2=(int8u)-1, compr=(int8u)-1, compr2=(int8u)-1, dynrng=(int8u)-1, dynrng2=(int8u)-1, cmixlev=(int8u)-1, surmixlev=(int8u)-1, mixlevel=(int8u)-1, roomtyp=(int8u)-1, dmixmod=(int8u)-1, ltrtcmixlev=(int8u)-1, ltrtsurmixlev=(int8u)-1, lorocmixlev=(int8u)-1, lorosurmixlev=(int8u)-1;
    int8u  strmtyp=0, substreamid=0, acmod=0, bsmod=0, dsurmod=0;
    bool   compre=false, compr2e=false, dynrnge=false, dynrng2e=false;
    bool   lfeon=false, chanmape=false;
    bool   addbsie=false, adconvtyp=false;

    if (bsid<=0x09)
    {
        Element_Begin1("synchinfo");
            Skip_B2(                                                "syncword");
            Skip_B2(                                                "crc1");
            BS_Begin();
            Get_S1 (2, fscod,                                       "fscod - Sample Rate Code"); Param_Info2(AC3_SamplingRate[fscod], " Hz");
            Get_S1 (6, frmsizecod,                                  "frmsizecod - Frame Size Code"); Param_Info2C(frmsizecod/2<19,AC3_BitRate[frmsizecod/2]*1000, " bps");
        Element_End0();

        Element_Begin1("bsi");
            Get_S1 (5, bsid,                                        "bsid - Bit Stream Identification");
            Get_S1 (3, bsmod,                                       "bsmod - Bit Stream Mode"); Param_Info1(AC3_Mode[bsmod]);
            Get_S1 (3, acmod,                                       "acmod - Audio Coding Mode"); Param_Info1(AC3_ChannelPositions[acmod]);
            if ((acmod&1) && acmod!=1) { //central present
                Get_S1 (2, cmixlev,                                 "cmixlev - Center Mix Level"); Param_Info2(-3 - cmixlev * 1.5, " dB"); }
            if (acmod&4) { //back present
                Get_S1 (2, surmixlev,                               "surmixlev - Surround Mix Level"); Param_Info2(cmixlev == 2 ? "-inf" : to_string(-3 - cmixlev * 3), " dB"); }
            if (acmod==2) {
                Get_S1 (2, dsurmod,                                 "dsurmod - Dolby Surround Mode"); Param_Info1(AC3_Surround[dsurmod]); }
            Get_SB (   lfeon,                                       "lfeon - Low Frequency Effects");
            Get_S1 (5, dialnorm,                                    "dialnorm - Dialogue Normalization");
            Get_SB (   compre,                                      "compre - Compression Gain Word Exists");
            if (compre)
                Get_S1 (8, compr,                                   "compr - Compression Gain Word");
            TEST_SB_SKIP(                                           "langcode - Language Code Exists");
                Skip_S1(8,                                          "langcod - Language Code");
            TEST_SB_END();
            TEST_SB_SKIP(                                           "audprodie - Audio Production Information Exists");
                Get_S1 (5, mixlevel,                                "mixlevel - Mixing Level");
                Get_S1 (2, roomtyp,                                 "roomtyp - Room Type");
            TEST_SB_END();
            if (acmod==0) //1+1 mode
            {
                Get_S1 (5, dialnorm2,                               "dialnorm2 - Dialogue Normalization");
                Get_SB (   compr2e,                                 "compr2e - Compression Gain Word Exists");
                if (compr2e)
                    Get_S1 (8, compr2,                              "compr2 - Compression Gain Word");
                TEST_SB_SKIP(                                       "langcod2e - Language Code Exists");
                    Skip_S1(8,                                      "langcod2 - Language Code");
                TEST_SB_END();
                TEST_SB_SKIP(                                       "audprodi2e - Audio Production Information Exists");
                    Skip_S1(5,                                      "mixlevel2 - Mixing Level");
                    Skip_S1(2,                                      "roomtyp2 - Room Type");
                TEST_SB_END();
            }
            Skip_SB(                                                "copyrightb - Copyright Bit");
            Skip_SB(                                                "origbs - Original Bit Stream");
            if (bsid==0x06)
            {
                TEST_SB_SKIP(                                       "xbsi1e");
                    Get_S1 (2, dmixmod,                             "dmixmod");
                    Get_S1 (3, ltrtcmixlev,                         "ltrtcmixlev");
                    Get_S1 (3, ltrtsurmixlev,                       "ltrtsurmixlev");
                    Get_S1 (3, lorocmixlev,                         "lorocmixlev");
                    Get_S1 (3, lorosurmixlev,                       "lorosurmixlev");
                TEST_SB_END();
                TEST_SB_SKIP(                                       "xbsi2e");
                    Get_S1 (2, dsurexmod,                           "dsurexmod");
                    Get_S1 (2, dheadphonmod,                        "dheadphonmod");
                    Get_SB (   adconvtyp,                           "adconvtyp");
                    Skip_S1(8,                                      "xbsi2");
                    Skip_SB(                                        "encinfo");
                TEST_SB_END();
            }
            else
            {
                TEST_SB_SKIP(                                       "timecod1e");
                    Skip_S2(14,                                     "timecod1");
                TEST_SB_END();
                TEST_SB_SKIP(                                       "timecod2e");
                    Skip_S2(14,                                     "timecod2");
                TEST_SB_END();
            }
            TEST_SB_GET (addbsie,                                   "addbsie");
                int8u addbsil;
                Get_S1 (6, addbsil,                                 "addbsil");
                for (int8u Pos=0; Pos<=addbsil; Pos++) //addbsil+1 bytes
                    Skip_S1(8,                                      "addbsi");
            TEST_SB_END();
        Element_End0();
        Element_Begin1("audblk");
            for (int8u Pos=0; Pos<AC3_Channels[acmod]; Pos++)
                Skip_SB(                                            "blksw - Block Switch Flag");
            for (int8u Pos=0; Pos<AC3_Channels[acmod]; Pos++)
                Skip_SB(                                            "dithflag - Dither Flag");
            Get_SB (   dynrnge,                                     "dynrnge - Dynamic Range Gain Word Exists");
            if (dynrnge)
                Get_S1 (8, dynrng,                                  "dynrng - Dynamic Range Gain Word");
            if (acmod==0) //1+1 mode
            {
                Get_SB (   dynrng2e,                                "dynrng2e - Dynamic Range Gain Word Exists");
                if (dynrng2e)
                    Get_S1 (8, dynrng2,                             "dynrng2 - Dynamic Range Gain Word");
            }
        Element_End0();
    }
    else if (bsid>0x0A && bsid<=0x10)
    {
        /* Not finished, for reference only
        static const size_t MAX_AUD_BLK = 6;
        static const size_t MAX_CHANNELS = 5;
        struct Aud_Blk
        {
            bool  cplstre;
            bool  cplinu;
            bool  lfeexpstr;
            int8u blkmixcfginfo;
            int8u cplexpstr;

            int8u chexpstr[MAX_CHANNELS];
        };
        Aud_Blk aud_blks[MAX_AUD_BLK];

        struct Aud_Chan_Blk
        {
            bool  blksw;
            bool  chahtinu;
            bool  chincpl;
            bool  chinspx;
            bool  cplcoe;
            bool  dithflag;
            bool  ecplparam1e;
            bool  firstspxcos;
            bool  firstcplcos;
            bool  rsvdfieldse;
            bool  spxcoe;

            int8u chactivegaqbins; //todo
            int8u chbwcod;
            int8u convexpstr;
            int8u deltbae;
            int8u endmant;
            int8u frmchexpstr;
            int8u mstrspxco;
            int8u spxblnd;
            int8u strtmant;
        };
        Aud_Chan_Blk aud_chan_blk[MAX_CHANNELS];
        bool firstcplleak = false;
        //*/

        Element_Begin1("synchinfo");
            Skip_B2(                                               "syncword");
        Element_End0();
        Element_Begin1("bsi");
            BS_Begin();
            Get_S1 ( 2, strmtyp,                                    "strmtyp");
            Get_S1 ( 3, substreamid,                                "substreamid");
            Get_S2 (11, frmsiz,                                     "frmsiz");
            Get_S1 ( 2, fscod,                                      "fscod"); Param_Info2(AC3_SamplingRate[fscod], " Hz");
            if (fscod==3)
            {
                Get_S1 ( 2, fscod2,                                 "fscod2"); Param_Info2(AC3_SamplingRate2[fscod2], " Hz");
                numblkscod=3;
            }
            else
                Get_S1 ( 2, numblkscod,                             "numblkscod");
            Get_S1 (3, acmod,                                       "acmod - Audio Coding Mode"); Param_Info1(AC3_ChannelPositions[acmod]);
            Get_SB (   lfeon,                                       "lfeon - Low Frequency Effects");
            Get_S1 (5, bsid,                                        "bsid - Bit Stream Identification");
            Get_S1 (5, dialnorm,                                    "dialnorm");
            TEST_SB_GET(compre,                                     "compre");
                Get_S1 (8, compr,                                   "compr");
            TEST_SB_END();
            if (acmod==0) //1+1 mode
            {
                Get_S1 (5, dialnorm2,                               "dialnorm2");
                TEST_SB_GET(compr2e,                                "compr2e");
                    Get_S1 (8, compr2,                              "compr2");
                TEST_SB_END();
            }
            if (strmtyp==1) //dependent stream
            {
                TEST_SB_GET (chanmape,                              "chanmape");
                    Get_S2(16, chanmap,                             "chanmap"); Param_Info1(AC3_chanmap_ChannelPositions(chanmap));
                TEST_SB_END();
            }
            TEST_SB_SKIP(                                           "mixmdate");
                int8u mixdef;
                if(acmod > 0x2)
                    Get_S1 (2, dmixmod,                             "dmixmod");
                if((acmod&0x1) && (acmod>0x2))
                {
                    Get_S1 (3, ltrtcmixlev,                         "ltrtcmixlev");
                    Get_S1 (3, lorocmixlev,                         "lorocmixlev");
                }
                if(acmod>0x4)
                {
                    Get_S1 (3, ltrtsurmixlev,                       "ltrtsurmixlev");
                    Get_S1 (3, lorosurmixlev,                       "lorosurmixlev");
                }
                if(lfeon)
                {
                    TEST_SB_SKIP(                                   "lfemixlevcode");
                        Skip_S1 (5,                                 "lfemixlevcod");
                    TEST_SB_END();
                }
                if(strmtyp == 0x0)
                {
                    TEST_SB_SKIP(                                   "pgmscle");
                        Skip_S1 (6,                                 "pgmscl");
                    TEST_SB_END();
                    if (acmod == 0x0)
                    {
                        TEST_SB_SKIP(                               "pgmscle12e");
                            Skip_S1 (6,                             "pgmscl12e");
                        TEST_SB_END();
                    }
                    TEST_SB_SKIP(                                   "extpgmscle");
                        Skip_S1 (6,                                 "extpgmscl");
                    TEST_SB_END();
                    Get_S1 (2, mixdef,                              "mixdef");
                    if(mixdef == 0x1)
                    {
                        Skip_S1 (1,                                 "premixcmpsel");
                        Skip_S1 (1,                                 "drcsrc");
                        Skip_S1 (3,                                 "premixcmpscl");
                    }
                    else if(mixdef == 0x2) Skip_S2 (12,             "mixdata");
                    else if(mixdef == 0x3)
                    {
                        int8u mixdeflen;
                        Get_S1 (5, mixdeflen,                       "mixdeflen");
                        TEST_SB_SKIP(                               "mixdata2e");
                            Skip_S1 (6,                             "premixcmpsel");
                            Skip_S1 (1,                             "drcsrc");
                            Skip_S1 (3,                             "premixcmpscl");
                            TEST_SB_SKIP(                           "extpgmlscle");
                                Skip_S1 (4,                         "extpgmlscl");
                            TEST_SB_END();
                            TEST_SB_SKIP(                           "extpgmcscle");
                                Skip_S1 (4,                         "extpgmcscl");
                            TEST_SB_END();
                            TEST_SB_SKIP(                           "extpgmrscle");
                                Skip_S1 (4,                         "extpgmrscl");
                            TEST_SB_END();
                            TEST_SB_SKIP(                           "extpgmlssle");
                                Skip_S1 (4,                         "extpgmlssl");
                            TEST_SB_END();
                            TEST_SB_SKIP(                           "extpgmrssle");
                                Skip_S1 (4,                         "extpgmrssl");
                            TEST_SB_END();
                            TEST_SB_SKIP(                           "extpgmlfescle");
                                Skip_S1 (4,                         "extpgmlfescl");
                            TEST_SB_END();
                            TEST_SB_SKIP(                           "dmixscle");
                                Skip_S1 (4,                         "dmixscl");
                            TEST_SB_END();
                            TEST_SB_SKIP(                           "addche");
                                TEST_SB_SKIP(                       "extpgmaux1scle");
                                    Skip_S1 (4,                     "extpgmaux1scl");
                                TEST_SB_END();
                                TEST_SB_SKIP(                       "extpgmaux2scle");
                                    Skip_S1 (4,                     "extpgmaux2scl");
                                TEST_SB_END();
                            TEST_SB_END();
                        TEST_SB_END();
                        TEST_SB_SKIP(                               "mixdata3e");
                            Skip_S1 (5,                             "spchdat");
                            TEST_SB_SKIP(                           "addspchdate");
                                Skip_S1 (5,                         "spchdat1");
                                Skip_S1 (2,                         "spchan1att");
                                TEST_SB_SKIP(                       "addspdat1e");
                                    Skip_S1 (5,                     "spchdat2");
                                    Skip_S1 (2,                     "spchan2att");
                                TEST_SB_END();
                            TEST_SB_END();
                        TEST_SB_END();
                        Skip_S2 (8*(mixdeflen+2),                   "mixdata");
                        //Skip_S1 (,                                 "mixdatafill");
                    }
                    if(acmod<0x2)
                    {
                        TEST_SB_SKIP(                               "paninfoe");
                            Skip_S1 (6,                             "panmean");
                            Skip_S1 (8,                             "paninfo");
                        TEST_SB_END();
                        if(acmod==0x0)
                        {
                            TEST_SB_SKIP(                           "paninfo2e");
                               Skip_S1 (6,                          "panmean2");
                               Skip_S1 (8,                          "paninfo2");
                            TEST_SB_END();
                        }
                    }
                    TEST_SB_SKIP(                                   "frmmixcfginfoe");
                        if(numblkscod==0x0)
                        {
                            int8u blkmixcfginfo;
                            Get_S1 (5, blkmixcfginfo,               "blkmixcfginfo[0]");
                            //aud_blks[0].blkmixcfginfo = blkmixcfginfo;
                        }
                        else
                        {
                            int8u nb_blocks_per_syncframe = numblkscod == 3 ? 6 : (numblkscod + 1);
                            for (int8u blk = 0; blk < nb_blocks_per_syncframe; ++blk)
                            {
                                TEST_SB_SKIP(                       "blkmixcfginfoe");
                                    int8u blkmixcfginfo;
                                    Get_S1 (5, blkmixcfginfo,       "blkmixcfginfo[x]");
                                    //aud_blks[blk].blkmixcfginfo = blkmixcfginfo;
                                TEST_SB_END();
                            }
                        }
                    TEST_SB_END();
                }
            TEST_SB_END();

            TEST_SB_SKIP(                                           "infomdate");
                Get_S1 (3, bsmod,                                   "bsmod - Bit Stream Mode"); Param_Info1(AC3_Mode[bsmod]);
                Skip_SB(                                            "copyrightb - Copyright Bit");
                Skip_SB(                                            "origbs - Original Bit Stream");
                if (acmod==0x2)
                {
                    Get_S1 (2, dsurmod,                             "dsurmod");
                    Get_S1 (2, dheadphonmod,                        "dheadphonmod");
                }
                if (acmod>=0x6)
                    Get_S1 (2, dsurexmod,                           "dsurexmod");
                TEST_SB_SKIP(                                       "audprodie");
                    Get_S1 (5, mixlevel,                            "mixlevel");
                    Get_S1 (2, roomtyp,                             "roomtyp");
                    Skip_S1(1,                                      "adconvtyp");
                TEST_SB_END();
                if (acmod==0x0)
                {
                    TEST_SB_SKIP(                                   "audprodi2e");
                        Skip_S1(5,                                  "mixlevel2");
                        Skip_S1(2,                                  "roomtyp2");
                        Skip_S1(1,                                  "adconvtyp2");
                    TEST_SB_END();
                }
                if (fscod < 0x3)
                    Skip_S1(1,                                      "sourcefscod");
            TEST_SB_END();

            if (strmtyp==0x0 && numblkscod!=0x3)
                Skip_S1(1,                                          "convsync");

            if (strmtyp == 0x2)
            {
                int8u blkid = 0;
                if (numblkscod==0x3)
                    blkid = 1;
                else
                    Get_S1(1, blkid,                                "blkid");
                if (blkid)
                    Get_S1(6, frmsizecod,                           "frmsizecod");
            }

            TEST_SB_GET (addbsie,                                   "addbsie");
                int8u addbsil;
                Get_S1 (6, addbsil,                                 "addbsil");
                if (addbsil+1!=addbsi_Buffer_Size)
                {
                    delete[] addbsi_Buffer;
                    addbsi_Buffer_Size=addbsil+1;
                    addbsi_Buffer=new int8u[addbsi_Buffer_Size];
                }
                for (int8u Pos=0; Pos<=addbsil; Pos++) //addbsil+1 bytes
                    Get_S1 (8, addbsi_Buffer[Pos],                  "addbsi");
            TEST_SB_END();
        Element_End0();

        /* Not finished, for reference only
        int8u numblks = numblkscod == 3 ? 6 : (numblkscod + 1);

        Element_Begin1("audfrm");
        int8u snroffststr, ncplblks = 0;
        bool expstre = true, ahte = false;
        bool transproce, blkswe, dithflage, bamode, frmfgaincode, dbaflde, skipflde, spxattene;
        if (numblkscod==0x3)
        {
            Get_SB(expstre,                                         "expstre");
            Get_SB(ahte,                                            "ahte");
        }
        Get_S1 (2, snroffststr,                                     "snroffststr");
        Get_SB (transproce,                                         "transproce");
        Get_SB (blkswe,                                             "blkswe");
        Get_SB (dithflage,                                          "dithflage");
        Get_SB (bamode,                                             "bamode");
        Get_SB (frmfgaincode,                                       "frmfgaincode");
        Get_SB (dbaflde,                                            "dbaflde");
        Get_SB (skipflde,                                           "skipflde");
        Get_SB (spxattene,                                          "spxattene");

        if (acmod>0x1)
        {
            aud_blks[0].cplstre = 1;
            aud_blks[0].cplinu = 0;
            for (int8u blk = 1; blk < numblks; ++blk)
            {
                Get_SB (aud_blks[blk].cplstre,                      "cplstre[x]");
                if (aud_blks[blk].cplstre==1)
                    Get_SB (aud_blks[blk].cplinu,                   "cplinu[x]");
                else
                    aud_blks[blk].cplinu = aud_blks[blk - 1].cplinu;
            }
        }
        else
        {
            for(int8u blk = 0; blk < numblks; ++blk)
                aud_blks[blk].cplinu = 0;
        }

        if (expstre)
        {
            for(int8u blk = 0; blk < numblks; ++blk)
            {
                if (aud_blks[blk].cplinu==1)
                    Get_S1 (2, aud_blks[blk].cplexpstr,             "cplexpstr[x]");
                for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                    Get_S1(2, aud_blks[blk].chexpstr[Pos],          "chexpstr[blk][ch]");
            }
        }
        else
        {
            int8u frmcplexpstr = 0;
            ncplblks = 0;
            for (int8u blk = 0; blk < numblks; ++blk)
                ncplblks += aud_blks[blk].cplinu;
            if (acmod > 0x1 && ncplblks > 0)
                Get_S1(5, frmcplexpstr,                             "frmcplexpstr");
            for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                Get_S1(5, aud_chan_blk[Pos].frmchexpstr,            "frmchexpstr[ch]");
        }
        if (lfeon)
            for (int8u blk = 0; blk < numblks; ++blk)
                Get_SB(aud_blks[blk].lfeexpstr,                     "lfeexpstr[blk]");

        if (strmtyp == 0x0)
        {
            bool convexpstre = true;
            if (numblkscod!=0x3)
                Get_SB (convexpstre,                                "convexpstre");
            if (convexpstre)
                for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                    Get_S1(5, aud_chan_blk[Pos].convexpstr,         "convexpstr[ch]");
        }

        int8u cplahtinu = 0;
        int8u lfeahtinu = 0;
        int8u *hebap = NULL; //TODO in ahte
        if (ahte)
        {
            ncplblks = 0;

            int8u ncplregs = 0;
            for (int8u blk = 0; blk < numblks; ++blk)
            {
                //reuse corresponds to 0
                if (aud_blks[blk].cplstre == 1 || aud_blks[blk].cplexpstr != 0)
                    ++ncplregs;
            }

            if (ncplblks==6 && ncplregs==1)
            {
                bool tmp;
                Get_SB (tmp,                                        "cplahtinu");
                if (tmp)
                    cplahtinu = 1;
            }

            for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
            {
                int8u nchregs = 0;
                for (int8u blk = 0; blk < numblks; ++blk)
                {
                    if (aud_blks[blk].chexpstr[Pos]!=0)
                        ++nchregs;
                }

                aud_chan_blk[Pos].chahtinu = 0;
                if (nchregs==1)
                {
                    bool tmp;
                    Get_SB (tmp,                                    "chahtinu[ch]");
                    if (tmp)
                        aud_chan_blk[Pos].chahtinu = 1;
                }
            }

            if (lfeon)
            {
                int8u nlferegs = 0;
                for (int8u blk = 0; blk < numblks; ++blk)
                {
                    if (aud_blks[blk].lfeexpstr!=false)
                        ++nlferegs;
                }

                if (nlferegs==1)
                {
                    bool tmp = false;
                    Get_SB (tmp,                                    "lfeahtinu");
                    if (tmp)
                        lfeahtinu = 1;
                }
            }
        }

        if (snroffststr)
        {
            Skip_S1 (6,                                             "frmcsnroffst");
            Skip_S1 (4,                                             "frmfsnroffst");
        }

        if (transproce)
        {
            for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
            {
                TEST_SB_SKIP(                                       "chintransproc[ch]");
                    Skip_S2(10,                                     "transprocloc[ch]");
                    Skip_S1(8,                                      "transproclen[ch]");
                TEST_SB_END();
            }
        }

        if (spxattene)
        {
            for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
            {
                TEST_SB_SKIP(                                       "chinspxatten[ch]");
                    Skip_S1(5,                                      "spxattencod[ch]");
                TEST_SB_END();
            }
        }

        bool blkstrtinfoe = false;
        int8u blkstrtinfo = 0;
        if (numblkscod != 0x0)
            Get_SB (blkstrtinfoe,                                   "blkstrtinfoe");
        if (blkstrtinfoe)
            blkstrtinfo = (numblks - 1) * (4 + ceil(log2(frmsiz + 1)));

        // These fields for syntax state initialization
        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
        {
            aud_chan_blk[Pos].firstspxcos = true;
            aud_chan_blk[Pos].firstcplcos = true;
        }
        firstcplleak = true;
        Element_End0();

        Element_Begin1("audblks");
            for (int8u blk = 0; blk < numblks; ++blk)
            {
                Element_Begin1("audblk");
                    if (blkswe)
                        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            Get_SB (aud_chan_blk[Pos].blksw,        "blksw[ch]");
                    else
                        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            aud_chan_blk[Pos].blksw = false;

                    if (dithflage)
                        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            Get_SB (aud_chan_blk[Pos].dithflag,     "dithflag[ch]");
                    else
                        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            aud_chan_blk[Pos].dithflag = true;

                    int8u dynrng = 0;
                    int8u dynrng2 = 0;
                    TEST_SB_SKIP(                                   "dynrnge");
                        Get_S1 (8, dynrng,                          "dynrng");
                    TEST_SB_END();

                    if (acmod==0x0)
                    {
                        TEST_SB_SKIP(                               "dynrng2e");
                            Get_S1 (8, dynrng,                      "dynrng2");
                        TEST_SB_END();
                    }

                    int8u spx_begin_subbnd, spx_end_subbnd;
                    int8u spxbegf = 0;
                    bool spxbndstrc[256];
                    bool spxstre = true;
                    bool spxinu = false;
                    if (blk!=0)
                        Peek_SB(spxstre);

                    if (spxstre)
                    {
                        Element_Begin1("spxstr");
                        Skip_SB(                                    "spxstre");
                        Get_SB (spxinu,                             "spxinu");
                        if (spxinu)
                        {
                            if (acmod==0x1)
                                aud_chan_blk[0].chinspx = true;
                            else
                            {
                                for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                                    Get_SB (aud_chan_blk[Pos].chinspx,"chinspx[ch]");
                            }

                            int8u spxstrtf, spxendf;

                            Get_S1 (2, spxstrtf,                    "spxstrtf");
                            Get_S1 (3, spxbegf,                     "spxbegf");
                            Get_S1 (3, spxendf,                     "spxendf");

                            if (spxbegf<6)
                                spx_begin_subbnd = spxbegf + 2;
                            else
                                spx_begin_subbnd = spxbegf * 2 - 3;

                            if (spxendf < 3)
                                spx_end_subbnd = spxendf + 5;
                            else
                                spx_end_subbnd = spxendf * 2 + 3;

                            TEST_SB_SKIP(                           "spxbndstrce");
                            for (int8u bnd = spx_begin_subbnd+1; bnd < spx_end_subbnd; ++bnd)
                                Get_SB (spxbndstrc[bnd],            "spxbndstrc[bnd]");
                            TEST_SB_END();
                        }
                        else
                        {
                            for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            {
                                aud_chan_blk[Pos].chinspx = false;
                                aud_chan_blk[Pos].firstspxcos = true;
                            }
                        }
                        Element_End0();
                    }

                    if (spxinu)
                    {
                        Element_Begin1("spxin"); 
                        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                        {
                            Element_Begin1("Channel"); 
                            if (aud_chan_blk[Pos].chinspx)
                            {
                                if (aud_chan_blk[Pos].firstspxcos)
                                {
                                    aud_chan_blk[Pos].spxcoe = true;
                                    aud_chan_blk[Pos].firstspxcos = false;
                                }
                                else
                                    Get_SB (aud_chan_blk[Pos].spxcoe,   "spxcoe[ch]");

                                if (aud_chan_blk[Pos].spxcoe)
                                {
                                    Get_S1 (5, aud_chan_blk[Pos].spxblnd,"spxblnd[ch]");
                                    Get_S1 (2, aud_chan_blk[Pos].mstrspxco,"mstrspxco[ch]");

                                    int8u nspxbnds = 1;
                                    int8u spxbndsztab[256] = {0};
                                    spxbndsztab[0] = 12;
                                    for (int8u bnd = spx_begin_subbnd+1; bnd < spx_end_subbnd; ++bnd)
                                    {
                                        if (spxbndstrc[bnd] == false)
                                        {
                                            spxbndsztab[nspxbnds] = 12;
                                            ++nspxbnds;
                                        }
                                        else
                                            spxbndsztab[nspxbnds - 1] += 12;
                                    }

                                    for (int8u bnd = 0; bnd < nspxbnds; ++bnd)
                                    {
                                        Element_Begin1("Bnd"); 
                                        Skip_S1 (4,                     "spxcoexp[ch][bnd]");
                                        Skip_S1 (2,                     "spxcomant[ch][bnd]");
                                        Element_End0();
                                    }
                                }
                            }
                            else
                                aud_chan_blk[Pos].firstspxcos = true;
                            Element_End0();
                        }
                        Element_End0();
                    }

                    bool  ecplinu = false;
                    bool  phsflginu = false;
                    bool  cplbndstrce = false;
                    int8u ncplsubnd = 0;
                    bool *cplbndstrc = NULL;
                    int16u necplbnd = 0;
                    int8u ecplbegf, ecplendf = 0;
                    int8u cplbegf, cplendf = 0;
                    int16u ecpl_begin_subbnd = 0, ecpl_end_subbnd = 0;
                    size_t cplstrtmant = 0;
                    size_t lfeactivegaqbins = 0;
                    size_t cplendmant = 0;

                    if (aud_blks[blk].cplstre)
                    {
                        Element_Begin1("cplstr");
                        if (aud_blks[blk].cplinu)
                        {
                            Element_Begin1("cplin"); 
                            Get_SB (ecplinu,                            "ecplinu");

                            if (acmod==0x2)
                            {
                                aud_chan_blk[0].chincpl = true;
                                aud_chan_blk[1].chincpl = true;
                            }
                            else
                            {
                                for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                                    Get_SB (aud_chan_blk[1].chincpl,    "chincpl[ch]");
                            }

                            if (!ecplinu)
                            {
                                if (acmod==0x2)
                                    Get_SB (phsflginu,                  "phsflginu");
                                Get_S1 (4, cplbegf,                     "cplbegf");
                                cplstrtmant = (cplbegf * 12) + 37;;

                                if (!spxinu)
                                    Get_S1 (4, cplendf,                 "cplendf");
                                else
                                {
                                    if (spxbegf < 6)
                                        cplendf = spxbegf - 2;
                                    else
                                        cplendf = (spxbegf * 2) - 7;
                                }

                                ncplsubnd = 3 + cplendf - cplbegf;
                                Get_SB (cplbndstrce,                    "cplbndstrce");
                                if (cplbndstrce)
                                {
                                    cplbndstrc = new bool [ncplsubnd];
                                    cplbndstrc[0] = false;
                                    for (int8u bnd = 1; bnd < ncplsubnd; ++bnd)
                                        Get_SB (cplbndstrc[bnd],        "cplbndstrc[bnd]");
                                }
                            }
                            else
                            {
                                Get_S1 (4, ecplbegf,                    "ecplbegf");
                                cplstrtmant = (cplbegf * 12) + 37;;

                                if (ecplbegf<3)
                                    ecpl_begin_subbnd = ecplbegf * 2;
                                else if (ecplbegf<13)
                                    ecpl_begin_subbnd = ecplbegf + 2;
                                else
                                    ecpl_begin_subbnd = ecplbegf * 2 - 10;

                                if (!spxinu)
                                {
                                    Get_S1 (4, ecplendf,                "ecplendf");
                                    ecpl_end_subbnd = ecplendf + 7;
                                }
                                else
                                {
                                    if (spxbegf < 6)
                                        ecpl_end_subbnd = spxbegf + 5;
                                    else
                                        ecpl_end_subbnd = spxbegf * 2;
                                }

                                necplbnd = ecpl_end_subbnd - ecpl_begin_subbnd;
                                TEST_SB_SKIP(                           "ecplbndstrce");
                                int16u max = 9;
                                if (max < ecpl_begin_subbnd+1)
                                    max = ecpl_begin_subbnd+1;
                                ncplsubnd = ecpl_end_subbnd;

                                cplbndstrc = new bool [ecpl_end_subbnd];
                                for (size_t i = 0; i < max; ++i)
                                    cplbndstrc[i] = false;

                                for (int16u sbnd = max; sbnd < ecpl_end_subbnd; ++sbnd)
                                    Get_SB (cplbndstrc[sbnd],           "cplbndstrc[bnd]");

                                necplbnd = 0;
                                for (int16u i = ecpl_begin_subbnd; i < ecpl_end_subbnd; ++i)
                                    necplbnd += cplbndstrc[i];
                                TEST_SB_END();
                            }
                            Element_End0();
                        }
                        else
                        {
                            for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            {
                                aud_chan_blk[Pos].chincpl = false;
                                aud_chan_blk[Pos].firstcplcos = true;
                            }
                            firstcplleak = true;
                        }
                        Element_End0();
                    }

                    if (aud_blks[blk].cplinu)
                    {
                        if (!ecplinu)
                        {
                            int16u ncplbnd = ncplsubnd;
                            for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            {
                                if (aud_chan_blk[Pos].chincpl)
                                {
                                    if (aud_chan_blk[Pos].firstcplcos)
                                    {
                                        aud_chan_blk[Pos].cplcoe = true;
                                        aud_chan_blk[Pos].firstcplcos = false;
                                    }
                                    else
                                        Get_SB(aud_chan_blk[Pos].cplcoe,"cplcoe[ch]");

                                    if (aud_chan_blk[Pos].cplcoe)
                                    {
                                        Skip_S1 (2,                     "mstrcplco[ch]");
                                        ncplbnd = ncplsubnd;
                                        for (size_t i = 1; i < ncplsubnd; ++i)
                                            ncplbnd += cplbndstrc[i];
                                        for (int16u bnd = 0; bnd < ncplbnd; bnd++)
                                        {
                                            Skip_S1(4,                  "cplcoexp[ch][bnd]");
                                            Skip_S1(4,                  "cplcomant[ch][bnd]");
                                        }
                                    }
                                }
                                else
                                {
                                    aud_chan_blk[Pos].firstcplcos = true;
                                }
                            }

                            if (acmod==0x2 && phsflginu && (aud_chan_blk[0].cplcoe || aud_chan_blk[1].cplcoe))
                            {
                                for (size_t bnd = 0; bnd < ncplbnd; ++bnd)
                                    Skip_SB(                            "phsflg[bnd]}");
                            }
                        }
                        else
                        {
                            int firstchincpl = -1;
                            Skip_SB(                                    "reserved");
                            for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            {
                                if (aud_chan_blk[Pos].chincpl)
                                {
                                    if (firstchincpl == -1)
                                        firstchincpl = Pos;

                                    if (aud_chan_blk[Pos].firstcplcos)
                                    {
                                        aud_chan_blk[Pos].ecplparam1e = true;
                                        if (Pos > firstchincpl)
                                            aud_chan_blk[Pos].rsvdfieldse = true;
                                        else
                                            aud_chan_blk[Pos].rsvdfieldse = false;
                                        aud_chan_blk[Pos].firstcplcos = false;
                                    }
                                    else
                                    {
                                        Get_SB (aud_chan_blk[Pos].ecplparam1e, "ecplparam1e");
                                        if (Pos > firstchincpl)
                                            Get_SB (aud_chan_blk[Pos].rsvdfieldse, "rsvdfieldse");
                                        else
                                            aud_chan_blk[Pos].rsvdfieldse = false;
                                    }

                                    if (aud_chan_blk[Pos].ecplparam1e)
                                    {
                                        if (aud_chan_blk[Pos].ecplparam1e)
                                        {
                                            for (size_t bnd = 0; bnd < necplbnd; bnd++)
                                                Skip_S1(5,                  "ecplamp[ch][bnd]");
                                        }

                                        if (aud_chan_blk[Pos].rsvdfieldse)
                                            Skip_BS(9 * (necplbnd - 1),     "reserved");
                                        if (Pos > firstchincpl)
                                            Skip_SB(                        "reserved");
                                    }
                                }
                                else
                                    aud_chan_blk[Pos].firstcplcos = true;
                            }
                        }
                    }

                    //aud_chan_blk_init(acmod, cplbegf);
                    for (int8u Pos = 0; Pos<AC3_Channels[acmod]; ++Pos)
                    {
                        aud_chan_blk[Pos].strtmant = 0;
                        if (aud_chan_blk[Pos].chincpl)
                            aud_chan_blk[Pos].endmant = 37 + (12 * cplbegf);
                        else
                            aud_chan_blk[Pos].endmant = 37 + (3 * (aud_chan_blk[Pos].chbwcod + 12));
                    }

                    if (acmod==0x2)
                    {
                        bool rematstr = true;
                        if (blk!=0)
                            Get_SB(rematstr,                                "rematstr");

                        if (rematstr)
                        {
                            size_t nrematbd = 0;
                            if (aud_blks[blk].cplinu)
                            {
                                if (ecplinu)
                                {
                                    if (ecplbegf == 0)
                                        nrematbd = 0;
                                    else if (ecplbegf == 1)
                                        nrematbd = 1;
                                    else if (ecplbegf == 2)
                                        nrematbd = 2;
                                    else if (ecplbegf < 5)
                                        nrematbd = 3;
                                    else
                                        nrematbd = 4;
                                }
                                else
                                {
                                    if (cplbegf == 0)
                                        nrematbd = 2;
                                    else if (cplbegf < 3)
                                        nrematbd = 3;
                                    else
                                        nrematbd = 4;
                                }
                            }
                            else if (spxinu)
                            {
                                if (spxbegf < 2)
                                    nrematbd = 3;
                                else
                                    nrematbd = 4;
                            }
                            else
                            {
                                nrematbd = 4;
                            }

                            for (size_t bnd = 0; bnd < nrematbd; ++bnd)
                                Skip_SB(                                    "rematflg[bnd]");
                        }
                    }

                    for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                    {
                        if (aud_blks[blk].chexpstr[Pos])
                        {
                            if (!aud_chan_blk[Pos].chincpl && !aud_chan_blk[Pos].chinspx)
                                Get_S1 (6, aud_chan_blk[Pos].chbwcod,       "chbwcod[ch]");
                        }
                    }

                    if (aud_blks[blk].cplinu)
                    {
                        if (aud_blks[blk].cplexpstr!=0)
                        {
                            Skip_S1(4,                                      "cplabsexp");

                            size_t ncplgrps = 0;
                            if (ecplinu)
                            {
                                int8u ecplstartmant = ecplsubbndtab[ecpl_begin_subbnd];
                                int8u ecplendmant = ecplsubbndtab[ecpl_end_subbnd];
                                if (aud_blks[blk].cplexpstr == 0x01)
                                    ncplgrps = (ecplendmant - ecplstartmant) / 3;
                                else if (aud_blks[blk].cplexpstr == 0x02)
                                    ncplgrps = (ecplendmant - ecplstartmant) / 6;
                                else if (aud_blks[blk].cplexpstr == 0x03)
                                    ncplgrps = (ecplendmant - ecplstartmant) / 12;
                            }
                            else
                            {
                                cplendmant = ((cplendf + 3) * 12) + 37;
                                if (aud_blks[blk].cplexpstr == 0x01)
                                    ncplgrps = (cplendmant - cplstrtmant) / 3;
                                else if (aud_blks[blk].cplexpstr == 0x02)
                                    ncplgrps = (cplendmant - cplstrtmant) / 6;
                                else if (aud_blks[blk].cplexpstr == 0x03)
                                    ncplgrps = (cplendmant - cplstrtmant) / 12;
                            }

                            for(size_t grp = 0; grp < ncplgrps; grp++)
                                Skip_S1(7,                                  "cplexps[grp]");
                        }
                    }

                    for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                    {
                        if (aud_blks[blk].chexpstr[Pos]!=0)
                        {
                            Skip_S1(4,                                      "exps[ch][0]");
                            size_t nchgrps = 0;
                            if (!aud_chan_blk[Pos].chincpl)
                                aud_chan_blk[Pos].endmant = ((aud_chan_blk[Pos].chbwcod + 12) * 3) + 37;
                            else
                                aud_chan_blk[Pos].endmant = cplstrtmant;

                            if (aud_blks[blk].cplexpstr==0x01)
                                nchgrps = (aud_chan_blk[Pos].endmant - 1) / 3;
                            else if (aud_blks[blk].cplexpstr==0x02)
                                nchgrps = (aud_chan_blk[Pos].endmant - 1) / 3;
                            else if (aud_blks[blk].cplexpstr==0x03)
                                nchgrps = (aud_chan_blk[Pos].endmant - 1) / 3;

                            for (size_t grp = 1; grp <= nchgrps; grp++)
                                Skip_S1(7,                                  "exps[ch][grp]");
                            Skip_S1(2,                                      "gainrng[ch]");
                        }
                    }

                    if (lfeon)
                    {
                        if (aud_blks[blk].lfeexpstr!=0)
                        {
                            Skip_S1(4,                                      "lfeexps[0]");
                            size_t nlfegrps = 2;
                            for (size_t grp = 1; grp <= nlfegrps; ++grp)
                                Skip_S1(7,                                  "lfeexps[grp]");
                        }
                    }

                    int8u sdcycod = 0x2;
                    int8u fdcycod = 0x1;
                    int8u sgaincod = 0x1;
                    int8u dbpbcod = 0x2;
                    int8u floorcod = 0x7;
                    if (bamode)
                    {
                        bool baie = false;
                        Get_SB(baie,                                        "baie");
                        if (baie)
                        {
                            Get_S1(2, sdcycod,                              "sdcycod");
                            Get_S1(2, fdcycod,                              "fdcycod");
                            Get_S1(2, sgaincod,                             "sgaincod");
                            Get_S1(2, dbpbcod,                              "dbpbcod");
                            Get_S1(3, floorcod,                             "floorcod");
                        }
                    }

                    if (snroffststr!=0x0)
                    {
                        bool snroffste = true;
                        if (blk!=0)
                            Get_SB (snroffste,                              "snroffste");

                        if (snroffste)
                        {
                            Skip_S1(6,                                      "csnroffst");
                            if (snroffststr==0x1)
                                Skip_S1(4,                                  "blkfsnroffst");
                            else if (snroffststr == 0x2)
                            {
                                if (aud_blks[blk].cplinu)
                                    Skip_S1(4,                              "cplfsnroffst");
                                for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                                    Skip_S1(4,                              "fsnroffst[ch]");
                                if (lfeon)
                                    Skip_S1(4,                              "lfefsnroffst");
                            }
                        }
                    }

                    bool fgaincode = false;
                    if (frmfgaincode)
                        Get_SB(fgaincode,                                   "fgaincode");

                    if (fgaincode)
                    {
                        if (aud_blks[blk].cplinu)
                            Skip_S1(3, "cplfgaincod");
                        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            Skip_S1(3,                                      "fgaincod[ch]");
                        if (lfeon)
                            Skip_S1(3,                                      "lfefgaincod");
                    }

                    if (strmtyp == 0x0)
                    {
                        TEST_SB_SKIP(                                       "convsnroffste");
                        Skip_S2(10,                                         "convsnroffst");
                        TEST_SB_END();
                    }

                    if (aud_blks[blk].cplinu)
                    {
                        bool cplleake = true;
                        if (firstcplleak)
                            firstcplleak = false;
                        else
                            Get_SB (cplleake,                               "cplleake");

                        if (cplleake)
                        {
                            Skip_S1(3,                                      "cplfleak");
                            Skip_S1(3,                                      "cplsleak");
                        }
                    }

                    if (dbaflde)
                    {
                        TEST_SB_SKIP(                                       "deltbaie");
                        int8u cpldeltbae = 0;
                        if (aud_blks[blk].cplinu)
                            Get_S1 (2, cpldeltbae,                          "cpldeltbae");
                        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                            Get_S1 (2, aud_chan_blk[Pos].deltbae,        "deltbae[ch]");

                        if (aud_blks[blk].cplinu)
                        {
                            if (cpldeltbae==0x01)
                            {
                                int8u cpldeltnseg;
                                Get_S1 (3, cpldeltnseg,                 "cpldeltnseg");
                                for (int8u seg = 0; seg <= cpldeltnseg; ++seg)
                                {
                                    Skip_S1(5,                          "cpldeltoffst[seg]");
                                    Skip_S1(4,                          "cpldeltlen[seg]");
                                    Skip_S1(3,                          "cpldeltba[seg]");
                                }
                            }
                        }

                        for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                        {
                            if (aud_chan_blk[Pos].deltbae==0x01)
                            {
                                int8u deltnseg;
                                Get_S1 (3, deltnseg,                    "deltnseg[ch]");
                                for (int8u seg = 0; seg <= deltnseg; ++seg)
                                {
                                    Skip_S1(5,                          "deltoffst[ch][seg]");
                                    Skip_S1(4,                          "deltlen[ch][seg]");
                                    Skip_S1(3,                          "deltba[ch][seg]");
                                }
                            }
                        }
                        TEST_SB_END();
                    }

                    if (skipflde)
                    {
                        TEST_SB_SKIP(                                   "skiple");
                        int16u skipl;
                        Get_S2 (9, skipl,                               "skipl");
                        Skip_BS(skipl * 8,                              "skipfld");
                        //TODO: EMDF();
                        TEST_SB_END();
                    }

                    bool   got_cplchan = false;
                    size_t cplactivegaqbins = 0;
                    int8u  lfegaqmod = 0;
                    int8u* lfegaqbin = NULL;
                    size_t lfeendmant = 7;
                    int8u  cplgaqmod = 0;
                    lfegaqbin = new int8u[lfeendmant];

                    int8u *cplgaqbin = NULL;
                    cplgaqbin = new int8u[cplendmant];
                    for (size_t bin = 0; bin < lfeendmant; ++bin)
                        lfegaqbin[bin] = 0;

                    for (int8u Pos=0; Pos<AC3_Channels[acmod]; ++Pos)
                    {
                        if (!aud_chan_blk[Pos].chahtinu)
                        {
                            aud_chan_blk[Pos].endmant = ((aud_chan_blk[Pos].chbwcod + 12) * 3) + 37;
                            for (size_t bin = 0; bin < aud_chan_blk[Pos].endmant; ++bin)
                            {
                                //TODO
                                // chmant[ch][bin];
                                // (0-16);
                            }
                        }

                        else if (aud_chan_blk[Pos].chahtinu)
                        {
                            int8u chgaqmod = 0;
                            size_t chgaqsections = 0;
                            Get_S1 (2, chgaqmod,                        "chgaqmod[ch]");
                            switch (chgaqmod)
                            {
                                case 0:
                                {
                                    chgaqsections = 0;
                                    break;
                                }
                                case 1:
                                case 2:
                                {
                                    chgaqsections = aud_chan_blk[Pos].chactivegaqbins;
                                    break;
                                }
                                case 3:
                                {
                                    chgaqsections = aud_chan_blk[Pos].chactivegaqbins / 3;
                                    if (aud_chan_blk[Pos].chactivegaqbins % 3)
                                        ++chgaqsections;
                                    break;
                                }
                            };

                            if (chgaqmod>0x0 && chgaqmod<0x3)
                            {
                                for (size_t n = 0; n < chgaqsections; ++n)
                                    Skip_SB(                        "chgaqgain[ch][n]");
                            }
                            else if (chgaqmod==0x3)
                            {
                                for (size_t n = 0; n < chgaqsections; ++n)
                                    Skip_S1(5,                          "chgaqgain[ch][n]");
                            }

                            aud_chan_blk[Pos].endmant = cplstrtmant;
                            int8u* chgaqbin = new int8u [aud_chan_blk[Pos].endmant];
                            if (aud_chan_blk[Pos].chahtinu == 0)
                            {
                                for (size_t i = 0; i < aud_chan_blk[Pos].endmant; ++i)
                                    chgaqbin[i] = 0;
                            }
                            else
                            {
                                int8u endbap = 17;
                                if (chgaqmod < 2)
                                    endbap = 12;

                                aud_chan_blk[Pos].chactivegaqbins = 0;
                                for (size_t i = 0; i < aud_chan_blk[Pos].endmant; ++i)
                                {
                                    if (hebap[i] > 7 && hebap[i] < endbap)
                                    {
                                        chgaqbin[i] = 1;
                                        ++aud_chan_blk[Pos].chactivegaqbins;
                                    }
                                    else if (hebap[i] >= endbap)
                                        chgaqbin[i] = -1;
                                    else
                                        chgaqbin[i] = 0;
                                }
                            }

                            for (size_t bin = 0; bin < aud_chan_blk[Pos].endmant; ++bin)
                            {

                                if (!cplahtinu)
                                {
                                    for (size_t i = cplstrtmant; i < cplendmant; ++i)
                                        cplgaqbin[i] = 0;
                                }
                                else
                                {
                                    int8u endbap = 17;
                                    if (cplgaqmod < 2)
                                        endbap = 12;

                                    cplactivegaqbins = 0;
                                }

                                if (chgaqbin[bin])
                                {
                                    for (size_t n = 0; n < 6; n++)
                                    {
                                        //TODO
                                        // {pre_chmant[n][ch][bin]} ...........................
                                        //     (0-16);
                                    }
                                }
                                else
                                {
                                    //TODO
                                    // pre_chmant[0][ch][bin] .................................................;
                                    // (0-9);
                                }
                            }
                            aud_chan_blk[Pos].chahtinu = -1;
                        }

                        if (aud_blks[blk].cplinu && aud_chan_blk[Pos].chincpl && !got_cplchan)
                        {
                            size_t ncplmant = 12 * ncplsubnd;
                            if (cplahtinu==0)
                            {
                                for (size_t bin = 0; bin < ncplmant; ++bin)
                                {
                                    //TODO
                                    // cplmant[bin] ...........................;
                                    // (0-16);
                                }
                                got_cplchan = true;
                            }
                            else if (cplahtinu==1)
                            {
                                Get_S1 (2, cplgaqmod,                   "cplgaqmod");

                                if (lfeahtinu == 0)
                                {
                                    for (size_t bin = 0; bin < lfeendmant; bin++)
                                        lfegaqbin[bin] = 0;
                                }
                                else
                                {
                                    int8u endbap = 17;
                                    if (lfegaqmod<2)
                                        endbap = 12;

                                    for (size_t bin = 0; bin < lfeendmant; bin++)
                                    {
                                        if (hebap[bin] > 7 && hebap[bin] < endbap)
                                        {
                                            lfegaqbin[bin] = 1;
                                            ++lfeactivegaqbins;
                                        }
                                        else if (hebap[bin] >= endbap)
                                            lfegaqbin[bin] = -1;
                                        else
                                            lfegaqbin[bin] = 0;
                                    }
                                }

                                if (cplgaqmod>0x0 && cplgaqmod<0x3)
                                {
                                    size_t cplgaqsections = cplactivegaqbins;
                                    for (size_t n = 0; n < cplgaqsections; n++)
                                        Skip_SB(                        "cplgaqgain[n]");
                                }
                                else if (cplgaqmod == 0x3)
                                {
                                    size_t cplgaqsections = cplactivegaqbins / 3;
                                    if (cplactivegaqbins % 3)
                                        ++cplgaqsections;

                                    for (size_t n = 0; n < cplgaqsections; n++)
                                        Skip_S1(5,                      "cplgaqgain[n]");
                                }

                                for (size_t bin = 0; bin < ncplmant; ++bin)
                                {
                                    if (cplgaqbin[bin])
                                    {
                                        for (size_t n = 0; n < 6; ++n)
                                        {
                                            //TODO
                                            // pre_cplmant[n][bin] ...........................;
                                            // (0-16);
                                        }
                                    }
                                    else
                                    {
                                        //TODO
                                        // pre_cplmant[0][bin] .................................................;
                                        // (0-9);
                                    }
                                }
                                got_cplchan = true;
                                cplahtinu = -1;
                            }
                            else
                                got_cplchan = true;
                        }
                    }

                    if (lfeon)
                    {
                        int8u nlfemant = 7;
                        if (lfeahtinu == 0)
                        {
                            for (size_t bin = 0; bin < nlfemant; ++bin)
                            {
                                //TODO
                                // lfemant[bin]..............................;
                                // (0-16);
                            }
                        }
                        else if (lfeahtinu == 1)
                        {
                            size_t lfegaqsections = 0;
                            Get_S1 (2, lfegaqmod,                        "lfegaqmod");

                            if (lfeahtinu!=0)
                            {
                                size_t sumgaqbins = 0;
                                for (size_t bin = 0; bin < lfeendmant; bin++)
                                    sumgaqbins += lfegaqbin[bin];

                                switch (lfegaqmod)
                                {
                                    case 0:
                                    {
                                        lfegaqsections = 0;
                                        break;
                                    }
                                    case 1:
                                    case 2:
                                    {
                                        lfegaqsections = lfeactivegaqbins;
                                        break;
                                    }
                                    case 3:
                                    {
                                        lfegaqsections = lfeactivegaqbins / 3;
                                        if (lfeactivegaqbins % 3)
                                            lfegaqsections++;
                                        break;
                                    }
                                };
                            }

                            if (lfegaqmod>0x0 && lfegaqmod<0x3)
                            {
                                for (size_t n = 0; n < lfegaqsections; ++n)
                                    Skip_SB(                             "lfegaqgain[n]");
                            }
                            else if (lfegaqmod==0x3)
                            {
                                for (size_t n = 0; n < lfegaqsections; ++n)
                                    Skip_S1(5,                           "lfegaqgain[n]");
                            }

                            for (size_t bin = 0; bin < nlfemant; ++bin)
                            {
                                if (lfegaqbin[bin])
                                {
                                    for (size_t n = 0; n < 6; ++n)
                                    {
                                        //TODO
                                        // pre_lfemant[n][bin] ..............................;
                                        // (0-16);
                                    }
                                }
                                else
                                {
                                    //TODO
                                    // pre_lfemant[0][bin] ....................................................;
                                    // (0-9);
                                }
                            }

                            lfeahtinu = (int8u)-1;
                        }
                    }

                Element_End0();
            }
            //*/
    }
    else
        Skip_XX(Element_Size-Element_Offset,                        "Unknown");

    if (bsid<=0x10)
    {
        size_t BitsAtEnd=18; //auxdatae+errorcheck
        if (auxdatal!=(int16u)-1)
            BitsAtEnd+=auxdatal+14; //auxbits+auxdatal
        if (Data_BS_Remain()>=BitsAtEnd)
        {
            if (EMDF_RemainPos!=(size_t)-1 && BitsAtEnd<EMDF_RemainPos)
            {
                Skip_BS(Data_BS_Remain()-EMDF_RemainPos,            bsid<=0x0A?"(Unparsed audblk(continue)+5*audblk+padding)":"(Unparsed bsi+6*audblk+padding)");
                emdf();
                Skip_BS(Data_BS_Remain()-BitsAtEnd,                 bsid<=0x0A?"(Unparsed audblk(continue)+5*audblk+padding)":"(Unparsed bsi+6*audblk+padding)");
            }
            else if (Data_BS_Remain()>BitsAtEnd)
                Skip_BS(Data_BS_Remain()-BitsAtEnd,                 bsid<=0x0A?"(Unparsed audblk(continue)+5*audblk+padding)":"(Unparsed bsi+6*audblk+padding)");
            Element_Begin1("auxdata");
                if (auxdatal!=(int16u)-1)
                {
                    Skip_BS(auxdatal,                               "auxbits");
                    Skip_S2(14,                                     "auxdatal");
                }
                Skip_SB(                                            "auxdatae");
            Element_End0();
            Element_Begin1("errorcheck");
                Skip_SB(                                            "encinfo");
                BS_End();
                Skip_B2(                                            "crc2");
            Element_End0();
        }
        else
            BS_End();

        if (Element_Offset<Element_Size)
            Skip_XX(Element_Size-Element_Offset,                    "Unknown");
        Element_Size=Element_Size_Save;
    }

    FILLING_BEGIN();
        if (bsid>0x10)
            return; //Not supported

        Formats[bsid<=0x09?0:1]++;

        // addbsi
        if (!joc_num_objects_map.empty() && addbsie && addbsi_Buffer_Size >= 2 && (addbsi_Buffer[0]&0x01))
        {
            joc_complexity_index_Stream = addbsi_Buffer[1];
        }

        //Information
        if (strmtyp>1)
            strmtyp=0; //TODO: check a file with strmtyp==2
        if (strmtyp==0)
            substreamid_Independant_Current=substreamid;
        if (bsid_Max==(int8u)-1 || bsid>bsid_Max)
            bsid_Max=bsid;

        //Specific to first frame
        if (!frmsizplus1_Max[substreamid_Independant_Current][strmtyp+substreamid])
        {
            frmsizplus1_Max[substreamid_Independant_Current][strmtyp+substreamid]=((bsid<=0x09)?(frmsizecod/2<19?AC3_BitRate[frmsizecod/2]*4:0):((frmsiz+1)*2));
            acmod_Max[substreamid_Independant_Current][strmtyp+substreamid]=acmod;
            lfeon_Max[substreamid_Independant_Current][strmtyp+substreamid]=lfeon;
            bsmod_Max[substreamid_Independant_Current][strmtyp+substreamid]=bsmod;
            cmixlev_Max[substreamid_Independant_Current][strmtyp+substreamid]=cmixlev;
            surmixlev_Max[substreamid_Independant_Current][strmtyp+substreamid]=surmixlev;
            mixlevel_Max[substreamid_Independant_Current][strmtyp+substreamid]=mixlevel;
            roomtyp_Max[substreamid_Independant_Current][strmtyp+substreamid]=roomtyp;
            dmixmod_Max[substreamid_Independant_Current][strmtyp+substreamid]=dmixmod;
            ltrtcmixlev_Max[substreamid_Independant_Current][strmtyp+substreamid]=ltrtcmixlev;
            ltrtsurmixlev_Max[substreamid_Independant_Current][strmtyp+substreamid]=ltrtsurmixlev;
            lorocmixlev_Max[substreamid_Independant_Current][strmtyp+substreamid]=lorocmixlev;
            lorosurmixlev_Max[substreamid_Independant_Current][strmtyp+substreamid]=lorosurmixlev;
            adconvtyp_Max[substreamid_Independant_Current][strmtyp+substreamid]=adconvtyp;
            dsurmod_Max[substreamid_Independant_Current][strmtyp+substreamid]=dsurmod;
            chanmape_Max[substreamid_Independant_Current][strmtyp+substreamid]=chanmape;
            chanmap_Max[substreamid_Independant_Current][strmtyp+substreamid]=chanmap;

            FirstFrame_Dolby.dialnorm=dialnorm;
            if (compre)
                FirstFrame_Dolby.compr=compr;
            if (dynrnge)
                FirstFrame_Dolby.dynrng=dynrng;
            FirstFrame_Dolby.compre=compre;
            FirstFrame_Dolby.dynrnge=dynrnge;
        }

        //Stats
        if (dialnorms.empty())
            dialnorms.resize(32);
        if (dialnorm!=(int8u)-1)
            dialnorms[dialnorm]++;
        if (compre)
        {
            if (comprs.empty())
                comprs.resize(256);
            if (compr!=(int8u)-1)
                comprs[compr]++;
        }
        if (dynrnge)
        {
            //Saving new value
            dynrnge_Exists=true;
        }
        if (!dynrnge)
            dynrng=0;
        if (dynrngs.empty())
            dynrngs.resize(256);
        if (dynrng!=(int8u)-1)
            dynrngs[dynrng]++;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ac3::emdf()
{
    //JOC reinit
    joc_complexity_index_Stream=(int8u)-1;
    num_dynamic_objects=(int8u)-1;
    nonstd_bed_channel_assignment_mask=(int32u)-1;

    Element_Begin1("emdf");
    emdf_sync();
    emdf_container();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::emdf_sync()
{
    int16u emdf_container_length;
    Element_Begin1("emdf_sync");
    Skip_S2(16,                                                 "syncword");
    Get_S2 (16, emdf_container_length,                          "emdf_container_length");
    Element_End0();

    RemainAfterEMDF=Data_BS_Remain()-emdf_container_length*8; //emdf_container_length coherency was already tested in sync layer
}

//---------------------------------------------------------------------------
void File_Ac3::emdf_container()
{
    int32u version, key_id;
    Element_Begin1("emdf_container");
    Get_S4 (2, version,                                         "emdf_version");
    if (version == 3)
    {
        int32u add;
        Get_V4(2, add,                                          "emdf_version addition");
        version += add;
    }
    if (version)
    {
        Skip_BS(Data_BS_Remain()-RemainAfterEMDF,               "(Unparsed emdf_container data)");
        Element_End0(); 
        return;
    }

    Get_S4 (3, key_id,                                          "key_id");
    if (key_id == 7)
    {
        int32u add;
        Get_V4 (3, add,                                         "key_id addition");
        key_id += add;
    }

    int32u emdf_payload_id = 0;
        
    for(;;)
    {
        Element_Begin1("emdf_payload");
        Get_S4 (5, emdf_payload_id,                             "emdf_payload_id");
        if (emdf_payload_id==0x1F)
        {
            int32u add;
            Get_V4 (5, add,                                     "emdf_payload_id addition");
            emdf_payload_id += add;
        }

        if (emdf_payload_id<16)
            Element_Info1(Ac3_emdf_payload_id[emdf_payload_id]);
        if (emdf_payload_id == 0x00)
        {
            Element_End0();
            break;
        }

        emdf_payload_config();

        int32u emdf_payload_size = 0;
        Get_V4 (8, emdf_payload_size,                           "emdf_payload_size");
        size_t emdf_payload_End=Data_BS_Remain()-emdf_payload_size*8; //emdf_payload_size coherency was already tested in sync layer

        Element_Begin1("emdf_payload_bytes");
            switch (emdf_payload_id)
            {
                case  1: loudness_data(); break;
                case  2: programme_information(); break;
                case 11: object_audio_metadata_payload(); break;
                case 14: joc(); break;
                default: Skip_BS(emdf_payload_size*8,           "(Unknown)");
            }
            size_t RemainginBits=Data_BS_Remain();
            if (RemainginBits>=emdf_payload_End)
            {
                if (RemainginBits>emdf_payload_End)
                    Skip_BS(RemainginBits-emdf_payload_End,     "(Unparsed bits)");
            }
            else
            {
                //There is a problem, too many bits were consumed by the parser. //TODO: prevent the parser to consume more bits than count of bits in this element
                if (Data_BS_Remain() >= RemainAfterEMDF)
                    Skip_BS(Data_BS_Remain() - RemainAfterEMDF, "(Problem during emdf_payload parsing)");
                else
                    Skip_BS(Data_BS_Remain(),                   "(Problem during emdf_payload parsing, going to end directly)");
                Element_End0();
                Element_End0();
                break;
            }
        Element_End0();

        Element_End0();
    }

    emdf_protection();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::emdf_payload_config()
{
    Element_Begin1("emdf_payload_config");
    bool smploffste = false;
    Get_SB (smploffste,                                         "smploffste");
    if (smploffste)
    {
        Skip_S2(11,                                             "smploffst");
        Skip_SB(                                                "reserved");
    }

    TEST_SB_SKIP(                                               "duratione");
        Skip_V4(11,                                             "duration");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "groupide");
        Skip_V4(2,                                              "groupid");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "codecdatae");
        Skip_S1(8,                                              "reserved");
    TEST_SB_END();

    bool discard_unknown_payload = false;
    Get_SB(discard_unknown_payload,                             "discard_unknown_payload");
    if (!discard_unknown_payload)
    {
        bool payload_frame_aligned = false;
        if (!smploffste)
        {
            Get_SB (payload_frame_aligned,                      "payload_frame_aligned");
            if (payload_frame_aligned)
            {
                Skip_SB(                                        "create_duplicate");
                Skip_SB(                                        "remove_duplicate");
            }
        }

        if (smploffste || payload_frame_aligned)
        {
            Skip_S1(5,                                          "priority");
            Skip_S1(2,                                          "proc_allowed");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::emdf_protection()
{
    int8u len_primary = 0, len_second = 0;
    Element_Begin1("emdf_protection");
    Get_S1(2, len_primary,                                      "protection_length_primary");
    Get_S1(2, len_second,                                       "protection_length_secondary");

    switch (len_primary)
    {
        //case 0: break; //protection_length_primary coherency was already tested in sync layer
        case 1: len_primary = 8; break;
        case 2: len_primary = 32; break;
        case 3: len_primary = 128; break;
        default:; //Cannot append, read only 2 bits
    };
    switch (len_second)
    {
        case 0: len_second = 0; break;
        case 1: len_second = 8; break;
        case 2: len_second = 32; break;
        case 3: len_second = 128; break;
        default:; //Cannot append, read only 2 bits
    };
    Skip_BS(len_primary,                                        "protection_bits_primary");
    Skip_BS(len_second,                                         "protection_bits_secondary");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::loudness_data()
{
#if MEDIAINFO_TRACE

    Element_Begin1("loudness_data");
    auto RemainingBitsBegin = Data_BS_Remain();

    int8u version, loudpractyp;
    bool loudcorrdialgat{}, loudrelgate, loudspchgate, loudstrm3se, truepke;
    Get_S1(2, version,                                          "version");
    if (version == 0x3) {
        int8u extended_version;
        Get_S1(4, extended_version,                             "extended_version");
        version += extended_version;
    }
    TEST_SB_SKIP(                                               "dialchane");
        Skip_S1(3,                                              "dialchan");
    TEST_SB_END();
    Get_S1(4, loudpractyp,                                      "loudpractyp");
    if (loudpractyp != 0x0) {
        Get_SB (loudcorrdialgat,                                "loudcorrdialgat");
        Skip_S1(1,                                              "loudcorrtyp");
    }
    TEST_SB_GET(loudrelgate,                                    "loudrelgate");
        Skip_S1(7,                                              "loudrelgat");
    TEST_SB_END();
    TEST_SB_GET(loudspchgate,                                   "loudspchgate");
        Skip_S1(7,                                              "loudspchgat");
    TEST_SB_END();
    TEST_SB_GET(loudstrm3se,                                    "loudstrm3se");
        Skip_S1(8,                                              "loudstrm3s");
    TEST_SB_END();
    TEST_SB_GET(truepke,                                        "truepke");
        Skip_S1(8,                                              "truepk");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "dmixloudoffste");
        Skip_S1(5,                                              "dmixloudoffst");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "prgmbndye");
        bool prgmbndy_bit = 0;
        while (prgmbndy_bit == 0) {
            Get_SB(prgmbndy_bit,                                "prgmbndy_bit");
        }
        Skip_SB(                                                "end_or_start");
        TEST_SB_SKIP(                                           "prgmbndyoffste");
            Skip_S2(11,                                         "prgmbndyoffst");
        TEST_SB_END();
    TEST_SB_END();
    auto RemainingBitsEnd = Data_BS_Remain();
    auto size = RemainingBitsBegin - RemainingBitsEnd;
    Skip_BS((8 - (size & 7)) & 7,                               "fill bits = 0");

    if (version >= 1) {
        bool extloudspchgate;
        if (loudrelgate) {
            TEST_SB_SKIP(                                       "hrloudrelgate");
                Skip_S1(3,                                      "hrloudrelgat");
            TEST_SB_END();
        }
        if (loudspchgate) {
            TEST_SB_SKIP(                                       "hrloudspchgate");
                Skip_S1(3,                                      "hrloudspchgat");
            TEST_SB_END();
        }
        if (loudstrm3se) {
            TEST_SB_SKIP(                                       "hrloudstrm3se");
                Skip_S1(3,                                      "hrloudstrm3s");
            TEST_SB_END();
        }
        if (truepke) {
            TEST_SB_SKIP(                                       "hrtruepke");
                Skip_S1(3,                                      "hrtruepk");
            TEST_SB_END();
        }
        TEST_SB_SKIP(                                           "hrtruepke");
            Skip_S1(3,                                          "hrtruepk");
        TEST_SB_END();
        if (loudcorrdialgat)
            Skip_S1(3,                                          "loudcorrdialgattyp");
        if (loudrelgate == 0) {
            TEST_SB_SKIP(                                       "extloudrelgate");
                Skip_S2(11,                                     "extloudrelgat");
            TEST_SB_END();
        }
        if (loudspchgate == 0) {
            TEST_SB_GET(extloudspchgate,                        "extloudspchgate");
                Skip_S2(11,                                     "extloudspchgat");
            TEST_SB_END();
        }
        if (loudspchgate || extloudspchgate)
            Skip_S1(3,                                          "loudspchdialgattyp");
        if (loudstrm3se == 0) {
            TEST_SB_SKIP(                                       "extloudstrm3se");
                Skip_S2(11,                                     "extloudstrm3s");
            TEST_SB_END();
        }
        if (truepke == 0) {
            TEST_SB_SKIP(                                       "exttruepke");
                Skip_S2(11,                                     "exttruepk");
            TEST_SB_END();
        }
        TEST_SB_SKIP(                                           "maxloudstrm3se");
            Skip_S2(11,                                         "maxloudstrm3s");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "maxtruepke");
            Skip_S2(11,                                         "maxtruepk");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "lrae");
            Skip_S2(10,                                         "lra");
            Skip_S1(3,                                          "lrapractyp");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "loudmntrye");
            Skip_S2(11,                                         "loudmntry");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "maxloudmntrye");
            Skip_S2(11,                                         "maxloudmntry");
        TEST_SB_END();
        RemainingBitsEnd = Data_BS_Remain();
        size = RemainingBitsBegin - RemainingBitsEnd;
        Skip_BS((8 - (size & 7)) & 7,                           "fill bits = 0");
    }

    Element_End0();

#endif // MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Ac3::programme_information()
{
    Element_Begin1("programme_information");
    auto RemainingBitsBegin = Data_BS_Remain();
    int8u version;
    Get_S1(2, version,                                          "version");
    if (version == 0x3) {
        int8u extended_version;
        Get_S1(4, extended_version,                             "extended_version");
        version += extended_version;
    }
    TEST_SB_SKIP(                                               "activechane");
        Skip_S2(16,                                             "activechan");
    TEST_SB_END();
    bool dmixtype;
    Get_SB(dmixtype,                                            "dmixtype");
    if (!dmixtype) {
        TEST_SB_SKIP(                                           "upmixtype");
            Skip_S1(4,                                          "upmixtyp");
        TEST_SB_END();
    }
    else {
        Skip_S1(4,                                              "dmixtyp");
    }
    TEST_SB_SKIP(                                               "preproinfoe");
        Skip_SB(                                                "suratten");
        Skip_SB(                                                "ph90filt");
        Skip_SB(                                                "lfefilt");
        Skip_S1(2,                                              "lfemonlevcod");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "drcprofinfoe");
        Get_S1(3, dynrngprof,                                   "dynrngprof"); Param_Info1(AC3_dynrngprof_Get(dynrngprof));
        Get_S1(3, comprprof,                                    "comprprof"); Param_Info1(AC3_dynrngprof_Get(comprprof));
    TEST_SB_END();
    TEST_SB_SKIP(                                               "specprocinfoe");
        Skip_S1(3,                                              "specprocstartf");
        Skip_S1(3,                                              "specprocendf");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "chancplinfoe");
        Skip_S1(5,                                              "chancplstartf");
        Skip_S1(5,                                              "chancplendf");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "enhncrnge");
        Skip_S1(2,                                              "enhncrng");
    TEST_SB_END();
    auto RemainingBitsEnd = Data_BS_Remain();
    auto size = RemainingBitsBegin - RemainingBitsEnd;
    Skip_BS((8 - (size & 7)) & 7,                               "fill bits = 0");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::object_audio_metadata_payload()
{
    Element_Begin1("object_audio_metadata_payload");
    auto RemainingBitsBegin = Data_BS_Remain();
    int8u oa_md_version_bits;
    Get_S1 (2, oa_md_version_bits,                              "oa_md_version_bits");
    if (oa_md_version_bits == 0x3)
    {
        int8u oa_md_version_bits_ext;
        Get_S1 (3, oa_md_version_bits_ext,                      "oa_md_version_bits_ext");
        oa_md_version_bits += oa_md_version_bits_ext;
    }

    int8u object_count_bits;
    Get_S1 (5, object_count_bits,                               "object_count_bits");
    num_dynamic_objects = object_count_bits + 1;
    if (object_count_bits == 0x1F)
    {
        int8u object_count_bits_ext;
        Get_S1 (7, object_count_bits_ext,                       "object_count_bits_ext");
        num_dynamic_objects += object_count_bits_ext;
    }
    int8u object_count{ num_dynamic_objects };

    program_assignment();

    #if MEDIAINFO_TRACE

    bool b_alternate_object_data_present;
    Get_SB(b_alternate_object_data_present,                     "b_alternate_object_data_present");
    
    int8u oa_element_count_bits;
    int8u num_obj_info_blocks;
    std::vector<std::vector<bool>> b_obj_not_active_Vec;
    Get_S1(4, oa_element_count_bits,                            "oa_element_count_bits");
    if (oa_element_count_bits == 0xF)
    {
        int8u oa_element_count_bits_ext;
        Get_S1(3, oa_element_count_bits_ext,                    "oa_element_count_bits_ext");
        oa_element_count_bits += oa_element_count_bits_ext;
    }
    for (int8u i = 0; i < oa_element_count_bits; ++i) {
        Element_Begin1("oa_element_md");
        int8u oa_element_id_idx;
        int32u oa_element_size_bits;
        Get_S1 (4, oa_element_id_idx,                           "oa_element_id_idx");
        Get_V4 (4, oa_element_size_bits,                        "oa_element_size_bits");
        auto oa_element_EndRemain = Data_BS_Remain() - (oa_element_size_bits + 1LL) * 8;
        if (b_alternate_object_data_present)
            Skip_S1(4,                                          "alternate_object_data_id_idx");
        Skip_SB(                                                "b_discard_unknown_element");
        switch (oa_element_id_idx)
        {
        case 1:                                                 // object_element
        {
            Element_Begin1("object_element");
            Element_Begin1("md_update_info");
            int8u sample_offset_code;
            Get_S1(2, sample_offset_code,                       "sample_offset_code");
            if (sample_offset_code == 0b01)
                Skip_S1(2,                                      "sample_offset_idx");
            else
                if (sample_offset_code == 0b10)
                    Skip_S1(5,                                  "sample_offset_bits");
            int8u num_obj_info_blocks_bits;
            Get_S1(3, num_obj_info_blocks_bits,                 "num_obj_info_blocks_bits");
            num_obj_info_blocks = num_obj_info_blocks_bits + 1;
            for (int8u blk = 0; blk < num_obj_info_blocks; ++blk) {
                Element_Begin1("block_update_info");
                Skip_S1(6,                                      "block_offset_factor_bits");
                int8u ramp_duration_code;
                Get_S1(2, ramp_duration_code,                   "ramp_duration_code");
                if (ramp_duration_code == 0b11) {
                    bool b_use_ramp_duration_idx;
                    Get_SB(b_use_ramp_duration_idx,             "b_use_ramp_duration_idx");
                        if (b_use_ramp_duration_idx)
                            Skip_S1(6,                          "ramp_duration_idx");
                        else
                            Skip_S2(11,                         "ramp_duration_bits");
                }
                Element_End0(); //block_update_info
            }
            Element_End0(); //md_update_info
            bool b_reserved_data_not_present;
            Get_SB(b_reserved_data_not_present,                 "b_reserved_data_not_present");
            if (!b_reserved_data_not_present)
                Skip_S1(5,                                      "reserved");
            
            for (int8u j = 0; j < object_count; ++j) {
                b_obj_not_active_Vec.push_back(std::vector<bool>());
                Element_Begin1("object_data");
                for (int8u blk = 0; blk < num_obj_info_blocks; ++blk) {
                    Element_Begin1("object_info_block");
                    bool b_object_not_active;
                    int8u object_basic_info_status_idx;
                    Get_SB(b_object_not_active,                 "b_object_not_active");
                    b_obj_not_active_Vec[j].push_back(b_object_not_active);
                    if (b_object_not_active)
                        object_basic_info_status_idx = 0b00;
                    else {
                        if (blk == 0)
                            object_basic_info_status_idx = 0b01;
                        else
                            Get_S1(2, object_basic_info_status_idx, "object_basic_info_status_idx");
                    }
                    if ((object_basic_info_status_idx == 0b01) || (object_basic_info_status_idx == 0b11)) {
                        Element_Begin1("object_basic_info");
                        bool object_basic_info[2]{};
                        if (object_basic_info_status_idx == 0b01) {
                            object_basic_info[1] = true;
                            object_basic_info[0] = true;
                        }
                        else {
                            Get_SB(object_basic_info[1],        "object_basic_info[1]");
                            Get_SB(object_basic_info[0],        "object_basic_info[0]");
                        }
                        if (object_basic_info[1]) {
                            int8u object_gain_idx;
                            Get_S1(2, object_gain_idx,          "object_gain_idx");
                            if (object_gain_idx == 0b10)
                                Skip_S1(6,                      "object_gain_bits");
                        }
                        if (object_basic_info[0]) {
                            bool b_default_object_priority;
                            Get_SB(b_default_object_priority,   "b_default_object_priority");
                            if (!b_default_object_priority)
                                Skip_S1(5,                      "object_priority_bits");
                        }
                        Element_End0(); //object_basic_info
                    }
                    int8u object_render_info_status_idx{};
                    if (b_object_not_active) {
                        object_render_info_status_idx = 0b00;
                    }
                    else {
                        bool b_object_in_bed_or_isf = j < (num_bed_objects + num_isf_objects);
                        if (!b_object_in_bed_or_isf) {
                            if (blk == 0) {
                                object_render_info_status_idx = 0b01;
                            }
                            else {
                                Get_S1(2, object_render_info_status_idx, "object_render_info_status_idx");
                            }
                        }
                        else {
                            object_render_info_status_idx = 0b00;
                        }
                    }
                    if ((object_render_info_status_idx == 0b01) || (object_render_info_status_idx == 0b11)) {
                        Element_Begin1("object_render_info");
                        int8u obj_render_info{};
                        if (object_render_info_status_idx == 0b01)
                            obj_render_info = 0b1111;
                        else
                            Get_S1(4, obj_render_info,          "obj_render_info");
                        if (obj_render_info & 0b0001) {
                            bool b_differential_position_specified{};
                            if (blk == 0)
                                b_differential_position_specified = false;
                            else
                                Get_SB(b_differential_position_specified, "b_differential_position_specified");
                            if (b_differential_position_specified) {
                                Skip_S1(3,                      "diff_pos3D_X_bits");
                                Skip_S1(3,                      "diff_pos3D_Y_bits");
                                Skip_S1(3,                      "diff_pos3D_Z_bits");
                            }
                            else {
                                Skip_S1(6,                      "pos3D_X_bits");
                                Skip_S1(6,                      "pos3D_Y_bits");
                                Skip_SB(                        "pos3D_Z_sign_bits");
                                Skip_S1(4,                      "pos3D_Z_bits");
                            }
                            bool b_object_distance_specified;
                            Get_SB(b_object_distance_specified, "b_object_distance_specified");
                            if (b_object_distance_specified) {
                                bool b_object_at_infinity;
                                Get_SB(b_object_at_infinity,    "b_object_at_infinity");
                                if (b_object_at_infinity) {
                                    // object_distance = inf
                                }
                                else
                                    Skip_S1(4,                  "distance_factor_idx");
                            }
                        }
                        if (obj_render_info & 0b0010) {
                            Skip_S1(3,                          "zone_constraints_idx");
                            Skip_SB(                            "b_enable_elevation");
                        }
                        if (obj_render_info & 0b0100) {
                            int8u object_size_idx;
                            Get_S1(2, object_size_idx,          "object_size_idx");
                            if (object_size_idx == 0b01)
                                Skip_S1(5,                      "object_size_bits");
                            else {
                                if (object_size_idx == 0b10) {
                                    Skip_S1(5,                  "object_width_bits");
                                    Skip_S1(5,                  "object_depth_bits");
                                    Skip_S1(5,                  "object_height_bits");
                                }
                            }
                        }
                        if (obj_render_info & 0b1000) {
                            bool b_object_use_screen_ref;
                            Get_SB(b_object_use_screen_ref,     "b_object_use_screen_ref");
                            if (b_object_use_screen_ref) {
                                Skip_S1(3,                      "screen_factor_bits");
                                Skip_S1(2,                      "depth_factor_idx");
                            }
                            else {
                                // screen_factor_bits = 0;
                            }
                        }
                        Skip_SB(                                "b_object_snap");
                        Element_End0(); //object_render_info
                    }
                    bool b_additional_table_data_exists;
                    Get_SB(b_additional_table_data_exists,      "b_additional_table_data_exists");
                    if (b_additional_table_data_exists) {
                        int8u additional_table_data_size_bits;
                        Get_S1(4, additional_table_data_size_bits, "additional_table_data_size_bits");
                        Skip_BS((additional_table_data_size_bits+1LL)*8, "additional_table_data");
                    }
                    Element_End0(); //object_info_block
                }
                Element_End0(); //object_data
            }
            
            Element_End0(); //object_element
            break;
        }
        case 2:                                                 // trim_element
        {
            Element_Begin1("trim_element");
            const int8u NUM_TRIM_CONFIGS{ 9 };
            Skip_S1(2,                                          "warp_mode");
            Skip_S1(2,                                          "reserved");
            int8u global_trim_mode;
            Get_S1(2, global_trim_mode,                         "global_trim_mode"); Param_Info1(global_trim_mode == 0 ? "default_trim" : global_trim_mode == 1 ? "disable_trim" : global_trim_mode == 2 ? "custom_trim" : "");
            if (global_trim_mode == 0b10) {
                for (int8u cfg = 0; cfg < NUM_TRIM_CONFIGS; ++cfg) {
                    bool b_default_trim;
                    Get_SB(b_default_trim,                      "b_default_trim");
                    if (b_default_trim == 0) {
                        bool b_disable_trim;
                        Get_SB(b_disable_trim,                  "b_disable_trim");
                        if (b_disable_trim == 0) {
                            int8u trim_balance_presence;
                            Get_S1(5, trim_balance_presence,    "trim_balance_presence");
                            if (trim_balance_presence & 0b10000)
                                Skip_S1(4,                      "trim_centre");
                            if (trim_balance_presence & 0b01000)
                                Skip_S1(4,                      "trim_surround");
                            if (trim_balance_presence & 0b00100)
                                Skip_S1(4,                      "trim_height");
                            if (trim_balance_presence & 0b00010) {
                                Skip_SB(                        "bal3D_Y_sign_tb_code");
                                Skip_S1(4,                      "bal3D_Y_amount_tb");
                            }
                            if (trim_balance_presence & 0b00010) {
                                Skip_SB(                        "bal3D_Y_sign_lis_code");
                                Skip_S1(4,                      "bal3D_Y_amount_lis");
                            }
                        }
                    }
                }
            }
            TEST_SB_SKIP(                                       "b_disable_trim_per_obj");
                for (int8u obj = 0; obj < object_count; obj++)
                    Skip_SB(                                    "b_disable_trim");
            TEST_SB_END();
            Element_End0(); // trim_element
            break;
        }
        case 5:                                                 // extended_object_element
        {
            Element_Begin1("extended_object_element");
            TEST_SB_SKIP(                                       "b_obj_div_block");
                for (int8u obj = 0; obj < object_count; ++obj) {
                    for (int8u blk = 0; blk < num_obj_info_blocks; ++blk) {
                        Element_Begin1("obj_div_block");
                        if (b_obj_not_active_Vec[obj][blk]) {
                            // object_divergence = 0;
                        }
                        else {
                            bool obj_type_DYNAMIC = obj >= (num_bed_objects + num_isf_objects);
                            if (obj_type_DYNAMIC) {
                                TEST_SB_SKIP(                   "b_object_divergence");
                                    int8u object_div_mode;
                                    Get_S1(2, object_div_mode,  "object_div_mode");
                                    if (object_div_mode == 0)
                                        Skip_S1(2,              "object_div_table");
                                    if (object_div_mode == 2 || object_div_mode == 3) {
                                        Skip_S1(6,              "object_div_code");
                                    }
                                    else {
                                        // object_divergence = 0;
                                    }
                                TEST_SB_END();
                            }
                        }
                        Element_End0(); // obj_div_block
                    }
                }
            TEST_SB_END();
            TEST_SB_SKIP(                                       "b_ext_prec_pos_block");
                for (int8u obj = 0; obj < object_count; ++obj) {
                    for (int8u blk = 0; blk < num_obj_info_blocks; ++blk) {
                        Element_Begin1("ext_prec_pos_block");
                        if (b_obj_not_active_Vec[obj][blk]) {
                            // b_ext_prec_pos = false;
                        }
                        else {
                            bool obj_type_DYNAMIC = obj >= (num_bed_objects + num_isf_objects);
                            if (obj_type_DYNAMIC) {
                                TEST_SB_SKIP(                   "b_ext_prec_pos");
                                    int8u ext_prec_pos_presence;
                                    Get_S1(3, ext_prec_pos_presence, "ext_prec_pos_presence");
                                    if (ext_prec_pos_presence & 0b100)
                                        Skip_S1(2,              "ext_prec_pos3D_X");
                                    if (ext_prec_pos_presence & 0b010)
                                        Skip_S1(2,              "ext_prec_pos3D_Y");
                                    if (ext_prec_pos_presence & 0b001)
                                        Skip_S1(2,              "ext_prec_pos3D_Z");
                                TEST_SB_END();
                            }
                        }
                        Element_End0(); // ext_prec_pos_block
                    }
                }
            TEST_SB_END();
            Element_End0(); // extended_object_element
            break;
        }
        default:                                                // reserved
        {
            auto RemainingBits = Data_BS_Remain();
            if (RemainingBits > oa_element_EndRemain)
                Skip_BS(RemainingBits - oa_element_EndRemain,   "(Not parsed)");
            break;
        }
        }
        auto RemainingBits = Data_BS_Remain();
        if (RemainingBits > oa_element_EndRemain)
            Skip_BS(RemainingBits - oa_element_EndRemain,       "padding");
        Element_End0(); //oa_element_md
    }
    auto RemainingBitsEnd = Data_BS_Remain();
    auto size = RemainingBitsBegin - RemainingBitsEnd;
    Skip_BS((8 - (size & 7)) & 7,                               "padding");

    #endif // MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::program_assignment()
{
    Element_Begin1("program_assignment");
    bool b_dyn_object_only_program = false;
    Get_SB (b_dyn_object_only_program,                          "b_dyn_object_only_program");
    if (b_dyn_object_only_program)
    {
        bool b_lfe_present;
        Get_SB (b_lfe_present,                                  "b_lfe_present");
        if (b_lfe_present)
        {
            nonstd_bed_channel_assignment_mask=(1<<3);
            if (num_dynamic_objects!=(int8u)-1)
                num_dynamic_objects--;
        }
    }
    else
    {
        int8u content_description_mask;
        Get_S1 (4, content_description_mask,                    "content_description_mask");
        if (content_description_mask & 0x1)
        {
            bool b_bed_object_chan_distribute, b_multiple_bed_instances_present;

            Get_SB (b_bed_object_chan_distribute,               "b_bed_object_chan_distribute");
            Get_SB (b_multiple_bed_instances_present,           "b_multiple_bed_instances_present");
            int32u num_bed_instances = 1;
            if (b_multiple_bed_instances_present)
            {
                int8u num_bed_instances_bits = 0;
                Get_S1 (3, num_bed_instances_bits,              "num_bed_instances_bits");
                num_bed_instances = num_bed_instances_bits + 2;
            }

            for (int32u bed = 0; bed < num_bed_instances; ++bed)
            {
                Element_Begin1("Bed");
                bool b_lfe_only = true;
                Get_SB (b_lfe_only,                             "b_lfe_only");
                if (!b_lfe_only)
                {
                    bool b_standard_chan_assign;
                    Get_SB (b_standard_chan_assign,             "b_standard_chan_assign");
                    if (b_standard_chan_assign)
                    {
                        int16u bed_channel_assignment_mask;
                        Get_S2 (10, bed_channel_assignment_mask, "bed_channel_assignment_mask");
                        nonstd_bed_channel_assignment_mask=AC3_bed_channel_assignment_mask_2_nonstd(bed_channel_assignment_mask);
                    }
                    else
                        Get_S3 (17, nonstd_bed_channel_assignment_mask, "nonstd_bed_channel_assignment_mask");
                }
                Element_End0();
            }
        }

        if (content_description_mask & 0x2) {
            const int8u num_isf_objects_lookup[] = {4, 8, 10, 14, 15, 30};
            int8u intermediate_spatial_format_idx;
            Get_S1(3, intermediate_spatial_format_idx,          "intermediate_spatial_format_idx");
            num_isf_objects = num_isf_objects_lookup[intermediate_spatial_format_idx];
        }

        if (content_description_mask & 0x4)
        {
            int8u num_dynamic_objects_bits;
            Get_S1(5, num_dynamic_objects_bits,                 "num_dynamic_objects_bits");
            if (num_dynamic_objects_bits == 0x1F)
            {
                int8u num_dynamic_objects_bits_ext = 0;
                Get_S1 (7, num_dynamic_objects_bits_ext,        "num_dynamic_objects_bits_ext");
                num_dynamic_objects_bits += num_dynamic_objects_bits_ext;
            }
            num_dynamic_objects = num_dynamic_objects_bits + 1;
        }
        else
            num_dynamic_objects = 0;

        if (content_description_mask & 0x8)
        {
            int8u reserved_data_size_bits;
            Get_S1 (4, reserved_data_size_bits,                 "reserved_data_size_bits");
            Skip_BS((reserved_data_size_bits + 1LL) * 8,        "reserved_data() + padding");
        }
    }

    num_bed_objects = 0;
    int32u n{ nonstd_bed_channel_assignment_mask };
    while (n) {
        n &= (n - 1);
        ++num_bed_objects;
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::joc()
{
    Element_Begin1("joc");
    joc_header();
    joc_info();
    joc_data();
    if (joc_ext_config_idx > 0)
        joc_ext_data();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::joc_header()
{
    Element_Begin1("joc_header");
        Skip_S1(3,                                              "joc_dmx_config_idx");
        int8u joc_num_objects_bits = 0;
        Get_S1 (6, joc_num_objects_bits,                        "joc_num_objects_bits");
        joc_num_objects = joc_num_objects_bits + 1;
        joc_num_objects_map[joc_num_objects] ++;
        Get_S1 (3, joc_ext_config_idx,                          "joc_ext_config_idx");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::joc_info()
{
    Element_Begin1("joc_info");
    Skip_S1(3,                                                  "joc_clipgain_x_bits");
    Skip_S1(5,                                                  "joc_clipgain_y_bits");
    Skip_S2(10,                                                 "joc_seq_count_bits");
    for (int8u obj = 0; obj < joc_num_objects; ++obj)
    {
        TEST_SB_SKIP(                                           "b_joc_obj_present[obj]");
            Skip_S1(3,                                          "joc_num_bands_idx[obj]");
            Skip_SB(                                            "b_joc_sparse[obj]");
            Skip_S1(1,                                          "joc_num_quant_idx[obj]");
            joc_data_point_info();
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::joc_data_point_info()
{
    Element_Begin1("joc_data_point_info");
    int8u joc_slope_idx_obj, joc_num_dpoints_bits_obj;
    Get_S1(1, joc_slope_idx_obj,                                "joc_slope_idx[obj]");
    Get_S1(1, joc_num_dpoints_bits_obj,                         "joc_num_dpoints_bits[obj]");
    if (joc_slope_idx_obj == 1) {
        for (int8u dp = 0; dp <= joc_num_dpoints_bits_obj; ++dp) {
            Skip_S1(5,                                          "joc_offset_ts_bits[obj,dp]");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::joc_data()
{
    Element_Begin1("joc_data");
    //TODO
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::joc_ext_data()
{
    Element_Begin1("joc_ext_data");
    //TODO
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac3::HD()
{
    //Parsing
    bool MajorSyncOK{ true };
    int32u Synch;
    Peek_B3(Synch);
    if (Synch==0xF8726F)
    {
        if (Buffer_Offset+28>Buffer_Size)
        {
            Trusted_IsNot("Not enough data");
            return; //Need more data
        }

        Element_Info1("major_sync");
        Element_Begin1("major_sync_info");
        int32u format_sync;
        Get_B4(format_sync,                                     "format_sync");
        HD_StreamType=(int8u)format_sync; Param_Info1(AC3_HD_StreamType(HD_StreamType));

        if ((HD_StreamType&0xFE)!=0xBA)
        {
            Skip_XX(Element_Size-Element_Offset,                "Data");
            return;
        }
        HD_format_info();
        Skip_B2(                                                "signature");
        Get_B2 (HD_flags,                                       "flags");
        Skip_B2(                                                "reserved");
        BS_Begin();
        Get_SB (    HD_IsVBR,                                   "variable_rate");
        Get_S2 (15, HD_BitRate_Max,                             "peak_data_rate"); Param_Info2((HD_BitRate_Max*(AC3_HD_SamplingRate(HD_SamplingRate2)?AC3_HD_SamplingRate(HD_SamplingRate2):AC3_HD_SamplingRate(HD_SamplingRate1))+8)>>4, " bps");
        Get_S1 ( 4, HD_SubStreams_Count,                        "substreams");
        Skip_S1( 2,                                             "reserved");
        Get_S1 ( 2, presentations[p_16ch].presentation,         "extended_substream_info"); Param_Info1C(HD_StreamType == 0xBA, AC3_TrueHD_ExtendedSubstreamInfo(presentations[p_16ch].presentation));
        presentations[p_2ch].presentation = 1;
        if (HD_StreamType==0xBA)
        {
            Element_Begin1("substream_info");
                Get_SB (    HD_HasAtmos,                        "16-channel presentation is present");
                Get_S1 ( 3, presentations[p_8ch].presentation,  "8-ch presentation"); Param_Info1(AC3_TrueHD_SubstreamInfo8ch(presentations[p_8ch].presentation));
                Get_S1 ( 2, presentations[p_6ch].presentation,  "6-ch presentation"); Param_Info1(AC3_TrueHD_SubstreamInfo6ch(presentations[p_6ch].presentation));
                Skip_S1( 2,                                     "reserved");
            Element_End0();
        }
        else
            Skip_S1(8,                                          "Unknown");
        BS_End();
        Element_Begin1("channel_meaning");
            if (HD_StreamType==0xBA)
            {
                bool HasExtend;
                BS_Begin();
                Skip_S1( 6,                                     "reserved");
                Get_SB (  presentations[p_2ch].control_enabled, "2ch_control_enabled");
                Get_SB (  presentations[p_6ch].control_enabled, "6ch_control_enabled");
                Get_SB (  presentations[p_8ch].control_enabled, "8ch_control_enabled");
                Skip_S1( 1,                                     "reserved");
                Skip_S1( 7,                                     "drc_start_up_gain");
                Get_S1 ( 6, presentations[p_2ch].dialogue_norm, "2ch_dialogue_norm"); Param_Info2(presentations[p_2ch].dialogue_norm == 0 ? -31 : -presentations[p_2ch].dialogue_norm, " LKFS");
                Get_S1 ( 6, presentations[p_2ch].mix_level,     "2ch_mix_level");     Param_Info2(70 + presentations[p_2ch].mix_level, " dB");
                Get_S1 ( 5, presentations[p_6ch].dialogue_norm, "6ch_dialogue_norm"); Param_Info2(presentations[p_6ch].dialogue_norm == 0 ? -31 : -presentations[p_6ch].dialogue_norm, " LKFS");
                Get_S1 ( 6, presentations[p_6ch].mix_level,     "6ch_mix_level");     Param_Info2(70 + presentations[p_6ch].mix_level, " dB");
                Skip_S1( 5,                                     "6ch_source_format");
                Get_S1 ( 5, presentations[p_8ch].dialogue_norm, "8ch_dialogue_norm"); Param_Info2(presentations[p_8ch].dialogue_norm == 0 ? -31 : -presentations[p_8ch].dialogue_norm, " LKFS");
                Get_S1 ( 6, presentations[p_8ch].mix_level,     "8ch_mix_level");     Param_Info2(70 + presentations[p_8ch].mix_level, " dB");
                Skip_S1( 6,                                     "8ch_source_format");
                Skip_S1( 1,                                     "reserved");
                Get_SB (    HasExtend,                          "extra_channel_meaning_present");
                BS_End();
                if (HasExtend)
                {
                    int8u Extend;
                    BS_Begin();
                    Get_S1( 4, Extend,                          "extra_channel_meaning_length");
                    size_t After=(((size_t)Extend)+1)*16-4;
                    if (After>=Data_BS_Remain())
                        After=0;
                    else
                        After=Data_BS_Remain()-After;
                    if (HD_HasAtmos)
                    {
                        Element_Begin1("16ch_channel_meaning");
                        Get_S1 (5, presentations[p_16ch].dialogue_norm, "16ch_dialogue_norm");  Param_Info2(presentations[p_16ch].dialogue_norm == 0 ? -31 : -presentations[p_16ch].dialogue_norm, " LKFS");
                        Get_S1 (6, presentations[p_16ch].mix_level,     "16ch_mix_level");      Param_Info2(70 + presentations[p_16ch].mix_level, " dB");
                        Get_S1 (5, num_dynamic_objects,                 "16ch_channel_count");
                        num_dynamic_objects++;                                                  Param_Info2(num_dynamic_objects, " channels");
                        presentations[p_16ch].elements = num_dynamic_objects;
                        program_assignment();
                        Element_End0();
                    }
                    size_t RemainginBits=Data_BS_Remain();
                    if (RemainginBits>After)
                        Skip_BS(RemainginBits-After,            HD_HasAtmos ? "reserved" : "(Unparsed bits)");
                    BS_End();
                }
            }
            else
                Skip_B8(                                        "Unknown");
        Element_End0();

        int16u major_sync_info_CRC;
        Get_B2 (major_sync_info_CRC,                            "major_sync_info_CRC");

        FILLING_BEGIN();
            // major_sync_info CRC check
            //  MLP uses checksums that seem to be based on the standard CRC algorithm, but are not (in implementation terms, the table lookup and XOR are reversed).
            //  This behavior can be implemented using standard CRC on all but the last element, then XOR that with the last element. -FFmpeg
            if (!CRC_16_Table_HD) {
                CRC_16_Table_HD.reset(new int16u[256]);
                CRC16_Init(CRC_16_Table_HD.get(), 0x002D);
            }
            int16u CRC_16 = 0x0000;
            const int8u* CRC_16_Buffer = Buffer + Buffer_Offset;
            while (CRC_16_Buffer < Buffer + Buffer_Offset + Element_Offset - 4) {
                CRC_16 = (CRC_16 << 8) ^ CRC_16_Table_HD[(CRC_16 >> 8) ^ (*CRC_16_Buffer++)];
            }
            CRC_16 ^= BigEndian2int16u(Buffer + Buffer_Offset + Element_Offset - 4);
            Param_Info1(CRC_16 == major_sync_info_CRC ? "OK" : "NOK");
            MajorSyncOK = (CRC_16 == major_sync_info_CRC ? true : false);
        FILLING_END();

        Element_End0();
    }
    else if (!HD_MajorSync_Parsed)
    {
        return; // Wait for major sync
    }

    int64u PosBeforeDirectory=Element_Offset;
    bool crc_present{};
    std::vector<int16u> substream_end_ptrs;
    BS_Begin();
    for (int8u i=0; i<HD_SubStreams_Count; i++)
    {
        Element_Begin1("substream_directory");
        bool extra_substream_word, restart_nonexistent;
        int16u substream_end_ptr;
        Get_SB (extra_substream_word,                           "extra_substream_word");
        Get_SB (restart_nonexistent,                            "restart_nonexistent");
        if ((!restart_nonexistent && Synch!=0xF8726F) || (restart_nonexistent && Synch==0xF8726F))
        {
            Element_End0();
            return;
        }
        Get_SB (crc_present,                                    "crc_present");
        Skip_S1( 1,                                             "reserved");
        Get_S2 (12, substream_end_ptr,                          "substream_end_ptr");
        if (extra_substream_word)
        {
            Skip_S2(9,                                          "drc_gain_update");
            Skip_S1(3,                                          "drc_time_update");
            Skip_S1(4,                                          "reserved");
        }
        Element_End0();
        substream_end_ptrs.push_back(substream_end_ptr);
    }
    BS_End();

    FILLING_BEGIN();
        //CRC compute
        size_t Size=(size_t)(Element_Offset-PosBeforeDirectory);
        int8u crc=0;
        for (int8u i=0; i<4; i++)
        {
            int8u Value=Buffer[Buffer_Offset-4+i];
            crc^=(Value&0xF);
            Value>>=4;
            crc^=Value;
        }
        for (int8u i=0; i<Size; i++)
        {
            int8u Value=Buffer[Buffer_Offset+(size_t)(Element_Offset-Size+i)];
            crc^=(Value&0xF);
            Value>>=4;
            crc^=Value;
        }
        if (crc!=0xF || !MajorSyncOK)
        {
            return;
        }
        HD_MajorSync_Parsed=true;

        if (HD_SubStreams_Count==1 && HD_StreamType==0xBB) //MLP with only 1 stream
        {
            HD_Resolution2=HD_Resolution1;
            HD_SamplingRate2=HD_SamplingRate1;
        }
    FILLING_END();

    if (HD_MajorSync_Parsed && HD_StreamType == 0xBA) {
        auto substreams_begin_ptr = Element_Offset;
        for (int8u i = 0; i < HD_SubStreams_Count; ++i) {
            Element_Begin1("substream_segment");
            auto substream_segment_begin = Element_Offset;
            #if MEDIAINFO_TRACE
            BS_Begin();
            //bool last_block_in_segment;
            //do {
                Element_Begin1("block");
                bool block_header_exists;
                Get_SB(block_header_exists,                     "block_header_exists");
                if (block_header_exists) {
                    TEST_SB_SKIP(                               "restart_header");
                        auto restart_BeginRemain = Data_BS_Remain();
                        int8u max_matrix_chan{};
                        Skip_S2(14,                             "restart_sync_word");
                        Skip_S2(16,                             "output_timing");
                        Skip_S1( 4,                             "min_chan");
                        Skip_S1( 4,                             "max_chan");
                        Get_S1 ( 4, max_matrix_chan,            "max_matrix_chan");
                        Skip_S1( 4,                             "dither_shift");
                        Skip_S3(23,                             "dither_seed");
                        Skip_S1( 4,                             "max_shift");
                        Skip_S1( 5,                             "max_lsbs");
                        Skip_S1( 5,                             "max_bits");
                        Skip_S1( 5,                             "max_bits");
                        Skip_SB(                                "error_protect");
                        Skip_S1( 8,                             "lossless_check");
                        Skip_S2(16,                             "reserved");
                        for (int8u ch = 0; ch <= max_matrix_chan; ++ch) {
                            Skip_S1(6,                          ("ch_assign[" + std::to_string(ch) + "]").c_str());
                        }
                        auto restart_size = restart_BeginRemain - Data_BS_Remain();
                        int8u restart_header_CRC;
                        Get_S1 (8, restart_header_CRC,          "restart_header_CRC");
                        FILLING_BEGIN();
                        if (Trace_Activated) {
                            if (!CRC_8_1D_Table_HD) {
                                CRC_8_1D_Table_HD.reset(new int8u[256]);
                                CRC8_Init(CRC_8_1D_Table_HD.get(), 0x1D);
                            }
                            const int8u* CRC_8_Buffer = Buffer + Buffer_Offset + substream_segment_begin;
                            int8u CRC_8 = CRC_8_1D_Table_HD[*CRC_8_Buffer & 0x3F];
                            auto aligned_size = restart_size + 2;
                            auto num_bytes = aligned_size / 8;
                            for (int8u c = 1; c < num_bytes - 1; ++c) {
                                CRC_8 = CRC_8_1D_Table_HD[CRC_8 ^ CRC_8_Buffer[c]];
                            }
                            CRC_8 ^= CRC_8_Buffer[num_bytes - 1];
                            for (int8u c = 0; c < (aligned_size & 7); ++c) {
                                if (CRC_8 & 0x80)
                                    CRC_8 = (CRC_8 << 1) ^ 0x1D;
                                else
                                    CRC_8 <<= 1;
                                CRC_8 ^= (CRC_8_Buffer[num_bytes] >> (7 - c)) & 1;
                            }
                            Param_Info1(CRC_8 == restart_header_CRC ? "OK" : "NOK");
                        }
                        FILLING_END();
                    TEST_SB_END();
                    Element_Begin1("block_header");
                    // Not parsed, too much data
                    Element_End0();
                }
                //if (error_protect) {
                //    Skip_S2(16,                               "block_data_bits");
                //}
                //Element_Begin1("block_data");
                //Element_End0();
                //if (error_protect) {
                //    Skip_S1( 8,                               "block_header_CRC");
                //}
                //Get_SB(last_block_in_segment,                 "last_block_in_segment");
                Element_End0();
            //} while (!last_block_in_segment);

            // padding

            // if (last_access_unit_in_stream) {
            // ...
            // }

            BS_End();
            #endif // MEDIAINFO_TRACE
            Skip_XX(substreams_begin_ptr + substream_end_ptrs[i] * 2LL - (crc_present ? 2 : 0) - Element_Offset, "(Not parsed)");
            Element_End0();
            if (crc_present) {
                int8u substream_parity;
                Get_B1 (substream_parity,                       "substream_parity");
                #if MEDIAINFO_TRACE
                FILLING_BEGIN();
                if (Trace_Activated) {
                    auto size = Element_Offset - 1 - substream_segment_begin;
                    // The 8-bit substream_parity field is a parity check. The value of the substream_parity field is equal
                    // to the exclusive - OR of all the bytes in the expansion of substream_segment(), exclusive - ORed with
                    // the constant 0xA9 (the purpose of the latter being to force the check to fail in the event of the
                    // stream consisting entirely of zeroes).
                    int8u parity{};
                    for (int16u j = 0; j < size; ++j) {
                        int8u Value = Buffer[Buffer_Offset + substream_segment_begin + j];
                        parity ^= Value;
                    }
                    parity ^= 0xA9;
                    Param_Info1(parity == substream_parity ? "OK" : "NOK");
                }
                FILLING_END();
                #endif // MEDIAINFO_TRACE
                int8u substream_CRC;
                Get_B1(substream_CRC,                           "substream_CRC");
                #if MEDIAINFO_TRACE
                FILLING_BEGIN();
                if (Trace_Activated) {
                    if (!CRC_8_Table_HD) {
                        CRC_8_Table_HD.reset(new int8u[256]);
                        CRC8_Init(CRC_8_Table_HD.get(), 0x63);
                    }
                    int8u CRC_8 = CRC_8_Table_HD[0xA2];
                    const int8u* CRC_8_Buffer = Buffer + Buffer_Offset + substream_segment_begin;
                    while (CRC_8_Buffer < Buffer + Buffer_Offset + Element_Offset - 3) {
                        CRC_8 = CRC_8_Table_HD[CRC_8 ^ *CRC_8_Buffer++];
                    }
                    CRC_8 ^= BigEndian2int8u(Buffer + Buffer_Offset + Element_Offset - 3);
                    Param_Info1(CRC_8 == substream_CRC ? "OK" : "NOK");
                }
                FILLING_END();
                #endif // MEDIAINFO_TRACE
            }
        }
        if (Element_Size - Element_Offset >= 2) {
            Element_Begin1("EXTRA_DATA");
            int8u length_check_nibble;
            int16u EXTRA_DATA_length;
            BS_Begin();
            Get_S1( 4, length_check_nibble,                     "length_check_nibble");
            Get_S2(12, EXTRA_DATA_length,                       "EXTRA_DATA_length");
            BS_End();
            if (length_check_nibble == 0 && EXTRA_DATA_length == 0) {
                Skip_XX(Element_Size - Element_Offset,          "EXTRA_DATA_padding");
            }
            else if (((length_check_nibble ^ (EXTRA_DATA_length & 0x000F) ^ ((EXTRA_DATA_length >> 4) & 0x000F) ^ ((EXTRA_DATA_length >> 8) & 0x000F)) == 0xF)
                && (Element_Offset + EXTRA_DATA_length * 2LL) <= Element_Size) {
                int32u extra_data_size = EXTRA_DATA_length * 2 - 1;
                auto extra_data_end = Element_Offset + extra_data_size;
                bool ExtraDataParityOK{};
                int8u parity{};
                FILLING_BEGIN();
                    // The 8-bit EXTRA_DATA_parity field is a parity check. The value of the field is equal to the exclusive-
                    // OR of all the bytes calculated over all the bytes in the expansion of EXTRA_DATA(excluding
                    // length_check_nibble, EXTRA_DATA_length, and the EXTRA_DATA_parity fields), exclusive - ORed
                    // with the constant 0xA9 (the purpose of the latter being to force the check to fail in the event of the
                    // stream consisting entirely of zeroes).
                    for (int16u i = 0; i < extra_data_size; ++i) {
                        int8u Value = Buffer[Buffer_Offset + Element_Offset + i];
                        parity ^= Value;
                    }
                    parity ^= 0xA9;
                    if (parity == Buffer[Buffer_Offset + extra_data_end])
                        ExtraDataParityOK = true;
                FILLING_END();
                if (ExtraDataParityOK && (HD_flags & 0x1000)) {
                    Element_Begin1("emdf_frame");
                    int16u length;
                    BS_Begin();
                    Skip_S1( 4,                                 "reserved");
                    Get_S2 (12, length,                         "length");
                    BS_End();
                    if (length <= extra_data_size - 2) {
                        auto frame_begin = Element_Offset;
                        BS_Begin();
                        emdf_container();
                        BS_End(); 
                        Skip_XX(frame_begin + length - Element_Offset, "(Not parsed)");
                    }
                    Element_End0();
                    if (extra_data_end - Element_Offset == 1)
                        Skip_XX(1,                              "EXTRA_DATA_padding");
                    else
                        Skip_XX(extra_data_end - Element_Offset, "(Not parsed)");
                }
                else {
                    Skip_XX(extra_data_size,                    "data");
                }
                int8u EXTRA_DATA_parity;
                Get_B1 (EXTRA_DATA_parity,                      "EXTRA_DATA_parity");
                Param_Info1(parity == EXTRA_DATA_parity ? "OK" : "NOK");
            }
            else {
                Skip_XX(Element_Size - Element_Offset,          "(Data)");
            }
            Element_End0();
        }
    }

    /*
    if (HD_MajorSync_Parsed)
    {
        Element_Begin1("Sizes");
        std::vector<int16u> Sizes;
        for (int8u Pos=0; Pos<HD_SubStreams_Count; Pos++)
        {
            Element_Begin1("Size");
            int16u Size;
            bool HD_Unknown1_Present, HD_NoRestart, HD_ExtraParity;
            BS_Begin();
            Get_SB (    HD_Unknown1_Present,                    "Unknown present"); //Only TrueHD
            Get_SB (    HD_NoRestart,                           "No restart"); //Not present if MajorSync, present if no MajorSync
            Get_SB (    HD_ExtraParity,                         "Extra parity information");
            Skip_SB(                                            "Unknown");
            Get_S2 (12, Size,                                   "Size");
            BS_End();
            if (HD_Unknown1_Present)
                Skip_B2(                                        "Unknown");
            Sizes.push_back(Size);
            Element_End0();
        }
        Element_End0();

        int64u Element_Offset_Begin=Element_Offset;
        for (int8u Pos=0; Pos<HD_SubStreams_Count; Pos++)
        {
            Element_Begin1("Block");
            bool DecodingParameterBlockPresent;
            BS_Begin();
            Get_SB (DecodingParameterBlockPresent,              "Decoding parameter block is present");
            if (DecodingParameterBlockPresent)
            {
                TEST_SB_SKIP(                                   "Restart header");
                    int16u SyncWord;
                    int8u max_matrix_channel;
                    Get_S2(13, SyncWord,                        "SyncWord");
                    if (SyncWord==0x18F5)
                    {
                        Skip_SB(                                "noise_type"); //Only for TrueHD
                        Skip_S2(16,                             "Output timestamp");
                        Skip_S1( 4,                             "min_channel");
                        Skip_S1( 4,                             "max_channel");
                        Get_S1 ( 4, max_matrix_channel,         "max_matrix_channel");
                        Skip_S1( 4,                             "noise_shift");
                        Skip_S3(23,                             "noisegen_seed");
                        Skip_S3(19,                             "unknown");
                        Skip_SB(                                "data_check_present");
                        Skip_S1( 8,                             "lossless_check");
                        Skip_S2(16,                             "unknown");
                        for (int8u matrix_channel=0; matrix_channel<max_matrix_channel; matrix_channel++)
                            Skip_S1(6,                          "ch_assign");
                        Skip_S1( 8,                             "checksum");
                    }
                TEST_SB_END();
            }
            BS_End();
            Skip_XX(Element_Offset_Begin+Sizes[Pos]*2-Element_Offset, "Data");
            Element_End0();
        }
    }
    else
        Skip_XX(Element_Size-Element_Offset,                    "Waiting for MajorSync...");
    */
        Skip_XX(Element_Size-Element_Offset,                    "(Data)");

    FILLING_BEGIN_PRECISE();
        if (Frame_Count==0)
            PTS_Begin=FrameInfo.PTS;
        Frame_Count++;
        Frame_Count_InThisBlock++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
        FrameInfo.DUR=833333;
        int64u HD_SamplingRate=AC3_HD_SamplingRate(HD_SamplingRate1);
        if (HD_SamplingRate && HD_SamplingRate!=48000)
        {
            FrameInfo.DUR*=48000;
            FrameInfo.DUR/=HD_SamplingRate;
        }
        if (FrameInfo.DTS!=(int64u)-1)
            FrameInfo.DTS+=FrameInfo.DUR;
        if (FrameInfo.PTS!=(int64u)-1)
            FrameInfo.PTS+=FrameInfo.DUR;

        //Filling
        if (!Status[IsAccepted])
        {
            Accept("AC-3");
            if (Frame_Count_Valid>1 && Frame_Count_Valid<10000)
                Frame_Count_Valid*=32;
        }
        if (!Status[IsFilled] && !Core_IsPresent && Frame_Count>=Frame_Count_Valid)
        {
            Fill("AC-3");

            //No more need data
            if (!IsSub && Config->ParseSpeed<1.0)
                Finish("AC-3");
        }
    FILLING_END();
}

void File_Ac3::HD_format_info()
{
    if (HD_StreamType==0xBA)
    {
        Element_Begin1("format_info");
        int8u dsurexmod_6ch;
        BS_Begin();
        Get_S1 ( 4, HD_SamplingRate1,                           "audio_sampling_frequency"); Param_Info2(AC3_HD_SamplingRate(HD_SamplingRate1), " Hz");
        Skip_SB(                                                "6ch_multichannel_type");
        Skip_SB(                                                "8ch_multichannel_type");
        Skip_S1( 2,                                             "reserved");
        Get_S1 ( 2, presentations[p_2ch].modifier,              "2ch_presentation_channel_modifier");
        Get_S1 ( 2, dsurexmod_6ch,                              "6ch_presentation_channel_modifier");
        Get_S1 ( 5, HD_Channels1,                               "6ch_presentation_channel_assignment"); Param_Info1(AC3_TrueHD_Channels(HD_Channels1)); Param_Info1(Ztring().From_UTF8(AC3_TrueHD_Channels_Positions(HD_Channels1)));
        Get_S1 ( 2, dsurexmod,                                  "8ch_presentation_channel_modifier");
        Get_S2 (13, HD_Channels2,                               "8ch_presentation_channel_assignment"); Param_Info1(AC3_TrueHD_Channels(HD_Channels2)); Param_Info1(Ztring().From_UTF8(AC3_TrueHD_Channels_Positions(HD_Channels2)));
        BS_End();
        HD_Resolution2=HD_Resolution1=2; //24-bits
        HD_SamplingRate2=HD_SamplingRate1;
        if (dsurexmod_6ch && !(HD_Channels2 & (1 << 3)))
            dsurexmod_6ch = 0; //Only if Ls/Rs
        if (dsurexmod && !(HD_Channels2&(1<<3)))
            dsurexmod=0; //Only if Ls/Rs
        presentations[p_2ch].ch_assign = 1;
        presentations[p_6ch].ch_assign = HD_Channels1;
        presentations[p_8ch].ch_assign = HD_Channels2;
        presentations[p_6ch].modifier = dsurexmod_6ch;
        presentations[p_8ch].modifier = dsurexmod;
        Element_End0();
    }
    if (HD_StreamType==0xBB)
    {
        BS_Begin();
        Get_S1 ( 4, HD_Resolution1,                             "Resolution1"); Param_Info2(AC3_MLP_Resolution[HD_Resolution1], " bits");
        Get_S1 ( 4, HD_Resolution2,                             "Resolution2"); Param_Info2(AC3_MLP_Resolution[HD_Resolution2], " bits");
        Get_S1 ( 4, HD_SamplingRate1,                           "Sampling rate"); Param_Info2(AC3_HD_SamplingRate(HD_SamplingRate1), " Hz");
        Get_S1 ( 4, HD_SamplingRate2,                           "Sampling rate"); Param_Info2(AC3_HD_SamplingRate(HD_SamplingRate2), " Hz");
        Skip_S1(11,                                             "Unknown");
        Get_S1 ( 5, HD_Channels1,                               "Channels"); Param_Info1(AC3_MLP_Channels[HD_Channels1]);
        BS_End();
        HD_Channels2=HD_Channels1;
    }
}

//---------------------------------------------------------------------------
void File_Ac3::TimeStamp()
{
    // Format looks like a Sync word 0x0110 then SMPTE ST 339 Time stamp
    
    //Parsing
    int16u SampleNumber;
    int8u H1, H2, M1, M2, S1, S2, F1, F2, FrameRate;
    bool DropFrame;
    Skip_B2(                                                    "Sync word");
    BS_Begin();
    Skip_S2(10,                                                 "H");
    Get_S1 ( 2, H1,                                             "H");
    Get_S1 ( 4, H2,                                             "H");
    Skip_S2( 9,                                                 "M");
    Get_S1 ( 3, M1,                                             "M");
    Get_S1 ( 4, M2,                                             "M");
    Skip_S2( 9,                                                 "S");
    Get_S1 ( 3, S1,                                             "S");
    Get_S1 ( 4, S2,                                             "S");
    Skip_S2( 9,                                                 "F");
    Get_SB (    DropFrame,                                      "Drop frame");
    Get_S1 ( 2, F1,                                             "F");
    Get_S1 ( 4, F2,                                             "F");
    Get_S2 (16, SampleNumber,                                   "Sample number");
    Skip_S2( 9,                                                 "Unknown");
    Skip_SB(                                                    "Status");
    Get_S1 ( 4, FrameRate,                                      "Frame rate"); Param_Info1(Mpegv_frame_rate[FrameRate]);
    Skip_SB(                                                    "Status");
    Skip_SB(                                                    "Drop frame");
    BS_End();
    Skip_B2(                                                    "User private");

    FILLING_BEGIN();
        TimeCode Temp(H1*10+H2, M1*10+M2, S1*10+S2, F1*10+F2, (int32u)(float64_int64s(Mpegv_frame_rate[FrameRate])-1), TimeCode::DropFrame(DropFrame).FPS1001(float64_int64s(Mpegv_frame_rate[FrameRate])!=Mpegv_frame_rate[FrameRate]));
        #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            auto Temp_String=Temp.ToString();
            if (SampleNumber)
                Temp_String+='-'+to_string(SampleNumber);
            Element_Info1(Temp_String);
        }
        #endif //MEDIAINFO_TRACE
        if (TimeStamp_Count==0)
        {
            TimeStamp_FirstFrame=Temp;
            TimeStamp_FirstFrame_SampleNumber=SampleNumber;
        }
        TimeStamp_IsParsing=false;
        TimeStamp_Parsed=true;
        TimeStamp_Count++;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ac3::dac3()
{
    BS_Begin();
    Get_S1 (2, fscod,                                           "fscod");
    Get_S1 (5, bsid,                                            "bsid");
    Get_S1 (3, bsmod_Max[0][0],                                 "bsmod");
    Get_S1 (3, acmod_Max[0][0],                                 "acmod");
    Get_SB (   lfeon_Max[0][0],                                 "lfeon");
    Get_S1 (5, frmsizecod,                                      "bit_rate_code"); frmsizecod*=2;
    Skip_S1(5,                                                  "reserved");
    BS_End();

    MustParse_dac3=false;
    dxc3_Parsed=true;
}

//---------------------------------------------------------------------------
void File_Ac3::dec3()
{
    //Parsing
    BS_Begin();
    int8u num_ind_sub;
    Skip_S2(13,                                                 "data_rate");
    Get_S1 ( 3, num_ind_sub,                                    "num_ind_sub");
    for (int8u Pos=0; Pos<=num_ind_sub; Pos++)
    {
        Element_Begin1("independent substream");
        int8u num_dep_sub;
        Get_S1 (2, fscod,                                       "fscod");
        Get_S1 (5, bsid,                                        "bsid");
        if (bsid_Max<bsid || bsid_Max==(int8u)-1)
            bsid_Max=bsid;
        Skip_SB(                                                "reserved");
        Skip_SB(                                                "asvc");
        Get_S1 (3, bsmod_Max[Pos][0],                           "bsmod");
        Get_S1 (3, acmod_Max[Pos][0],                           "acmod");
        Get_SB (   lfeon_Max[Pos][0],                           "lfeon");
        Skip_S1(3,                                              "reserved");
        Get_S1 (4, num_dep_sub,                                 "num_dep_sub");
        if (num_dep_sub>0)
            Skip_S2(9,                                          "chan_loc");
        else
            Skip_SB(                                            "reserved");
        Element_End0();
    }
    if (Data_BS_Remain())
    {
        Skip_S1( 7,                                             "reserved");
        TEST_SB_SKIP(                                           "flag_ec3_extension_type_joc");
            Get_S1 ( 8, joc_complexity_index_Container,         "joc_complexity_index");
        TEST_SB_END();
    }
    BS_End();
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "reserved");

    MustParse_dec3=false;
    dxc3_Parsed=true;

    FILLING_BEGIN()
        Accept();
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Ac3::dmlp()
{
    //Parsing
    HD_StreamType=0xBA;
    HD_format_info();
    BS_Begin();
    Get_S2 (15, HD_BitRate_Max,                                 "peak_data_rate"); Param_Info2((HD_BitRate_Max*(AC3_HD_SamplingRate(HD_SamplingRate2)?AC3_HD_SamplingRate(HD_SamplingRate2):AC3_HD_SamplingRate(HD_SamplingRate1))+8)>>4, " bps");
    Skip_S8(33,                                                 "reserved");
    BS_End();
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "unknown");

    FILLING_BEGIN()
        //Configure
        MustParse_dmlp=false;
        MustSynchronize=true;
        Frame_Count_Valid=1;
    FILLING_ELSE()
        Reject();
    FILLING_END()
}

//---------------------------------------------------------------------------
bool File_Ac3::FrameSynchPoint_Test()
{
    if (Save_Buffer)
        return true; //Test already made by Synchronize()

    if (Buffer[Buffer_Offset  ]==0x0B
     && Buffer[Buffer_Offset+1]==0x77) //AC-3
    {
        bsid=CC1(Buffer+Buffer_Offset+5)>>3;
        int16u  Size=0;
        if (bsid<=0x09)
        {
            int8u fscod     =(CC1(Buffer+Buffer_Offset+4)>>6)&0x03;
            int8u frmsizecod=(CC1(Buffer+Buffer_Offset+4)   )&0x3F;
            Size=AC3_FrameSize_Get(frmsizecod, fscod);
        }
        else if (bsid>0x0A && bsid<=0x10)
        {
            int16u frmsiz=CC2(Buffer+Buffer_Offset+2)&0x07FF;
            Size=2+frmsiz*2;
        }
        if (Size>=6)
        {
            if (Buffer_Offset+Size>Buffer_Size)
                return false; //Need more data
            if (CRC_Compute(Size))
            {
                Synched=true;
                return true;
            }
        }
    }

    if (Buffer[Buffer_Offset+0]==0x77
     && Buffer[Buffer_Offset+1]==0x0B) //AC-3 LE
    {
        bsid=CC1(Buffer+Buffer_Offset+4)>>3;
        int16u  Size=0;
        if (bsid<=0x09)
        {
            int8u fscod     =(CC1(Buffer+Buffer_Offset+5)>>6)&0x03;
            int8u frmsizecod=(CC1(Buffer+Buffer_Offset+5)   )&0x3F;
            Size=AC3_FrameSize_Get(frmsizecod, fscod);
        }
        else if (bsid>0x0A && bsid<=0x10)
        {
            int16u frmsiz=LittleEndian2int16u(Buffer+Buffer_Offset+2)&0x07FF;
            Size=2+frmsiz*2;

            //TODO: case with multiple substreams
        }
        if (Size>=6)
        {
            size_t Size_Total=Core_Size_Get();
            if (Element_IsWaitingForMoreData() || Buffer_Offset+Size_Total>=Buffer_Size)
                return false; //Need more data

            Save_Buffer=Buffer;
            Save_Buffer_Offset=Buffer_Offset;
            Save_Buffer_Size=Buffer_Size;

            {
                int8u* Buffer_Little=new int8u[Size_Total];
                for (size_t Pos=0; Pos+1<Size_Total; Pos+=2)
                {
                    Buffer_Little[Pos+1]=Save_Buffer[Buffer_Offset+Pos  ];
                    Buffer_Little[Pos  ]=Save_Buffer[Buffer_Offset+Pos+1];
                }
                Buffer=Buffer_Little;
                Buffer_Offset=0;
                Buffer_Size=Size_Total;

                Synched=CRC_Compute(Size);

                if (Synched)
                {
                    BigEndian=false;

                    swap(Buffer, Save_Buffer);
                    swap(Buffer_Offset, Save_Buffer_Offset);
                    swap(Buffer_Size, Save_Buffer_Size);

                    return true;
                }

                delete[] Buffer_Little;
            }
            Buffer=Save_Buffer; Save_Buffer=NULL;
            Buffer_Offset=Save_Buffer_Offset;
            Buffer_Size=Save_Buffer_Size;
        }
    }

    if (HD_MajorSync_Parsed
     || (Buffer[Buffer_Offset+4]==0xF8
      && Buffer[Buffer_Offset+5]==0x72
      && Buffer[Buffer_Offset+6]==0x6F
      && (Buffer[Buffer_Offset+7]&0xFE)==0xBA)) //TrueHD or MLP
    {
        HD_IsPresent=true;
        Synched=true;
        return true;
    }

    Synched=false;
    return true;
}

//---------------------------------------------------------------------------
bool File_Ac3::CRC_Compute(size_t Size)
{
    //Config
    if (!IgnoreCrc_Done)
    {
        IgnoreCrc=Config->File_Ac3_IgnoreCrc_Get();
        IgnoreCrc_Done=true;
    }
    if (IgnoreCrc && !Status[IsAccepted]) //Else there are some wrong synchronizations
    {
        MediaInfo_Internal MI;
        Ztring ParseSpeed_Save=MI.Option(__T("ParseSpeed_Get"), __T(""));
        Ztring Demux_Save=MI.Option(__T("Demux_Get"), __T(""));
        MI.Option(__T("ParseSpeed"), __T("0"));
        MI.Option(__T("Demux"), Ztring());
        size_t MiOpenResult=MI.Open(File_Name);
        MI.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
        MI.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
        if (MiOpenResult)
        {
            Ztring Format=MI.Get(Stream_General, 0, General_Format);
            if (Format!=__T("AC-3") && Format!=__T("E-AC-3"))
                IgnoreCrc=false;
        }
        else
            IgnoreCrc=false; // Problem
    }
    if (IgnoreCrc)
        return true;

    int16u CRC_16=0x0000;
    const int8u* CRC_16_Buffer=Buffer+Buffer_Offset+2; //After syncword
    const int8u* CRC_16_Buffer_5_8=Buffer+Buffer_Offset+(((Size>>2)+(Size>>4))<<1); //Magic formula to meet 5/8 frame size from Dolby
    const int8u* CRC_16_Buffer_EndMinus3=Buffer+Buffer_Offset+Size-3; //End of frame minus 3
    const int8u* CRC_16_Buffer_End=Buffer+Buffer_Offset+Size; //End of frame
    while(CRC_16_Buffer<CRC_16_Buffer_End)
    {
        CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^(*CRC_16_Buffer)];

        //CRC bytes inversion
        if (CRC_16_Buffer==CRC_16_Buffer_EndMinus3 && bsid<=0x09 && ((*CRC_16_Buffer)&0x01)) //CRC inversion bit
        {
            CRC_16_Buffer++;
            CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^((int8u)(~(*CRC_16_Buffer)))];
            CRC_16_Buffer++;
            CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^((int8u)(~(*CRC_16_Buffer)))];
        }

        CRC_16_Buffer++;

        //5/8 intermediate test
        if (CRC_16_Buffer==CRC_16_Buffer_5_8 && bsid<=0x09 && CRC_16!=0x0000)
            break;
    }

    return (CRC_16==0x0000);
}

//---------------------------------------------------------------------------
size_t File_Ac3::Core_Size_Get()
{
    bool IsLE=(Buffer[Buffer_Offset+0]==0x77);

    int16u Size=1;
    bsid=(Buffer[(size_t)(Buffer_Offset+5-IsLE)]&0xF8)>>3;
    if (bsid<=0x09)
    {
        int8u fscod     =(Buffer[(size_t)(Buffer_Offset+4+IsLE)]&0xC0)>>6;
        int8u frmsizecod= Buffer[(size_t)(Buffer_Offset+4+IsLE)]&0x3F;

        //Filling
        fscods[fscod]++;
        frmsizecods[frmsizecod]++;
        Size=AC3_FrameSize_Get(frmsizecod, fscod);
    }
    else if (bsid>0x0A && bsid<=0x10)
    {
        int16u frmsiz    =((int16u)(Buffer[(size_t)(Buffer_Offset+2+IsLE)]&0x07)<<8)
                        | (         Buffer[(size_t)(Buffer_Offset+3-IsLE)]         );

        //Filling
        Size=2+frmsiz*2;
    }

        substreams_Count=0;
        int8u substreams_Count_Independant=0;
        int8u substreams_Count_Dependant=0;

        for (;;)
        {
            if (Buffer_Offset+Size+6>Buffer_Size)
            {
                if (!IsSub && !Save_Buffer && File_Offset+Buffer_Offset+Size<File_Size)
                    Element_WaitForMoreData();
                break;
            }

            int8u bsid=Buffer[Buffer_Offset+Size+5-IsLE]>>3;
            if (bsid<=0x09 || bsid>0x10)
                break; //Not E-AC-3

            int8u substreamid=(Buffer[Buffer_Offset+Size+2+IsLE]>>3)&0x07;
            if (substreamid!=substreams_Count_Independant)
                break; //Problem
            if (substreamid!=substreams_Count_Dependant)
                break; //Problem

            int8u strmtyp = Buffer[Buffer_Offset+Size+2+IsLE]>>6;
            if (substreamid==0 && strmtyp==0)
                break; //Next block

            int16u frmsiz =((int16u)(Buffer[(size_t)(Buffer_Offset+Size+2+IsLE)]&0x07)<<8)
                          | (         Buffer[(size_t)(Buffer_Offset+Size+3-IsLE)]         );

            //Filling
            Size+=2+frmsiz*2;

            if (strmtyp == 0)
            {
                substreams_Count_Independant++;
                substreams_Count_Dependant=0;
            }
            else
                substreams_Count_Dependant++;
            substreams_Count++;
        }

    return Size;
}

//---------------------------------------------------------------------------
size_t File_Ac3::HD_Size_Get()
{
    size_t Size=BigEndian2int16u(Buffer+Buffer_Offset)&0x0FFF;
    Size*=2;

    return Size;
}

//---------------------------------------------------------------------------
void File_Ac3::Get_V4(int8u  Bits, int32u  &Info, const char* Name)
{
    Info = 0;

#if MEDIAINFO_TRACE
    int8u Count = 0;
#endif //MEDIAINFO_TRACE
    for (;;)
    {
        Info += BS->Get4(Bits);
#if MEDIAINFO_TRACE
        Count += Bits;
#endif //MEDIAINFO_TRACE
        if (!BS->GetB())
            break;
        Info <<= Bits;
        Info += (1 << Bits);
    }
#if MEDIAINFO_TRACE
    if (Trace_Activated)
    {
        Param(Name, Info, Count);
        Param_Info(__T("(") + Ztring::ToZtring(Count) + __T(" bits)"));
    }
#endif //MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Ac3::Skip_V4(int8u  Bits, const char* Name)
{
#if MEDIAINFO_TRACE
    if (Trace_Activated)
    {
        int32u Info = 0;
        Get_V4(Bits, Info, Name);
    }
    else
#endif //MEDIAINFO_TRACE
    {
        do
            BS->Skip(Bits);
        while (BS->GetB());
    }
}

} //NameSpace

#endif //MEDIAINFO_AC3_YES
