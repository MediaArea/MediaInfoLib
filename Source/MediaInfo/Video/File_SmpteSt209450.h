/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about SMPTE ST 2094-50 metadata (Dynamic Metadata for Color
// Volume Transform, Application #5)
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_SmpteSt209450H
#define MediaInfo_File_SmpteSt209450H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <map>
#include <vector>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_SmpteSt209450
//***************************************************************************

class File_SmpteSt209450 : public File__Analyze
{
public :
    //Out
    std::map<std::string, Ztring> HDR; //HDR format fields (HDR_Format, HDR_Format_Version, HDR_Format_Settings) from the metadata of the first frame, for Fill_HDR_Format()
    std::vector<std::pair<std::string, Ztring> > Infos; //Other fields from the metadata of the first frame

    //Constructor/Destructor
    File_SmpteSt209450();

private :
    //Buffer - Global
    void Read_Buffer_Init();
    void Read_Buffer_Continue();

    //Elements
    void smpte_st_2094_50_application_info();
};

} //NameSpace

#endif
