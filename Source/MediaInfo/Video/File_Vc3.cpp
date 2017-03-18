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
#if defined(MEDIAINFO_VC3_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Vc3.h"
#if defined(MEDIAINFO_CDP_YES)
    #include "MediaInfo/Text/File_Cdp.h"
#endif
#include <sstream>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const bool Vc3_FromCID_IsSupported (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1250 :
        case 1251 :
        case 1252 :
        case 1253 :
                    return true;
        default   : return false;
    }
}

//---------------------------------------------------------------------------
static const int32u Vc3_CompressedFrameSize(int32u CompressionID)
{
    int32u Size;
    switch (CompressionID)
    {
        case 1253 : 
                    Size= 188416; break;
        case 1252 :
                    Size= 303104; break;
        case 1250 : 
        case 1251 :
                    Size= 458752; break;
        case 1237 : 
        case 1242 : 
        case 1244 :
                    Size= 606208; break;
        case 1235 :
        case 1238 : 
        case 1241 : 
        case 1243 :
                    Size= 917504; break;
        default   : return 0;
    }

    return Size;
};

//---------------------------------------------------------------------------
static const int8u Vc3_SBD(int32u SBD) //Sample Bit Depth
{
    switch (SBD)
    {
        case 1 : return  8;
        case 2 : return 10;
        default: return  0;
    }
};

//---------------------------------------------------------------------------
static const int8u Vc3_SBD_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1237 :
        case 1238 :
        case 1242 :
        case 1243 :
        case 1251 :
        case 1252 :
        case 1253 :
                    return 8;
        case 1235 :
        case 1241 :
        case 1250 :
                    return 10;
        default   : return 0;
    }
}

//---------------------------------------------------------------------------
static const char* Vc3_FFC[4]=
{
    "",
    "Progressive",
    "Interlaced",
    "Interlaced",
};

//---------------------------------------------------------------------------
static const char* Vc3_FFC_ScanOrder[4]=
{
    "",
    "",
    "TFF",
    "BFF",
};

//---------------------------------------------------------------------------
static const char* Vc3_FFE[2]=
{
    "Interlaced",
    "Progressive",
};

//---------------------------------------------------------------------------
static const char* Vc3_SST[2]=
{
    "Progressive",
    "Interlaced",
};

//---------------------------------------------------------------------------
static const char* Vc3_SST_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1235 :
        case 1237 :
        case 1238 :
        case 1250 :
        case 1251 :
        case 1252 :
        case 1253 :
                    return Vc3_SST[0];
        case 1241 :
        case 1242 :
        case 1243 :
                    return Vc3_SST[1];
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const int16u Vc3_SPL_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1250 :
        case 1251 :
        case 1252 :
                    return 1280;
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1253 :
                    return 1920;
        default   : return 0;
    }
}

//---------------------------------------------------------------------------
static const int16u Vc3_ALPF_PerFrame_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1250 :
        case 1251 :
        case 1252 :
                    return 720;
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1253 :
                    return 1080;
        default   : return 0;
    }
}

//---------------------------------------------------------------------------
static const char* Vc3_SSC[2]=
{
    "4:2:2",
    "4:4:4",
};

