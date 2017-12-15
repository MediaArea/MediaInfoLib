/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Helper class for parser having references to external files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File__HasReferencesH
#define MediaInfo_File__HasReferencesH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
class File__ReferenceFilesHelper;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File__HasReferences
//***************************************************************************

class File__HasReferences
{
public:
    //Constructor/Destructor
    File__HasReferences();
    ~File__HasReferences();

    // Streams management
    void ReferenceFiles_Accept(File__Analyze* MI, MediaInfo_Config_MediaInfo* Config);
    void ReferenceFiles_Finish();

    // Buffer - Global
    size_t ReferenceFiles_Seek(size_t Method, int64u Value, int64u ID);

    //Temp
    #if defined(MEDIAINFO_REFERENCES_YES)
    File__ReferenceFilesHelper*     ReferenceFiles;
    #endif //MEDIAINFO_REFERENCES_YES
};

} //NameSpace

#endif
