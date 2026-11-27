/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef File__ReferenceFilesHelper_SequenceH
#define File__ReferenceFilesHelper_SequenceH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File__ReferenceFilesHelper_Resource.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class resource;
typedef std::vector<resource*> resources;

//***************************************************************************
// Class sequence
//***************************************************************************

class sequence
{
public:
    //Constructor/Desctructor
                                    sequence();
                                    ~sequence();

    //In
    void                            AddFileName(const Ztring& FileName, size_t Pos=(size_t)-1);
    void                            AddResource(resource* NewResource, size_t Pos=(size_t)-1);
    void                            UpdateFileName(const Ztring& OldFileName, const Ztring& NewFileName);
    #if MEDIAINFO_ADVANCED
    void                            UpdateMetaDataFromSourceEncoding(const string& SourceEncoding, const string& Name, const string& Value);
    #endif //MEDIAINFO_ADVANCED
    stream_t                        StreamKind;
    size_t                          StreamPos;
    int64u                          StreamID;
    size_t                          MenuPos;
    bool                            Enabled;
    bool                            IsMain;
    bool                            PathOnly; //The file is searched only at its path, not with its name alone next to the referencing file (the same name may be in several folders)
    std::map<std::string, Ztring>   Config;
    bool                            ConformanceFromGeneral=false; //Conformance items of the General stream of the referenced file are added to the first stream of this file
    std::map<std::string, Ztring>   Infos;
    stream_t                        Infos_StreamKind; //Infos only for the streams of this kind, Stream_Max for all streams
    void                            FrameRate_Set(float64 NewFrameRate);

    //Out
    bool                            IsFinished()                                    {return !Enabled || Resources_Current>=Resources.size();}
    size_t                          State;
    bool                            IsCircular;
    #if MEDIAINFO_ADVANCED || MEDIAINFO_HASH
        bool                        List_Compute_Done;
    #endif //MEDIAINFO_ADVANCED || MEDIAINFO_HASH

    //Config
    #if MEDIAINFO_NEXTPACKET && MEDIAINFO_IBIUSAGE
        ibi::stream                 IbiStream;
    #endif //MEDIAINFO_NEXTPACKET && MEDIAINFO_IBIUSAGE

    resources                       Resources;
    size_t                          Resources_Current;

public:
    ZtringList          FileNames;
    Ztring              Source; //Source file name (relative path)
    float64             FrameRate;
    int64u              Delay;
    int64u              FileSize;
    bool                FileSize_IsPresent; //TODO: merge with FileSize after regression tests
    MediaInfo_Internal* MI;
    std::bitset<32> Status;
};

typedef std::vector<sequence*> sequences;

} //NameSpace

#endif
