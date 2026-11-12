/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about GXF files
// SMPTE 360M - General Exchange Format
// SMPTE RDD 14-2007 - General Exchange Format-2
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Gxf_TimeCodeH
#define MediaInfo_File_Gxf_TimeCodeH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#if defined(MEDIAINFO_ANCILLARY_YES)
    #include <MediaInfo/Multiple/File_Ancillary.h>
#endif //defined(MEDIAINFO_ANCILLARY_YES)
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Gxf_TimeCode
//***************************************************************************

class File_Gxf_TimeCode : public File__Analyze
{
public :
    //In
    int32u FrameRate_Code;
    int32u FieldsPerFrame_Code;
    bool   IsAtc; // SMPTE ST 12-2
    bool   IsBigEndian;
    bool   IsTimeCodeTrack;
    bool   IsBlockAddition; // Matroska BlockAdditional, 1 time code of 1 frame of the track per buffer
    string Format;
    #if MEDIAINFO_ADVANCED
    string id;
    #endif //MEDIAINFO_ADVANCED

    //Out
    int64u TimeCode_FirstFrame_ms;
    string TimeCode_FirstFrame;
    string Settings;

    //Constructor/Destructor
    File_Gxf_TimeCode();
    ~File_Gxf_TimeCode();
private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Global
    void Read_Buffer_Continue ();

    //Temp - Matroska BlockAdditional
    string TimeCode_LastFrame; //Valid time code of the frame with the highest time stamp
    int64u TimeCode_LastFrame_FrameNumber;
    int64u TimeCode_LastFrame_PTS;
    struct blockaddition_value
    {
        int8u  Digits[8]; //Hours, minutes, seconds and frames, tens then units
        bool   DropFrame;
        int64u Frame_Count;
        int64u Frame_Count_NotParsedIncluded;
        int64u PTS;
        int64u Offset;
    };
    std::vector<blockaddition_value> BlockAddition_Pending; //Values checked once the count of frames per second is known
    int8u  BlockAddition_FramesPerSecond; //(int8u)-1 if not yet known, 0 if unknown
    void   BlockAddition_Check(const blockaddition_value& Value);
    void   BlockAddition_FramesPerSecond_Set(int64u FrameDuration);
};

} //NameSpace

#endif
