/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_MgaH
#define MediaInfo_MgaH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Mga
//***************************************************************************

class File_Mga : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;

    //Out
    #if MEDIAINFO_CONFORMANCE
    struct checks
    {
        int64u  Frame_Count=0;
        std::set<int64u> AudioEssence_Lengths; //Distinct lengths of the Audio Essence sections, limited to the first ones
        std::map<int64u, int64u> Sections; //Audio Metadata sections of the first frame, key is the index, value is the identifier
        std::map<int64u, int64u> Payloads; //First Payload Tag of the Audio Metadata Payload sections, key is the index
        std::set<std::string> Errors;
        struct programme
        {
            std::string Name;
            std::string Label; //First one
        };
        std::map<std::string, programme> Programmes; //audioProgramme elements of all the S-ADM metadata parsed, key is the ID
        bool    IsStopped=false; //Frames after a seek are not checked
        std::set<std::string> Adm_FrameTypes; //Types of the S-ADM frames parsed, limited to the first ones
    };
    checks Checks;
    #endif //MEDIAINFO_CONFORMANCE

    //Constructor/Destructor
    File_Mga();
    ~File_Mga();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - Global
    #if MEDIAINFO_CONFORMANCE
    void Read_Buffer_Unsynched();
    #endif //MEDIAINFO_CONFORMANCE

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void AudioMetadataPayload();
    void SerialAudioDefinitionModelMetadataPayload(int64u Length);

    //Helpers
    void Get_BER(int64u& Value, const char* Name);

    //Temp
    File__Analyze* Parser;
};

} //NameSpace

#endif
