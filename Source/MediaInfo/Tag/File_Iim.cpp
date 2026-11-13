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
#if defined(MEDIAINFO_IIM_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_Iim.h"
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

string IPTC_record_name(int8u record, int8u dataset)
{
    string result;

    switch (record) {
    case 1  : result = "Envelope Record"; break;
    case 2  : result = "Application Record"; break;
    case 3  : result = "Digital Newsphoto Parameter Record"; break;
    case 6  : result = "Abstact Relationship Record"; break;
    case 7  : result = "Pre-object Data Descriptor Record"; break;
    case 8  : result = "Objectdata Record"; break;
    case 9  : result = "Post-objectdata Descriptor Record"; break;
    case 240: result = "Foto Station"; break;
    default : result = to_string(record);
    }

    result += " - ";
    
    #define ELEMENT_CASE(record, dataset, call) case ((unsigned)record << 8) | dataset: result += call; break;

    switch (((unsigned)record << 8) | dataset) {
    ELEMENT_CASE(1,   0, "Record Version")
    ELEMENT_CASE(1,   5, "Destination")
    ELEMENT_CASE(1,  20, "File Format")
    ELEMENT_CASE(1,  22, "File Format Version")
    ELEMENT_CASE(1,  30, "Service Identifier")
    ELEMENT_CASE(1,  40, "Envelope Number")
    ELEMENT_CASE(1,  50, "Product I.D.")
    ELEMENT_CASE(1,  60, "Envelope Priority")
    ELEMENT_CASE(1,  70, "Data Sent")
    ELEMENT_CASE(1,  80, "Time Sent")
    ELEMENT_CASE(1,  90, "Coded Character Set")
    ELEMENT_CASE(1, 100, "UNO")
    ELEMENT_CASE(1, 120, "ARM Identifier")
    ELEMENT_CASE(1, 122, "ARM Version")
    ELEMENT_CASE(2,   0, "Record Version")
    ELEMENT_CASE(2,   3, "Object Type Reference")
    ELEMENT_CASE(2,   4, "Object Attribute Reference")
    ELEMENT_CASE(2,   5, "Object Name")
    ELEMENT_CASE(2,   7, "Edit Status")
    ELEMENT_CASE(2,   8, "Editorial Update")
    ELEMENT_CASE(2,  10, "Urgency")
    ELEMENT_CASE(2,  12, "Subject Reference")
    ELEMENT_CASE(2,  15, "Category")
    ELEMENT_CASE(2,  20, "Supplemental Category")
    ELEMENT_CASE(2,  22, "Fixture Identifier")
    ELEMENT_CASE(2,  25, "Keywords")
    ELEMENT_CASE(2,  26, "Content Location Code")
    ELEMENT_CASE(2,  27, "Content Location Name")
    ELEMENT_CASE(2,  30, "Release Date")
    ELEMENT_CASE(2,  35, "Release Time")
    ELEMENT_CASE(2,  37, "Expiration Date")
    ELEMENT_CASE(2,  38, "Expiration Time")
    ELEMENT_CASE(2,  40, "Special Instructions")
    ELEMENT_CASE(2,  42, "Action Advised")
    ELEMENT_CASE(2,  45, "Reference Service")
    ELEMENT_CASE(2,  47, "Reference Date")
    ELEMENT_CASE(2,  50, "Reference Number")
    ELEMENT_CASE(2,  55, "Date Created")
    ELEMENT_CASE(2,  60, "Time Created")
    ELEMENT_CASE(2,  62, "Digital Creation Date")
    ELEMENT_CASE(2,  63, "Digital Creation Time")
    ELEMENT_CASE(2,  65, "Originating Program")
    ELEMENT_CASE(2,  70, "Program Version")
    ELEMENT_CASE(2,  75, "Object Cycle")
    ELEMENT_CASE(2,  80, "By-line")
    ELEMENT_CASE(2,  85, "By-line Title")
    ELEMENT_CASE(2,  90, "City")
    ELEMENT_CASE(2,  92, "Sub-location")
    ELEMENT_CASE(2,  95, "Province/State")
    ELEMENT_CASE(2, 100, "Country/Primary Location Code")
    ELEMENT_CASE(2, 101, "Country/Primary Location Name")
    ELEMENT_CASE(2, 103, "Original Transmission Reference")
    ELEMENT_CASE(2, 105, "Headline")
    ELEMENT_CASE(2, 110, "Credit")
    ELEMENT_CASE(2, 115, "Source")
    ELEMENT_CASE(2, 116, "Copyright Notice")
    ELEMENT_CASE(2, 118, "Contact")
    ELEMENT_CASE(2, 120, "Caption/Abstract")
    ELEMENT_CASE(2, 122, "Writer/Editor")
    ELEMENT_CASE(2, 125, "Rasterized Caption")
    ELEMENT_CASE(2, 130, "Image Type")
    ELEMENT_CASE(2, 131, "Image Orientation")
    ELEMENT_CASE(2, 135, "Language Identifier")
    ELEMENT_CASE(2, 150, "Audio Type")
    ELEMENT_CASE(2, 151, "Audio Sampling Rate")
    ELEMENT_CASE(2, 152, "Audio Sampling Resolution")
    ELEMENT_CASE(2, 153, "Audio Duration")
    ELEMENT_CASE(2, 154, "Audio Outcue")
    ELEMENT_CASE(2, 200, "ObjectData Preview File Format")
    ELEMENT_CASE(2, 201, "ObjectData Preview File Foramt Version")
    ELEMENT_CASE(2, 202, "ObjectData Preview Data")
    ELEMENT_CASE(2, 221, "Photo Mechanic Preferences")
    ELEMENT_CASE(3,   0, "Record Version")
    ELEMENT_CASE(3,  10, "Picture Number")
    ELEMENT_CASE(3,  20, "Pixels Per Line")
    ELEMENT_CASE(3,  30, "Number of Lines")
    ELEMENT_CASE(3,  40,  "Pixel Size In Scanning Direction")
    ELEMENT_CASE(3,  50, "Pixel Size Perpendicular To Scanning Direction")
    ELEMENT_CASE(3,  55, "Supplement Type")
    ELEMENT_CASE(3,  60, "Colour Representation")
    ELEMENT_CASE(3,  64, "Interchange Colour Space")
    ELEMENT_CASE(3,  65, "Colour Sequence")
    ELEMENT_CASE(3,  66, "ICC Input Colour Profile")
    ELEMENT_CASE(3,  70, "Colour Calibration Matrix Table")
    ELEMENT_CASE(3,  80, "Lookup Table")
    ELEMENT_CASE(3,  84, "Number of Index Entries")
    ELEMENT_CASE(3,  85, "Colour Palette")
    ELEMENT_CASE(3,  86, "Number of Bits per Sample")
    ELEMENT_CASE(3,  90, "Sampling Structure")
    ELEMENT_CASE(3, 100, "Scanning Direction")
    ELEMENT_CASE(3, 102, "Image Rotation")
    ELEMENT_CASE(3, 110, "Data Compression Method")
    ELEMENT_CASE(3, 120, "Quantisation Method")
    ELEMENT_CASE(3, 125, "End Points")
    ELEMENT_CASE(3, 130, "Excursion Tolerance")
    ELEMENT_CASE(3, 135, "Bits Per Component")
    ELEMENT_CASE(3, 140, "Maximum Density Range")
    ELEMENT_CASE(3, 145, "Gamma Compensated Value")
    ELEMENT_CASE(7,  10, "Size Mode")
    ELEMENT_CASE(7,  20, "Max Subfile Size")
    ELEMENT_CASE(7,  90, "ObjectData Size Announced")
    ELEMENT_CASE(7,  95, "Maximum ObjectData Size")
    ELEMENT_CASE(8,  10, "Subfile")
    ELEMENT_CASE(9,  10, "Confirmed ObjectData Size")
    default: result += to_string(dataset);
    }

    #undef ELEMENT_CASE

    return result;
}

