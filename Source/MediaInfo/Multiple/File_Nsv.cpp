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
#if defined(MEDIAINFO_NSV_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Nsv.h"
#if defined(MEDIAINFO_MPEG4V_YES)
    #include "MediaInfo/Video/File_Mpeg4v.h"
#endif
#if defined(MEDIAINFO_AAC_YES)
    #include "MediaInfo/Audio/File_Aac.h"
#endif
#if defined(MEDIAINFO_MPEGA_YES)
    #include "MediaInfo/Audio/File_Mpega.h"
#endif
#include "MediaInfo/File_Unknown.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <algorithm>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Private
//***************************************************************************

struct stream
{
    File__Analyze*  Parser=nullptr;
    int32u          len=0;
    int32u          codecid=0;
};

class Private
{
public:
    stream          Streams[2];
    int64u          AudioDelay=0;
    int32u          AuxTotalLen=0;
};

//***************************************************************************
// Const
//***************************************************************************

int8u Nsv_FrameRate_Multiplier[4] =
{
    30,
    30,
    25,
    34,
};

//---------------------------------------------------------------------------
namespace Elements
{
    const int32u AAC_=0x41414320;
    const int32u AACP=0x41414350;
    const int32u DIVX=0x44495658;
    const int32u MP3_=0x4D503320;
    const int32u NONE=0x4E4F4E45;
    const int32u NSVf=0x4E535666;
    const int32u NSVs=0x4E535673;
    const int32u PCM_=0x50434D20;
    const int32u RGB3=0x52474233;
    const int32u SPX_=0x53505820;
    const int32u VLB_=0x564C4220;
    const int32u VP3_=0x56503320;
    const int32u VP30=0x56503330;
    const int32u VP31=0x56503331;
    const int32u VP4_=0x56503420;
    const int32u VP40=0x56503430;
    const int32u VP5_=0x56503520;
    const int32u VP50=0x56503530;
    const int32u VP6_=0x56503620;
    const int32u VP60=0x56503630;
    const int32u VP61=0x56503631;
    const int32u VP62=0x56503632;
    const int32u XVID=0x58564944;
    const int32u YV12=0x59563132;
}

