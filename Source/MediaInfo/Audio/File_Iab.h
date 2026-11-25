/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_IabH
#define MediaInfo_IabH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Iab
//***************************************************************************

class File_Iab : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;

    //Out
    #if MEDIAINFO_CONFORMANCE
    struct checks
    {
        struct bed
        {
            std::vector<int32u> ChannelIDs;
            int8u       AudioDescription=0;
            std::string AudioDescriptionText;
            std::string Signature; //Items which are constant in all frames in all editions of SMPTE ST 2067-201
        };
        int64u  Frame_Count=0; //Complete frames parsed
        int32u  SampleRate=0;
        int8u   BitDepth=0;
        int32u  FrameRate_Num=0; //0 if unknown
        int32u  FrameRate_Den=1;
        size_t  Objects_Max=0; //Maximum count of objects per frame
        std::map<int32u, bed> Beds; //First frame, key is MetaID
        std::map<int32u, std::string> Objects; //Signature of all objects, key is MetaID
        bool    Characteristics_AreNotConstant=false;
        bool    Dlc_IsPresent=false;
        bool    BedRemap_IsPresent=false;
        bool    SubElements_ArePresent=false; //In Bed Definitions and Object Definitions
        bool    Conditional_IsNotAlways=false;
        bool    MetaID_IsDuplicated=false;
        bool    Beds_AreNotConstant=false;
        bool    Beds_DescriptionsAreNotConstant=false; //Audio description, SMPTE ST 2067-201:2026 only
        bool    Objects_AreNotConstant=false; //SMPTE ST 2067-201:2026 only
    };
    checks Checks;
    #endif //MEDIAINFO_CONFORMANCE

    //Constructor/Destructor
    File_Iab();
    ~File_Iab();

    //Extra
    void Streams_Fill_ForAdm();

private :
    //Streams management
    void Streams_Fill();

    //Buffer - Global
    void Read_Buffer_Continue();
    #if MEDIAINFO_CONFORMANCE
    void Read_Buffer_Unsynched();
    #endif //MEDIAINFO_CONFORMANCE

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void IAFrame();
    void BedDefinition();
    void ObjectDefinition();
    void BedRemap();
    void AudioDataPCM();

    //Temp
    int8u Version;
    int8u SampleRate;
    int8u BitDepth;
    int8u FrameRate;
    struct object
    {
        vector<int32u> ChannelLayout;
    };
    struct frame
    {
        vector<object> Objects;
    };
    frame                       Frame;
    frame                       F;
    #if MEDIAINFO_CONFORMANCE
    std::map<int32u, checks::bed> Checks_Beds; //Current frame
    std::set<int32u>            Checks_Objects; //Current frame
    size_t                      Checks_Objects_Count=0; //Current frame
    bool                        Checks_IsStarted=false;
    bool                        Checks_IsStopped=false; //Frames after a seek are not checked
    void Checks_Frame_Finish();
    #endif //MEDIAINFO_CONFORMANCE

    //Helpers
    void Get_Plex(int8u Bits, int32u& Info, const char* Name);
    void Get_Plex8(int32u& Info, const char* Name);
    void Skip_Plex(int8u Bits, const char* Name) { int32u Info; Get_Plex(Bits, Info, Name); }
    void Skip_Plex8(const char* Name) { int32u Info; Get_Plex8(Info, Name); }
};

} //NameSpace

#endif
