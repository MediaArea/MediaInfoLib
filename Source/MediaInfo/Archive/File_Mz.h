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
    bool FileHeader_Begin() override;

    //Buffer - Global
    void Read_Buffer_Continue() override;

    void Goto_Next();
    void Parse_ReadonlyData();
    void Parse_ImageDebugDirectory();
    void Parse_Resources();
    bool Parse_StringFileInfo(int8u level = 0);
    void Parse_SBAT();

    //Temp
    enum class State : int8u {
        Main,
        ReadonlyData,
        ImageDebug,
        Resources,
        SBAT
    };
    State parsing_state{ State::Main };
    struct PESectionInfo {
        int32u virtual_address;
        int32u size;
        int32u offset;
    };
    std::map<State, PESectionInfo> to_parse;
    map<int32u, Ztring> Named_Resource;
    map<int32u, int32u> Resource;
};

} //NameSpace

#endif
