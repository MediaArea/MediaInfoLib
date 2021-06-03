/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Ac3H
#define MediaInfo_Ac3H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Ac3
//***************************************************************************

class File_Ac3 : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    bool   MustParse_dac3;
    bool   MustParse_dec3;
    bool   MustParse_dmlp;
    bool   CalculateDelay;

    //Constructor/Destructor
    File_Ac3();
    ~File_Ac3();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Synchro
    bool Synchronize();
    void Synched_Init();
    bool Synched_Test();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test();
    #endif //MEDIAINFO_DEMUX

    //Buffer - Global
    void Read_Buffer_OutOfBand();
    void Read_Buffer_Continue ();
    void Read_Buffer_Unsynched();
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID);
    #endif //MEDIAINFO_SEEK

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Core();
    void Core_Frame();
    void HD();
    void HD_format_info();
    void emdf();
    void emdf_sync();
    void emdf_container();
    void emdf_payload_config();
    void emdf_protection();
    void object_audio_metadata_payload();
    void program_assignment();
    void joc();
    void joc_header();
    void joc_info();
    void joc_data_point_info();
    void joc_data();
    void joc_ext_data();

    void TimeStamp();
    void dac3();
    void dec3();
    void dmlp();
    bool FrameSynchPoint_Test();
    bool CRC_Compute(size_t Size);
    size_t Core_Size_Get();
    size_t HD_Size_Get();

    //Parsing
    void Get_V4(int8u  Bits, int32u  &Info, const char* Name);
    void Skip_V4(int8u  Bits, const char* Name);

    //Buffer
    const int8u* Save_Buffer;
    size_t Save_Buffer_Offset;
    size_t Save_Buffer_Size;

    //Temp EMDF
    size_t EMDF_RemainPos;
    size_t RemainAfterEMDF;

    //Temp JOC
    int8u joc_num_objects;
    std::map<int8u, size_t> joc_num_objects_map;
    int8u joc_ext_config_idx;
    
    //Temp
    struct dolby
    {
        int8u  dialnorm;
        int8u  compr;
        int8u  dynrng;  //This is only the first occurence of aufblk
        bool   compre;
        bool   dynrnge;  //This is only the first occurence of aufblk

        dolby()
            :
            dialnorm(0),
            compr(0),
            dynrng(0),
            compre(false),
            dynrnge(false)
        {
        }
    };
    dolby  FirstFrame_Dolby;
    int8u* addbsi_Buffer;
    size_t addbsi_Buffer_Size;
    int64u Formats[2]; // AC-3 or E-AC-3
    std::vector<int64u> dialnorms;
    std::vector<int64u> comprs;
    std::vector<int64u> dynrngs;
    std::map<int8u, int64u> fscods;
    std::map<int8u, int64u> frmsizecods;
    int64u Frame_Count_HD;
    int32u nonstd_bed_channel_assignment_mask;
    int16u chanmap_Max[8][9];
    int16u frmsizplus1_Max[8][9];
    int16u HD_BitRate_Max;
    int16u HD_Channels2;
    int16u HD_flags;
    int8u  num_dynamic_objects;
    int8u  fscod;
    int8u  fscod2;
    int8u  frmsizecod;
    int8u  bsid;
    int8u  bsid_Max;
    int8u  bsmod_Max[8][9];
    int8u  acmod_Max[8][9];
    int8u  dsurmod_Max[8][9];
    int8u  numblkscod;
    int8u  dsurexmod;
    int8u  dheadphonmod;
    int8u  HD_StreamType;
    int8u  HD_SubStreams_Count;
    int8u  HD_SamplingRate1;
    int8u  HD_SamplingRate2;
    int8u  HD_Channels1;
    int8u  HD_Resolution1;
    int8u  HD_Resolution2;
    int8u  substreamid_Independant_Current;
    int8u  substreams_Count;
    int8u  joc_complexity_index_Container;
    int8u  joc_complexity_index_Stream;
    bool   b_lfe_present;
    bool   lfeon_Max[8][9];
    bool   dxc3_Parsed;
    bool   HD_MajorSync_Parsed;
    bool   HD_NoRestart;
    bool   HD_ExtraParity;
    bool   HD_IsVBR;
    bool   HD_HasAtmos;
    bool   Core_IsPresent;
    bool   HD_IsPresent;
    bool   dynrnge_Exists;
    bool   chanmape_Max[8][9];
    bool   TimeStamp_IsPresent;
    bool   TimeStamp_IsParsing;
    bool   TimeStamp_Parsed;
    bool   BigEndian;
    bool   IgnoreCrc_Done;
    bool   IgnoreCrc;
    TimeCode TimeStamp_FirstFrame;
    size_t TimeStamp_Count;
};

} //NameSpace

#endif