//---------------------------------------------------------------------------
static const char* Vc3_SSC_FromCID (int32u CompressionID)
{
    switch (CompressionID)
    {
        case 1235 :
        case 1237 :
        case 1238 :
        case 1241 :
        case 1242 :
        case 1243 :
        case 1250 :
        case 1251 :
        case 1252 :
        case 1253 :
        case 1258 :
        case 1259 :
        case 1260 :
                    return Vc3_SSC[0];
        case 1256 :
                    return Vc3_SSC[1];
        default   : return "";
    }
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Vc3::File_Vc3()
:File__Analyze()
{
    //In
    Frame_Count_Valid=2;
    FrameRate=0;

    //Parsers
    #if defined(MEDIAINFO_CDP_YES)
        Cdp_Parser=NULL;
    #endif //defined(MEDIAINFO_CDP_YES)

    //Temp
    FFC_FirstFrame=(int8u)-1;
}

//---------------------------------------------------------------------------
File_Vc3::~File_Vc3()
{
    #if defined(MEDIAINFO_CDP_YES)
        delete Cdp_Parser; //Cdp_Parser=NULL;
    #endif //defined(MEDIAINFO_CDP_YES)
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vc3::Streams_Fill()
{
    //Filling
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "VC-3");
    Fill(Stream_Video, 0, Video_BitRate_Mode, "CBR");
    if (FrameRate && Vc3_CompressedFrameSize(CID))
        Fill(Stream_Video, 0, Video_BitRate, Vc3_CompressedFrameSize(CID)*8*FrameRate, 0);
    Fill(Stream_Video, 0, Video_Format_Version, __T("Version ")+Ztring::ToZtring(HVN));
    if (FFC_FirstFrame!=(int8u)-1)
        Fill(Stream_Video, 0, Video_ScanOrder, Vc3_FFC_ScanOrder[FFC_FirstFrame]);
    if (Vc3_FromCID_IsSupported(CID))
    {
        if (Vc3_SPL_FromCID(CID))
            Fill(Stream_Video, 0, Video_Width, Vc3_SPL_FromCID(CID));
        if (Vc3_ALPF_PerFrame_FromCID(CID))
            Fill(Stream_Video, 0, Video_Height, Vc3_ALPF_PerFrame_FromCID(CID));
        if (Vc3_SBD_FromCID(CID))
            Fill(Stream_Video, 0, Video_BitDepth, Vc3_SBD_FromCID(CID));
        Fill(Stream_Video, 0, Video_ScanType, Vc3_SST_FromCID(CID));
        Fill(Stream_Video, 0, Video_ColorSpace, "YUV");
        Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:2:2");
        Fill(Stream_Video, 0, Video_PixelAspectRatio, 1.0);
    }
    else
    {
        Fill(Stream_Video, 0, Video_Width, SPL);
        Fill(Stream_Video, 0, Video_Height, ALPF*(SST?2:1));
        Fill(Stream_Video, 0, Video_BitDepth, Vc3_SBD(SBD));
        Fill(Stream_Video, 0, Video_ScanType, Vc3_SST[SST]);
        Fill(Stream_Video, 0, Video_ColorSpace, "YUV");
        Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:2:2");
        Fill(Stream_Video, 0, Video_PixelAspectRatio, 1.0);
    }

    if (!TimeCode_FirstFrame.empty())
        Fill(Stream_Video, 0, Video_TimeCode_FirstFrame, TimeCode_FirstFrame);
}

//---------------------------------------------------------------------------
void File_Vc3::Streams_Finish()
{
    #if defined(MEDIAINFO_CDP_YES)
        if (Cdp_Parser && !Cdp_Parser->Status[IsFinished] && Cdp_Parser->Status[IsAccepted])
        {
            Finish(Cdp_Parser);
            for (size_t StreamPos=0; StreamPos<Cdp_Parser->Count_Get(Stream_Text); StreamPos++)
            {
                Merge(*Cdp_Parser, Stream_Text, StreamPos, StreamPos);
                Ztring MuxingMode=Cdp_Parser->Retrieve(Stream_Text, StreamPos, "MuxingMode");
                Fill(Stream_Text, StreamPos, "MuxingMode", __T("VC-3 / Nexio user data / ")+MuxingMode, true);
            }

            Ztring LawRating=Cdp_Parser->Retrieve(Stream_General, 0, General_LawRating);
            if (!LawRating.empty())
                Fill(Stream_General, 0, General_LawRating, LawRating, true);
            Ztring Title=Cdp_Parser->Retrieve(Stream_General, 0, General_Title);
            if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                Fill(Stream_General, 0, General_Title, Title);
        }
    #endif //defined(MEDIAINFO_CDP_YES)
}

//***************************************************************************
// Buffer - Demux
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
bool File_Vc3::Demux_UnpacketizeContainer_Test()
{
    if (Buffer_Offset+0x2C>Buffer_Size)
        return false;

    CID = BigEndian2int32u(Buffer+Buffer_Offset+0x28);
    size_t Size=Vc3_CompressedFrameSize(CID);
    if (!Size)
    {
        if (!IsSub)
        {
            Reject();
            return false;
        }
        Size=Buffer_Size; //Hoping that the packet is complete. TODO: add a flag in the container parser saying if the packet is complete
    }
    Demux_Offset=Buffer_Offset+Size;

    if (Demux_Offset>Buffer_Size && File_Offset+Buffer_Size!=File_Size)
        return false; //No complete frame

    Demux_UnpacketizeContainer_Demux();

    return true;
}
#endif //MEDIAINFO_DEMUX

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vc3::Read_Buffer_Unsynched()
{
    #if defined(MEDIAINFO_CDP_YES)
        if (Cdp_Parser)
            Cdp_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_CDP_YES)
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Vc3::Header_Begin()
{
    if (Buffer_Offset+0x00000280>Buffer_Size)
        return false;

    return true;
}

//---------------------------------------------------------------------------
void File_Vc3::Header_Parse()
{
    CID = BigEndian2int32u(Buffer+Buffer_Offset+0x28);

    Header_Fill_Code(0, "Frame");
    size_t Size=Vc3_CompressedFrameSize(CID);
    if (!Size)
    {
        if (!IsSub)
        {
            Reject();
            return;
        }
        Size=Buffer_Size; //Hoping that the packet is complete. TODO: add a flag in the container parser saying if the packet is complete
    }
    Header_Fill_Size(Size);
}

//---------------------------------------------------------------------------
void File_Vc3::Data_Parse()
{
    //Parsing
    if (Status[IsFilled])
    {
        Skip_XX(Element_Size,                                   "Data");
    }
    else
    {
    Element_Info1(Frame_Count);
    Element_Begin1("Header");
    HeaderPrefix();
    if (HVN <= 1)
    {
        CodingControlA();
        Skip_XX(16,                                             "Reserved");
        ImageGeometry();
        Skip_XX( 5,                                             "Reserved");
        CompressionID();
        CodingControlB();
        Skip_XX( 3,                                             "Reserved");
        TimeCode();
        Skip_XX(38,                                             "Reserved");
        UserData();
        Skip_XX( 3,                                             "Reserved");
        MacroblockScanIndices();
        Element_End0();
        Element_Begin1("Payload");
        Skip_XX(Element_Size-Element_Offset-4,                  "Data");
        Element_End0();
        Element_Begin1("EOF");
        Skip_B4(                                                CRCF?"CRC":"Signature");
        Element_End0();
    }
    else
    {
        Element_End0();
        Skip_XX(Element_Size-Element_Offset,                    "Data");
    }
    }

    FILLING_BEGIN();
        Frame_Count++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
        if (FrameRate)
        {
            FrameInfo.PTS=FrameInfo.DTS+=float64_int64s(1000000000/FrameRate);
            FrameInfo.DUR=float64_int64s(1000000000/FrameRate);
        }
        else
        {
            FrameInfo.PTS=FrameInfo.DTS=FrameInfo.DUR=(int64u)-1;
        }
        if (!Status[IsAccepted])
            Accept("VC-3");
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
        {
            Fill("VC-3");

            if (!IsSub && Config->ParseSpeed<1)
                Finish("VC-3");
        }
    FILLING_END();
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vc3::HeaderPrefix()
{
    //Parsing
    Element_Begin1("Header Prefix");
    Get_B4 (HS,                                                 "HS, Header Size");
    Get_B1 (HVN,                                                "HVN, Header Version Number");
    Element_End0();

    FILLING_BEGIN();
        if (HS<0x00000280)
            Reject("VC-3");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Vc3::CodingControlA()
{
    //Parsing
    Element_Begin1("Coding Control A");
    BS_Begin();

    int8u FFC;
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Get_S1 (2, FFC,                                             "FFC, Field/Frame Count"); Param_Info1(Vc3_FFC[FFC]);

    Mark_1();
    Mark_0();
    Mark_0();
    Get_SB (   CRCF,                                            "CRCF, CRC flag");
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();

    Mark_1();
    Mark_0();
    Mark_1();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();

    BS_End();
    Element_End0();

    FILLING_BEGIN();
        if (FFC_FirstFrame==(int8u)-1)
            FFC_FirstFrame=FFC;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Vc3::ImageGeometry()
{
    //Parsing
    Element_Begin1("Image Geometry");
    Get_B2 (ALPF,                                               "Active lines-per-frame");
    Get_B2 (SPL,                                                "Samples-per-line");
    Skip_B1(                                                    "Zero");
    Skip_B2(                                                    "Number of active lines");
    Skip_B2(                                                    "Zero");

    BS_Begin();

    Get_S1 (3, SBD,                                             "Sample bit depth");
    Mark_1();
    Mark_1();
    Mark_0();
    Mark_0();
    Mark_0();

    Mark_1();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_1();
    Get_SB (   SST,                                             "Source scan type"); Param_Info1(Vc3_SST[SST]);
    Mark_0();
    Mark_0();

    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::CompressionID()
{
    //Parsing
    Element_Begin1("Compression ID");
    int32u Data;
    Get_B4 (Data,                                               "Compression ID");
    Element_End0();

    FILLING_BEGIN();
        CID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Vc3::CodingControlB()
{
    //Parsing
    Element_Begin1("Coding Control B");
    BS_Begin();

    Info_S1(1, FFE,                                             "Field/Frame Count"); Param_Info1(Vc3_FFE[FFE]);
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();

    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::TimeCode()
{
    //Parsing
    Element_Begin1("Time Code");
    bool TCP;

    BS_Begin();
    Get_SB (   TCP,                                             "TCP, Time Code Present");
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_0();
    if (TCP)
        Mark_0();
    else
    {
        TCP=Peek_SB();
        if (TCP)
            Skip_SB(                                            "TCP, Time Code Present (wrong order)");
        else
            Mark_0();
    }

    if (TCP)
    {
        Element_Begin1("Time Code");
            int8u HHT, HHU, MMT, MMU, SST, SSU, FFT, FFU;
            bool DF;
            Skip_S1(4,                                          "Binary Group 1");
            Get_S1 (4, FFU,                                     "Units of Frames");
            Skip_S1(4,                                          "Binary Group 2");
            Skip_SB(                                            "Color Frame");
            Get_SB (   DF,                                      "Drop Frame");
            Get_S1 (2, FFT,                                     "Tens of Frames");
            Skip_S1(4,                                          "Binary Group 3");
            Get_S1 (4, SSU,                                     "Units of Seconds");
            Skip_S1(4,                                          "Binary Group 4");
            Skip_SB(                                            "Field ID");
            Get_S1 (3, SST,                                     "Tens of Seconds");
            Skip_S1(4,                                          "Binary Group 5");
            Get_S1 (4, MMU,                                     "Units of Minutes");
            Skip_S1(4,                                          "Binary Group 6");
            Skip_SB(                                            "X");
            Get_S1 (3, MMT,                                     "Tens of Minutes");
            Skip_S1(4,                                          "Binary Group 7");
            Get_S1 (4, HHU,                                     "Units of Hours");
            Skip_S1(4,                                          "Binary Group 8");
            Skip_SB(                                            "X");
            Skip_SB(                                            "X");
            Get_S1 (2, HHT,                                     "Tens of Hours");
            FILLING_BEGIN();
                if (TimeCode_FirstFrame.empty() && FFU<10 && SSU<10 && SST<6 && MMU<10 && MMT<6 && HHU<10)
                {
                    std::ostringstream S;
                    S<<(size_t)HHT<<(size_t)HHU<<':'<<(size_t)MMT<<(size_t)MMU<<':'<<(size_t)SST<<(size_t)SSU<<(DF?';':':')<<(size_t)FFT<<(size_t)FFU;
                    TimeCode_FirstFrame=S.str();
                }
            FILLING_END();
        Element_End0();
        BS_End();
    }
    else
    {
        BS_End();
        Skip_B8(                                                "Junk");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::UserData()
{
    //Parsing
    int8u UserDataLabel;

    Element_Begin1("User Data Control");
    BS_Begin();
    Get_S1 (4, UserDataLabel,                                   "User Data Label");
    Mark_0();
    Mark_0();
    Mark_0();
    Mark_1();
    BS_End();
    Element_End0();

    Element_Begin1("User Data Payload");
    switch (UserDataLabel)
    {
        case 0x00: Skip_XX(260,                                 "Reserved"); break;
        case 0x08: UserData_8(); break;
        default  : Skip_XX(260,                                 "Reserved for future use");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::MacroblockScanIndices()
{
    Element_Begin1("Macroblock Scan Indices Control");
        Skip_XX(9,                                              "ToDo");
    Element_End0();
    Element_Begin1("Macroblock Scan Indices Payload");
        Skip_XX(HS-Element_Offset,                              "ToDo");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Vc3::UserData_8()
{
    if (Element_Offset + 0x104 < Element_Size
        && Buffer[Buffer_Offset + (size_t)Element_Offset + 0xBA] == 0x96
        && Buffer[Buffer_Offset + (size_t)Element_Offset + 0xBB] == 0x69)
    {
        Skip_XX(0xBA,                                           "Nexio private data?");
        #if defined(MEDIAINFO_CDP_YES)
            if (Cdp_Parser==NULL)
            {
                Cdp_Parser=new File_Cdp;
                Open_Buffer_Init(Cdp_Parser);
                Frame_Count_Valid=300;
            }
            if (!Cdp_Parser->Status[IsFinished])
            {
                ((File_Cdp*)Cdp_Parser)->AspectRatio=16.0/9.0;
                Open_Buffer_Continue(Cdp_Parser, Buffer + Buffer_Offset + (size_t)Element_Offset, 0x49);
            }
            Element_Offset+=0x49;
            Skip_B1(                                            "Nexio private data?");
        #else //MEDIAINFO_CDP_YES
            Skip_XX(0x4A                                        "CDP data");
        #endif //MEDIAINFO_CDP_YES
    }
    else
        Skip_XX(260,                                            "Nexio private data?");

}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_VC3_*
