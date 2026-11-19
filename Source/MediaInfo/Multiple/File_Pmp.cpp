/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Contributor: Lionel Duchateau, kurtnoise@free.fr
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#if defined(MEDIAINFO_PMP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Pmp.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Pmp_video_format(int32u video_format)
{
    switch (video_format)
    {
        case   0 : return "MPEG-4 Visual";
        case   1 : return "AVC";
        default  : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Pmp_audio_format(int32u audio_format)
{
    switch (audio_format)
    {
        case   0 : return "MPEG Audio";
        case   1 : return "AAC";
        default  : return "";
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Pmp::FileHeader_Begin()
{
    //Synchro
    if (4>Buffer_Size)
        return false;
    if (Buffer[0]!=0x70 //"pmpm"
     || Buffer[1]!=0x6D
     || Buffer[2]!=0x70
     || Buffer[3]!=0x6D)
    {
        Reject();
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
void File_Pmp::FileHeader_Parse()
{
     //Parsing
    int32u version, video_format=0, nb_frames=0, video_width=0, video_height=0, time_base_num=0, time_base_den=0, audio_format=0, sample_rate=0, channels_minus1=0;
    int16u audio_count=0;

    Skip_C4(                                                    "Signature");
    Get_L4 (version,                                            "Version");
    if (version==1)
    {
        Get_L4 (video_format,                                   "video_format");
        Get_L4 (nb_frames,                                      "number of frames");
        Get_L4 (video_width,                                    "video_width");
        Get_L4 (video_height,                                   "video_height");
        Get_L4 (time_base_num,                                  "time_base_num");
        Get_L4 (time_base_den,                                  "time_base_den");
        Get_L4 (audio_format,                                   "audio_format");
        Get_L2 (audio_count,                                    "number of audio streams");
        Skip_XX(10,                                             "unknown");
        Get_L4 (sample_rate,                                    "sample_rate");
        Get_L4 (channels_minus1,                                "channels minus 1");
        int64u Remaining=File_Offset+Buffer_Offset+Element_Offset<File_Size?(File_Size-(File_Offset+Buffer_Offset+Element_Offset)):0;
        Loop_CheckValue(audio_count, Remaining, 1, "number of audio streams");
    }

    FILLING_BEGIN();
        Accept("PMP");

        Fill(Stream_General, 0, General_Format, "PMP");

        if (version==1)
        {
            Stream_Prepare(Stream_Video);
            Fill(Stream_Video, 0, Video_Format, Pmp_video_format(video_format));
            Fill(Stream_Video, 0, Video_FrameCount, nb_frames);
            Fill(Stream_Video, 0, Video_Width, video_width);
            Fill(Stream_Video, 0, Video_Height, video_height);
            Fill(Stream_Video, 0, Video_FrameRate, (float)time_base_den / 100);
            for (int16u i=0; i<audio_count; i++)
            {
                Stream_Prepare(Stream_Audio);
                Fill(Stream_Audio, StreamPos_Last, Audio_Format, Pmp_audio_format(audio_format));
                Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, (int64u)channels_minus1+1);
                Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, sample_rate);
            }
        }

        //No more need data
        Finish("PMP");
    FILLING_END();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_PMP_YES
