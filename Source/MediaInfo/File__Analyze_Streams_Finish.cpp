/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Init and Finalize part
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
#include "ZenLib/Utils.h"
#include "ZenLib/File.h"
#include "ZenLib/FileName.h"
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if MEDIAINFO_IBI
    #include "MediaInfo/Multiple/File_Ibi.h"
#endif //MEDIAINFO_IBI
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern MediaInfo_Config Config;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_Global()
{
    if (IsSub)
        return;

    #if MEDIAINFO_ADVANCED
        //Default frame rate
        if (Count_Get(Stream_Video)==1 && Retrieve(Stream_Video, 0, Video_FrameRate).empty() && Config->File_DefaultFrameRate_Get())
            Fill(Stream_Video, 0, Video_FrameRate, Config->File_DefaultFrameRate_Get());
    #endif //MEDIAINFO_ADVANCED

    //Video Frame count
    if (Count_Get(Stream_Video)==1 && Count_Get(Stream_Audio)==0 && Retrieve(Stream_Video, 0, Video_FrameCount).empty())
    {
        if (Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
            Fill(Stream_Video, 0, Video_FrameCount, Frame_Count_NotParsedIncluded);
        else if (Config->File_Names.size()>1)
            Fill(Stream_Video, 0, Video_FrameCount, Config->File_Names.size());
        #if MEDIAINFO_IBIUSAGE
        else
        {
            //External IBI
            std::string IbiFile=Config->Ibi_Get();
            if (!IbiFile.empty())
            {
                if (IbiStream)
                    IbiStream->Infos.clear(); //TODO: support IBI data from different inputs
                else
                    IbiStream=new ibi::stream;

                File_Ibi MI;
                Open_Buffer_Init(&MI, IbiFile.size());
                MI.Ibi=new ibi;
                MI.Open_Buffer_Continue((const int8u*)IbiFile.c_str(), IbiFile.size());
                if (!MI.Ibi->Streams.empty())
                    (*IbiStream)=(*MI.Ibi->Streams.begin()->second);
            }

            if (IbiStream && !IbiStream->Infos.empty() && IbiStream->Infos[IbiStream->Infos.size()-1].IsContinuous && IbiStream->Infos[IbiStream->Infos.size()-1].FrameNumber!=(int64u)-1)
                Fill(Stream_Video, 0, Video_FrameCount, IbiStream->Infos[IbiStream->Infos.size()-1].FrameNumber);
        }
        #endif //MEDIAINFO_IBIUSAGE
    }

    Streams_Finish_StreamOnly();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();

    if (!IsSub && !Config->File_IsReferenced_Get() && MediaInfoLib::Config.ReadByHuman_Get())
        Streams_Finish_HumanReadable();
}

//---------------------------------------------------------------------------
void File__Analyze::TestContinuousFileNames(size_t CountOfFiles, Ztring FileExtension, bool SkipComputeDelay)
{
    if (IsSub || !Config->File_TestContinuousFileNames_Get())
        return;

    size_t Pos=Config->File_Names.size();
    if (!Pos)
        return;

    //Trying to detect continuous file names (e.g. video stream as an image or HLS)
    size_t Pos_Base = (size_t)-1;
    bool AlreadyPresent=Config->File_Names.size()==1?true:false;
    FileName FileToTest(Config->File_Names.Read(Config->File_Names.size()-1));
    Ztring FileToTest_Name=FileToTest.Name_Get();
    Ztring FileToTest_Name_After=FileToTest_Name;
    size_t FileNameToTest_End=FileToTest_Name.size();
    while (FileNameToTest_End && !(FileToTest_Name[FileNameToTest_End-1]>=__T('0') && FileToTest_Name[FileNameToTest_End-1]<=__T('9')))
        FileNameToTest_End--;
    size_t FileNameToTest_Pos=FileNameToTest_End;
    while (FileNameToTest_Pos && FileToTest_Name[FileNameToTest_Pos-1]>=__T('0') && FileToTest_Name[FileNameToTest_Pos-1]<=__T('9'))
        FileNameToTest_Pos--;
    if (FileNameToTest_Pos!=FileToTest_Name.size() && FileNameToTest_Pos!=FileNameToTest_End)
    {
        size_t Numbers_Size=FileNameToTest_End-FileNameToTest_Pos;
        int64u Pos=Ztring(FileToTest_Name.substr(FileNameToTest_Pos)).To_int64u();
        FileToTest_Name.resize(FileNameToTest_Pos);
        FileToTest_Name_After.erase(0, FileToTest_Name.size()+Numbers_Size);

        /*
        for (;;)
        {
            Pos++;
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest.Path_Get()+PathSeparator+FileToTest_Name+Pos_Ztring+__T('.')+(FileExtension.empty()?FileToTest.Extension_Get():FileExtension);
            if (!File::Exists(Next))
                break;
            Config->File_Names.push_back(Next);
        }
        */

        //Detecting with a smarter algo (but missing frames are not detected)
        Ztring FileToTest_Name_Begin=FileToTest.Path_Get()+PathSeparator+FileToTest_Name;
        Ztring FileToTest_Name_End=FileToTest_Name_After+__T('.')+(FileExtension.empty()?FileToTest.Extension_Get():FileExtension);
        Pos_Base = (size_t)Pos;
        size_t Pos_Add_Max = 1;
        #if MEDIAINFO_ADVANCED
            bool File_IgnoreSequenceFileSize=Config->File_IgnoreSequenceFilesCount_Get(); //TODO: double check if it is expected

            size_t SequenceFileSkipFrames=Config->File_SequenceFilesSkipFrames_Get();
            if (SequenceFileSkipFrames)
            {
                for (;;)
                {
                    size_t Pos_Add_Max_Old=Pos_Add_Max;
                    for (size_t TempPos=Pos_Add_Max; TempPos<=Pos_Add_Max+SequenceFileSkipFrames; TempPos++)
                    {
                        Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+TempPos);
                        if (Numbers_Size>Pos_Ztring.size())
                            Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
                        Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
                        if (File::Exists(Next))
                        {
                            Pos_Add_Max=TempPos+1;
                            break;
                        }
                    }
                    if (Pos_Add_Max==Pos_Add_Max_Old)
                        break;
                }
            }
            else
            {
        #endif //MEDIAINFO_ADVANCED
        for (;;)
        {
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+Pos_Add_Max);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
            if (!File::Exists(Next))
                break;
            Pos_Add_Max<<=1;
            #if MEDIAINFO_ADVANCED
                if (File_IgnoreSequenceFileSize && Pos_Add_Max>=CountOfFiles)
                    break;
            #endif //MEDIAINFO_ADVANCED
        }
        size_t Pos_Add_Min = Pos_Add_Max >> 1;
        while (Pos_Add_Min+1<Pos_Add_Max)
        {
            size_t Pos_Add_Middle = Pos_Add_Min + ((Pos_Add_Max - Pos_Add_Min) >> 1);
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+Pos_Add_Middle);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
            if (File::Exists(Next))
                Pos_Add_Min=Pos_Add_Middle;
            else
                Pos_Add_Max=Pos_Add_Middle;
        }

        #if MEDIAINFO_ADVANCED
            } //SequenceFileSkipFrames
        #endif //MEDIAINFO_ADVANCED

        size_t Pos_Max = Pos_Base + Pos_Add_Max;
        Config->File_Names.reserve(Pos_Add_Max);
        for (Pos=Pos_Base+1; Pos<Pos_Max; ++Pos)
        {
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Config->File_Names.push_back(FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End);
        }

        if (!Config->File_IsReferenced_Get() && Config->File_Names.size()<CountOfFiles && AlreadyPresent)
            Config->File_Names.resize(1); //Removing files, wrong detection
    }

    if (Config->File_Names.size()==Pos)
        return;

    Config->File_IsImageSequence=true;
    Frame_Count_NotParsedIncluded=Pos_Base;
    #if MEDIAINFO_DEMUX
        float64 Demux_Rate=Config->Demux_Rate_Get();
        if (!Demux_Rate)
            Demux_Rate=24;
        if (!SkipComputeDelay)
            Fill(Stream_Video, 0, Video_Delay, float64_int64s(Frame_Count_NotParsedIncluded*1000/Demux_Rate));
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFileSize_Get() || Config->File_Names.size()<=1)
    #endif //MEDIAINFO_ADVANCED
    {
        for (; Pos<Config->File_Names.size(); Pos++)
        {
            int64u Size=File::Size_Get(Config->File_Names[Pos]);
            Config->File_Sizes.push_back(Size);
            Config->File_Size+=Size;
        }
    }
    #if MEDIAINFO_ADVANCED
        else
        {
            Config->File_Size=(int64u)-1;
            File_Size=(int64u)-1;
            Clear(Stream_General, 0, General_FileSize);
        }
    #endif //MEDIAINFO_ADVANCED

    File_Size=Config->File_Size;
    Element[0].Next=File_Size;
    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFileSize_Get() || Config->File_Names.size()<=1)
    #endif //MEDIAINFO_ADVANCED
        Fill (Stream_General, 0, General_FileSize, File_Size, 10, true);
    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFilesCount_Get())
    #endif //MEDIAINFO_ADVANCED
    {
        Fill (Stream_General, 0, General_CompleteName_Last, Config->File_Names[Config->File_Names.size()-1], true);
        Fill (Stream_General, 0, General_FolderName_Last, FileName::Path_Get(Config->File_Names[Config->File_Names.size()-1]), true);
        Fill (Stream_General, 0, General_FileName_Last, FileName::Name_Get(Config->File_Names[Config->File_Names.size()-1]), true);
        Fill (Stream_General, 0, General_FileExtension_Last, FileName::Extension_Get(Config->File_Names[Config->File_Names.size()-1]), true);
    }

    #if MEDIAINFO_ADVANCED
        if (Config->File_Source_List_Get())
        {
            Ztring SourcePath=FileName::Path_Get(Retrieve(Stream_General, 0, General_CompleteName));
            size_t SourcePath_Size=SourcePath.size()+1; //Path size + path separator size
            for (size_t Pos=0; Pos<Config->File_Names.size(); Pos++)
            {
                Ztring Temp=Config->File_Names[Pos];
                Temp.erase(0, SourcePath_Size);
                Fill(Stream_General, 0, "Source_List", Temp);
            }
            (*Stream_More)[Stream_General][0](Ztring().From_Local("Source_List"), Info_Options)=__T("N NT");
        }
    #endif //MEDIAINFO_ADVANCED
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly()
{
    //Generic
    for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            Streams_Finish_StreamOnly((stream_t)StreamKind, StreamPos);

    //For each kind of (*Stream)
    for (size_t Pos=0; Pos<Count_Get(Stream_General);  Pos++) Streams_Finish_StreamOnly_General(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Video);    Pos++) Streams_Finish_StreamOnly_Video(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Audio);    Pos++) Streams_Finish_StreamOnly_Audio(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Text);     Pos++) Streams_Finish_StreamOnly_Text(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Other);    Pos++) Streams_Finish_StreamOnly_Other(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Image);    Pos++) Streams_Finish_StreamOnly_Image(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Menu);     Pos++) Streams_Finish_StreamOnly_Menu(Pos);
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly(stream_t StreamKind, size_t Pos)
{
    //BitRate from Duration and StreamSize
    if (StreamKind!=Stream_General && StreamKind!=Stream_Other && StreamKind!=Stream_Menu && Retrieve(StreamKind, Pos, "BitRate").empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty())
    {
        float64 Duration=0;
        if (StreamKind==Stream_Video && !Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && !Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_float64();
        int64u StreamSize=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).To_int64u();
        if (Duration>0 && StreamSize>0)
            Fill(StreamKind, Pos, "BitRate", StreamSize*8*1000/Duration, 0);
    }

    //BitRate_Encoded from Duration and StreamSize_Encoded
    if (StreamKind!=Stream_General && StreamKind!=Stream_Other && StreamKind!=Stream_Menu && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Encoded)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize_Encoded)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty())
    {
        float64 Duration=0;
        if (StreamKind==Stream_Video && !Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && !Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_float64();
        int64u StreamSize_Encoded=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize_Encoded)).To_int64u();
        if (Duration>0 && StreamSize_Encoded>0)
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Encoded), StreamSize_Encoded*8*1000/Duration, 0);
    }

    //Duration from Bitrate and StreamSize
    if (StreamKind!=Stream_Other && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, "BitRate").empty() && Count_Get(Stream_Video)+Count_Get(Stream_Audio)>1) //If only one stream, duration will be copied later, useful for exact bitrate calculation
    {
        int64u BitRate=Retrieve(StreamKind, Pos, "BitRate").To_int64u();
        int64u StreamSize=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).To_int64u();
        if (BitRate>0 && StreamSize>0)
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration), StreamSize*8*1000/BitRate);
    }

    //StreamSize from BitRate and Duration
    if (StreamKind!=Stream_Other && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, "BitRate").empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty() && Retrieve(StreamKind, Pos, "BitRate").find(__T(" / "))==std::string::npos) //If not done the first time or by other routine
    {
        int64u BitRate=Retrieve(StreamKind, Pos, "BitRate").To_int64u();
        int64u Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_int64u();
        if (BitRate>0 && Duration>0)
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize), BitRate*Duration/8/1000);
    }

    //Bit rate and maximum bit rate
    if (!Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate)).empty() && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate))==Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Maximum)))
    {
        Clear(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Maximum));
        if (Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Mode)).empty())
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Mode), "CBR");
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_General(size_t UNUSED(StreamPos))
{
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Video(size_t Pos)
{
    //Frame count
    if (Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
    {
        if (Count_Get(Stream_Video)==1 && Count_Get(Stream_Audio)==0)
            Fill(Stream_Video, 0, Video_FrameCount, Frame_Count_NotParsedIncluded);
    }

    //FrameCount from Duration and FrameRate
    if (Retrieve(Stream_Video, Pos, Video_FrameCount).empty())
    {
        int64s Duration=Retrieve(Stream_Video, Pos, Video_Duration).To_int64s();
        if (Duration==0)
            Duration=Retrieve(Stream_General, 0, General_Duration).To_int64s();
        float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
        if (Duration && FrameRate)
           Fill(Stream_Video, Pos, Video_FrameCount, Duration*FrameRate/1000, 0);
    }

    //Duration from FrameCount and FrameRate
    if (Retrieve(Stream_Video, Pos, Video_Duration).empty())
    {
        int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
        float64 FrameRate=Retrieve(Stream_Video, Pos, "FrameRate").To_float64();
        if (FrameCount && FrameRate)
           Fill(Stream_Video, Pos, Video_Duration, FrameCount/FrameRate*1000, 0);
    }

    //FrameRate from FrameCount and Duration
    if (Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
    {
        int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
        float64 Duration=Retrieve(Stream_Video, Pos, Video_Duration).To_float64()/1000;
        if (FrameCount && Duration)
           Fill(Stream_Video, Pos, Video_FrameRate, FrameCount/Duration, 3);
    }

    //Pixel Aspect Ratio forced to 1.000 if none
    if (Retrieve(Stream_Video, Pos, Video_PixelAspectRatio).empty())
        Fill(Stream_Video, Pos, Video_PixelAspectRatio, 1.000);

    //Standard
    if (Retrieve(Stream_Video, Pos, Video_Standard).empty() && (Retrieve(Stream_Video, Pos, Video_Width)==__T("720") || Retrieve(Stream_Video, Pos, Video_Width)==__T("704")))
    {
             if (Retrieve(Stream_Video, Pos, Video_Height)==__T("576") && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("25.000"))
            Fill(Stream_Video, Pos, Video_Standard, "PAL");
        else if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("486") || Retrieve(Stream_Video, Pos, Video_Height)==__T("480")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("29.970"))
            Fill(Stream_Video, Pos, Video_Standard, "NTSC");
    }
    if (Retrieve(Stream_Video, Pos, Video_Standard).empty() && Retrieve(Stream_Video, Pos, Video_Width)==__T("352"))
    {
             if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("576") || Retrieve(Stream_Video, Pos, Video_Height)==__T("288")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("25.000"))
            Fill(Stream_Video, Pos, Video_Standard, "PAL");
        else if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("486") || Retrieve(Stream_Video, Pos, Video_Height)==__T("480") || Retrieve(Stream_Video, Pos, Video_Height)==__T("243") || Retrieve(Stream_Video, Pos, Video_Height)==__T("240")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("29.970"))
            Fill(Stream_Video, Pos, Video_Standard, "NTSC");
    }

    //Known ScanTypes
    if (Retrieve(Stream_Video, Pos, Video_ScanType).empty()
     && (Retrieve(Stream_Video, Pos, Video_Format)==__T("RED")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("CineForm")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("DPX")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("EXR")))
            Fill(Stream_Video, Pos, Video_ScanType, "Progressive");

    //Useless chroma subsampling
    if (Retrieve(Stream_Video, Pos, Video_ColorSpace)==__T("RGB")
     && Retrieve(Stream_Video, Pos, Video_ChromaSubsampling)==__T("4:4:4"))
        Clear(Stream_Video, Pos, Video_ChromaSubsampling);

    //Chroma subsampling position
    if (Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_String).empty() && !Retrieve(Stream_Video, Pos, Video_ChromaSubsampling).empty())
    {
        if (Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_Position).empty())
            Fill(Stream_Video, Pos, Video_ChromaSubsampling_String, Retrieve(Stream_Video, Pos, Video_ChromaSubsampling));
        else
            Fill(Stream_Video, Pos, Video_ChromaSubsampling_String, Retrieve(Stream_Video, Pos, Video_ChromaSubsampling)+__T(" (")+ Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_Position)+__T(')'));
    }

    //Commercial name
    #if defined(MEDIAINFO_VC3_YES)
        if (Retrieve(Stream_Video, Pos, Video_Format_Commercial_IfAny).empty() && Retrieve(Stream_Video, Pos, Video_Format)==__T("VC-3"))
        {
            //http://www.avid.com/static/resources/US/documents/dnxhd.pdf
            int64u Height=Retrieve(Stream_Video, Pos, Video_Height).To_int64u();
            int64u BitRate=float64_int64s(Retrieve(Stream_Video, Pos, Video_BitRate).To_float64()/1000000);
            int64u FrameRate=float64_int64s(Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64());
            int64u BitRate_Final=0;
            if (Height>=900 && Height<=1300)
            {
                if (FrameRate==60)
                {
                    if (BitRate>=420 && BitRate<440) //440
                        BitRate_Final=440;
                    if (BitRate>=271 && BitRate<311) //291
                        BitRate_Final=290;
                    if (BitRate>=80 && BitRate<100) //90
                        BitRate_Final=90;
                }
                if (FrameRate==50)
                {
                    if (BitRate>=347 && BitRate<387) //367
                        BitRate_Final=365;
                    if (BitRate>=222 && BitRate<262) //242
                        BitRate_Final=240;
                    if (BitRate>=65 && BitRate<85) //75
                        BitRate_Final=75;
                }
                if (FrameRate==30)
                {
                    if (BitRate>=420 && BitRate<440) //440
                        BitRate_Final=440;
                    if (BitRate>=200 && BitRate<240) //220
                        BitRate_Final=220;
                    if (BitRate>=130 && BitRate<160) //145
                        BitRate_Final=145;
                    if (BitRate>=90 && BitRate<110) //100
                        BitRate_Final=100;
                    if (BitRate>=40 && BitRate<50) //45
                        BitRate_Final=45;
                }
                if (FrameRate==25)
                {
                    if (BitRate>=347 && BitRate<387) //367
                        BitRate_Final=365;
                    if (BitRate>=164 && BitRate<204) //184
                        BitRate_Final=185;
                    if (BitRate>=111 && BitRate<131) //121
                        BitRate_Final=120;
                    if (BitRate>=74 && BitRate<94) //84
                        BitRate_Final=85;
                    if (BitRate>=31 && BitRate<41) //36
                        BitRate_Final=36;
                }
                if (FrameRate==24)
                {
                    if (BitRate>=332 && BitRate<372) //352
                        BitRate_Final=350;
                    if (BitRate>=156 && BitRate<196) //176
                        BitRate_Final=175;
                    if (BitRate>=105 && BitRate<125) //116
                        BitRate_Final=116;
                    if (BitRate>=70 && BitRate<90) //80
                        BitRate_Final=80;
                    if (BitRate>=31 && BitRate<41) //36
                        BitRate_Final=36;
                }
            }
            if (Height>=600 && Height<=800)
            {
                if (FrameRate==60)
                {
                    if (BitRate>=200 && BitRate<240) //220
                        BitRate_Final=220;
                    if (BitRate>=130 && BitRate<160) //145
                        BitRate_Final=145;
                    if (BitRate>=90 && BitRate<110) //110
                        BitRate_Final=100;
                }
                if (FrameRate==50)
                {
                    if (BitRate>=155 && BitRate<195) //175
                        BitRate_Final=175;
                    if (BitRate>=105 && BitRate<125) //115
                        BitRate_Final=115;
                    if (BitRate>=75 && BitRate<95) //85
                        BitRate_Final=85;
                }
                if (FrameRate==30)
                {
                    if (BitRate>=100 && BitRate<120) //110
                        BitRate_Final=110;
                    if (BitRate>=62 && BitRate<82) //72
                        BitRate_Final=75;
                    if (BitRate>=44 && BitRate<56) //51
                        BitRate_Final=50;
                }
                if (FrameRate==25)
                {
                    if (BitRate>=82 && BitRate<102) //92
                        BitRate_Final=90;
                    if (BitRate>=55 && BitRate<65) //60
                        BitRate_Final=60;
                    if (BitRate>=38 && BitRate<48) //43
                        BitRate_Final=45;
                }
                if (FrameRate==24)
                {
                    if (BitRate>=78 && BitRate<98) //88
                        BitRate_Final=90;
                    if (BitRate>=53 && BitRate<63) //58
                        BitRate_Final=60;
                    if (BitRate>=36 && BitRate<46) //41
                        BitRate_Final=41;
                }
            }

            if (BitRate_Final)
            {
                int64u BitDepth=Retrieve(Stream_Video, Pos, Video_BitDepth).To_int64u();
                if (BitDepth==8 || BitDepth==10)
                    Fill(Stream_Video, Pos, Video_Format_Commercial_IfAny, __T("DNxHD ")+Ztring::ToZtring(BitRate_Final)+(BitDepth==10?__T("x"):__T(""))); //"x"=10-bit
            }
        }
    #endif //defined(MEDIAINFO_VC3_YES)
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Audio(size_t Pos)
{
    //SamplingCount
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingCount).empty())
    {
        int64s Duration=Retrieve(Stream_Audio, Pos, Audio_Duration).To_int64s();
        if (Duration==0)
            Duration=Retrieve(Stream_General, 0, General_Duration).To_int64s();
        float SamplingRate=Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_float32();
        if (Duration && SamplingRate)
            Fill(Stream_Audio, Pos, Audio_SamplingCount, ((float64)Duration)/1000*SamplingRate, 0);
    }

    //Frame count
    if (Retrieve(Stream_Audio, Pos, Audio_FrameCount).empty() && Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
    {
        if (Count_Get(Stream_Video)==0 && Count_Get(Stream_Audio)==1)
            Fill(Stream_Audio, 0, Audio_FrameCount, Frame_Count_NotParsedIncluded);
    }

    //FrameRate same as SampleRate
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_float64() == Retrieve(Stream_Audio, Pos, Audio_FrameRate).To_float64())
        Clear(Stream_Audio, Pos, Audio_FrameRate);

    //SamplingRate
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingRate).empty())
    {
        float64 BitDepth=Retrieve(Stream_Audio, Pos, Audio_BitDepth).To_float64();
        float64 Channels=Retrieve(Stream_Audio, Pos, Audio_Channel_s_).To_float64();
        float64 BitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_float64();
        if (BitDepth && Channels && BitRate)
            Fill(Stream_Audio, Pos, Audio_SamplingRate, BitRate/Channels/BitDepth, 0);
    }

    //SamplesPerFrames
    if (Retrieve(Stream_Audio, Pos, Audio_SamplesPerFrame).empty())
    {
        float64 FrameRate=Retrieve(Stream_Audio, Pos, Audio_FrameRate).To_float64();
        float64 SamplingRate=0;
        ZtringList SamplingRates;
        SamplingRates.Separator_Set(0, " / ");
        SamplingRates.Write(Retrieve(Stream_Audio, Pos, Audio_SamplingRate));
        for (size_t i=0; i<SamplingRates.size(); ++i)
        {
            SamplingRate = SamplingRates[i].To_float64();
            if (SamplingRate)
                break; // Using the first valid one
        }
        if (FrameRate && SamplingRate && FrameRate!=SamplingRate)
        {
            float64 SamplesPerFrameF=SamplingRate/FrameRate;
            Ztring SamplesPerFrame;
            if (SamplesPerFrameF>1601 && SamplesPerFrameF<1602)
                SamplesPerFrame = __T("1601.6"); // Usually this is 29.970 fps PCM. TODO: check if it is OK in all cases
            else if (SamplesPerFrameF>800 && SamplesPerFrameF<801)
                SamplesPerFrame = __T("800.8"); // Usually this is 59.940 fps PCM. TODO: check if it is OK in all cases
            else
                SamplesPerFrame.From_Number(SamplesPerFrameF, 0);
            Fill(Stream_Audio, Pos, Audio_SamplesPerFrame, SamplesPerFrame);
        }
    }

    //Duration
    if (Retrieve(Stream_Audio, Pos, Audio_Duration).empty() && Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_int64u()!=0)
    {
        int64u Duration=Retrieve(Stream_Audio, Pos, Audio_SamplingCount).To_int64u()*1000/Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_int64u();
        if (Duration)
           Fill(Stream_Audio, Pos, Audio_Duration, Duration);
    }

    //Stream size
    if (Retrieve(Stream_Audio, Pos, Audio_StreamSize).empty() && !Retrieve(Stream_Audio, Pos, Audio_BitRate).empty() && !Retrieve(Stream_Audio, Pos, Audio_Duration).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate_Mode)==__T("CBR"))
    {
        int64u Duration=Retrieve(Stream_Audio, Pos, Audio_Duration).To_int64u();
        int64u BitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_int64u();
        if (Duration && BitRate)
            Fill(Stream_Audio, Pos, Audio_StreamSize, Duration*BitRate/8/1000);
    }

    //CBR/VBR
    if (Retrieve(Stream_Audio, Pos, Audio_BitRate_Mode).empty() && !Retrieve(Stream_Audio, Pos, Audio_Codec).empty())
    {
        Ztring Z1=MediaInfoLib::Config.Codec_Get(Retrieve(Stream_Audio, Pos, Audio_Codec), InfoCodec_BitRate_Mode, Stream_Audio);
        if (!Z1.empty())
            Fill(Stream_Audio, Pos, Audio_BitRate_Mode, Z1);
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Text(size_t Pos)
{
    //FrameRate from FrameCount and Duration
    if (Retrieve(Stream_Text, Pos, Text_FrameRate).empty())
    {
        int64u FrameCount=Retrieve(Stream_Text, Pos, Text_FrameCount).To_int64u();
        float64 Duration=Retrieve(Stream_Text, Pos, Text_Duration).To_float64()/1000;
        if (FrameCount && Duration)
           Fill(Stream_Text, Pos, Text_FrameRate, FrameCount/Duration, 3);
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Other(size_t UNUSED(StreamPos))
{
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Image(size_t UNUSED(StreamPos))
{
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Menu(size_t UNUSED(StreamPos))
{
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_InterStreams()
{
    //Duration if General not filled
    if (Retrieve(Stream_General, 0, General_Duration).empty())
    {
        int64u Duration=0;
        //For all streams (Generic)
        for (size_t StreamKind=Stream_Video; StreamKind<Stream_Max; StreamKind++)
            for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind); Pos++)
            {
                if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).empty())
                {
                    int64u Duration_Stream=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).To_int64u();
                    if (Duration_Stream>Duration)
                        Duration=Duration_Stream;
                }
            }

        //Filling
        if (Duration>0)
            Fill(Stream_General, 0, General_Duration, Duration);
    }

    //(*Stream) size if all stream sizes are OK
    if (Retrieve(Stream_General, 0, General_StreamSize).empty())
    {
        int64u StreamSize_Total=0;
        bool IsOK=true;
        //For all streams (Generic)
        for (size_t StreamKind=Stream_Video; StreamKind<Stream_Max; StreamKind++)
        {
            if (StreamKind!=Stream_Other && StreamKind!=Stream_Menu) //They have no big size, we never calculate them
                for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind); Pos++)
                {
                    if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize_Encoded)).empty())
                        StreamSize_Total+=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize_Encoded)).To_int64u();
                    else if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize)).empty())
                        StreamSize_Total+=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize)).To_int64u();
                    else
                        IsOK=false; //StreamSize not available for 1 stream, we can't calculate
                }
        }

        //Filling
        if (IsOK && StreamSize_Total>0 && StreamSize_Total<File_Size)
            Fill(Stream_General, 0, General_StreamSize, File_Size-StreamSize_Total);
    }

    //OverallBitRate if we have one Audio stream with bitrate
    if (Retrieve(Stream_General, 0, General_Duration).empty() && Retrieve(Stream_General, 0, General_OverallBitRate).empty() && Count_Get(Stream_Video)==0 && Count_Get(Stream_Audio)==1 && Retrieve(Stream_Audio, 0, Audio_BitRate).To_int64u()!=0 && (Retrieve(Stream_General, 0, General_Format)==Retrieve(Stream_Audio, 0, Audio_Format) || !Retrieve(Stream_General, 0, General_HeaderSize).empty()))
        Fill(Stream_General, 0, General_OverallBitRate, Retrieve(Stream_Audio, 0, Audio_BitRate));

    //OverallBitRate if Duration
    if (Retrieve(Stream_General, 0, General_OverallBitRate).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()!=0 && !Retrieve(Stream_General, 0, General_FileSize).empty())
    {
        float64 Duration=0;
        if (Count_Get(Stream_Video)==1 && Retrieve(Stream_General, 0, General_Duration)==Retrieve(Stream_Video, 0, General_Duration) && !Retrieve(Stream_Video, 0, Video_FrameCount).empty() && !Retrieve(Stream_Video, 0, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(Stream_General, 0, General_Duration).To_float64();
        Fill(Stream_General, 0, General_OverallBitRate, Retrieve(Stream_General, 0, General_FileSize).To_int64u()*8*1000/Duration, 0);
    }

    //Duration if OverallBitRate
    if (Retrieve(Stream_General, 0, General_Duration).empty() && Retrieve(Stream_General, 0, General_OverallBitRate).To_int64u()!=0)
        Fill(Stream_General, 0, General_Duration, Retrieve(Stream_General, 0, General_FileSize).To_float64()*8*1000/Retrieve(Stream_General, 0, General_OverallBitRate).To_float64(), 0);

    //Video bitrate can be the nominal one if <4s (bitrate estimation is not enough precise
    if (Count_Get(Stream_Video)==1 && Retrieve(Stream_Video, 0, Video_BitRate).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()<4000)
    {
        Fill(Stream_Video, 0, Video_BitRate, Retrieve(Stream_Video, 0, Video_BitRate_Nominal));
        Clear(Stream_Video, 0, Video_BitRate_Nominal);
    }

    //Video bitrate if we have all audio bitrates and overal bitrate
    if (Count_Get(Stream_Video)==1 && Retrieve(Stream_General, 0, General_OverallBitRate).size()>4 && Retrieve(Stream_Video, 0, Video_BitRate).empty() && Retrieve(Stream_Video, 0, Video_BitRate_Encoded).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()>=1000) //BitRate is > 10 000 and Duration>10s, to avoid strange behavior
    {
        double GeneralBitRate_Ratio=0.98;  //Default container overhead=2%
        int32u GeneralBitRate_Minus=5000;  //5000 bps because of a "classic" stream overhead
        double VideoBitRate_Ratio  =0.98;  //Default container overhead=2%
        int32u VideoBitRate_Minus  =2000;  //2000 bps because of a "classic" stream overhead
        double AudioBitRate_Ratio  =0.98;  //Default container overhead=2%
        int32u AudioBitRate_Minus  =2000;  //2000 bps because of a "classic" stream overhead
        double TextBitRate_Ratio   =0.98;  //Default container overhead=2%
        int32u TextBitRate_Minus   =2000;  //2000 bps because of a "classic" stream overhead
        //Specific value depends of Container
        if (IsRawStream)
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =1;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =1;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =1;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MPEG-TS"))
        {
            GeneralBitRate_Ratio=0.98;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.97;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.96;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.96;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MPEG-PS"))
        {
            GeneralBitRate_Ratio=0.99;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.99;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.99;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.99;
            TextBitRate_Minus   =0;
        }
        if (MediaInfoLib::Config.Format_Get(Retrieve(Stream_General, 0, General_Format), InfoFormat_KindofFormat)==__T("MPEG-4"))
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =1;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =1;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =1;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("Matroska"))
        {
            GeneralBitRate_Ratio=0.99;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.99;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.99;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.99;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MXF"))
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=1000;
            VideoBitRate_Ratio  =1.00;
            VideoBitRate_Minus  =1000;
            AudioBitRate_Ratio  =1.00;
            AudioBitRate_Minus  =1000;
            TextBitRate_Ratio   =1.00;
            TextBitRate_Minus   =1000;
        }

        //Testing
        float64 VideoBitRate=Retrieve(Stream_General, 0, General_OverallBitRate).To_float64()*GeneralBitRate_Ratio-GeneralBitRate_Minus;
        bool VideobitRateIsValid=true;
        for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
        {
            float64 AudioBitRate=0;
            if (!Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded)[0]<=__T('9')) //Note: quick test if it is a number
                AudioBitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).To_float64();
            else if (!Retrieve(Stream_Audio, Pos, Audio_BitRate).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate)[0]<=__T('9')) //Note: quick test if it is a number
                AudioBitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_float64();
            else
                VideobitRateIsValid=false;
            if (VideobitRateIsValid && AudioBitRate_Ratio)
                VideoBitRate-=AudioBitRate/AudioBitRate_Ratio+AudioBitRate_Minus;
        }
        for (size_t Pos=0; Pos<Count_Get(Stream_Text); Pos++)
        {
            float64 TextBitRate;
            if (Retrieve(Stream_Text, Pos, Text_BitRate_Encoded).empty())
                TextBitRate=Retrieve(Stream_Text, Pos, Text_BitRate).To_float64();
            else
                TextBitRate=Retrieve(Stream_Text, Pos, Text_BitRate_Encoded).To_float64();
            if (TextBitRate_Ratio)
                VideoBitRate-=TextBitRate/TextBitRate_Ratio+TextBitRate_Minus;
            else
                VideoBitRate-=1000; //Estimation: Text stream are not often big
        }
        if (VideobitRateIsValid && VideoBitRate>=10000) //to avoid strange behavior
        {
            VideoBitRate=VideoBitRate*VideoBitRate_Ratio-VideoBitRate_Minus;
            Fill(Stream_Video, 0, Video_BitRate, VideoBitRate, 0); //Default container overhead=2%
            if (Retrieve(Stream_Video, 0, Video_StreamSize).empty() && !Retrieve(Stream_Video, 0, Video_Duration).empty())
            {
                float64 Duration=0;
                if (!Retrieve(Stream_Video, 0, Video_FrameCount).empty() && !Retrieve(Stream_Video, 0, Video_FrameRate).empty())
                {
                    int64u FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount).To_int64u();
                    float64 FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate).To_float64();
                    if (FrameCount && FrameRate)
                        Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
                }
                if (Duration==0)
                    Duration=Retrieve(Stream_Video, 0, Video_Duration).To_float64();
                if (Duration)
                {
                    int64u StreamSize=float64_int64s(VideoBitRate/8*Duration/1000);
                    if (IsRawStream && File_Size!=(int64u)-1 && StreamSize>=File_Size*0.99)
                        StreamSize=File_Size;
                    Fill(Stream_Video, 0, Video_StreamSize, StreamSize);
                }
            }
        }
    }

    //General stream size if we have all streamsize
    if (File_Size!=(int64u)-1 && Retrieve(Stream_General, 0, General_StreamSize).empty())
    {
        //Testing
        int64s StreamSize=File_Size;
        bool StreamSizeIsValid=true;
        for (size_t StreamKind_Pos=Stream_General+1; StreamKind_Pos<Stream_Menu; StreamKind_Pos++)
            for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind_Pos); Pos++)
            {
                int64u StreamXX_StreamSize=Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize)).To_int64u();
                if (StreamXX_StreamSize>0 || StreamKind_Pos==Stream_Text)
                    StreamSize-=StreamXX_StreamSize;
                else
                    StreamSizeIsValid=false;
            }
        if (StreamSizeIsValid && StreamSize>=0) //to avoid strange behavior
            Fill(Stream_General, 0, General_StreamSize, StreamSize);
    }

    //General_OverallBitRate_Mode
    if (Retrieve(Stream_General, 0, General_OverallBitRate_Mode).empty())
    {
        bool IsValid=false;
        bool IsCBR=true;
        bool IsVBR=false;
        for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Menu; StreamKind++)
            for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            {
                if (!IsValid)
                    IsValid=true;
                if (Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_BitRate_Mode))!=__T("CBR"))
                    IsCBR=false;
                if (Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_BitRate_Mode))==__T("VBR"))
                    IsVBR=true;
            }
        if (IsValid)
        {
            if (IsCBR)
                Fill(Stream_General, 0, General_OverallBitRate_Mode, "CBR");
            if (IsVBR)
                Fill(Stream_General, 0, General_OverallBitRate_Mode, "VBR");
        }
    }

    //FrameRate if General not filled
    if (Retrieve(Stream_General, 0, General_FrameRate).empty() && Count_Get(Stream_Video))
    {
        Ztring FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate);
        bool IsOk=true;
        if (FrameRate.empty())
        {
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Ztring FrameRate2=Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_FrameRate));
                    if (!FrameRate2.empty() && FrameRate2!=FrameRate)
                        IsOk=false;
                }
        }
        if (IsOk)
            Fill(Stream_General, 0, General_FrameRate, FrameRate);
    }

    //FrameCount if General not filled
    if (Retrieve(Stream_General, 0, General_FrameCount).empty() && Count_Get(Stream_Video) && Retrieve(Stream_General, 0, "IsTruncated").empty())
    {
        Ztring FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount);
        bool IsOk=true;
        if (FrameCount.empty())
        {
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Ztring FrameCount2=Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_FrameCount));
                    if (!FrameCount2.empty() && FrameCount2!=FrameCount)
                        IsOk=false;
                }
        }
        if (IsOk)
            Fill(Stream_General, 0, General_FrameCount, FrameCount);
    }

    //Tags
    Tags();
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_HumanReadable()
{
    //Generic
    for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            for (size_t Parameter=0; Parameter<Count_Get((stream_t)StreamKind, StreamPos); Parameter++)
                Streams_Finish_HumanReadable_PerStream((stream_t)StreamKind, StreamPos, Parameter);
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_HumanReadable_PerStream(stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    Ztring ParameterName=Retrieve(StreamKind, StreamPos, Parameter, Info_Name);
    Ztring Value=Retrieve(StreamKind, StreamPos, Parameter, Info_Text);

    //Strings
    const Ztring &List_Measure_Value=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure);
            if (List_Measure_Value==__T(" byte"))
        FileSize_FileSize123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T(" bps") || List_Measure_Value==__T(" Hz"))
        Kilo_Kilo123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T(" ms"))
        Duration_Duration123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T("Yes"))
        YesNo_YesNo(StreamKind, StreamPos, Parameter);
    else
    {
        Value_Value123(StreamKind, StreamPos, Parameter);

        //Special cases - 120 fps 24/30 mode
        if (StreamKind==Stream_Video
         && List_Measure_Value==__T(" fps")
         && Retrieve(StreamKind, StreamPos, Video_FrameRate).To_int32u()==120
         && Retrieve(StreamKind, StreamPos, Video_FrameRate_Minimum).To_int32u()==24
         && Retrieve(StreamKind, StreamPos, Video_FrameRate_Maximum).To_int32u()==30)
            Fill(Stream_Video, StreamPos_Last, Video_FrameRate_String, MediaInfoLib::Config.Language_Get(Retrieve(StreamKind, StreamPos, Video_FrameRate)+__T(" (24/30)"), __T(" fps")), true);

        //Special cases - Frame rate
        if (StreamKind==Stream_Video
         && Parameter==Video_FrameRate
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate).empty()
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate_Num).empty()
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate_Den).empty())
            Fill(Stream_Video, StreamPos, Video_FrameRate_String, MediaInfoLib::Config.Language_Get(Retrieve(StreamKind, StreamPos, Video_FrameRate)+__T(" (")+Retrieve(StreamKind, StreamPos, Video_FrameRate_Num)+__T("/")+Retrieve(StreamKind, StreamPos, Video_FrameRate_Den)+__T(")"), __T(" fps")), true);
        if (StreamKind==Stream_Video
         && Parameter==Video_FrameRate_Original
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate_Original).empty()
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Num).empty()
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Den).empty())
            Fill(Stream_Video, StreamPos, Video_FrameRate_Original_String, MediaInfoLib::Config.Language_Get(Retrieve(StreamKind, StreamPos, Video_FrameRate_Original)+__T(" (")+Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Num)+__T("/")+Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Den)+__T(")"), __T(" fps")), true);
    }

    //BitRate_Mode / OverallBitRate_Mode
    if (ParameterName==(StreamKind==Stream_General?__T("OverallBitRate_Mode"):__T("BitRate_Mode")) && MediaInfoLib::Config.ReadByHuman_Get())
    {
        Clear(StreamKind, StreamPos, StreamKind==Stream_General?"OverallBitRate_Mode/String":"BitRate_Mode/String");

        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        List.Write(Retrieve(StreamKind, StreamPos, Parameter));

        //Per value
        for (size_t Pos=0; Pos<List.size(); Pos++)
            List[Pos]=MediaInfoLib::Config.Language_Get(Ztring(__T("BitRate_Mode_"))+List[Pos]);

        Ztring Translated=List.Read();
        Fill(StreamKind, StreamPos, StreamKind==Stream_General?"OverallBitRate_Mode/String":"BitRate_Mode/String", Translated.find(__T("BitRate_Mode_"))?Translated:Value);
    }

    //Encoded_Application
    if ((   ParameterName==__T("Encoded_Application")
         || ParameterName==__T("Encoded_Application_CompanyName")
         || ParameterName==__T("Encoded_Application_Name")
         || ParameterName==__T("Encoded_Application_Version")
         || ParameterName==__T("Encoded_Application_Date"))
        && Retrieve(StreamKind, StreamPos, "Encoded_Application/String").empty())
    {
        Ztring CompanyName=Retrieve(StreamKind, StreamPos, "Encoded_Application_CompanyName");
        Ztring Name=Retrieve(StreamKind, StreamPos, "Encoded_Application_Name");
        Ztring Version=Retrieve(StreamKind, StreamPos, "Encoded_Application_Version");
        Ztring Date=Retrieve(StreamKind, StreamPos, "Encoded_Application_Date");
        if (!Name.empty())
        {
            Ztring String;
            if (!CompanyName.empty())
            {
                String+=CompanyName;
                String+=__T(" ");
            }
            String+=Name;
            if (!Version.empty())
            {
                String+=__T(" ");
                String+=Version;
            }
            if (!Date.empty())
            {
                String+=__T(" (");
                String+=Date;
                String+=__T(")");
            }
            Fill(StreamKind, StreamPos, "Encoded_Application/String", String, true);
        }
        else
            Fill(StreamKind, StreamPos, "Encoded_Application/String", Retrieve(StreamKind, StreamPos, "Encoded_Application"), true);
    }

    //Encoded_Library
    if ((   ParameterName==__T("Encoded_Library")
         || ParameterName==__T("Encoded_Library_CompanyName")
         || ParameterName==__T("Encoded_Library_Name")
         || ParameterName==__T("Encoded_Library_Version")
         || ParameterName==__T("Encoded_Library_Date"))
        && Retrieve(StreamKind, StreamPos, "Encoded_Library/String").empty())
    {
        Ztring CompanyName=Retrieve(StreamKind, StreamPos, "Encoded_Library_CompanyName");
        Ztring Name=Retrieve(StreamKind, StreamPos, "Encoded_Library_Name");
        Ztring Version=Retrieve(StreamKind, StreamPos, "Encoded_Library_Version");
        Ztring Date=Retrieve(StreamKind, StreamPos, "Encoded_Library_Date");
        if (!Name.empty())
        {
            Ztring String;
            if (!CompanyName.empty())
            {
                String+=CompanyName;
                String+=__T(" ");
            }
            String+=Name;
            if (!Version.empty())
            {
                String+=__T(" ");
                String+=Version;
            }
            if (!Date.empty())
            {
                String+=__T(" (");
                String+=Date;
                String+=__T(")");
            }
            Fill(StreamKind, StreamPos, "Encoded_Library/String", String, true);
        }
        else
            Fill(StreamKind, StreamPos, "Encoded_Library/String", Retrieve(StreamKind, StreamPos, "Encoded_Library"), true);
    }

    //Format_Settings_Matrix
    if (StreamKind==Stream_Video && Parameter==Video_Format_Settings_Matrix)
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Format_Settings_Matrix_"))+Value);
        Fill(Stream_Video, StreamPos, Video_Format_Settings_Matrix_String, Translated.find(__T("Format_Settings_Matrix_"))?Translated:Value);
    }

    //Scan type
    if (StreamKind==Stream_Video && Parameter==Video_ScanType)
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Interlaced_"))+Value);
        Fill(Stream_Video, StreamPos, Video_ScanType_String, Translated.find(__T("Interlaced_"))?Translated:Value);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanType_Original)
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Interlaced_"))+Value);
        Fill(Stream_Video, StreamPos, Video_ScanType_Original_String, Translated.find(__T("Interlaced_"))?Translated:Value);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanType_StoreMethod)
    {
        Ztring ToTranslate=Ztring(__T("StoreMethod_"))+Value;
        if (!Retrieve(Stream_Video, StreamPos, Video_ScanType_StoreMethod_FieldsPerBlock).empty())
            ToTranslate+=__T('_')+Retrieve(Stream_Video, StreamPos, Video_ScanType_StoreMethod_FieldsPerBlock);
        Ztring Translated=MediaInfoLib::Config.Language_Get(ToTranslate);
        Fill(Stream_Video, StreamPos, Video_ScanType_StoreMethod_String, Translated.find(__T("StoreMethod_"))?Translated:Value);
    }

    //Scan order
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder)
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Interlaced_"))+Value);
        Fill(Stream_Video, StreamPos, Video_ScanOrder_String, Translated.find(__T("Interlaced_"))?Translated:Value);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder_Stored)
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Interlaced_"))+Value);
        Fill(Stream_Video, StreamPos, Video_ScanOrder_Stored_String, Translated.find(__T("Interlaced_"))?Translated:Value);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder_Original)
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Interlaced_"))+Value);
        Fill(Stream_Video, StreamPos, Video_ScanOrder_Original_String, Translated.find(__T("Interlaced_"))?Translated:Value);
    }

    //Interlacement
    if (StreamKind==Stream_Video && Parameter==Video_Interlacement)
    {
        const Ztring &Z1=Retrieve(Stream_Video, StreamPos, Video_Interlacement);
        if (Z1.size()==3)
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, MediaInfoLib::Config.Language_Get(Ztring(__T("Interlaced_"))+Z1));
        else
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, MediaInfoLib::Config.Language_Get(Z1));
        if (Retrieve(Stream_Video, StreamPos, Video_Interlacement_String).empty())
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, Z1, true);
    }

    //FrameRate_Mode
    if (StreamKind==Stream_Video && Parameter==Video_FrameRate_Mode)
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("FrameRate_Mode_"))+Value);
        Fill(Stream_Video, StreamPos, Video_FrameRate_Mode_String, Translated.find(__T("FrameRate_Mode_"))?Translated:Value);
    }

    //Compression_Mode
    if (Parameter==Fill_Parameter(StreamKind, Generic_Compression_Mode))
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Compression_Mode_"))+Value);
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Compression_Mode_String), Translated.find(__T("Compression_Mode_"))?Translated:Value);
    }

    //Delay_Source
    if (Parameter==Fill_Parameter(StreamKind, Generic_Delay_Source))
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Delay_Source_"))+Value);
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Delay_Source_String), Translated.find(__T("Delay_Source_"))?Translated:Value);
    }

    //Gop_OpenClosed
    if (StreamKind==Stream_Video && (Parameter==Video_Gop_OpenClosed || Parameter==Video_Gop_OpenClosed_FirstFrame))
    {
        Ztring Translated=MediaInfoLib::Config.Language_Get(Ztring(__T("Gop_OpenClosed_"))+Value);
        Fill(Stream_Video, StreamPos, Parameter+1, Translated.find(__T("Gop_OpenClosed_"))?Translated:Value, true);
    }
}

} //NameSpace
