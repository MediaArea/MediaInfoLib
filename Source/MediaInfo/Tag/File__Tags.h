/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about all kind of framed tags tagged files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File__TagsH
#define MediaInfo_File__TagsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File__UnCompressedSize_Helper
//***************************************************************************
class File__UnCompressedSize_Helper
{
    protected:
        int64u Duration;
        int64u UncompressedSize;
    public:
        File__UnCompressedSize_Helper() :Duration(0), UncompressedSize(0) {}
        bool CalcDurationUncompressedSize(int64u Samples, int32u SampleRate, int8u BitsPerSample, int8u Channels)
        {
            if (SampleRate == 0)
                return false;
            Duration = Samples * 1000 / SampleRate;
            if (Duration == 0)
                return false;
            UncompressedSize = Samples * Channels * BitsPerSample  / 8;
            if (UncompressedSize == 0)
                return false;
            return true;
        }
};

//***************************************************************************
// Class File__Tags_Helper
//***************************************************************************

class File__Tags_Helper
{
public :
    //In
    File__Analyze* Base;

    //Out
    int64u TagsSize;
    int64u File_BeginTagSize;
    int64u File_EndTagSize;
    int64u File_BeginPaddingSize; //Zero bytes parsed by Synchronize_TagsAndPadding() before the first byte that is neither zero nor in a tag
    int64u NotZero_Offset; //First byte since the last synchronization that is neither zero nor in a tag parsed by Synchronize_TagsAndPadding(), reset by the parser at each synchronization

    //Constructor/Destructor
    File__Tags_Helper();
    ~File__Tags_Helper();

    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin() {return Synched_Test();}

    //Buffer - Synchro
    bool Synchronize(bool &Tag_Found, size_t Synchro_Offset=0);
    bool Synchronize_TagsAndPadding(bool &Tag_Found); //Tags and zero bytes before the stream data, tags after zero bytes are tested as tags at the start
    bool SingleFrame_IsEnough(); //Before the stream is accepted, a single frame followed by the end of the file or by a tag, after other data, is likely a coincidence
    bool Synched_Test();

    //Buffer - Global
    bool Read_Buffer_Continue ();

    //Per element
    bool Header_Begin() {return Synched_Test();}

    //Streams
    size_t Stream_Prepare(stream_t StreamKind);

    //End
    void GoTo           (int64u GoTo, const char* ParserName=NULL);
    void GoToFromEnd    (int64u GoToFromEnd=0, const char* ParserName=NULL);
    void Accept         (const char* ParserName=NULL);
    void Reject         (const char* ParserName=NULL);
    void Finish         (const char* ParserName=NULL);

private :
    //Temp
    File__Analyze* Parser;
    File__Analyze::tags_format Parser_Format;
    vector<File__Analyze*> Parser_Streams_Fill; //Parser to merge when filling
    vector<File__Analyze::tags_format> Parser_Streams_Fill_Format;
    void Parser_Merge(File__Analyze& MI, File__Analyze::tags_format Format);
    size_t         Parser_Buffer_Size;
    int64u Id3v1_Offset;
    int64u Lyrics3_Offset;
    int64u Lyrics3v2_Offset;
    int64u ApeTag_Offset;
    int64u Id3v2_Offset;
    int64u JumpTo_WantedByParser;
    int64u Id3v1_Size;
    int64u Lyrics3_Size;
    int64u Lyrics3v2_Size;
    int64u ApeTag_Size;
    int64u Id3v2_Size;
    bool TagSizeIsFinal;
    bool BeginPaddingSizeIsFinal;
    bool SearchingForEndTags;
    bool SkippingUpToEnd;
    int64u LastTag_End; //File offset after the last tag parsed

    //Helpers
    bool DetectBeginOfEndTags();        //return true if we can continue, false if want return
    bool DetectBeginOfEndTags_Test();

};} //NameSpace

#endif
