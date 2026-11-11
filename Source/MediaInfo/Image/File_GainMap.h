/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about ISO 21496-1 Gain Map Metadata
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_GainMapH
#define MediaInfo_File_GainMapH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_GainMap
//***************************************************************************

class File_GainMap : public File__Analyze
{
public:
    //In
    bool fromAvif{ false };
    stream_t StreamKind{ Stream_Image }; //Kind of the stream filled with the metadata, to be merged in the image using the gain map

private :
    void Data_Parse();
};

} //NameSpace

#endif

