/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ARIB MMT descriptors (STD-B60), carried in the MMT signaling tables. Fed a
// descriptor loop by File_Mmt; writes the decoded fields into the shared
// mmt_stream. Analogous to File_Mpeg_Descriptors for MPEG-TS.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Mmt_DescriptorsH
#define MediaInfo_File_Mmt_DescriptorsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Mmt.h" //mmt_stream
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Mmt_Descriptors
//***************************************************************************

class File_Mmt_Descriptors : public File__Analyze
{
public :
    //In - set by File_Mmt before feeding a descriptor loop.
    mmt_stream* Complete_Stream;
    asset*      CurrentAsset;   //the MPT asset being built (NULL for non-asset loops)

    File_Mmt_Descriptors();

private :
    //Buffer - Global
    void FileHeader_Parse() override;

    //Buffer - Per element (one descriptor)
    void Header_Parse() override;
    void Data_Parse() override;

    //Descriptors
    void Descriptor_0001(); //MPU_Timestamp_Descriptor: video presentation-time span
    void Descriptor_8000(); //Asset_Group_Descriptor
    void Descriptor_8010(); //Video_Component_Descriptor
    void Descriptor_8014(); //MH-Audio_Component_Descriptor
    void Descriptor_8019(); //MH-Service_Descriptor: provider + service name
    void Descriptor_8020(); //MH-Data_Component_Descriptor: superimpose vs subtitle
    void Descriptor_F001(); //MH-Short_Event_Descriptor: event name + text
};

} //NameSpace

#endif
