/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about XMP files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_XmpH
#define MediaInfo_File_XmpH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//GContainer items
struct gc_item
{
    string Mime;
    string Semantic;
    int32u Length;
    string Label;
    int32u Padding;
    string URI;
};
typedef std::vector<gc_item> gc_items;

//***************************************************************************
// Class File_Xmp
//***************************************************************************

class File_Xmp : public File__Analyze
{
public:
    bool Wait = false;
    gc_items* GContainerItems = nullptr;

private :
    //Buffer - File header
    bool FileHeader_Begin();
};

} //NameSpace

#endif
