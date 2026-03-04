/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Av1H
#define MediaInfo_Av1H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#if defined(MEDIAINFO_T35_YES)
    #include "MediaInfo/Multiple/File_T35.h"
#endif
#include "MediaInfo/File__Duplicate.h"
#include <cmath>
#include <set>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Av1
//***************************************************************************

class File_Av1 : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    bool IsAnnexB = {};

    //Constructor/Destructor
    File_Av1();
    ~File_Av1();

private :
    File_Av1(const File_Av1 &File_Av1); //No copy
    File_Av1 &operator =(const File_Av1 &); //No copy

    //Streams management
    void Streams_Accept();
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Global
    void Read_Buffer_OutOfBand();
    void Read_Buffer_Init();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    #if !defined(MEDIAINFO_T35_YES)
    #define T35(x) { Skip_XX(Element_Size - Element_Offset, "(Not parsed)"); }
    #endif
    void trailing_bits();
    void sequence_header();
    void temporal_delimiter();
    void frame_header();
    void frame_header_uncompressed_header();
    void tile_group();
    void metadata();
    void metadata_hdr_cll() { T35(File_T35::style::light_level); }
    void metadata_hdr_mdcv() { T35(File_T35::style::mastering_display_colour_volume); }
    void metadata_itu_t_t35() { T35(File_T35::style::itu_t_t35); }
    void metadata_scalability();
    void scalability_structure();
    void metadata_timecode();
    void frame();
    void padding();

    //Temp
    bool  sequence_header_Parsed{};
    bool  SeenFrameHeader{};
    bool  reduced_still_picture_header{};
    bool  show_existing_frame{};
    string GOP;
    std::set<int8u> scalability_structure_seen;
    #if defined(MEDIAINFO_T35_YES)
    std::unique_ptr<File__Analyze> T35_Parser{};
    #endif

    //Helpers
    std::string GOP_Detect(std::string PictureTypes);
    void Get_leb128(int64u& Info, const char* Name);
    #if defined(MEDIAINFO_T35_YES)
    void T35(File_T35::style Style);
    #endif
};

} //NameSpace

#endif
