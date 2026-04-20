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
#if defined(MEDIAINFO_JPEGXL_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_JpegXL.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_JpegXL::File_JpegXL()
{
}

//***************************************************************************
// File Header
//***************************************************************************

//---------------------------------------------------------------------------
void File_JpegXL::FileHeader_Parse()
{
    int16u signature;
    Get_B2(signature,                                               "signature");
    if (signature != 0xFF0A) {
        Reject();
        return;
    }
    BS_Begin_LE();
    Element_Begin1("size");
    int32u width, height;
    bool div8;
    Get_TB(div8,                                                    "div8");
    if (div8) {
        int8u h_div8;
        Get_T1(5, h_div8,                                           "h_div8");
        h_div8 += 1;
        height = h_div8 * 8;
    }
    else {
        Get_U32(B(9, 1), B(13, 1), B(18, 1), B(30, 1), height,      "height");
    }
    int8u ratio;
    Get_T1(3, ratio,                                                "ratio");
    if (ratio) {
        switch (ratio) {
        case 1: width = height; break;
        case 2: width = height * 6 / 5; break;
        case 3: width = height * 4 / 3; break;
        case 4: width = height * 3 / 2; break;
        case 5: width = height * 16 / 9; break;
        case 6: width = height * 5 / 4; break;
        case 7: width = height * 2; break;
        }
    }
    if (div8 && !ratio) {
        int8u w_div8;
        Get_T1(5, w_div8,                                           "w_div8");
        w_div8 += 1;
        width = w_div8 * 8;
    }
    else {
        Get_U32(B(9, 1), B(13, 1), B(18, 1), B(30, 1), width,       "width");
    }
    Element_End0();
    Element_Begin1("metadata");
    bool all_default, extra_fields{};
    int8u orientation;
    bool float_sample{ false }, xyb_encoded{ true };
    int32u bits_per_sample{ 8 }, colour_space{ 0 };
    Get_TB(all_default,                                             "all_default");
    if (!all_default)
        Get_TB(extra_fields,                                        "extra_fields");
    if (extra_fields) {
        Get_T1(3, orientation,                                      "orientation");
        orientation += 1;
        TEST_SB_SKIP(                                               "have_intr_size");
        Element_Begin1("intrinsic_size");
        int32u width, height;
        bool div8;
        Get_TB(div8,                                                "div8");
        if (div8) {
            int8u h_div8;
            Get_T1(5, h_div8,                                       "h_div8");
            h_div8 += 1;
            height = h_div8 * 8;
        }
        else {
            Get_U32(B(9, 1), B(13, 1), B(18, 1), B(30, 1), height,  "height");
        }
        int8u ratio;
        Get_T1(3, ratio,                                            "ratio");
        if (ratio) {
            switch (ratio) {
            case 1: width = height; break;
            case 2: width = height * 6 / 5; break;
            case 3: width = height * 4 / 3; break;
            case 4: width = height * 3 / 2; break;
            case 5: width = height * 16 / 9; break;
            case 6: width = height * 5 / 4; break;
            case 7: width = height * 2; break;
            }
        }
        if (div8 && !ratio) {
            int8u w_div8;
            Get_T1(5, w_div8,                                       "w_div8");
            w_div8 += 1;
            width = w_div8 * 8;
        }
        if (!div8 && !ratio) {
            Get_U32(B(9, 1), B(13, 1), B(18, 1), B(30, 1), width,   "width");
        }
        Element_End0();
        TEST_SB_END();
        TEST_SB_SKIP(                                               "have_preview");
        Element_Begin1("preview");
        int32u width, height;
        bool div8;
        Get_TB(div8,                                                "div8");
        if (div8) {
            int32u h_div8;
            Get_U32(V(16), V(32), B(5, 1), B(9, 33), h_div8,        "h_div8");
            height = h_div8 * 8;
        }
        else {
            Get_U32(B(6, 1), B(8, 65), B(10, 321), B(12, 1345), height, "height");
        }
        int8u ratio;
        Get_T1(3, ratio,                                            "ratio");
        if (ratio) {
            switch (ratio) {
            case 1: width = height; break;
            case 2: width = height * 6 / 5; break;
            case 3: width = height * 4 / 3; break;
            case 4: width = height * 3 / 2; break;
            case 5: width = height * 16 / 9; break;
            case 6: width = height * 5 / 4; break;
            case 7: width = height * 2; break;
            }
        }
        if (div8 && !ratio) {
            int32u w_div8;
            Get_U32(V(16), V(32), B(5, 1), B(9, 33), w_div8,        "w_div8");
            width = w_div8 * 8;
        }
        if (!div8 && !ratio) {
            Get_U32(B(6, 1), B(8, 65), B(10, 321), B(12, 1345), width, "width");
        }
        Element_End0();
        TEST_SB_END();
        TEST_SB_SKIP(                                               "have_animation");
        Element_Begin1("animation");
        int32u tps_numerator, tps_denominator, num_loops;
        Get_U32(V(100), V(1000), B(10, 1), B(30, 1), tps_numerator, "tps_numerator");
        Get_U32(V(1), V(1001), B(8, 1), B(10, 1), tps_denominator,  "tps_denominator");
        Get_U32(V(0), B(3, 0), B(16, 0), B(32, 0), num_loops,       "num_loops");
        Skip_TB(                                                    "have_timecodes");
        Element_End0();
        TEST_SB_END();
    }
    if (!all_default) {
        Element_Begin1("bit_depth");
        Get_TB(float_sample,                                        "float_sample");
        if (!float_sample)
            Get_U32(V(8), V(10), V(12), B(6, 1), bits_per_sample,   "bits_per_sample");
        else {
            Get_U32(V(32), V(16), V(24), B(6, 1), bits_per_sample,  "bits_per_sample");
            Skip_T1(4,                                              "exp_bits");
        }
        Element_End0();
        Skip_TB(                                                    "modular_16bit_buffers");
        int32u num_extra;
        Get_U32(V(0), V(1), B(4, 2), B(12, 1), num_extra,           "num_extra");
        for (int32u i = 0; i < num_extra; ++i) {
            Element_Begin1("ex_info");
            bool d_alpha;
            Get_TB(d_alpha,                                         "d_alpha");
            if (!d_alpha) {
                int32u type;
                bool float_sample_i;
                int32u bits_per_sample_i;
                Get_Enum(type,                                      "type");
                Element_Begin1("bit_depth");
                Get_TB(float_sample_i,                              "float_sample");
                if (!float_sample_i)
                    Get_U32(V(8), V(10), V(12), B(6, 1), bits_per_sample_i, "bits_per_sample");
                else {
                    Get_U32(V(32), V(16), V(24), B(6, 1), bits_per_sample_i, "bits_per_sample");
                    Skip_T1(4,                                      "exp_bits");
                }
                Element_End0();
                int32u temp;
                Get_U32(V(0), V(3), V(4), B(3, 1), temp,            "dim_shift");
                Get_U32(V(0), B(4), B(5, 16), B(10, 48), temp,      "name_len");
                Skip_BT(static_cast<size_t>(temp) * 8,              "name");
                switch (type) {
                case 0:
                    Skip_TB(                                        "alpha_associated");
                    break;
                case 2:
                    Skip_T2(16,                                     "red");
                    Skip_T2(16,                                     "green"); 
                    Skip_T2(16,                                     "blue"); 
                    Skip_T2(16,                                     "solidity");
                    break;
                case 5: 
                    Get_U32(V(1), B(2), B(4, 3), B(8, 19), temp,    "cfa_channel");
                    break;
                }
            }
            Element_End0();
        }
        Get_TB(xyb_encoded,                                         "xyb_encoded");
        Element_Begin1("colour_encoding");
        Get_TB(all_default,                                         "all_default");
        if (!all_default) {
            Skip_TB(                                                "want_icc");
            Get_Enum(colour_space,                                  "colour_space");
            Param_Info1(colour_space == 0 ? "RGB" : colour_space == 1 ? "Y" : colour_space == 2 ? "XYB" : "");
        }
        Element_End0();
    }
    Element_End0();
    BS_End_LE();

    FILLING_BEGIN();
    Accept();
    Stream_Prepare(Stream_Image);
    Fill(Stream_Image, 0, Image_Format, "JPEG XL");
    Fill(Stream_Image, 0, Image_Width, width);
    Fill(Stream_Image, 0, Image_Height, height);
    Fill(Stream_Image, 0, Image_BitDepth, bits_per_sample);
    Fill(Stream_Image, 0, Image_ColorSpace, xyb_encoded ? "XYB" : colour_space == 0 ? "RGB" : colour_space == 1 ? "Y" : colour_space == 2 ? "XYB" : "");
    Finish();
    FILLING_END();
}

