/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// From http://www.onicos.com/staff/iz/formats/gif.html
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
#if defined(MEDIAINFO_GIF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Gif.h"
#include <cmath>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Static stuff
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Gif::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<3)
        return false; //Must wait for more data

    if (CC3(Buffer)!=0x474946) //"GIF"
    {
        Reject("GIF");
        return false;
    }

    //All should be OK...
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Gif::Read_Buffer_Continue()
{
    if (Buffer_Offset+Element_Size<File_Size)
    {
        Element_WaitForMoreData();
        return;
    }

    //Parsing
    Ztring Version;
    int16u Width, Height;
    int8u  BackgroundColorIndex, PixelAspectRatio, Resolution, GCT_Size;
    bool GCT_Flag, Sort;
    Element_Begin1("Header");
        Skip_UTF8 (3,                                           "Header");
        Get_UTF8  (3, Version,                                  "Version");
    Element_End0();
    Element_Begin1("Logical Screen");
        Get_L2 (Width,                                          "Logical Screen Width");
        Get_L2 (Height,                                         "Logical Screen Height");
        BS_Begin();
        Get_SB (   GCT_Flag,                                    "Global Color Table Flag");
        Get_S1 (3, Resolution,                                  "Color Resolution");
        Get_SB (   Sort,                                        "Sort Flag to Global Color Table");
        Get_S1 (3, GCT_Size,                                    "Size of Global Color Table"); Param_Info1(Ztring::ToZtring((int16u)pow(2.0, 1+GCT_Size)));
        BS_End();
        Get_L1 (BackgroundColorIndex,                           "Background Color Index");
        Get_L1 (PixelAspectRatio,                               "Pixel Aspect Ratio");
    Element_End0();
    if (GCT_Flag)
        Skip_XX((int16u)pow(2.0, 1+GCT_Size)*3,                 "Global Color Table");
    int64u DelayTime_Total=0;
    Element_Begin1("Data");
        while (Element_Offset<Element_Size-1)
        {
            Element_Begin0();
                int8u Label;
                Get_L1(Label,                                       "Label");
                switch (Label)
                {
                    case 0x2C:
                        Element_Name("Table-Based Image");
                        Param_Info1("Image Separator");
                        Element_Begin1("Image Descriptor");
                        Skip_L2(                                    "Image Left Position");
                        Skip_L2(                                    "Image Top Position");
                        Skip_L2(                                    "Image Width");
                        Skip_L2(                                    "Image Height");
                        BS_Begin();
                        Get_SB (   GCT_Flag,                        "Local Color Table Flag");
                        Skip_SB(                                    "Interlace Flag");
                        Skip_SB(                                    "Sort Flag");
                        Skip_SB(                                    "Reserved");
                        Skip_SB(                                    "Reserved");
                        Get_S1 (3, GCT_Size,                        "Size of Local Color Table"); Param_Info1(Ztring::ToZtring((int16u)pow(2.0, 1+GCT_Size)));
                        BS_End();
                        Element_End0();
                        if (GCT_Flag)
                            Skip_XX((int16u)pow(2.0, 1+GCT_Size)*3, "Local Color Table");
                        Element_Begin1("Image Data");
                        Skip_L1(                                    "LZW Minimum Code Size");
                        while (Element_Offset<Element_Size)
                        {
                            int8u Size;
                            Get_L1(Size,                            "Block Size");
                            if (!Size)
                                break;
                            Skip_XX(Size,                           "Data Values");
                        }
                        Element_End0();
                        Element_End0();
                        break;
                    case 0x21:
                        Element_Name("Extension");
                        Param_Info1("Extension Introducer");
                        Get_L1 (Label,                              "Extension Label");
                        Element_Begin1("Extension");
                        while (Element_Offset<Element_Size)
                        {
                            int8u Size;
                            Get_L1(Size,                            "Block Size");
                            if (!Size)
                                break;
                            switch (Label)
                            {
                                case 0xF9 :
                                    if (Size==4)
                                    {
                                        Element_Info1("Graphic Control Extension");
                                        Skip_L1("Flags");
                                        int16u DelayTime;
                                        Get_L2 (DelayTime,          "Delay Time");
                                        Skip_L1("Transparent Color Index");
                                        DelayTime_Total+=DelayTime;
                                        Frame_Count++;
                                        break;
                                    }
                                    // Fallthrough
                                default:
                                    Skip_XX(Size, "Data Values");
                            }
                        }
                        Element_End0();
                        break;
                    default: ;
                        if (Element_Offset<Element_Size)
                            Skip_XX(Element_Size-Element_Offset-1,  "Unknown");
                }
            Element_End0();
        Element_End0();
    }
    Skip_L1(                                                    "Trailer");

    FILLING_BEGIN();
        Accept("GIF");

        Stream_Prepare(DelayTime_Total?Stream_Video:Stream_Image);
        if (DelayTime_Total)
        {
            Fill(Stream_Video, 0, Video_Duration, ((float)DelayTime_Total)*10);
            Fill(Stream_Video, 0, Video_FrameCount, Frame_Count);
        }
        Fill(StreamKind_Last, 0, "Width", Width);
        Fill(StreamKind_Last, 0, "Height", Height);
        Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), __T("GIF"));
        Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format_Profile), Version);
        Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), __T("GIF")+Version);
        if (PixelAspectRatio)
            Fill(StreamKind_Last, 0, "PixelAspectRatio", (((float)PixelAspectRatio)+15)/64);

        Finish("GIF");
    FILLING_END();
}

} //NameSpace

#endif
