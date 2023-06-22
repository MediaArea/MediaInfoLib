/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_DtsUhdH
#define MediaInfo_DtsUhdH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#ifdef ES
   #undef ES //Solaris defines this somewhere
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DtsUhd
//***************************************************************************

extern const int32u FrequencyCodeTable[2];
extern const char* RepresentationTypeTable[8];

struct DTSUHD_ChannelMaskInfo
{
    int32u ChannelCount=0;
    int32u CountFront=0, CountSide=0, CountRear=0, CountLFE=0, CountHeights=0, CountLows=0;
    std::string ChannelLayoutText, ChannelPositionsText, ChannelPositions2Text;
};

struct DTSUHD_ChannelMaskInfo DTSUHD_DecodeChannelMask(int32u ChannelMask);

class File_DtsUhd : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;

    //Constructor/Destructor
    File_DtsUhd();

protected :
    //Streams management
    void Streams_Fill();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    bool FrameSynchPoint_Test();

    struct MDObject
    {
        bool Started=false;  /* Object seen since last reset. */
        int PresIndex=0;
        int RepType=0;
        int ChActivityMask=0;
    };

    struct MD01
    {
        MDObject Object[257]; /* object id max value is 256 */
        bool StaticMDUpdateFlag=false;
        int Bit=0;
        int ChunkId=0;
        int ObjectList[256]={0};
        int ObjectListCount=0;
        int PacketsAcquired=0;
        int StaticMDExtracted=0;
        int StaticMDPackets=0;
        int StaticMDPacketSize=0;
        std::vector<int8u> Buffer;
    };

    struct NAVI
    {
        bool Present=false;
        int Bytes=0;
        int Id=0;
        int Index=0;
    };

    struct UHDAudio
    {
        int Mask=0;
        int Selectable=0;
    };

    struct UHDChunk
    {
        bool CrcFlag=false;
        int Bytes=0;
    };

    struct UHDFrameDescriptor
    {
        int BaseSampleFreqCode;
        int ChannelCount;
        int ChannelMask;
        int DecoderProfileCode;
        int MaxPayloadCode;
        int NumPresCode;
        int SampleCount;
        int RepType;
    };

    int32u ReadBitsMD01(MD01* MD01, int Bits);
    MD01* ChunkAppendMD01(int Id);
    MD01* ChunkFindMD01(int Id);
    MDObject* FindDefaultAudio();
    void ExtractObjectInfo(MDObject*);
    void UpdateDescriptor();
    int ParseExplicitObjectLists(int Mask, int Index);
    int ParseAudPresParams();
    void DecodeVersion();
    int ParseStreamParams();
    void NaviPurge();
    int NaviFindIndex(int DesiredIndex, int* ListIndex);
    int ParseChunkNavi();
    int ParseMDObjectList(MD01*);
    void SkipMPParamSet(MD01*, bool NominalFlag);
    int ParseStaticMDParams(MD01*, bool OnlyFirst);
    int ParseMultiFrameMd(MD01*);
    int IsSuitableForRender(MD01*, int ObjectId);
    void ParseChMaskParams(MD01*, MDObject*);
    int ParseObjectMetadata(MD01*, MDObject*, bool, int);
    int ParseMD01(MD01*, int PresIndex);
    int ParseChunks();
    bool CheckCurrentFrame(bool SyncFrame);
    int DtsUhd_Frame();

    UHDAudio Audio[256];
    UHDFrameDescriptor FrameDescriptor;
    bool FullChannelMixFlag;
    bool InteractiveObjLimitsPresent;
    bool SyncFrameFlag;
    const int8u* FrameStart;
    int ChunkBytes;
    int ClockRate;
    int FTOCBytes;
    int FrameBit;
    int FrameDuration;
    int FrameDurationCode;
    int FrameRate;
    int MajorVersion;
    int NumAudioPres;
    int SampleRate;
    int SampleRateMod;
    int32u FrameSize;
    int32u FramesTotal;
    std::vector<MD01> MD01List;
    std::vector<NAVI> NaviList;
    std::vector<UHDChunk> ChunkList;
};

} //NameSpace

#endif
