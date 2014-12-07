/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef File__ReferenceFilesHelper_SequenceH
#define File__ReferenceFilesHelper_SequenceH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File__ReferenceFilesHelper_Resource.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper_Sequence_Common.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

struct resource;
typedef std::vector<resource*> resources;

class rfhs_common;

//***************************************************************************
// Class sequence
//***************************************************************************

struct sequence
{
    //Constructor/Desctructor
                                    sequence();
                                    ~sequence();

    //In
    void                            AddFileName(const Ztring& FileName, size_t Pos=(size_t)-1);
    void                            FrameRate_Set(float64 NewFrameRate);



	ZtringList          FileNames;
	Ztring              Source; //Source file name (relative path)
	stream_t            StreamKind;
	size_t              StreamPos;
	size_t              MenuPos;
	int64u              StreamID;
	float64             FrameRate;
	int64u              Delay;
	int64u              FileSize;
	bool                IsCircular;
	bool                IsMain;
	bool                FileSize_IsPresent; //TODO: merge with FileSize after regression tests
	#if MEDIAINFO_ADVANCED || MEDIAINFO_MD5
		bool            List_Compute_Done;
	#endif //MEDIAINFO_ADVANCED || MEDIAINFO_MD5
	size_t              State;
	std::map<std::string, Ztring> Infos;
	MediaInfo_Internal* MI;
	vector<resource*>           Resources;
	size_t                      Resources_Pos;
	#if MEDIAINFO_FILTER
		int64u          Enabled;
	#endif //MEDIAINFO_FILTER
	std::bitset<32> Status;
	#if MEDIAINFO_NEXTPACKET && MEDIAINFO_IBI
		ibi::stream IbiStream;
	#endif //MEDIAINFO_NEXTPACKET && MEDIAINFO_IBI

};

} //NameSpace

#endif
