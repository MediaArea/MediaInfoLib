/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about MZ files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_MzH
#define MediaInfo_File_MzH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Mz
//***************************************************************************

class File_Mz : public File__Analyze
{
protected :
    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Global
    void Read_Buffer_Continue ();

    void Parse_Resources();
    bool Parse_StringFileInfo(int8u level = 0);

    //Temp
    int32u rsrc_size{};
    int32u rsrc_virtual_addr{};
    int32u rsrc_offset{};
    map<int32u, Ztring> Named_Resource;
    map<int32u, int32u> Resource;
};

} //NameSpace

#endif
