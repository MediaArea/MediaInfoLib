/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about ITU-T T35
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_T35H
#define MediaInfo_File_T35H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Sami
//***************************************************************************

class File_T35 : public File__Analyze
{
public:
    // In
    enum class style {
        itu_t_t35,
        mastering_display_colour_volume,
        light_level,
    };
    mastering_metadata_2086* MasteringMetadata{};
    style Style{};
    int8u aspect_ratio_FromContainer{ (int8u)-1 };

    //Text
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
    //    File__Analyze*                  GA94_03_Parser;
    //    bool                            GA94_03_IsPresent;
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)

    // Out
    enum hdr_format {
        HdrFormat_SmpteSt209410,
        HdrFormat_SmpteSt209440,
        HdrFormat_EtsiTs103433,
        HdrFormat_HdrVivid,
        HdrFormat_SmpteSt2086,
        HdrFormat_Max,
    };
    typedef std::map<video, Ztring[HdrFormat_Max]> hdr;
    hdr                                 HDR;
    Ztring                              EtsiTS103433;

    //Constructor/Destructor
    enum class source {
        iso,
        aomedia,
    };
    File_T35(source Source = source::iso);

private :
    //Streams management
    void Streams_Fill();

    //Buffer - Global
    void Read_Buffer_Init();
    void Read_Buffer_Continue();

    //Elements
    void itu_t_t35();
    void itu_t_t35_26();
    void itu_t_t35_26_0004();
    void itu_t_t35_26_0004_0005();
    void itu_t_t35_B5();
    void itu_t_t35_B5_0031();
    void itu_t_t35_B5_0031_DTG1();
    void itu_t_t35_B5_0031_GA94();
    //void itu_t_t35_B5_0031_GA94_03();
    //void itu_t_t35_B5_0031_GA94_03_Delayed(int32u seq_parameter_set_id);
    void itu_t_t35_B5_0031_GA94_06();
    void itu_t_t35_B5_0031_GA94_09();
    void itu_t_t35_B5_003A();
    void itu_t_t35_B5_003A_00();
    void itu_t_t35_B5_003A_02();
    void itu_t_t35_B5_003B();
    void itu_t_t35_B5_003B_00000800();
    void itu_t_t35_B5_003C();
    void itu_t_t35_B5_003C_0001();
    void itu_t_t35_B5_003C_0001_04();
    void itu_t_t35_B5_5890();
    void itu_t_t35_B5_5890_01();
    void mastering_display_colour_volume();
    void light_level();

    // Temp
    Ztring  maximum_content_light_level;
    Ztring  maximum_frame_average_light_level;
    source Source;
};

} //NameSpace

#endif
