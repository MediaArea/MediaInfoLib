/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about JPEG XL files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_JpegXLH
#define MediaInfo_File_JpegXLH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_JpegXL
//***************************************************************************

class File_JpegXL : public File__Analyze
{
public :
    //Constructor/Destructor
    File_JpegXL();

    //File Header
    void FileHeader_Parse() override;

private :
    //Helpers
    struct U32Dist {
        int8u bits;
        int32u offset;
    };
    inline U32Dist V(int32u v) { return { 0, v }; }
    inline U32Dist B(int8u bits, int32u offset = 0) { return { bits, offset }; }
    void Get_U32(U32Dist d0, U32Dist d1, U32Dist d2, U32Dist d3, int32u& Info, const char* Name);
    void Get_Enum(int32u& Info, const char* Name);
};

} //NameSpace

#endif
