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
    ELEMENT_CASE(2, 0, "Record Version")
    default: result += to_string(dataset);
    }

    #undef ELEMENT_CASE

    return result;
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
    ELEMENT_CASE(  2,   0, RecordVersion)
    default: Skip_XX(Element_Size,                               "(Unknown)");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Iim::RecordVersion()
{
    Skip_B2(                                                    "Data");
}

} //NameSpace

#endif //MEDIAINFO_IIM_YES
