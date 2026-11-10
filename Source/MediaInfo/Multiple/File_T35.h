/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about ITU-T T.35 messages
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
// Class File_T35
//***************************************************************************

class File_T35 : public File__Analyze
{
public :
    //In
    int8u                       aspect_ratio_FromContainer{(int8u)-1};
    std::string                 Identifier; //Start of each message (country code, terminal provider code...) when the container carries it apart from the messages
    bool                        IsMetadataTrackOrItem{}; //Messages are the samples of a timed metadata track or the data of a metadata item, the first one is shown as the streams of the parser

    //Out
    int64u                      GA94_03_Offset{}; //Offset of the DTVCC transport data, parsed by the container as it depends on the picture order, 0 if none
    std::vector<std::pair<std::string, Ztring> > SmpteSt209450_Infos; //Fields of the first SMPTE ST 2094-50 message other than the HDR format ones

    //Out, filling of the stream of the container
    void SmpteSt209450_Fill(File__Analyze* Parent, stream_t StreamKind, size_t StreamPos);

    //Constructor/Destructor
    File_T35(File__Analyze* Container_, hdr* HDR_=nullptr, Ztring* EtsiTS103433_=nullptr, DV_RPU* DV_RPU_data_=nullptr);

private :
    //Streams management
    void Streams_Fill();

    //Buffer - Global
    void Read_Buffer_Init();
    void Read_Buffer_Continue();

    //Elements
    void itu_t_t35();
    void Get_Code(int32u& Code, int8u Size, const char* Name);
    void Get_Code1(int8u& Code, const char* Name);
    void Get_Code2(int16u& Code, const char* Name);
    void Get_Code4(int32u& Code, const char* Name);
    void Peek_Code(int32u& Code, int8u Size);
    int64u Code_Remain();
    void itu_t_t35_B5();
    void itu_t_t35_B5_0031();
    void itu_t_t35_B5_0031_DTG1();
    void itu_t_t35_B5_0031_GA94();
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
    void itu_t_t35_B5_0090();
    void itu_t_t35_B5_0090_0001();
    void itu_t_t35_B5_5890();
    void itu_t_t35_B5_5890_01();
    void itu_t_t35_26();
    void itu_t_t35_26_0004();
    void itu_t_t35_26_0004_0005();

    //Data of the container, own data if the container does not use it
    File__Analyze*              Container; //Its video stream gets the AFD and the SMPTE ST 2094-10 issues, nullptr if it has no video stream
    hdr                         HDR_Own;
    Ztring                      EtsiTS103433_Own;
    DV_RPU                      DV_RPU_data_Own;
    hdr&                        HDR;
    Ztring&                     EtsiTS103433;
    DV_RPU&                     DV_RPU_data;

    //Temp
    size_t                      Identifier_Offset{};
    const char*                 Message_Format{}; //Format of the first message, for the streams of the parser
};

} //NameSpace

#endif
