/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef File__ReferenceFilesHelper_ResourceH
#define File__ReferenceFilesHelper_ResourceH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include <vector>
#if MEDIAINFO_EVENTS
    #include <set>
#endif //MEDIAINFO_EVENTS
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

struct resource
{
    //Constructor/Desctructor
                                    resource();
                                    ~resource();

    //In
	Ztring FileName;
	MediaInfo_Internal* MI;
	int64u  IgnoreFramesBefore;
	int64u  IgnoreFramesAfterDuration; //temporary value, some formats have duration instead of frame position
	int64u  IgnoreFramesAfter;
	float64 IgnoreFramesRate;
	#if MEDIAINFO_DEMUX
		int64u Demux_Offset_Frame;
		int64u Demux_Offset_DTS;
		int64u Demux_Offset_FileSize;
	#endif //MEDIAINFO_DEMUX
};

} //NameSpace

#endif
