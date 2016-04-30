/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// http://www.ffmpeg.org/~michael/ffv1.html
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
#if defined(MEDIAINFO_FFV1_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Ffv1.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "ZenLib/BitStream.h"
//---------------------------------------------------------------------------

#include <algorithm>
#include <math.h>
using namespace std;

//---------------------------------------------------------------------------
namespace MediaInfoLib
{

//***************************************************************************
// Const
//***************************************************************************

extern int32u Psi_CRC_32_Table[256];
const int32u Slice::Context::N0 = 128;
const int32s Slice::Context::Cmax = 127;
const int32s Slice::Context::Cmin = -128;

//***************************************************************************
// RangeCoder
//***************************************************************************

//---------------------------------------------------------------------------
RangeCoder::RangeCoder (const int8u* Buffer, size_t Buffer_Size, const state_transitions default_state_transition)
{
    //Assign buffer
    Buffer_Cur=Buffer;
    Buffer_End=Buffer+Buffer_Size;

    //Init
    if (Buffer_Size>=2)
    {
        Current=BigEndian2int16u(Buffer_Cur);
        Buffer_Cur+=2;
        Mask=0xFF00;
    }
    else
    {
        Current=0;
        Mask=0;
    }

    AssignStateTransitions(default_state_transition);
}

void RangeCoder::AssignStateTransitions (const state_transitions new_state_transition)
{
    //Assign StateTransitions
    std::memcpy (one_state, new_state_transition, state_transitions_size);
    zero_state[0]=0;
    for (size_t i=1; i<state_transitions_size; i++)
        zero_state[i]=-one_state[state_transitions_size-i];
}

//---------------------------------------------------------------------------
bool RangeCoder::get_rac(int8u* States)
{
    //Here is some black magic... But it works. TODO: better understanding of the algorithm and maybe optimization
    int16u Mask2=(int16u)((((int32u)Mask) * (*States)) >> 8);
    Mask-=Mask2;
    bool Value;
    if (Current<Mask)
    {
        *States=zero_state[*States];
        Value=false;
    }
    else
    {
        Current-=Mask;
        Mask=Mask2;
        *States=one_state[*States];
        Value=true;
    }

    // Next byte
    if (Mask<0x100)
    {
        Mask<<=8;
        Current<<=8;
        if (Buffer_Cur < Buffer_End) // Set to 0 if end of stream. TODO: Find a way to detect buffer underrun
        {
            Current|=*Buffer_Cur;
            Buffer_Cur++;
        }
    }

    return Value;
}

//---------------------------------------------------------------------------
int32u RangeCoder::get_symbol_u(int8u* States)
{
    if (get_rac(States))
        return 0;

    int8u e=0;
    while (get_rac((States+1+min(e, (int8u)9)))) // 1..10
        e++;

    int8u a=1;
    if (e)
    {
        do
        {
            --e;
            a<<=1;
            if (get_rac((States+22+min(e, (int8u)9))))  // 22..31
                ++a;
        }
        while (e);
    }
    return a;
}

//---------------------------------------------------------------------------
int32s RangeCoder::get_symbol_s(int8u* States)
{
    if (get_rac(States))
        return 0;

    int8u e=0;
    while (get_rac(States+1+min(e, (int8u)9))) // 1..10
        e++;

    int32u a=1;
    if (e)
    {
        int8u i = e;
        do
        {
            --i;
            a<<=1;
            if (get_rac((States+22+min(i, (int8u)9))))  // 22..31
                ++a;
        }
        while (i);
    }

    if (get_rac((States+11+min(e, (int8u)10)))) // 11..21
        return -((int32s)a);
    else
        return a;
}

//***************************************************************************
// Info
//***************************************************************************

const char* Ffv1_coder_type(int8u coder_type)
{
    switch (coder_type)
    {
        case 0 :
                return "Golomb Rice";
        case 1 :
        case 2 :
                return "Range Coder";
        default:
                return "";
    }
}

const string Ffv1_colorspace_type(int8u colorspace_type, bool chroma_planes, bool alpha_plane)
{
    string ToReturn;
    switch (colorspace_type)
    {
        case 0 :
                    ToReturn=chroma_planes?"YUV":"Y";
                    break;
        case 1 :    ToReturn="RGB"; break;
        default:    return string();
    }

    if (alpha_plane)
        ToReturn+='A';

    return ToReturn;
}

const char* Ffv1_picture_structure_ScanType (int8u picture_structure)
{
    switch (picture_structure)
    {
        case 1 :
        case 2 : return "Interlaced";
        case 3 : return "Progressive";
        default: return "";
    }
}

const char* Ffv1_picture_structure_ScanOrder (int8u picture_structure)
{
    switch (picture_structure)
    {
        case 1 : return "TFF";
        case 2 : return "BFF";
        default: return "";
    }
}

const state_transitions Ffv1_default_state_transition =
{
      0,  0,  0,  0,  0,  0,  0,  0, 20, 21, 22, 23, 24, 25, 26, 27,
     28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 37, 38, 39, 40, 41, 42,
     43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57,
     58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
     74, 75, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
     89, 90, 91, 92, 93, 94, 94, 95, 96, 97, 98, 99,100,101,102,103,
    104,105,106,107,108,109,110,111,112,113,114,114,115,116,117,118,
    119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,133,
    134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,
    150,151,152,152,153,154,155,156,157,158,159,160,161,162,163,164,
    165,166,167,168,169,170,171,171,172,173,174,175,176,177,178,179,
    180,181,182,183,184,185,186,187,188,189,190,190,191,192,194,194,
    195,196,197,198,199,200,201,202,202,204,205,206,207,208,209,209,
    210,211,212,213,215,215,216,217,218,219,220,220,222,223,224,225,
    226,227,227,229,229,230,231,232,234,234,235,236,237,238,239,240,
    241,242,243,244,245,246,247,248,248,  0,  0,  0,  0,  0,  0,  0,
};

// Coming from FFv1 spec.
const int8u log2_run[41]={
    0 , 0, 0, 0, 1, 1, 1, 1,
    2 , 2, 2, 2, 3, 3, 3, 3,
    4 , 4, 5, 5, 6, 6, 7, 7,
    8 , 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,
    24,
};

//***************************************************************************
// Slice
//***************************************************************************

//---------------------------------------------------------------------------
void Slice::contexts_clean()
{
    for (size_t i = 0; i < MAX_PLANES; i++)
    {
        if (contexts[i])
            delete[] contexts[i];
    }
}

//---------------------------------------------------------------------------
void Slice::contexts_init(int32u plane_count, int32u quant_table_index[MAX_PLANES], int32u context_count[MAX_QUANT_TABLES])
{
    contexts_clean();

    for (size_t i = 0; i < MAX_PLANES; ++i)
    {
        if (i >= plane_count)
        {
            contexts[i] = NULL;
            continue;
        }
        int32u idx = quant_table_index[i];
        contexts[i] = new Context [context_count[idx]];
        for (size_t j = 0; j < context_count[idx]; j++)
        {
            Context *c = &contexts[i][j];

            c->N = 1;
            //c->A = (alpha + 32) / 64; // alpha == alphabet size (256)
            //if (c->A < 2)
            //    c->A = 2;
            c->A = 4;
            c->C = 0;
            c->B = c->C;
        }
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ffv1::File_Ffv1()
:File__Analyze()
{
    //Configuration
    ParserName=__T("FFV1");
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    IsRawStream=true;

    //use Ffv1_default_state_transition by default
    memcpy(state_transitions_table, Ffv1_default_state_transition,
           sizeof(Ffv1_default_state_transition));

    //Input
    Width = (int32u)-1;
    Height = (int32u)-1;

    //Temp
    for (size_t i=0; i < MAX_QUANT_TABLES; ++i)
        plane_states[i] = NULL;
    ConfigurationRecordIsPresent=false;
    RC=NULL;
    version=0;
    num_h_slices = 1;
    num_v_slices = 1;
    slices = NULL;
    picture_structure = (int32u)-1;
    sample_aspect_ratio_num = 0;
    sample_aspect_ratio_den = 0;
}

//---------------------------------------------------------------------------
File_Ffv1::~File_Ffv1()
{
    //Temp
    if (slices)
    {
        for (size_t y = 0; y < num_v_slices; ++y)
            for (size_t x = 0; x < num_h_slices; ++x)
                plane_states_clean(slices[x + y * num_h_slices].plane_states);
        delete[] slices;
    }
    for (size_t i = 0; i < MAX_QUANT_TABLES; ++i)
    {
        if (!plane_states[i])
            continue;
        for (size_t j = 0; j < context_count[i]; ++j)
            delete[] plane_states[i][j];
        delete[] plane_states[i];
        plane_states[i] = NULL;
    }
    delete RC; //RC=NULL
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ffv1::Streams_Accept()
{
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "FFV1");
}

//***************************************************************************
// RangeCoder
//***************************************************************************

#if MEDIAINFO_TRACE
//---------------------------------------------------------------------------
void File_Ffv1::Get_RB (states &States, bool &Info, const char* Name)
{
    Info=RC->get_rac(States);

    Element_Offset=RC->Buffer_Cur-Buffer;
    if (Trace_Activated)
        Param(Name, Info);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RU (states &States, int32u &Info, const char* Name)
{
    Info=RC->get_symbol_u(States);

    if (Trace_Activated)
        Param(Name, Info);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RS (states &States, int32s &Info, const char* Name)
{
    Info=RC->get_symbol_s(States);

    if (Trace_Activated)
        Param(Name, Info);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RS (int8u* &States, int32s &Info, const char* Name)
{
    Info=RC->get_symbol_s(States);

    Element_Offset=RC->Buffer_Cur-Buffer;
    if (Trace_Activated)
        Param(Name, Info);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RC (states &States, const char* Name)
{
    int8u Info=RC->get_rac(States);

    Element_Offset=RC->Buffer_Cur-Buffer;
    if (Trace_Activated)
        Param(Name, Info);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RU (states &States, const char* Name)
{
    if (Trace_Activated)
        Param(Name, RC->get_symbol_u(States));
    else
        RC->get_symbol_u(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RS (states &States, const char* Name)
{
    if (Trace_Activated)
        Param(Name, RC->get_symbol_s(States));
    else
        RC->get_symbol_s(States);
}

#else //MEDIAINFO_TRACE
//---------------------------------------------------------------------------
void File_Ffv1::Get_RB_ (states &States, bool &Info)
{
    Info=RC->get_rac(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RU_ (states &States, int32u &Info)
{
    Info=RC->get_symbol_u(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RS_ (states &States, int32s &Info)
{
    Info=RC->get_symbol_s(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RS_ (int8u* &States, int32s &Info)
{
    Info=RC->get_symbol_s(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RC_ (states &States)
{
    RC->get_rac(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RU_ (states &States)
{
    RC->get_symbol_u(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RS_ (states &States)
{
    RC->get_symbol_s(States);
}

#endif //MEDIAINFO_TRACE

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ffv1::Read_Buffer_OutOfBand()
{
    ConfigurationRecordIsPresent=true;

    int32u CRC_32=CRC_Compute((size_t)Element_Size);

    if (Buffer_Size < 4 || CRC_32)
    {
        Reject();
        return;
    }

    if (!RC)
        RC = new RangeCoder(Buffer, Buffer_Size-4, Ffv1_default_state_transition);

    FrameHeader();
    if (RC->Buffer_End!=RC->Buffer_Cur)
        Skip_XX(RC->Buffer_Cur - RC->Buffer_End,                "Reserved");
    Skip_B4(                                                    "CRC-32");

    delete RC; RC=NULL;
}

//---------------------------------------------------------------------------
void File_Ffv1::Read_Buffer_Continue()
{
    if (!Status[IsAccepted])
        Accept();

    if (!RC)
        RC = new RangeCoder(Buffer, Buffer_Size, Ffv1_default_state_transition);

    states States;
    memset(States, 128, states_size);

    Get_RB (States, keyframe,                                   "keyframe");

    if (!ConfigurationRecordIsPresent)
        FrameHeader();

    if (version>2)
    {
        int32u tail = 3;
        tail += error_correction == 1 ? 5 : 0;

        int64u Slices_BufferPos=Element_Size;
        vector<int32u> Slices_BufferSizes;
        while (Slices_BufferPos)
        {
            int32u Size = BigEndian2int24u(Buffer + Buffer_Offset + (size_t)Slices_BufferPos - tail);
            Size += tail;

            if (Slices_BufferPos < Size)
                Slices_BufferPos = Size;
            Slices_BufferPos-=Size;

            Slices_BufferSizes.insert(Slices_BufferSizes.begin(), Size);
        }

        Element_Offset=0;
        for (size_t Pos = 0; Pos < Slices_BufferSizes.size(); Pos++)
        {
            Element_Begin1("Slice");
            int64u End=Element_Offset+Slices_BufferSizes[Pos]-tail;
            int32u crc_left=0;

            if (error_correction == 1)
                crc_left=CRC_Compute(Slices_BufferSizes[Pos]);

            if (Pos)
            {
                delete RC; RC = new RangeCoder(Buffer+Buffer_Offset+(size_t)Element_Offset, Slices_BufferSizes[Pos], state_transitions_table);
            }
            else // ac=2
                RC->AssignStateTransitions(state_transitions_table);

            #if MEDIAINFO_TRACE
                if (!Frame_Count || Trace_Activated) // Parse slice only if trace feature is activated
                {
                    int64u Start=Element_Offset;

                    slice(States);

                    int64u SliceRealSize=Element_Offset-Start;
                    Element_Offset=Start;
                    Skip_XX(SliceRealSize,                          "slice_data");
                    if (Trusted_Get())
                        Param_Info1("OK");
                    else
                        Param_Info1("NOK");
                }
            #endif //MEDIAINFO_TRACE

            if (Element_Offset!=End)
                Skip_XX(End-Element_Offset,                         "Other data");
            Skip_B3(                                                "slice_size");
            if (error_correction == 1)
            {
                Skip_B1(                                                "error_status");
                Skip_B4(                                                "crc_parity");
                if (!crc_left)
                    Param_Info1("OK");
                else
                    Param_Info1("NOK");
            }
            Element_End0();
        }
    }

    FILLING_BEGIN();
        if (Frame_Count==0)
        {
            Fill(Stream_Video, 0, Video_ScanType, Ffv1_picture_structure_ScanType(picture_structure));
            Fill(Stream_Video, 0, Video_ScanOrder, Ffv1_picture_structure_ScanOrder(picture_structure));
            if (sample_aspect_ratio_num && sample_aspect_ratio_den)
                Fill(Stream_Video, 0, Video_PixelAspectRatio, sample_aspect_ratio_num/sample_aspect_ratio_den);
        }

        Frame_Count++;
    FILLING_END();

    delete RC;
    RC = NULL;

    Fill();
    if (Config->ParseSpeed<1.0)
        Finish();
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ffv1::FrameHeader()
{
    //Parsing
    states States;
    memset(States, 128, states_size);
    int32u coder_type, colorspace_type, bits_per_raw_sample=8, num_h_slices_minus1=0, num_v_slices_minus1=0, intra;

    micro_version = 0;
    Get_RU (States, version,                                    "version");
    if (( ConfigurationRecordIsPresent && version<=1)
     || (!ConfigurationRecordIsPresent && version> 1))
    {
        Trusted_IsNot("Invalid version in global header");
        return;
    }
    if (version>2)
        Get_RU (States, micro_version,                          "micro_version");
    Get_RU (States, coder_type,                                 "coder_type");
    this->coder_type = coder_type;
    if (coder_type == 2) //Range coder with custom state transition table
    {
        Element_Begin1("state_transition_deltas");
        for (int16u i = 1; i < state_transitions_size; i++)
        {
            int32s state_transition_delta;
            Get_RS (States, state_transition_delta,             "state_transition_delta");
            state_transitions_table[i]=state_transition_delta+RC->one_state[i];
            Param_Info1(state_transitions_table[i]);
        }
        Element_End0();
    }
    Get_RU (States, colorspace_type,                            "colorspace_type");
    this->colorspace_type = colorspace_type;
    if (version)
    {
        Get_RU (States, bits_per_raw_sample,                    "bits_per_raw_sample");
        if (bits_per_raw_sample==0)
            bits_per_raw_sample=8; //I don't know the reason, 8-bit is coded 0 and 10-bit coded 10 (not 2?).
        this->bits_per_sample = bits_per_raw_sample;
    }
    Get_RB (States, chroma_planes,                              "chroma_planes");
    Get_RU (States, chroma_h_shift,                             "log2(h_chroma_subsample)");
    Get_RU (States, chroma_v_shift,                             "log2(v_chroma_subsample)");
    Get_RB (States, alpha_plane,                                "alpha_plane");
    if (version>1)
    {
        Get_RU (States, num_h_slices_minus1,                    "num_h_slices_minus1");
        Get_RU (States, num_v_slices_minus1,                    "num_v_slices_minus1");
        num_h_slices = num_h_slices_minus1 + 1;
        num_v_slices = num_v_slices_minus1 + 1;
        Get_RU (States, quant_table_count,                      "quant_table_count");
    }
    else
        quant_table_count=1;

    if (!slices)
    {
        size_t nb_slices = (num_h_slices_minus1 + 1) * (num_v_slices_minus1 + 1);
        slices = new Slice[nb_slices];
        current_slice = &slices[0];
    }

    for (size_t i = 0; i < quant_table_count; i++)
        read_quant_tables(i);
    memset(quant_tables+quant_table_count, 0x00, (MAX_QUANT_TABLES-quant_table_count)*MAX_CONTEXT_INPUTS*256*sizeof(int16s));

    for (size_t i = 0; i < quant_table_count; i++)
    {
        Element_Begin1("initial_state");
        bool present;
        Get_RB (States, present,                                "present");

        if (coder_type)
            plane_states[i] = new int8u* [context_count[i]];

        for (size_t j = 0; j < context_count[i]; j++)
        {
            if (coder_type)
                plane_states[i][j] = new int8u [states_size];
            if (present)
            {
                Element_Begin1("initial_state");
                for (size_t k = 0; k < states_size; k++)
                {
                    int32s value;
                    Get_RS (States, value,                  "value");
                    if (coder_type)
                        plane_states[i][j][k] = value;
                }
                Element_End0();
            }
            else if (coder_type)
            {
                for (size_t k = 0; k < states_size; k++)
                    plane_states[i][j][k] = 128;
            }
        }
        Element_End0();
    }

    if (version > 2)
    {
        Get_RU (States, error_correction,                       "ec");
        if (micro_version)
            Get_RU (States, intra,                              "intra");
    }

    FILLING_BEGIN();
        if (Frame_Count==0)
        {
            Accept();

            Ztring Version=__T("Version ")+Ztring::ToZtring(version);
            if (version>2)
            {
                Version+=__T('.');
                Version+=Ztring::ToZtring(micro_version);
            }
            Fill(Stream_Video, 0, "coder_type", Ffv1_coder_type(coder_type));
            Fill(Stream_Video, 0, Video_Format_Version, Version);
            Fill(Stream_Video, 0, Video_BitDepth, bits_per_raw_sample);
            if (version > 1)
            {
                Fill(Stream_Video, 0, "MaxSlicesCount", (num_h_slices_minus1+1)*(num_v_slices_minus1+1));
            }
            if (version > 2)
            {
                if (error_correction)
                    Fill(Stream_Video, 0, "ErrorDetectionType", "Per slice");
                if (micro_version && intra)
                    Fill(Stream_Video, 0, Video_Format_Settings_GOP, "N=1");
            }
            Fill(Stream_Video, 0, Video_ColorSpace, Ffv1_colorspace_type(colorspace_type, chroma_planes, alpha_plane));
            if (colorspace_type==0 && chroma_planes)
            {
                string ChromaSubsampling;
                switch (chroma_h_shift)
                {
                    case 0 :
                            switch (chroma_v_shift)
                            {
                                case 0 : ChromaSubsampling="4:4:4"; break;
                                default: ;
                            }
                            break;
                    case 1 :
                            switch (chroma_v_shift)
                            {
                                case 0 : ChromaSubsampling="4:2:2"; break;
                                case 1 : ChromaSubsampling="4:2:0"; break;
                                default: ;
                            }
                            break;
                    case 2 :
                            switch (chroma_v_shift)
                            {
                                case 0 : ChromaSubsampling="4:1:1"; break;
                                case 1 : ChromaSubsampling="4:1:0"; break;
                                case 2 : ChromaSubsampling="4:1:0 (4x4)"; break;
                                default: ;
                            }
                            break;
                    default: ;
                }
                if (!ChromaSubsampling.empty() && alpha_plane)
                    ChromaSubsampling+=":4";
                Fill(Stream_Video, 0, Video_ChromaSubsampling, ChromaSubsampling);
            }
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ffv1::slice(states &States)
{
    if (version>2)
    {
        slice_header(States);

    }

    Trace_Activated = false; // Trace is too huge, deactivating it during pixel decoding

    if (!coder_type)
    {
        if ((version == 3 && micro_version > 1) || version > 3)
        {
            states States;
            memset(States, 129, states_size);
            Skip_RC(States,                                     "?");

            if ((version > 2 || (!current_slice->x && !current_slice->y)))
                Element_Offset--;
            else
                Element_Offset=0;
            BS_Begin();
        }
    }

    if (keyframe)
    {
        int8u plane_count=1+(alpha_plane?1:0);
        if (version < 4 || chroma_planes) // Warning: chroma is considered as 1 plane
            plane_count+=1;
        if (!coder_type)
            current_slice->contexts_init(plane_count, quant_table_index, context_count);
        else
            copy_plane_states_to_slice(plane_count);
    }
    current_slice->sample_buffer_new((current_slice->w + 6) * 3 * MAX_PLANES);

    if (colorspace_type == 0)
    {
        // YCbCr
        plane(0); // Y
        if (chroma_planes)
        {
            int32u w = current_slice->w;
            int32u h = current_slice->h;

            current_slice->w = current_slice->w >> chroma_h_shift;
            current_slice->h = current_slice->h >> chroma_v_shift;
            plane(1); // Cb
            plane(1); // Cr
            current_slice->w = w;
            current_slice->h = h;
        }
        if (alpha_plane)
            plane(2); // Alpha
    }
    else if (colorspace_type == 1)
        rgb();

    if (!coder_type && ((version == 3 && micro_version > 1) || version > 3))
        BS_End();

    if (coder_type && version > 2)
    {
        int8u s = 129;
        RC->get_rac(&s);
        Element_Offset=RC->Buffer_Cur-Buffer;
        Element_Offset--;
    }

    Trace_Activated = true; // Trace is too huge, reactivating after during pixel decoding
}

//---------------------------------------------------------------------------
void File_Ffv1::slice_header(states &States)
{
    Element_Begin1("SliceHeader");
    
    memset(States, 128, states_size);

    int32u slice_x, slice_y, slice_width, slice_height;
    Get_RU (States, slice_x,                                "slice_x");
    Get_RU (States, slice_y,                                "slice_y");
    Get_RU (States, slice_width,                            "slice_width_minus1");
    Get_RU (States, slice_height,                           "slice_height_minus1");

    current_slice = &slices[slice_x + slice_y * num_h_slices];

    current_slice->w = (slice_width + 1) * (Width / num_h_slices);
    current_slice->h = (slice_height + 1) * (Height / num_v_slices);
    current_slice->x = slice_x * current_slice->w;
    current_slice->y = slice_y * current_slice->h;


    int8u plane_count=1+(alpha_plane?1:0);
    if (version < 4 || chroma_planes) // Warning: chroma is considered as 1 plane
        plane_count += 1;
    for (int8u i = 0; i < plane_count; i++)
        Get_RU (States, quant_table_index[i],               "quant_table_index");
    Get_RU (States, picture_structure,                      "picture_structure");
    Get_RU (States, sample_aspect_ratio_num,                "sample_aspect_ratio num");
    Get_RU (States, sample_aspect_ratio_den,                "sample_aspect_ratio den");
    if (version > 3)
    {
        //TODO
    }

    RC->AssignStateTransitions(state_transitions_table);

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ffv1::plane(int32u pos)
{
    Element_Begin1("Plane");

    int16s *sample[2];
    sample[0] = current_slice->sample_buffer + 3;
    sample[1] = current_slice->sample_buffer + current_slice->w + 6 + 3;

    memset(current_slice->sample_buffer, 0, 2 * (current_slice->w + 6) * sizeof(*current_slice->sample_buffer));

    current_slice->run_index = 0;

    for (size_t y = 0; y < current_slice->h; y++)
    {
        Element_Begin1("Line");
        Element_Info1(y);

        int16s *temp = sample[0];

        sample[0] = sample[1];
        sample[1] = temp;

        sample[1][-1] = sample[0][0];
        sample[0][current_slice->w]  = sample[0][current_slice->w - 1];

        if (bits_per_sample <= 8)
            bits_max = 8;
        else
            bits_max = bits_per_sample;

        line(pos, sample);
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ffv1::rgb()
{
    Element_Begin1("rgb");

    int16s *sample[4][2];

    current_slice->run_index = 0;

    for (int x = 0; x < 4; x++) {
        sample[x][0] = current_slice->sample_buffer +  x * 2      * (current_slice->w + 6) + 3;
        sample[x][1] = current_slice->sample_buffer + (x * 2 + 1) * (current_slice->w + 6) + 3;
    }
    memset(current_slice->sample_buffer, 0, 8 * (current_slice->w + 6) * sizeof(*current_slice->sample_buffer));

    for (size_t y = 0; y < current_slice->h; y++)
    {
        Element_Begin1("Line");
        Element_Info1(y);

        for (size_t c = 0; c < (unsigned)(3 + alpha_plane); c++)
        {
            // Copy for next lines: 4.3 context
            int16s *temp = sample[c][0];

            sample[c][0] = sample[c][1];
            sample[c][1] = temp;

            sample[c][1][-1]= sample[c][0][0  ];
            sample[c][0][current_slice->w]= sample[c][0][current_slice->w - 1];
            bits_max = bits_per_sample + 1;
            line((c + 1) / 2, sample[c]);
        }

        Element_End0();
    }

    Element_End0();
}

static inline int get_context(int16s quant_table[MAX_CONTEXT_INPUTS][256], int16s *src, int16s *last, int16s *last2)
{
    const int LT = last[-1];
    const int T  = last[0];
    const int RT = last[1];
    const int L  = src[-1];

    if (quant_table[3][127])
    {
        const int TT = last2[0];
        const int LL = src[-2];
        return quant_table[0][(L - LT) & 0xFF]
             + quant_table[1][(LT - T) & 0xFF]
             + quant_table[2][(T - RT) & 0xFF]
             + quant_table[3][(LL - L) & 0xFF]
             + quant_table[4][(TT - T) & 0xFF];
    }
    else
        return quant_table[0][(L - LT) & 0xFF]
             + quant_table[1][(LT - T) & 0xFF]
             + quant_table[2][(T - RT) & 0xFF];
}

//---------------------------------------------------------------------------
int32s File_Ffv1::line_range_coder(int32s pos, int32s context)
{
    int32s u;

    Get_RS(current_slice->plane_states[pos][context], u, "symbol");
    return u;
}


//---------------------------------------------------------------------------
int32s File_Ffv1::line_adaptive_symbol_by_symbol(size_t x, int32s pos, int32s context)
{
    int32s u;

    // New symbol, go to "run mode"
    if (context == 0 && current_slice->run_mode == RUN_MODE_STOP)
        current_slice->run_mode = RUN_MODE_PROCESSING;

    // If not running, get the symbol
    if (current_slice->run_mode == RUN_MODE_STOP)
    {
        u = get_symbol_with_bias_correlation(&current_slice->contexts[pos][context]);
        #if MEDIAINFO_TRACE
        Param("symbol", u);
        #endif //MEDIAINFO_TRACE
        return u;
    }

    if (current_slice->run_segment_length == 0 && current_slice->run_mode == RUN_MODE_PROCESSING) // Same symbol length
    {
        bool hits;
        Get_SB (hits,                                           "hits/miss");
        //if (bsf.GetB()) // "hits"
        if (hits) // "hits"
        {
            current_slice->run_segment_length = 1 << log2_run[current_slice->run_index];
            if (x + current_slice->run_segment_length <= current_slice->w) //Do not go further as the end of line
                ++current_slice->run_index;
        }
        else // "miss"
        {
            //current_slice->run_segment_length = bsf.Get4(log2_run[current_slice->run_index]);
            int32u run_segment_length;
            Get_S4 (log2_run[current_slice->run_index], run_segment_length,  "run_segment_length");
            current_slice->run_segment_length=(int32s)run_segment_length;
            if (current_slice->run_index)
                --current_slice->run_index;
            current_slice->run_mode = RUN_MODE_INTERRUPTED;
        }
    }

    current_slice->run_segment_length--;
    if (current_slice->run_segment_length < 0) // we passed the length of same symbol, time to get the new symbol
    {
        u = get_symbol_with_bias_correlation(&current_slice->contexts[pos][context]);
        #if MEDIAINFO_TRACE
        Param("symbol", u);
        #endif //MEDIAINFO_TRACE
        if (u >= 0) // GR(u - 1, ...)
            u++;

        // Time for the new symbol length run
        current_slice->run_mode_init();
        current_slice->run_segment_length = 0;
    } else // same symbol as previous pixel, no difference, waiting
        u = 0;
    return u;
}

//---------------------------------------------------------------------------
void File_Ffv1::line(int pos, int16s *sample[2])
{
    // TODO: slice_coding_mode (version 4)

    if (!coder_type)
    {
        current_slice->run_segment_length = 0;
        current_slice->run_mode_init();
    }

    for (size_t x = 0; x < current_slice->w; x++)
    {
        int32s context;

        context = get_context(quant_tables[quant_table_index[pos]], sample[1] + x, sample[0] + x, sample[1] + x);

        bool negative = false;
        // Step 7
        if (context < 0)
        {
            context = -context;
            negative = true;
        }

        int32s u;
        if (coder_type)
            u = line_range_coder(pos, context);
        else
            u = line_adaptive_symbol_by_symbol(x, pos, context);

        // Step 7
        if (negative)
            u = -u;

        sample[1][x] = (predict(sample[1] + x, sample[0] + x) + u) & ((1 << bits_max) -1);
    }
}

//---------------------------------------------------------------------------
void File_Ffv1::read_quant_tables(int i)
{
    Element_Begin1("quant_table");

    int32u scale = 1;

    for (int j = 0; j < 5; j++)
    {
        read_quant_table(i, j, scale);
        scale *= 2 * len_count[i][j] - 1;
        if (scale > 32768U)
        {
            Element_End0();
            return;
        }

        context_count[i] = (scale + 1) / 2;
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ffv1::read_quant_table(int i, int j, size_t scale)
{
    Element_Begin1("per context");

    ;

    int8u States[states_size];
    memset(States, 128, sizeof(States));

    int v = 0;
    for (int k=0; k < 128;)
    {
        int32u len_minus1;
        Get_RU (States, len_minus1,                             "len_minus1");

        if (k+len_minus1 >= 128)
        {
            Element_End0();
            return;
        }

        for (size_t a=0; a<=len_minus1; a++)
        {
            quant_tables[i][j][k] = scale * v;
            k++;
        }

        v++;
    }

    for (int k = 1; k < 128; k++)
        quant_tables[i][j][256 - k] = -quant_tables[i][j][k];
    quant_tables[i][j][128] = -quant_tables[i][j][127];

    len_count[i][j]=v;

    Element_End0();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
int32u File_Ffv1::CRC_Compute(size_t Size)
{
    int32u CRC_32=0;
    const int8u* CRC_32_Buffer=Buffer+Buffer_Offset+(size_t)Element_Offset;
    const int8u* CRC_32_Buffer_End=CRC_32_Buffer+Size;

    while(CRC_32_Buffer<CRC_32_Buffer_End)
    {
        CRC_32=(CRC_32<<8) ^ Psi_CRC_32_Table[(CRC_32>>24)^(*CRC_32_Buffer)];
        CRC_32_Buffer++;
    }
    return CRC_32;
}

//---------------------------------------------------------------------------
int32s File_Ffv1::get_median_number(int32s one, int32s two, int32s three)
{
    if (one > two)
    {
        // one > two > three
        if (two > three)
            return two;

        // three > one > two
        if (three > one)
            return one;
        // one > three > two
        return three;
    }

    // three > two > one
    if (three > two)
        return two;

    // two > one && two > three

    // two > three > one
    if (three > one)
        return three;
    return one;
}

//---------------------------------------------------------------------------
int32s File_Ffv1::predict(int16s *current, int16s *current_top)
{
    int32s LeftTop = current_top[-1];
    int32s Top = current_top[0];
    int32s Left = current[-1];

    return get_median_number(Left, Left + Top - LeftTop, Top);
}

//---------------------------------------------------------------------------
int32s File_Ffv1::golomb_rice_decode(int k)
{
    int32u q = 0;
    int32u v;

    //while (bsf.Remain() > 0 && q < PREFIX_MAX && !bsf.GetB())
    while (Data_BS_Remain() > 0 && q < PREFIX_MAX)
    {
        bool Temp;
        Get_SB(Temp,                                            "golomb_rice_prefix_0");
        if (Temp)
            break;

        ++q;
    }

    if (q == PREFIX_MAX) // ESC
    {
        //v = bsf.Get4(bits_max) + 11;
        Get_S4(bits_max, v,                                     "escaped_value_minus_11");
        v+=11;
    }
    else
    {
        //int32u remain = bsf.Get8(k); // Read k bits
        int32u remain;
        Get_S4(k, remain,                                       "golomb_rice_remain");
        int32u mul = q << k; // q * pow(2, k)

        v = mul | remain;
    }

    // unsigned to signed
    int32s code = (v >> 1) ^ -(v & 1);
    return code;
}

//---------------------------------------------------------------------------
int32s File_Ffv1::get_symbol_with_bias_correlation(Slice::Context *context)
{
    int k = 0;
    // Step 8: compute the Golomb parameter k
    for (k = 0; (context->N << k) < context->A; ++k);

    // Step 10: Decode Golomb code (using limitation PREFIX_MAX == 12)
    int32s code = golomb_rice_decode(k);

    // Step 9: Mapping
    int32s M = 2 * context->B + context->N;
    code = code ^ (M >> 31);

    // Step 11
    context->B += code;
    context->A += code >= 0 ? code : -code;

    code += context->C;

    update_correlation_value_and_shift(context);

    // Step 7 (TODO better way)
    bool neg = code & (1 << (bits_max - 1)); // check if the number is negative
    code = code & ((1 << (bits_max - 1)) - 1); // Keep only the n bits
    if (neg)
        code = - 1 - (~code & ((1 << (bits_max - 1)) - 1)); // 0xFFFFFFFF - positive value on n bits

    return code;
}

//---------------------------------------------------------------------------
void File_Ffv1::update_correlation_value_and_shift(Slice::Context *c)
{
    if (!c)
        return;

    // Step 11: Resets
    if (c->N == c->N0)
    {
         // divide by 2, if >= 0 : ROUND, otherwise, CEIL
        c->N >>= 1;
        c->A >>= 1;
        c->B >>= 1;
    }
    ++c->N; // context meets

    // Step 12: Bias computation procedure
    // Keep B in (-N;0]
    if (c->B <= -c->N) {
        if (c->C > c->Cmin)
            --c->C;

        c->B = c->B + c->N;
        if (c->B <= -c->N)
            c->B = -c->N + 1;
    } else if (c->B > 0) {
        if (c->C < c->Cmax)
            ++c->C;

        c->B = c->B - c->N;
        if (c->B > 0)
            c->B = 0;
    }
}

//---------------------------------------------------------------------------
void File_Ffv1::copy_plane_states_to_slice(int8u plane_count)
{
    if (!coder_type)
        return;

    for (size_t i = 0; i < plane_count; i++)
    {
        int32u idx = quant_table_index[i];
        if (!current_slice->plane_states[i])
        {
            current_slice->plane_states[i] = new int8u* [context_count[idx] + 1];
            memset(current_slice->plane_states[i], 0, (context_count[idx] + 1) * sizeof(int8u*));
        }
        for (size_t j = 0; j < context_count[idx]; j++)
        {
            if (!current_slice->plane_states[i][j])
                current_slice->plane_states[i][j] = new int8u [states_size];
            for (size_t k = 0; k < states_size; k++)
            {
                current_slice->plane_states[i][j][k] = plane_states[idx][j][k];
            }
        }
    }
}

//---------------------------------------------------------------------------
void File_Ffv1::plane_states_clean(states_context_plane states[MAX_QUANT_TABLES])
{
    if (!coder_type)
        return;

    for (size_t i = 0; i < MAX_QUANT_TABLES && states[i]; ++i)
    {
        for (size_t j = 0; states[i][j]; ++j)
            delete[] states[i][j];

        delete[] states[i];
        states[i] = NULL;
    }
}

} //NameSpace

#endif //MEDIAINFO_FFV1_YES
