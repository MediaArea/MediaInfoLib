/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_REFERENCES_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper_Sequence_Common.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper_Common.h"
#include "ZenLib/FileName.h"
#include "ZenLib/Format/Http/Http_Utils.h"
#if MEDIAINFO_AES
    #include "base64.h"
#endif //MEDIAINFO_AES
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events_Internal.h"
    #include "MediaInfo/MediaInfo_Config_PerPackage.h"
#endif //MEDIAINFO_EVENTS
using namespace std;
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

resource::resource()
{
	MI=NULL;
	IgnoreFramesBefore=0;
	IgnoreFramesAfterDuration=(int64u)-1;
	IgnoreFramesAfter=(int64u)-1;
	IgnoreFramesRate=0;
	#if MEDIAINFO_DEMUX
		Demux_Offset_Frame=0;
		Demux_Offset_DTS=0;
		Demux_Offset_FileSize=0;
	#endif //MEDIAINFO_DEMUX
}

resource::~resource()
{
    delete MI;
}

} //NameSpace

#endif //MEDIAINFO_REFERENCES_YES