enum IPTC_CodedCharacterSet {
    UTF8
};

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iim::Streams_Finish()
{
    //Date as CCYYMMDD, time as HHMMSS then the offset from UTC as +HHMM or -HHMM
    if (Date.size()!=8 || Date.find_first_not_of(__T("0123456789"))!=string::npos)
        return;
    Ztring Value=Date.substr(0, 4)+__T('-')+Date.substr(4, 2)+__T('-')+Date.substr(6, 2);
    if (Time.size()==11 && Time.find_first_not_of(__T("0123456789"))==6 && (Time[6]==__T('+') || Time[6]==__T('-')) && Time.find_first_not_of(__T("0123456789"), 7)==string::npos)
        Value+=__T(' ')+Time.substr(0, 2)+__T(':')+Time.substr(2, 2)+__T(':')+Time.substr(4, 2)+Time.substr(6, 3)+__T(':')+Time.substr(9, 2);
    Fill(Stream_General, 0, General_Recorded_Date, Value);
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Iim::FileHeader_Begin()
{
    Accept();
    return true;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iim::Header_Parse()
{
    int8u record, dataset;
    int16u count;
    Skip_B1 (                                                   "Tag Marker");
    Get_B1  (record,                                            "Record Number");
    Get_B1  (dataset,                                           "DataSet Number");
    Get_B2  (count,                                             "Data Field Octet Count");

    FILLING_BEGIN()
        if (count & 1U << 15) {
            Finish();
            return; // not implemented
        }

        Header_Fill_Code(((unsigned)record << 8) | dataset, IPTC_record_name(record, dataset).c_str());
        Header_Fill_Size(Element_Offset + count);
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Iim::Data_Parse()
{
    #define ELEMENT_CASE(record, dataset, call) case ((unsigned)record << 8) | dataset: call(); break;

    switch (Element_Code) {
    ELEMENT_CASE(  1,  90, CodedCharacterSet)
    ELEMENT_CASE(  2,   0, RecordVersion)
    ELEMENT_CASE(  2,   5, ObjectName)
    ELEMENT_CASE(  2,  25, Keywords)
    ELEMENT_CASE(  2,  55, DateCreated)
    ELEMENT_CASE(  2,  60, TimeCreated)
    ELEMENT_CASE(  2,  80, Byline)
    ELEMENT_CASE(  2,  90, City)
    ELEMENT_CASE(  2,  92, Sublocation)
    ELEMENT_CASE(  2,  95, ProvinceState)
    ELEMENT_CASE(  2, 101, CountryName)
    ELEMENT_CASE(  2, 105, Headline)
    ELEMENT_CASE(  2, 110, Credit)
    ELEMENT_CASE(  2, 116, CopyrightNotice)
    ELEMENT_CASE(  2, 120, CaptionAbstract)
    ELEMENT_CASE(  2, 221, PhotoMechanicPrefs)
    default: Skip_XX(Element_Size,                               "(Unknown)");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iim::CodedCharacterSet()
{
    if (Element_Size == 3) {
        int32u characterset;
        Get_B3(characterset,                                    "Data");
        if (characterset == 0x1B2547) { // ESC % G
            CodedCharSet = UTF8;
            Param_Info1("UTF-8");
        }
    }
    else
        Skip_XX(Element_Size,                                   "Data");
}

//---------------------------------------------------------------------------
void File_Iim::RecordVersion()
{
    Skip_B2(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Iim::ObjectName()
{
    FillString(General_Title);
}

//---------------------------------------------------------------------------
void File_Iim::Keywords()
{
    FillString(General_Subject);
}

//---------------------------------------------------------------------------
void File_Iim::DateCreated()
{
    GetString(Date);
}

//---------------------------------------------------------------------------
void File_Iim::TimeCreated()
{
    GetString(Time);
}

//---------------------------------------------------------------------------
void File_Iim::Byline()
{
    FillString(General_Producer);
}

//---------------------------------------------------------------------------
void File_Iim::City()
{
    FillString("City");
}

//---------------------------------------------------------------------------
void File_Iim::Sublocation()
{
    FillString("Sublocation");
}

//---------------------------------------------------------------------------
void File_Iim::ProvinceState()
{
    FillString("State");
}

//---------------------------------------------------------------------------
void File_Iim::CountryName()
{
    FillString(General_Country);
}

//---------------------------------------------------------------------------
void File_Iim::Headline()
{
    FillString("Headline");
}

//---------------------------------------------------------------------------
void File_Iim::Credit()
{
    FillString(General_Copyright);
}

//---------------------------------------------------------------------------
void File_Iim::CopyrightNotice()
{
    FillString(General_Copyright);
}

//---------------------------------------------------------------------------
void File_Iim::CaptionAbstract()
{
    FillString(General_Description);
}

//---------------------------------------------------------------------------
void File_Iim::PhotoMechanicPrefs()
{
    SkipString();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iim::SkipString()
{
    switch (CodedCharSet) {
    case UTF8:
        Skip_UTF8(Element_Size,                                 "Data");
        break;
    default:
        Skip_XX(Element_Size,                                   "Data");
        break;
    }
}

//---------------------------------------------------------------------------
void File_Iim::GetString(Ztring& Value)
{
    switch (CodedCharSet) {
    case UTF8:
        Get_UTF8(Element_Size, Value,                           "Data");
        if (Value.empty())
            Value.From_ISO_8859_1((const char*)Buffer+Buffer_Offset, (size_t)Element_Size); //Not UTF-8
        break;
    default:
        Skip_XX(Element_Size,                                   "Data");
        break;
    }
}

//---------------------------------------------------------------------------
void File_Iim::FillString(size_t Parameter)
{
    Ztring Value;
    GetString(Value);
    Fill(Stream_General, 0, Parameter, Value);
}

//---------------------------------------------------------------------------
void File_Iim::FillString(const char* Parameter)
{
    Ztring Value;
    GetString(Value);
    Fill(Stream_General, 0, Parameter, Value);
}

} //NameSpace

#endif //MEDIAINFO_IIM_YES
