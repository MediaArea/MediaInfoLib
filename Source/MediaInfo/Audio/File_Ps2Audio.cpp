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
#if defined(MEDIAINFO_PS2A_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Ps2Audio.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
enum ps2audio_coding
{
    Coding_Unknown,
    Coding_Pcm, //16-bit, little endian
    Coding_Adpcm, //Frames of 16 bytes for 28 samples
};

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ps2Audio::Read_Buffer_Continue()
{
    //Parsing
    while (Element_Offset<Element_Size)
    {
        if (Element_Size-Element_Offset<4)
            return; //Waiting for the identifier of the next element

        int32u ID;
        Peek_B4(ID);
        switch (ID)
        {
            case 0x53536264 :   if (Element_Size-Element_Offset<8)
                                    return; //Waiting for the size of the body
                                SSbd(); break;
            case 0x53536864 :   if (Element_Size-Element_Offset<0x20)
                                    return; //Waiting for the end of the header
                                SShd(); break;
            default         :   Element_Offset=Element_Size; Reject("PS2 Audio");
        }
    }
}

//---------------------------------------------------------------------------
void File_Ps2Audio::SSbd()
{
    if (Count_Get(Stream_Audio)!=1)
    {
        Trusted_IsNot("Element should not be here");
        Element_Offset=Element_Size;
        return;
    }

    Element_Begin1("SSbd (Body)");
        int32u Size;
        Skip_C4(                                                "ID");
        Get_L4 (Size,                                           "Size");
        int64u Data_Begin=File_Offset+Buffer_Offset+Element_Offset;
        Skip_XX(Element_Size-Element_Offset,                    "Data (Partial)");
    Element_End0();

    FILLING_BEGIN();
        if (!IsSub && Header_Size==0x20 && Size<0x80000000 && (int64u)Size*2==File_Size-0x18)
            Size=Size*2-0x10; //Header size 32 and body size of (data size + 16) / 2 are found together
        if (!IsSub && Data_Begin+Size>File_Size)
        {
            //The file ends before the end of the body
            IsTruncated(Data_Begin+Size, false, "PS2 Audio");
            Size=(int32u)(File_Size-Data_Begin);
        }
        Fill(Stream_Audio, 0, Audio_StreamSize, Size);
        if (SamplingRate && Channels)
        {
            int64u Samples=(int64u)-1;
            switch (Coding)
            {
                case Coding_Pcm   : Samples=Size/Channels/2; break;
                case Coding_Adpcm : Samples=(int64u)(Size/Channels/16)*28; break;
                default           : ;
            }
            if (Samples!=(int64u)-1)
                Fill(Stream_Audio, 0, Audio_Duration, Samples*1000/SamplingRate);
        }

        Finish("PS2 Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ps2Audio::SShd()
{
    Element_Begin1("SShd (Header)");
        //Parsing
        int32u Size, Format_Header, SamplingRate_Header, Channels_Header;
        Skip_C4(                                                "ID");
        Get_L4 (Size,                                           "Size");
        if (Size!=0x18 && Size!=0x20 && (IsSub || Size!=File_Size-8)) //0x20 and the file size minus 8 are also found, the header has the same content
        {
            Trusted_IsNot("Bad size");
            Element_End0();
            return;
        }
        Get_L4 (Format_Header,                                  "Format");
        Get_L4 (SamplingRate_Header,                            "Sampling rate");
        Get_L4 (Channels_Header,                                "Channels");
        Skip_L4(                                                "Interleave");
        Skip_L4(                                                "Loop start");
        Skip_L4(                                                "Loop end");
    Element_End0();

    FILLING_BEGIN();
        if (Status[IsAccepted])
            return; //Only the first header is used
        Header_Size=Size;
        SamplingRate=SamplingRate_Header;
        Channels=Channels_Header;

        Accept("PS2 Audio");
        Fill(Stream_General, 0, General_Format, "PS2 Audio");

        Stream_Prepare(Stream_Audio);
        Ztring FormatS;
        float64 BitsPerSample=0;
        switch(Format_Header)
        {
            case 0x00000001 :
            case 0x80000001 : Coding=Coding_Pcm; FormatS=__T("PCM"); BitsPerSample=16; break;
            case 0x00000002 :
            case 0x00000010 : Coding=Coding_Adpcm; FormatS=__T("ADPCM"); BitsPerSample=16*8/28.0; break;
            default         : ;
        }
        const float64 BitRate=SamplingRate*BitsPerSample*Channels;
        Fill(Stream_Audio, 0, Audio_Format, FormatS);
        Fill(Stream_Audio, 0, Audio_Codec,  FormatS);
        Fill(Stream_Audio, 0, Audio_MuxingMode, "PS2");
        if (SamplingRate)
            Fill(Stream_Audio, 0, Audio_SamplingRate, SamplingRate);
        if (Channels)
            Fill(Stream_Audio, 0, Audio_Channel_s_, Channels);
        if (BitRate && BitRate<=(int32u)-1)
            Fill(Stream_Audio, 0, Audio_BitRate, BitRate, 0);
        else
            Coding=Coding_Unknown; //No duration from values out of range
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_PS2A_YES

