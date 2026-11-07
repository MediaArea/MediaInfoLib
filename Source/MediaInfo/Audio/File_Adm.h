/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about ADM files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_AdmH
#define MediaInfo_File_AdmH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <map>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class file_adm_private;

//***************************************************************************
// Class File_Adm
//***************************************************************************

class File_Adm : public File__Analyze
{
public :
    //In
    string MuxingMode;
    float Container_Duration = {};
    int64u TotalSize = 0;
    void chna_Add(int32u Index, const string& TrackUID);
    void* chna_Move();
    void chna_Move(File_Adm*);

    //Out
    bool NeedToJumpToEnd = false;

    //Out, for checks done by the container (e.g. SMPTE ST 2131) after parsing
    bool ID_IsPresent(const string& Element, const string& ID); //Element is the name of the ADM element, e.g. "audioProgramme"
    vector<string> ID_List(const string& Element); //IDs of all elements with this name, e.g. "audioTrackUID"
    bool audioTrackUID_Get(const string& UID, string& TrackFormatIDRef, string& ChannelFormatIDRef, string& PackFormatIDRef); //Returns false if the UID is not present
    bool audioProgramme_Get(const string& ID, string& Name, string& FirstLabel); //Returns false if the ID is not present
    size_t audioFormatExtended_Count();
    struct broadcast_metadata // Broadcast metadata e.g. converted from BWF bext or ubxt chunks (ITU-R BS.2088-2 11)
    {
        string CreatorName;
        string OrganisationName;
        string Description;
        string CreatedDate;
        string CreatedTime;
        string CodingHistory;
        string UMID;
        bool IsFromBext = false; // Description has a bext or ubxt typeDefinition
        bool empty() const { return CreatorName.empty() && OrganisationName.empty() && Description.empty() && CreatedDate.empty() && CreatedTime.empty() && CodingHistory.empty() && UMID.empty(); }
    };
    const broadcast_metadata* BroadcastMetadata_Get();
    bool audioMXFLookUp_IsPresent();

    //Constructor/Destructor
    File_Adm();
    ~File_Adm();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Global
    void Read_Buffer_Init();
    void Read_Buffer_Continue();
 
    //Temp
    file_adm_private* File_Adm_Private;
};

} //NameSpace

#endif

