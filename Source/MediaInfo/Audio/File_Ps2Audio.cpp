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
        if (!IsSub && Data_Begin+Size>File_Size)
        {
            //The file ends before the end of the body
            IsTruncated(Data_Begin+Size, false, "PS2 Audio");
            Size=(int32u)(File_Size-Data_Begin);
        }
        Fill(Stream_Audio, 0, Audio_StreamSize, Size);
        if (BitRate)
            Fill(Stream_Audio, 0, Audio_Duration, ((int64u)Size)*1000*8/BitRate);

        Finish("PS2 Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ps2Audio::SShd()
{
    Element_Begin1("SShd (Header)");
        //Parsing
        int32u Size, Format, SamplingRate, Channels;
        Skip_C4(                                                "ID");
        Get_L4 (Size,                                           "Size");
        if (Size!=0x18)
        {
            Trusted_IsNot("Bad size");
            Element_End0();
            return;
        }
        Get_L4 (Format,                                         "Format");
        Get_L4 (SamplingRate,                                   "Sampling rate");
        Get_L4 (Channels,                                       "Channels");
        Skip_L4(                                                "Bytes per channel");
        Skip_L4(                                                "Reserved");
        Skip_L4(                                                "Reserved");
    Element_End0();

    FILLING_BEGIN();
        if (Status[IsAccepted])
            return; //Only the first header is used

        Accept("PS2 Audio");
        Fill(Stream_General, 0, General_Format, "PS2 Audio");

        const int64u BitRate64=((int64u)SamplingRate)*Channels*16; //Always 16 bits
        BitRate=BitRate64>(int32u)-1?0:(int32u)BitRate64;

        Stream_Prepare(Stream_Audio);
        Ztring FormatS;
        switch(Format)
        {
            case 0x00000001 : FormatS=__T("PCM"); break;
            case 0x00000010 : FormatS=__T("ADPCM"); break;
            default         : ;
        }
        Fill(Stream_Audio, 0, Audio_Format, FormatS);
        Fill(Stream_Audio, 0, Audio_Codec,  FormatS);
        Fill(Stream_Audio, 0, Audio_MuxingMode, "PS2");
        if (SamplingRate)
            Fill(Stream_Audio, 0, Audio_SamplingRate, SamplingRate);
        if (Channels)
            Fill(Stream_Audio, 0, Audio_Channel_s_, Channels);
        if (BitRate)
            Fill(Stream_Audio, 0, Audio_BitRate, BitRate);
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_PS2A_YES