//***************************************************************************
// Helpers
//***************************************************************************

#define INTEGRITY(TOVIDATE, ERRORTEXT, OFFSET) \
if (!(TOVIDATE)) \
{ \
    Trusted_IsNot(ERRORTEXT); \
    return; \
} \

//---------------------------------------------------------------------------
void File_JpegXL::Get_U32(U32Dist d0, U32Dist d1, U32Dist d2, U32Dist d3, int32u& Info, const char* Name)
{
    Info = 0;
    U32Dist dist[]{ d0, d1, d2, d3 };
    INTEGRITY(2 <= BT->Remain(), "Size is wrong", BT->Offset_Get())
    int8u distribution = BT->Get(2);
    INTEGRITY(dist[distribution].bits <= BT->Remain(), "Size is wrong", BT->Offset_Get())
    Info = BT->Get4(dist[distribution].bits);
    Info += dist[distribution].offset;
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            Param(Name, Info, dist[distribution].bits + 2);
            Param_Info(__T("(") + Ztring::ToZtring(dist[distribution].bits + 2) + __T(" bits)"));
        }
    #endif
}

//---------------------------------------------------------------------------
void File_JpegXL::Get_Enum(int32u& Info, const char* Name)
{
    Get_U32(V(0), V(1), B(4, 2), B(6, 18), Info, Name);
}

} //NameSpace

#endif
