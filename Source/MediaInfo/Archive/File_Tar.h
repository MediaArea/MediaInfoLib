/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Tar files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_TarH
#define MediaInfo_File_TarH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Tar
//***************************************************************************

class File_Tar : public File__Analyze
{
protected :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Header();
    void Payload();
    void Data();
    void EndOfFile();

    // Pending state carried across Read_Buffer_Continue() calls.
    // A PAX 'x' header (or GNU 'L'/'K') describes the *next* real header,
    // so we stash the overrides here until we see that next header.
    std::map<std::string, std::string> Pending_PAXOverrides;
    std::map<std::string, std::string> Global_PAXOverrides; // for 'g' blocks
    std::string                        Pending_GNULongName;
    std::string                        Pending_GNULongLink;
    bool AnyMemberFound = false;
    bool Rejected = false;

    enum type {
        Type_Header,
        Type_Payload,
        Type_Data,
        Type_EndOfFile,
    };
    type Type = Type_Header;
    int8u TypeFlag = 0;
    int64u Size;
    std::string Payload_Content;
};

} //NameSpace

#endif
