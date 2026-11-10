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
#if defined(MEDIAINFO_SMPTEST209450_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_SmpteSt209450.h"
#include <algorithm>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* SmpteSt209450_ColorPrimaries[3]=
{
    "BT.709",
    "Display P3",
    "BT.2020",
};

//---------------------------------------------------------------------------
static const char* SmpteSt209450_ComponentMix[3]=
{
    "Max",
    "Component",
    "Average of mean and max",
};

//---------------------------------------------------------------------------
static const char* SmpteSt209450_ComponentMix_Names[6]=
{
    "Red",
    "Green",
    "Blue",
    "Max",
    "Min",
    "Component",
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_SmpteSt209450::File_SmpteSt209450()
:File__Analyze()
{
    //Configuration
    FrameIsAlwaysComplete=true; //The container provides whole metadata sets
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_SmpteSt209450::Read_Buffer_Init()
{
    Accept("SMPTE ST 2094-50");
}

//---------------------------------------------------------------------------
void File_SmpteSt209450::Read_Buffer_Continue()
{
    //Parsing
    smpte_st_2094_50_application_info();
    while (Element_Level)
        Element_End0(); //Elements not ended after a parsing problem
    Element_Offset=Element_Size; //Remaining data is not parsed

    Frame_Count++;
    if (!Status[IsFinished])
        Finish(); //Only the metadata of the first frame is shown
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_SmpteSt209450::smpte_st_2094_50_application_info()
{
    struct alternate_image
    {
        int16u alternate_hdr_headroom;
        int16u component_mixing_coefficient[6];
        int8u  component_mixing_type;
        int8u  gain_curve_num_control_points_minus_1;
        bool   gain_curve_use_pchip_slope_flag;
    };
    alternate_image alternate_images[4]{};
    int16u hdr_reference_white{203*5}, baseline_hdr_headroom{}, gain_application_space_chromaticities[8]{};
    int8u application_version, minimum_application_version, num_alternate_images{}, gain_application_space_chromaticities_mode{};
    bool has_custom_hdr_reference_white_flag, has_adaptive_tone_map_flag, use_reference_white_tone_mapping_flag{};

    //Parsing
    Element_Begin1("smpte_st_2094_50_application_info");
    BS_Begin();
    Get_S1 ( 3, application_version,                            "application_version");
    Get_S1 ( 3, minimum_application_version,                    "minimum_application_version");
    Skip_S1( 2,                                                 "reserved_zero");
    if (minimum_application_version)
    {
        //Not supported by this version of the specification, the whole metadata set is ignored
        BS_End();
        Element_End0();
        Skip_XX(Element_Size-Element_Offset,                    "(Unsupported)");
        FILLING_BEGIN();
            if (HDR.empty())
            {
                HDR["HDR_Format"]=__T("SMPTE ST 2094 App 5");
                HDR["HDR_Format_Version"]=Ztring::ToZtring(application_version);
            }
        FILLING_END();
        return;
    }
    Element_Begin1("color_volume_transform");
    Get_SB (   has_custom_hdr_reference_white_flag,             "has_custom_hdr_reference_white_flag");
    Get_SB (   has_adaptive_tone_map_flag,                      "has_adaptive_tone_map_flag");
    Skip_S1( 6,                                                 "reserved_zero");
    if (has_custom_hdr_reference_white_flag)
    {
        Get_S2 (16, hdr_reference_white,                        "hdr_reference_white"); Param_Info2(min(max(hdr_reference_white, (int16u)1), (int16u)50000)/5.0, " cd/m2");
    }
    if (has_adaptive_tone_map_flag)
    {
        Element_Begin1("adaptive_tone_map");
        Get_S2 (16, baseline_hdr_headroom,                      "baseline_hdr_headroom"); Param_Info1(Ztring::ToZtring(min(baseline_hdr_headroom, (int16u)60000)/10000.0, 4));
        Get_SB (   use_reference_white_tone_mapping_flag,       "use_reference_white_tone_mapping_flag");
        if (!use_reference_white_tone_mapping_flag)
        {
            bool has_common_component_mix_params_flag, has_common_curve_params_flag;
            Get_S1 ( 3, num_alternate_images,                   "num_alternate_images");
            Get_S1 ( 2, gain_application_space_chromaticities_mode, "gain_application_space_chromaticities_mode"); Param_Info1C(gain_application_space_chromaticities_mode<3, SmpteSt209450_ColorPrimaries[gain_application_space_chromaticities_mode]);
            Get_SB (   has_common_component_mix_params_flag,    "has_common_component_mix_params_flag");
            Get_SB (   has_common_curve_params_flag,            "has_common_curve_params_flag");
            if (gain_application_space_chromaticities_mode==3)
            {
                for (int8u r=0; r<8; r++)
                {
                    Get_S2 (16, gain_application_space_chromaticities[r], "gain_application_space_chromaticities"); Param_Info1(min(gain_application_space_chromaticities[r], (int16u)50000)/50000.0);
                }
            }
            if (num_alternate_images>4)
                num_alternate_images=4;
            for (int8u a=0; a<num_alternate_images; a++)
            {
                Element_Begin1("alternate_image");
                auto& Alt=alternate_images[a];
                Get_S2 (16, Alt.alternate_hdr_headroom,         "alternate_hdr_headroom"); Param_Info1(Ztring::ToZtring(min(Alt.alternate_hdr_headroom, (int16u)60000)/10000.0, 4));

                Element_Begin1("component_mixing");
                if (!a || !has_common_component_mix_params_flag)
                {
                    Get_S1 ( 2, Alt.component_mixing_type,      "component_mixing_type"); Param_Info1C(Alt.component_mixing_type<3, SmpteSt209450_ComponentMix[Alt.component_mixing_type]);
                    if (Alt.component_mixing_type!=3)
                        Skip_S1( 6,                             "reserved_zero");
                    else
                    {
                        bool has_component_mixing_coefficient_flag[6];
                        for (int8u k=0; k<6; k++)
                        {
                            Get_SB (   has_component_mixing_coefficient_flag[k], "has_component_mixing_coefficient_flag"); Param_Info1(SmpteSt209450_ComponentMix_Names[k]);
                        }
                        for (int8u k=0; k<6; k++)
                            if (has_component_mixing_coefficient_flag[k])
                            {
                                Get_S2 (16, Alt.component_mixing_coefficient[k], "component_mixing_coefficient"); Param_Info1(SmpteSt209450_ComponentMix_Names[k]); Param_Info1(min(Alt.component_mixing_coefficient[k], (int16u)50000)/50000.0);
                            }
                    }
                }
                else
                {
                    Alt.component_mixing_type=alternate_images[0].component_mixing_type;
                    memcpy(Alt.component_mixing_coefficient, alternate_images[0].component_mixing_coefficient, sizeof(Alt.component_mixing_coefficient));
                }
                Element_End0();

                Element_Begin1("gain_curve");
                if (!a || !has_common_curve_params_flag)
                {
                    Get_S1 ( 5, Alt.gain_curve_num_control_points_minus_1, "gain_curve_num_control_points_minus_1");
                    Get_SB (   Alt.gain_curve_use_pchip_slope_flag, "gain_curve_use_pchip_slope_flag");
                    Skip_S1( 2,                                 "reserved_zero");
                    for (int8u c=0; c<=Alt.gain_curve_num_control_points_minus_1; c++)
                    {
                        int16u gain_curve_control_points_x;
                        Get_S2 (16, gain_curve_control_points_x, "gain_curve_control_points_x"); Param_Info1(min(gain_curve_control_points_x, (int16u)64000)/1000.0);
                    }
                }
                else
                {
                    Alt.gain_curve_num_control_points_minus_1=alternate_images[0].gain_curve_num_control_points_minus_1;
                    Alt.gain_curve_use_pchip_slope_flag=alternate_images[0].gain_curve_use_pchip_slope_flag;
                }
                for (int8u c=0; c<=Alt.gain_curve_num_control_points_minus_1; c++)
                {
                    int16u gain_curve_control_points_y;
                    Get_S2 (16, gain_curve_control_points_y,    "gain_curve_control_points_y"); Param_Info1(Ztring::ToZtring(min(gain_curve_control_points_y, (int16u)60000)/10000.0, 4));
                }
                if (!Alt.gain_curve_use_pchip_slope_flag)
                {
                    for (int8u c=0; c<=Alt.gain_curve_num_control_points_minus_1; c++)
                    {
                        Skip_S2(16,                             "gain_curve_control_points_theta");
                    }
                }
                Element_End0();
                Element_End0();
            }
        }
        else
            Skip_S1( 7,                                         "reserved_zero");
        Element_End0();
    }
    Element_End0();
    BS_End();
    Element_End0();
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "Padding");

    FILLING_BEGIN();
        if (!HDR.empty())
            return; //Only the metadata of the first frame is shown
        HDR["HDR_Format"]=__T("SMPTE ST 2094 App 5");
        HDR["HDR_Format_Version"]=Ztring::ToZtring(application_version);
        if (has_adaptive_tone_map_flag)
        {
            if (use_reference_white_tone_mapping_flag)
                HDR["HDR_Format_Settings"]=__T("Reference white tone mapping");
            else
                HDR["HDR_Format_Settings"]=Ztring::ToZtring(num_alternate_images)+(num_alternate_images==1?__T(" alternate image"):__T(" alternate images"));
        }
        hdr_reference_white=min(max(hdr_reference_white, (int16u)1), (int16u)50000);
        Infos.emplace_back("HdrReferenceWhite", Ztring::ToZtring(hdr_reference_white/5.0, hdr_reference_white%5?1:0)+__T(" cd/m2"));
        Infos.emplace_back("HeadroomAdaptiveToneMap", has_adaptive_tone_map_flag?__T("Yes"):__T("No"));
        if (has_adaptive_tone_map_flag)
        {
            Infos.emplace_back("HeadroomAdaptiveToneMap BaselineHdrHeadroom", Ztring::ToZtring(min(baseline_hdr_headroom, (int16u)60000)/10000.0, 4));
            Infos.emplace_back("HeadroomAdaptiveToneMap ReferenceWhiteToneMapping", use_reference_white_tone_mapping_flag?__T("Yes"):__T("No"));
            if (!use_reference_white_tone_mapping_flag)
            {
                Ztring Primaries;
                if (gain_application_space_chromaticities_mode<3)
                    Primaries.From_UTF8(SmpteSt209450_ColorPrimaries[gain_application_space_chromaticities_mode]);
                else
                {
                    static const char* Names[4]={"R", "G", "B", "White point"};
                    for (int8u i=0; i<4; i++)
                    {
                        if (i)
                            Primaries+=__T(", ");
                        Primaries+=Ztring().From_UTF8(Names[i])+__T(": x=")+Ztring::ToZtring(min(gain_application_space_chromaticities[i*2], (int16u)50000)/50000.0, 6)
                                                               +__T(" y=")+Ztring::ToZtring(min(gain_application_space_chromaticities[i*2+1], (int16u)50000)/50000.0, 6);
                    }
                }
                Infos.emplace_back("HeadroomAdaptiveToneMap GainApplicationColorPrimaries", Primaries);
                Infos.emplace_back("HeadroomAdaptiveToneMap AlternateImageCount", Ztring::ToZtring(num_alternate_images));
                for (int8u a=0; a<num_alternate_images; a++)
                {
                    const auto& Alt=alternate_images[a];
                    const auto Prefix="HeadroomAdaptiveToneMap AlternateImage"+std::to_string(a);
                    Infos.emplace_back(Prefix, __T("Yes"));
                    Infos.emplace_back(Prefix+" HdrHeadroom", Ztring::ToZtring(min(Alt.alternate_hdr_headroom, (int16u)60000)/10000.0, 4));
                    Ztring ComponentMix;
                    if (Alt.component_mixing_type<3)
                        ComponentMix.From_UTF8(SmpteSt209450_ComponentMix[Alt.component_mixing_type]);
                    else
                    {
                        for (int8u k=0; k<6; k++)
                        {
                            if (k)
                                ComponentMix+=__T(", ");
                            ComponentMix+=Ztring().From_UTF8(SmpteSt209450_ComponentMix_Names[k])+__T('=')+Ztring::ToZtring(min(Alt.component_mixing_coefficient[k], (int16u)50000)/50000.0, 5);
                        }
                    }
                    Infos.emplace_back(Prefix+" ComponentMix", ComponentMix);
                    Infos.emplace_back(Prefix+" GainCurve", __T("Yes"));
                    Infos.emplace_back(Prefix+" GainCurve ControlPointCount", Ztring::ToZtring(Alt.gain_curve_num_control_points_minus_1+1));
                }
            }
        }
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_SMPTEST209450_YES
