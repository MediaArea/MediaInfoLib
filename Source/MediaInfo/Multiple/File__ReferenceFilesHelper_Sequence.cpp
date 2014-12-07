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
#include "MediaInfo/Multiple/File__ReferenceFilesHelper_Sequence.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper_Common.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
sequence::sequence()
{
	FileNames.Separator_Set(0, __T(","));
	StreamKind=Stream_Max;
	StreamPos=(size_t)-1;
	MenuPos=(size_t)-1;
	StreamID=(int64u)-1;
	FrameRate=0;
	Delay=0;
	FileSize=(int64u)-1;
	IsCircular=false;
	IsMain=false;
	FileSize_IsPresent=false;
	#if MEDIAINFO_ADVANCED || MEDIAINFO_MD5
		List_Compute_Done=false;
	#endif //MEDIAINFO_ADVANCED || MEDIAINFO_MD5
	State=0;
	MI=NULL;
	Resources_Pos=0;
	#if MEDIAINFO_FILTER
		Enabled=true;
	#endif //MEDIAINFO_FILTER
}

//---------------------------------------------------------------------------
sequence::~sequence()
{
        delete MI;
}

//***************************************************************************
// In
//***************************************************************************

//---------------------------------------------------------------------------
void sequence::AddFileName(const Ztring& FileName, size_t Pos)
{
	FileNames.push_back(FileName);
}

//---------------------------------------------------------------------------
void sequence::FrameRate_Set(float64 NewFrameRate)
{
	FrameRate=NewFrameRate;
}

} //NameSpace

#endif //MEDIAINFO_REFERENCES_YES
