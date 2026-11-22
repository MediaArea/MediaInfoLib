/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
//
// Source:
// http://wiki.multimedia.cx/index.php?title=YUV4MPEG2
//
//---------------------------------------------------------------------------

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
#if defined(MEDIAINFO_Y4M_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Y4m.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//***************************************************************************
// Infos
//***************************************************************************

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Y4m::File_Y4m()
:File__Analyze()
{
    //Configuration
    ParserName="YUV4MPEG2";
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;
    Frame_Count_NotParsedIncluded=0;

    //Temp
    HeaderEnd=0;
}

//---------------------------------------------------------------------------
File_Y4m::~File_Y4m()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Y4m::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "YUV4MPEG2");

    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "YUV");
    Fill(Stream_Video, 0, Video_ColorSpace, "YUV");
}

//---------------------------------------------------------------------------
void File_Y4m::Streams_Fill()
{
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Y4m::FileHeader_Begin()
{
    if (Buffer_Size<10)
        return false;

    if (!(Buffer[0]==0x59
       && Buffer[1]==0x55
       && Buffer[2]==0x56
       && Buffer[3]==0x34
       && Buffer[4]==0x4D
       && Buffer[5]==0x50
       && Buffer[6]==0x45
       && Buffer[7]==0x47
       && Buffer[8]==0x32
       && Buffer[9]==0x20))
    {
        Reject();
        return false;
    }

    for (; HeaderEnd<Buffer_Size; HeaderEnd++)
    {
        if (Buffer[HeaderEnd]==0x0A)
        {
            Accept();
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------
void File_Y4m::FileHeader_Parse()
{
    //Parsing
    Ztring Header;
    Get_UTF8(HeaderEnd, Header,                                 "Data");

    ZtringList List; List.Separator_Set(0, " ");
    List.Write(Header);
    int64u Width=0, Height=0, Multiplier=0, Divisor=1;
    float64 FrameRate=0;
    bool ColorSpace_IsPresent=false;
    for (size_t Pos=1; Pos<List.size(); Pos++)
    {
        if (!List[Pos].empty())
        {
            switch (List[Pos][0])
            {
                case 'A' :  //Pixel aspect ratio
                            {
                            ZtringList Value; Value.Separator_Set(0, ":");
                            Value.Write(List[Pos].substr(1));
                            if (Value.size()==2)
                            {
                                float64 x=Value[0].To_float64();
                                float64 y=Value[1].To_float64();
                                if (y)
                                {
                                    float64 PixelAspectRatio=x/y;
                                    if (PixelAspectRatio>=0.1 && PixelAspectRatio<=10) //Pixels up to 10 times higher or wider than square pixels, other values are not plausible
                                        Fill(Stream_Video, 0, Video_PixelAspectRatio, PixelAspectRatio, 3);
                                }
                            }
                            }
                            break;
                case 'C' :  //Color space
                            {
                            ColorSpace_IsPresent=true;
                            Ztring Value=List[Pos].substr(1);
                            int64u BytesPerSample=1;
                            size_t Digits_Pos=Value.find_last_not_of(__T("0123456789"));
                            if (Digits_Pos!=Ztring::npos && Digits_Pos+1<Value.size()) //High bit depth, e.g. 420p10 or mono16
                            {
                                Ztring Base=Value.substr(0, Value[Digits_Pos]==__T('p')?Digits_Pos:(Digits_Pos+1));
                                if (Base==__T("420") || Base==__T("422") || Base==__T("444") || Base==__T("mono"))
                                {
                                    int64u BitDepth=Ztring(Value.substr(Digits_Pos+1)).To_int64u();
                                    if (BitDepth>8 && BitDepth<=16)
                                    {
                                        Fill(Stream_Video, 0, Video_BitDepth, BitDepth);
                                        BytesPerSample=2;
                                        Value=Base;
                                    }
                                    else
                                        Value.clear(); //Unknown
                                }
                            }
                            if (Value==__T("420jpeg") || Value==__T("420paldv") || Value==__T("420mpeg2") || Value==__T("420"))
                            {
                                Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:2:0");
                                Multiplier=3;
                                Divisor=2;
                            }
                            if (Value==__T("411"))
                            {
                                Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:1:1");
                                Multiplier=3;
                                Divisor=2;
                            }
                            if (Value==__T("422"))
                            {
                                Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:2:2");
                                Multiplier=2;
                            }
                            if (Value==__T("444"))
                            {
                                Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:4:4");
                                Multiplier=3;
                            }
                            if (Value==__T("444alpha"))
                            {
                                Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:4:4");
                                Multiplier=4;
                            }
                            if (Value==__T("mono"))
                                Multiplier=1;
                            Multiplier*=BytesPerSample;
                            }
                            break;
                case 'F' :  //Frame rate
                            {
                            ZtringList Value; Value.Separator_Set(0, ":");
                            Value.Write(List[Pos].substr(1));
                            if (Value.size()==2)
                            {
                                float64 N=Value[0].To_float64();
                                float64 D=Value[1].To_float64();
                                if (N && D)
                                {
                                    FrameRate=N/D;
                                    Fill(Stream_Video, 0, Video_FrameRate, FrameRate, 3);
                                }
                            }
                            }
                            break;
                case 'H' :  //Height
                            {
                            Ztring Value=List[Pos].substr(1);
                            Height=Value.To_int64u();
                            Fill(Stream_Video, 0, Video_Height, Height);
                            }
                            break;
                case 'I' :  //Interlacing
                            if (List[Pos].size()==2)
                                switch (List[Pos][1])
                                {
                                    case 'p' : Fill(Stream_Video, 0, Video_ScanType, "Progressive"); break;
                                    case 't' : Fill(Stream_Video, 0, Video_ScanType, "Interlaced"); Fill(Stream_Video, 0, Video_ScanOrder, "TFF"); break;
                                    case 'b' : Fill(Stream_Video, 0, Video_ScanType, "Interlaced"); Fill(Stream_Video, 0, Video_ScanOrder, "BFF"); break;
                                    case 'm' : Fill(Stream_Video, 0, Video_ScanType, "Mixed"); break;
                                    default  : ;
                                }
                            break;
                case 'W' :  //Width
                            {
                            Ztring Value=List[Pos].substr(1);
                            Width=Value.To_int64u();
                            Fill(Stream_Video, 0, Video_Width, Width);
                            }
                            break;
                default  : ;
            }
        }
    }
    if (!ColorSpace_IsPresent) //420jpeg by default
    {
        Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:2:0");
        Multiplier=3;
        Divisor=2;
    }

    //Duration (we expect no meta per frame)
    if (Width && Height && Multiplier && Divisor && Width<=0x100000 && Height<=0x100000) //Avoid overflow with unrealistic values
    {
        int64u Frame_ByteSize=6+Width*Height*Multiplier/Divisor; //5 for "FRAME\0"
        Fill(Stream_Video, 0, Video_FrameCount, File_Size/Frame_ByteSize);
        if (FrameRate)
            Fill(Stream_Video, 0, Video_BitRate, Width*Height*Multiplier/Divisor*8*FrameRate);
    }

    Finish();
}

} //NameSpace

#endif //MEDIAINFO_Y4M_YES
