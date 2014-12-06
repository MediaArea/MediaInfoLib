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

class resource;
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
	struct completeduration
	{
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

		completeduration()
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

		~completeduration()
		{
			delete MI;
		}
	};
	vector<completeduration>    CompleteDuration;
	size_t                      CompleteDuration_Pos;
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
