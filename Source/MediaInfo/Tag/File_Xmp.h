/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about XMP files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_XmpH
#define MediaInfo_File_XmpH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//GContainer items
struct gc_item
{
    string Mime;
    string Semantic;
    int32u Length;
    string Label;
    int32u Padding;
    string URI;
};
typedef std::vector<gc_item> gc_items;

//***************************************************************************
// Class File_Xmp
//***************************************************************************

class File_Xmp : public File__Analyze
{
public:
    bool Wait{ false };
    bool NoTrace{ false };
    gc_items* GContainerItems{ nullptr };
    stream_t GainMap_StreamKind{ Stream_Max }; //If set, gain map metadata is filled in this kind of stream (one stream per metadata format), to be merged in the image using the gain map
    bool GainMap_Apple_Exif_Present{ false }; //Headroom from Apple Exif maker notes of the primary image, for Apple gain maps without headroom in XMP
    float64 GainMap_Apple_Exif_Headroom{ 0 };

private :
    //Buffer - File header
    bool FileHeader_Begin() override;
    void Read_Buffer_Continue() override;

    //Element namespaces
    void dc(const string& name, const string& value);
    void exif(const string& name, const string& value);
    void GIMP(const string& name, const string& value);
    void hdrgm(const string& name, const string& value, const int count = 0);
    void HDRGainMap(const string& name, const string& value);
    void HDRToneMap(const string& name, const string& value, const int count = 0);
    void pdf(const string& name, const string& value);
    void photoshop(const string& name, const string& value);
    void xmp(const string& name, const string& value);
    void Iptc4xmpExt(const string& name, const string& value);

    //Gain map
    void GainMap_Fill();
    struct gainmap_adobe
    {
        string Version;
        bool BaseRenditionIsHDR{ false };
        int ChannelCount{ 1 };
        bool HasValues{ false };
        bool HasHDRCapacity{ false };
        bool HasBaseAlternateHeadroom{ false };
        float32 HDRCapacityMin{ 0 };
        float32 HDRCapacityMax{};
        float32 BaseHeadroom{ 0 };
        float32 AlternateHeadroom{ 0 };
        float32 GainMapMin[3]{ 0, 0, 0 };
        float32 GainMapMax[3]{};
        float32 Gamma[3]{ 1, 1, 1 };
        float32 OffsetSDR[3]{ 1.0f / 64, 1.0f / 64, 1.0f / 64 };
        float32 OffsetHDR[3]{ 1.0f / 64, 1.0f / 64, 1.0f / 64 };
    };
    gainmap_adobe GainMap_Adobe;
    string GainMap_Apple_Version;
    string GainMap_Apple_Headroom;

    //Temp
    Ztring ModifyDate, CreateDate;
    string gimp_version, pdfaid, pdfaid_part, pdfaid_conformance;
};

} //NameSpace

#endif
