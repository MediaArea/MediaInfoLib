/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about AVS Video files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_Avs3V
#define MediaInfo_Avs3V
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/Multiple/File_Mpeg4.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Avs
//***************************************************************************

class File_Avs3V : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    bool   FrameIsAlwaysComplete;

    //constructor/Destructor
    File_Avs3V();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin() {return FileHeader_Begin_0x000001();}

    //Buffer - Synchro
    bool Synchronize() {return Synchronize_0x000001();}
    bool Synched_Test();
    void Synched_Init();

    //Buffer - Per element
    void Header_Parse();
    bool Header_Parser_QuickSearch();
    bool Header_Parser_Fill_Size();
    void Data_Parse();

    //Elements
    void slice();
    void video_sequence_start();
    void video_sequence_end();
    void user_data_start();
    void extension_start();
    void picture_start();
    void video_edit();
    void reserved();

    void reference_picture_list_set(int8u list, int32u rpls);
    void weight_quant_matrix();

    int8u NumberOfFrameCentreOffsets();

    //Count of a Packets
    size_t progressive_frame_Count;
    size_t Interlaced_Top;
    size_t Interlaced_Bottom;

    //From user_data
    Ztring Library;
    Ztring Library_Name;
    Ztring Library_Version;
    Ztring Library_Date;

    //Temp
    int32u  bit_rate;                           //From video_sequence_start
    int16u  horizontal_size;                    //From video_sequence_start
    int16u  vertical_size;                      //From video_sequence_start
    int16u  display_horizontal_size;            //From sequence_display
    int16u  display_vertical_size;              //From sequence_display
    int8u   profile_id;                         //From video_sequence_start
    int8u   level_id;                           //From video_sequence_start
    int8u   chroma_format;                      //From video_sequence_start
    int8u   sample_precision;                   //From video_sequence_start
    int8u   encoding_precision;                 //From video_sequence_start
    int8u   aspect_ratio;                       //From video_sequence_start
    int8u   frame_rate_code;                    //From video_sequence_start
    int8u   video_format;                       //From sequence_display;

    int8u   num_of_hmvp_cand;
    int8u   nn_tools_set_hook;
    int8u   hdr_dynamic_metadata_type;          //From HDR Dynamic Metadata extension
    int8u   colour_primaries;                   //From sequence display extension
    int8u   transfer_characteristics;           //From sequence display extension
    int8u   matrix_coefficients;                //From sequence display extension

    bool    have_MaxCLL;
    int16u  max_content_light_level;            //From mastering diaplay and content metadata extension
    bool    have_MaxFALL;
    int16u  max_picture_average_light_level;    //From mastering diaplay and content metadata extension

    int32u  num_ref_pic_list_set[2];
    bool    picture_structure;
    bool    top_field_first;
    bool    repeat_first_field;

    int8u   DMI_Found;

    bool    progressive_sequence;               //From video_sequence_start
    bool    field_coded_sequence;               //From video_sequence_start
    bool    library_stream_flag;                //From video_sequence_start
    bool    library_picture_enable_flag;        //From video_sequence_start
    bool    duplicate_sequence_header_flag;     //From video_sequence_start
    bool    low_delay;                          //From video_sequence_start
    bool    temporal_id_enable_flag;
    bool    video_sequence_start_IsParsed;      //From video_sequence_start
    bool    rpl1_same_as_rpl0_flag;             //From video_sequence_start
    bool    weight_quant_enable_flag;
    bool    load_seq_weight_quant_data_flag;

    bool    alf_enable_flag;
    bool    affine_enable_flag;
    bool    amvr_enable_flag;

    bool    ibc_enable_flag;
    bool    isc_enable_flag;

    bool    picture_alf_enable_flag[3];

    std::vector<stream_payload> Streams;

};

} //NameSpace

#endif