stream_t Stream_Type[2] =
{
    Stream_Video,
    Stream_Audio,
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Nsv::File_Nsv()
: P(nullptr)
{
}

//---------------------------------------------------------------------------
File_Nsv::~File_Nsv()
{
    delete P;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Nsv::Streams_Accept()
{
    P = new Private;

    Fill(Stream_General, 0, General_Format, "NSV");

    //Configuration
    ParserName="NSV";
    #if MEDIAINFO_EVENTS
        StreamIDs_Size=1;
        ParserIDs[0]=MediaInfo_Parser_Nsv;
        StreamIDs_Width[0]=1;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_Level=2; //Container
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(0); //Container1
    #endif //MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Nsv::Streams_Finish()
{
    for (int i=0; i<2; i++)
    {
        auto Parser=P->Streams[i].Parser;
        if (!Parser)
            continue;
        Fill(Parser);
        if (Config->ParseSpeed<1.0)
            Parser->Open_Buffer_Unsynch();
        Finish(Parser);
        Merge(*Parser, Stream_Type[i], 0, 0);
    }

    float64 DisplayAspectRatio=Retrieve_Const(Stream_Video, 0, Video_DisplayAspectRatio).To_float64();
    if (!DisplayAspectRatio)
    {
        float64 Width=Retrieve_Const(Stream_Video, 0, Video_Width).To_float64();
        float64 Height=Retrieve_Const(Stream_Video, 0, Video_Height).To_float64();
        float64 PixelAspectRatio=Retrieve_Const(Stream_Video, 0, Video_PixelAspectRatio).To_float64();
        if (Width && Height && PixelAspectRatio)
            Fill(Stream_Video, 0, Video_DisplayAspectRatio, Width/Height/PixelAspectRatio);
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Nsv::FileHeader_Parse()
{
    // Configuration
    MustSynchronize=true;

    //Parsing
    if (Buffer_Size<4)
    {
        Element_WaitForMoreData();
        return; //Must wait for more data
    }
    int32u nsv_sig;
    Peek_B4(nsv_sig);
    if (nsv_sig!=Elements::NSVf)
        return;
    if (Element_Size<28)
    {
        Element_WaitForMoreData();
        return; //Must wait for more data
    }
    int32u header_size, file_size, file_len_ms, metadata_len, toc_alloc, toc_size;
    Skip_C4(                                                    "nsv_sig");
    Get_L4 (header_size,                                        "header_size");
    Get_L4 (file_size,                                          "file_size");
    Get_L4 (file_len_ms,                                        "file_len_ms");
    Get_L4 (metadata_len,                                       "metadata_len");
    Get_L4 (toc_alloc,                                          "toc_alloc");
    Get_L4 (toc_size,                                           "toc_size");

    // Coherency
    if (!P)
    {
        if (header_size<28
         || header_size>file_size
         || !file_len_ms
         || metadata_len>header_size-28
         || toc_alloc<toc_size
         || toc_alloc>(header_size-28)/4
         || toc_size>(header_size-28)/4
         || ((int32u)-1)-toc_alloc<metadata_len // Next add overflow prevention
         || toc_alloc+metadata_len>header_size-28)
        {
            Reject();
            return;
        }
        Accept();
    }

    // Parsing
    if (Element_Size<header_size)
    {
        Element_WaitForMoreData();
        return; //Must wait for more data
    }
    Fill(Stream_General, 0, General_Duration, file_len_ms);
    Fill(Stream_Video, 0, Video_Duration, file_len_ms);
    Fill(Stream_Audio, 0, Audio_Duration, file_len_ms);
    if (file_size>File_Size)
        Fill(Stream_General, 0, "IsTruncated", "Yes");
    if (metadata_len)
    {
        Element_Begin1("metadata");
        int64u End=Element_Offset+metadata_len;
        while (Element_Offset<End)
        {
            while (Element_Offset<End)
            {
                int8u Space;
                Peek_B1(Space);
                if (Space!=' ')
                    break;
                Element_Offset++;
            }
            if (Element_Offset>=End)
                break;

            Element_Begin1("item");
            int64u Start_Offset=Element_Offset;
            while (Element_Offset<End)
            {
                int8u Equal;
                Peek_B1(Equal);
                if (Equal=='=')
                {
                    int64u Name_Size=Element_Offset-Start_Offset;
                    Element_Offset=Start_Offset;
                    string Name;
                    Get_String (Name_Size, Name,                "name");
                    Element_Offset++;
                    if (Element_Offset<End)
                    {
                        int8u Separator;
                        Peek_B1(Separator);
                        Element_Offset++;
                        Start_Offset=Element_Offset;
                        while (Element_Offset<End)
                        {
                            int8u SecondSeparator;
                            Peek_B1(SecondSeparator);
                            if (SecondSeparator==Separator)
                            {
                                int64u Value_Size=Element_Offset-Start_Offset;
                                Element_Offset=Start_Offset;
                                string Value;
                                Get_String (Value_Size, Value,  "value");
                                auto asciitolower = [](char in) {
                                    if (in <= 'Z' && in >= 'A')
                                        return (char)(in - ('Z' - 'z'));
                                    return in;
                                };
                                string Name2(Name);
                                transform(Name2.begin(), Name2.end(), Name2.begin(), asciitolower);
                                stream_t StreamKind=Stream_General;
                                if (Name2=="title")
                                    Name2="Title";
                                else if (Name2=="aspect")
                                {
                                    Name2="PixelAspectRatio";
                                    StreamKind=Stream_Video;
                                }
                                else
                                    Name2=Name;
                                Fill(StreamKind, 0, Name2.c_str(), Value);
                                Element_Offset++;
                                break;
                            }
                            Element_Offset++;
                        }
                    }
                    break;
                }
                Element_Offset++;
            }
            Element_End0();
        }
        Element_End0();
    }
    if (toc_size)
    {
        Element_Begin1("toc");
        int64u End=Element_Offset+toc_alloc;
        for (int32u i=0; i<toc_size; i++)
        {
            Info_L4(offset,                                     "offset"); Param_Info1(header_size+offset);
        }
        if (toc_size<(toc_alloc+1)/2)
        {
            int32u toc2_marker;
            Peek_B4(toc2_marker);
            if (toc2_marker==0x544F4332) // "TOC2"
            {
                Skip_C4("toc2_marker");
                for (int32u i=0; i<toc_size; i++)
                {
                    Info_L4(frame,                              "frame");
                }
            }
        }
        if (Element_Offset<End)
            Skip_XX(End-Element_Offset,                         "toc_padding");
        Element_End0();
    }
    if (Element_Offset<header_size)
        Skip_XX(header_size-Element_Offset,                     "header_padding");
    Element_End0();

    //Synched is OK
    Synched=true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Nsv::Synchronize()
{
    //Synchronizing
    while (Buffer_Size-Buffer_Offset>=4)
    {
        if (BigEndian2int32u(Buffer+Buffer_Offset)==Elements::NSVs)
        {
            if (Buffer_Size-Buffer_Offset<24)
                return false;
            auto aux_plus_video_len=LittleEndian2int24u(Buffer+Buffer_Offset+19);
            auto video_len=aux_plus_video_len>>4;
            auto audio_len=LittleEndian2int16u(Buffer+Buffer_Offset+22);
            auto Size=24+video_len+audio_len;
            if (File_Size-(File_Offset+Buffer_Offset)==Size)
                break;
            if (Buffer_Size-Buffer_Offset<Size+4)
                return false;
            auto sync_hdr=BigEndian2int32u(Buffer+Buffer_Offset+Size);
            if (!(sync_hdr!=Elements::NSVs && (sync_hdr>>16)!=0xEFBE))
                break;
        }
        Buffer_Offset++;
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+3==Buffer_Size && BigEndian2int24u(Buffer+Buffer_Offset)!=(Elements::NSVs>>8))
    {
        Buffer_Offset++;
        if (BigEndian2int16u(Buffer+Buffer_Offset)!=(Elements::NSVs>>16))
        {
            Buffer_Offset++;
            if (BigEndian2int8u(Buffer+Buffer_Offset)!=(Elements::NSVs>>24))
                Buffer_Offset++;
        }
    }

    if (Buffer_Size-Buffer_Offset<4)
        return false;

    //Synched is OK
    Synched=true;
    return true;
}

//---------------------------------------------------------------------------
bool File_Nsv::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Size-Buffer_Offset<4)
        return false;

    int32u sync_hdr=BigEndian2int32u(Buffer+Buffer_Offset);
    if (sync_hdr!=Elements::NSVs && (sync_hdr>>16)!=0xEFBE)
    {
        Synched=false;
        return true;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Nsv::Header_Parse()
{
    //Parsing
    int32u sync_hdr;
    Peek_B4(sync_hdr);
    if (sync_hdr==Elements::NSVs)
    {
        int32u vidfmt, audfmt;
        int16u width, height, syncoffs;
        int8u framerate_idx;
        Skip_C4(                                                "sync_hdr");
        Get_C4 (vidfmt,                                         "vidfmt");
        Get_C4 (audfmt,                                         "audfmt");
        Get_L2 (width,                                          "width");
        Get_L2 (height,                                         "height");
        Get_L1 (framerate_idx,                                  "framerate_idx");
        Get_L2 (syncoffs,                                       "syncoffs");

        if (!Frame_Count)
        {
            if (!P)
                Accept();
            if (Element_Size<24)
            {
                Element_WaitForMoreData();
                return; //Must wait for more data
            }
            P->Streams[0].codecid=vidfmt==Elements::NONE?0:vidfmt;
            P->Streams[1].codecid=audfmt==Elements::NONE?0:audfmt;
            if (framerate_idx)
            {
                if (!(framerate_idx>>7))
                    FrameInfo.DUR=1000000000/framerate_idx;
                else
                {
                    int8u T=(framerate_idx&0x7F)>>2;
                    float64 S;
                    if (T<16)
                        S=((float64)1)/(T+1);
                    else
                        S=T-1;
                    if (framerate_idx&1)
                        S=S/1.001;
                    FrameInfo.DUR=float64_int64s(1000000000/(S*Nsv_FrameRate_Multiplier[framerate_idx&3]));
                }
                FrameInfo.PTS=0;
            }
            if (width)
                Fill(Stream_Video, 0, Video_Width, width, 10, true);
            if (height)
                Fill(Stream_Video, 0, Video_Height, height, 10, true);
        }
        if (P->AudioDelay!=(int64u)-1)
            P->AudioDelay=FrameInfo.PTS==(int64u)-1?((int64u)-1):(FrameInfo.PTS+(int64u)syncoffs*1000000);
    }
    else if ((sync_hdr>>16)==0xEFBE)
    {
        Skip_B2(                                                "nosync_hdr");
    }
    int32u aux_plus_video_len;
    int16u audio_len, AuxTotalLen=0;
    Get_L3 (aux_plus_video_len,                                 "aux_plus_video_len");
    Get_L2 (audio_len,                                          "audio_len");
    auto num_aux=aux_plus_video_len&0xF;
    int32u video_len=aux_plus_video_len>>4;
    for (auto i=0; i<num_aux; i++)
    {
        int16u aux_chunk_len;
        Get_L2 (aux_chunk_len,                                  "aux_chunk_len");
        Skip_C4(                                                "aux_chunk_type");
        AuxTotalLen+=aux_chunk_len;
    }

    FILLING_BEGIN();
        if (video_len<AuxTotalLen)
        {
            Trusted_IsNot("aux size too big");
            return;
        }
        video_len-=AuxTotalLen;
        P->Streams[0].len=video_len;
        P->Streams[1].len=audio_len;
        P->AuxTotalLen=AuxTotalLen;
        Header_Fill_Code(0, "Frame");
        Header_Fill_Size(Element_Offset+video_len+audio_len);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Nsv::Data_Parse()
{
    Element_Info1(Frame_Count);
    if (FrameInfo.PTS!=(int64u)-1)
        Element_Info1(FrameInfo.PTS/1000000000.0);

    for (int i=0; i<2; i++)
    {
        auto& Stream=P->Streams[i];
        if (Stream.len)
        {
            // Init
            if (!Stream.Parser)
            {
                Stream_Prepare(Stream_Type[i]);
                Fill(Stream_Type[i], 0, Fill_Parameter(Stream_Type[i], Generic_CodecID), Ztring().From_CC4(Stream.codecid));
                File__Analyze* Parser;
                switch(Stream_Type[i])
                {
                    case Stream_Video:
                                        switch (Stream.codecid)
                                        {
                                            case Elements::DIVX:
                                            case Elements::XVID:
                                                                    #if defined(MEDIAINFO_MPEG4V_YES)
                                                                        Parser=new File_Mpeg4v();
                                                                    #else
                                                                        Parser=new File_Unknown();
                                                                        Open_Buffer_Init(Parser);
                                                                        Parser->Stream_Prepare(Stream_Video);
                                                                        Parser->Fill(Stream_Video, 0, Video_Format, "MPEG-4 Visual");
                                                                    #endif //defined(MEDIAINFO_MPEG4V_YES)
                                                                    break;
                                            case Elements::RGB3:
                                                                    Parser=new File_Unknown;
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Video);
                                                                    Parser->Fill(Stream_Video, 0, Video_Format, "RGB");
                                                                    Parser->Fill(Stream_Video, 0, Video_ColorSpace, "RGB");
                                                                    Parser->Fill(Stream_Video, 0, Video_BitDepth, 8);
                                                                    break;
                                            case Elements::VP3_:
                                            case Elements::VP30:
                                            case Elements::VP31:
                                            case Elements::VP4_:
                                            case Elements::VP40:
                                            case Elements::VP5_:
                                            case Elements::VP50:
                                            case Elements::VP6_:
                                            case Elements::VP60:
                                            case Elements::VP61:
                                            case Elements::VP62:
                                                                    Parser=new File_Unknown;
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Video);
                                                                    Parser->Fill(Stream_Video, 0, Video_Format, "VP"+string(1, (Stream.codecid>>8)&0xFF));
                                                                    break;
                                            case Elements::YV12:
                                                                    Parser=new File_Unknown;
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Video);
                                                                    Parser->Fill(Stream_Video, 0, Video_Format, "YUV");
                                                                    Parser->Fill(Stream_Video, 0, Video_ColorSpace, "YUV");
                                                                    Parser->Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:2:0");
                                                                    Parser->Fill(Stream_Video, 0, Video_BitDepth, 8);
                                                                    break;
                                            default            :    Parser=new File_Unknown();
                                        }
                                        break;
                    case Stream_Audio:
                                        switch (Stream.codecid)
                                        {
                                            case Elements::AAC_:
                                            case Elements::AACP:
                                            case Elements::VLB_:
                                                                    #if defined(MEDIAINFO_AAC_YES)
                                                                        Parser=new File_Aac();
                                                                        ((File_Aac*)Parser)->Mode=File_Aac::Mode_ADTS;
                                                                    #else
                                                                        Parser=new File_Unknown();
                                                                        Open_Buffer_Init(Parser);
                                                                        Parser->Stream_Prepare(Stream_Audio);
                                                                        Parser->Fill(Stream_Audio, 0, Audio_Format, "AAC");
                                                                        Parser->Fill(Stream_Audio, 0, Audio_MuxingMode, "ADTS");
                                                                    #endif //defined(MEDIAINFO_AAC_YES)
                                                                    break;
                                            case Elements::MP3_:
                                                                    #if defined(MEDIAINFO_MPEGA_YES)
                                                                        Parser=new File_Mpega();
                                                                    #else
                                                                        Parser=new File_Unknown();
                                                                        Open_Buffer_Init(Parser);
                                                                        Parser->Stream_Prepare(Stream_Audio);
                                                                        Parser->Fill(Stream_Audio, 0, Audio_Format, "MPEG Audio");
                                                                    #endif //defined(MEDIAINFO_MPEGA_YES)
                                                                    break;
                                            case Elements::PCM_:
                                                                    Parser=new File_Unknown();
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Audio);
                                                                    Parser->Fill(Stream_Audio, 0, Audio_Format, "PCM");
                                                                    if (Stream.len>=4)
                                                                    {
                                                                        auto AudioBuffer=Buffer+Buffer_Offset+(size_t)Element_Offset;
                                                                        Parser->Fill(Stream_Audio, 0, Audio_BitDepth, LittleEndian2int8u(AudioBuffer++));
                                                                        Parser->Fill(Stream_Audio, 0, Audio_Channel_s_, LittleEndian2int8u(AudioBuffer++));
                                                                        Parser->Fill(Stream_Audio, 0, Audio_SamplingRate, LittleEndian2int16u(AudioBuffer));
                                                                    }
                                                                    break;
                                            case Elements::SPX_:
                                                                    Parser=new File_Unknown();
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Audio);
                                                                    Parser->Fill(Stream_Audio, 0, Audio_Format, "Speex");
                                                                    break;
                                            default            :    Parser=new File_Unknown();
                                        }
                                        break;
                }
                Open_Buffer_Init(Parser);
                Stream.Parser=Parser;
            }

            // Parse
            Element_Begin1("stream");
            Element_Info1(i);
            Element_Code=i;
            Open_Buffer_Continue(Stream.Parser, Stream.len);
            if (Stream.Parser->Status[IsAccepted])
            {
                Demux(Buffer+Buffer_Offset+(size_t)Element_Offset-Stream.len, Stream.len, ContentType_MainStream);
                if (FrameInfo.DUR!=-1)
                {
                    switch(Stream_Type[i])
                    {
                        case Stream_Video:
                                            if (Retrieve(Stream_Video, 0, Video_Delay).empty())
                                                Fill(Stream_Video, 0, Video_Delay, float64_int64s(((float64)FrameInfo.PTS)/1000000));
                                            break;
                        case Stream_Audio:
                                            if (P->AudioDelay!=(int64u)-1 && Retrieve(Stream_Audio, 0, Audio_Delay).empty())
                                                Fill(Stream_Audio, 0, Audio_Delay, float64_int64s(((float64)P->AudioDelay)/1000000));
                                            break;
                    }
                }
            }
            else if (i)
                P->AudioDelay=(int64u)-1;
            #if MEDIAINFO_TRACE
            else
            {
                Element_Show();
                Element_Offset-=Stream.len;
                Skip_XX(Stream.len,                             "Can not be decoded");
            }
            #endif //MEDIAINFO_TRACE
            Element_End0();
        }
    }

    Frame_Count++;
    if (FrameInfo.DUR!=(int64u)-1)
        FrameInfo.PTS+=FrameInfo.DUR;
    if (Config->ParseSpeed<1.0
     && (Frame_Count>=300
      || ((!P->Streams[0].codecid || (P->Streams[0].Parser && P->Streams[0].Parser->Status[IsAccepted]))
       && (!P->Streams[1].codecid || (P->Streams[1].Parser && P->Streams[1].Parser->Status[IsAccepted])))))
        Finish();
}

} //NameSpace

#endif //MEDIAINFO_NSV_YES
