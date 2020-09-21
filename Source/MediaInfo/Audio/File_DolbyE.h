/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Dolby E files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_DolbyEH
#define MediaInfo_File_DolbyEH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DolbyE
//***************************************************************************

class File_DolbyE : public File__Analyze
{
public :
    //In
    int64s GuardBand_Before;
    
    //Out
    int64s GuardBand_After;

    //Constructor/Destructor
    File_DolbyE();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Fill_PerProgram();
    void Streams_Finish();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();

    //Buffer - Global
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void sync_segment();
    void metadata_segment();
    void audio_segment();
    void metadata_extension_segment();
    void audio_extension_segment();
    void meter_segment();
    void ac3_metadata_subsegment(bool xbsi);

    //Helpers
    void Descramble_20bit(int32u key, int16u size);

    //Temp
    int64u  SMPTE_time_code_StartTimecode;
    int16u  channel_subsegment_size[8];
    int8u   program_config;
    int8u   metadata_extension_segment_size;
    int8u   meter_segment_size;
    int8u   frame_rate_code;
    int8u   bit_depth;
    bool    key_present;
    int8u*  Descrambled_Buffer; //Used in case of key_present
    std::map<int64u, int64u> FrameSizes;
    std::map<int16u, int64u> channel_subsegment_sizes[8];
    int64u  GuardBand_Before_Initial;
    int64u  GuardBand_After_Initial;
    struct description_text_value
    {
        string Previous;
        string Current;
    };
    vector<description_text_value> description_text_Values;
};

} //NameSpace

#endif
