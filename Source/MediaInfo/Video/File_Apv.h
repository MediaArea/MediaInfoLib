/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_ApvH
#define MediaInfo_ApvH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Apv
//***************************************************************************

class File_Apv : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Apv();
    ~File_Apv() override;

private :
    //Streams management
    void Streams_Accept() override;
    void Streams_Fill() override;
    void Streams_Finish() override;

    //Buffer - File header
    bool FileHeader_Begin() override;

    //Buffer - Global
    void Read_Buffer_OutOfBand() override;

    //Buffer - Per element
    void Header_Parse() override;
    void Data_Parse() override;
    void frame();
    void frame_header();
    void frame_info();
    void quantization_matrix();
    void tile_info();
    void au_info();
    void metadata();
    void filler();
    void tile();
    void byte_alignment();
    void metadata_payload(int64u payloadType, int64u payloadSize);
    void metadata_filler();
    void metadata_itu_t_t35();
    void metadata_mdcv();
    void metadata_cll();
    void metadata_user_defined();
    void metadata_undefined();

    //Temp
    int32u au_size{};
    int8u  profile_idc{};
    int8u  level_idc{};
    int8u  band_idc{};
    int32u frame_width{};
    int32u frame_height{};
    int8u  chroma_format_idc{};
    int8u  bit_depth_minus8{};
    int8u  color_primaries{};
    int8u  transfer_characteristics{};
    int8u  matrix_coefficients{};
    int8u  NumComps{};
    bool   color_description_present_flag{};
    bool   full_range_flag{};
    int32u NumTiles{};
    enum hdr_format
    {
        HdrFormat_SmpteSt209440,
        HdrFormat_SmpteSt2086,
        HdrFormat_Max,
    };
    typedef std::map<video, Ztring[HdrFormat_Max]> hdr;
    hdr     HDR;
    Ztring  maximum_content_light_level;
    Ztring  maximum_frame_average_light_level;
};

} //NameSpace

#endif
