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
#if defined(MEDIAINFO_XMP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_Xmp.h"
#include "ThirdParty/tfsxml/tfsxml.h"
#include "ThirdParty/base64/base64.h"
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

#define NS(X) #X ":"

#define XML_XMP_START \
    Result = tfsxml_enter(&p); \
    if (Result > 0) { \
        return false; \
    } \
    Result = tfsxml_next(&p, &n); if (Result < 0) { \
        Reject("XMP"); \
        return false; \
    } if (Result > 0) { \
        return Result; \
    } \
    if (!tfsxml_strncmp_charp(n, "?xpacket begin=", 15)) { \
        Result = tfsxml_enter(&p); \
        if (Result > 0) { \
            return false; \
        } \
        Result = tfsxml_next(&p, &n); if (Result < 0) { \
            Reject("XMP"); \
            return false; \
        } if (Result > 0) { \
            return Result; \
        } \
    } \
    if (false) { \

#define XML_XMP_END \
    } \

#define XML_ELEMENT_START \
    Result = tfsxml_enter(&p); \
    if (Result > 0) { \
        return false; \
    } \
    for (;;) { \
            Result = tfsxml_next(&p, &n); \
            if (Result < 0) { \
                break; \
            } \
            if (Result > 0) { \
                return Result; \
            } \
        if (false) { \

#define XML_LIST(NAMESPACE, NAME) \
    Result = tfsxml_enter(&p); \
    if (Result > 0) { \
        return false; \
    } \
    for (;;) { \
            Result = tfsxml_next(&p, &n); \
            if (Result < 0) { \
                break; \
            } \
            if (Result > 0) { \
                return Result; \
            } \
        if (tfsxml_strcmp_charp(n, "rdf:li")) { \
            continue; \
        } \
        else { \
            Result = tfsxml_value(&p, &v); \
            if (Result > 0) { \
                return Result; \
            } \
            NAMESPACE(NAME, tfsxml_decode(v)); \
        } \
    } \

#define XML_ELEMENT(NAME) \
        } \
        else if (!tfsxml_strcmp_charp(n, NAME)) { \

#define XML_ELEMENT_NAMESPACE(NAMESPACE) \
        } \
        else if (!tfsxml_strncmp_charp(n, NS(NAMESPACE), sizeof(#NAMESPACE))) { \
            XML_VALUE \
            NAMESPACE(tfsxml_decode(n), tfsxml_decode(v)); \

#define XML_ELEMENT_LIST(TYPE) \
        XML_ELEMENT(TYPE) \
            XML_ELEMENT_START \
            XML_ELEMENT("rdf:li") \
                XML_ELEMENT_START \

#define XML_ELEMENT_LIST_NAMESPACE(NAMESPACE, NAME, LISTTYPE) \
        XML_ELEMENT(NAME) \
            XML_ELEMENT_START \
            XML_ELEMENT(LISTTYPE) \
                XML_LIST(NAMESPACE, NAME) \
            XML_ELEMENT_END \

#define XML_ACCEPT \
        Accept("XMP"); \

#define XML_ELSE_REJECT \
        } \
        else { \
            Reject("XMP"); \
            return false;

#define XML_VALUE \
            Result = tfsxml_value(&p, &v); \
            if (Result > 0) { \
                return Result; \
            } \

#define XML_ATTRIBUTE_START \
    for (;;) { \
        { \
            Result = tfsxml_attr(&p, &n, &v); \
            if (Result < 0) { \
                break; \
            } \
            if (Result > 0) { \
                return Result; \
            } \
        } \
        if (false) { \

#define XML_ATTRIBUTE(NAME) \
        } \
        else if (!tfsxml_strcmp_charp(n, NAME)) { \


#define XML_ATTRIBUTE_NAMESPACE(NAMESPACE) \
        } \
        else if (!tfsxml_strncmp_charp(n, NS(NAMESPACE), sizeof(#NAMESPACE))) { \
            NAMESPACE(tfsxml_decode(n), tfsxml_decode(v)); \
         
#define XML_ATTRIBUTE_END \
        } \
    } \

#define XML_ELEMENT_END \
        } \
    } \

#define XML_ELEMENT_LIST_END \
                XML_ELEMENT_END \
            XML_ELEMENT_END \

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Xmp::FileHeader_Begin()
{
    if (Wait || (!IsSub && Buffer_Size < File_Size)) {
        Element_WaitForMoreData();
        return false;
    }

    auto ParseBase64Image = [this](const char* input_data, const char* muxing_mode, const char* description) -> void {
            if (input_data) {
                std::string Data_Raw(Base64::decode(input_data));
                auto Buffer_Save = Buffer;
                auto Buffer_Offset_Save = Buffer_Offset;
                auto Buffer_Size_Save = Buffer_Size;
                auto Element_Offset_Save = Element_Offset;
                auto Element_Size_Save = Element_Size;
                Buffer = (const int8u*)Data_Raw.c_str();
                Buffer_Offset = 0;
                Buffer_Size = Data_Raw.size();
                Element_Offset = 0;
                Element_Size = Buffer_Size;

                //Filling
                Attachment(muxing_mode, Ztring(), description);

                Buffer = Buffer_Save;
                Buffer_Offset = Buffer_Offset_Save;
                Buffer_Size = Buffer_Size_Save;
                Element_Offset = Element_Offset_Save;
                Element_Size = Element_Size_Save;
            }
        };

    tfsxml_string p{}, n{}, v{};
    auto Result = tfsxml_init(&p, Buffer, Buffer_Size, 0);
    XML_XMP_START
    XML_ELEMENT("x:xmpmeta")
        XML_ELEMENT_START
        XML_ELEMENT("rdf:RDF")
            XML_ACCEPT
            XML_ELEMENT_START
            XML_ELEMENT("rdf:Description")
                XML_ATTRIBUTE_START
                XML_ATTRIBUTE("GCamera:RelitInputImageData")
                    ParseBase64Image(tfsxml_decode(v).c_str(), "Extended XMP / GCamera", "Relit Input Image");
                XML_ATTRIBUTE("GDepth:Data")
                    ParseBase64Image(tfsxml_decode(v).c_str(), "Extended XMP / GDepth Data", "Depth");
                XML_ATTRIBUTE("GDepth:Confidence")
                    ParseBase64Image(tfsxml_decode(v).c_str(), "Extended XMP / GDepth Confidence", "Confidence");
                XML_ATTRIBUTE("GImage:Data")
                    ParseBase64Image(tfsxml_decode(v).c_str(), "Extended XMP / GImage Data", "Image");
                XML_ATTRIBUTE("xmlns:pdfaid")
                    pdfaid = tfsxml_decode(v);
                XML_ATTRIBUTE("pdfaid:part")
                    pdfaid_part = tfsxml_decode(v);
                XML_ATTRIBUTE("pdfaid:conformance")
                    pdfaid_conformance = tfsxml_decode(v);
                XML_ATTRIBUTE_NAMESPACE(exif)
                XML_ATTRIBUTE_NAMESPACE(pdf)
                XML_ATTRIBUTE_NAMESPACE(photoshop)
                XML_ATTRIBUTE_NAMESPACE(xmp)
                XML_ATTRIBUTE_NAMESPACE(Iptc4xmpExt)
                XML_ATTRIBUTE_END
                XML_ELEMENT_START
                XML_ELEMENT_LIST_NAMESPACE(dc, "dc:title", "rdf:Alt")
                XML_ELEMENT_LIST_NAMESPACE(dc, "dc:description", "rdf:Alt")
                XML_ELEMENT_LIST_NAMESPACE(dc, "dc:subject", "rdf:Bag")
                XML_ELEMENT_LIST_NAMESPACE(dc, "dc:creator", "rdf:Seq")
                XML_ELEMENT_LIST_NAMESPACE(dc, "dc:rights", "rdf:Alt")
                XML_ELEMENT("GDepth:Data")
                    XML_VALUE
                    ParseBase64Image(tfsxml_decode(v).c_str(), "Extended XMP / GDepth Data", "Depth");
                XML_ELEMENT("GDepth:Confidence")
                    XML_VALUE
                    ParseBase64Image(tfsxml_decode(v).c_str(), "Extended XMP / GDepth Confidence", "Confidence");
                XML_ELEMENT("GImage:Data")
                    XML_VALUE
                    ParseBase64Image(tfsxml_decode(v).c_str(), "Extended XMP / GImage Data", "Image");
                XML_ELEMENT("Container:Directory")
                    if (GContainerItems) {
                        XML_ELEMENT_START
                        XML_ELEMENT_LIST("rdf:Seq")
                            XML_ELEMENT("Container:Item")
                                gc_item GCItem{};
                                XML_ATTRIBUTE_START
                                XML_ATTRIBUTE("Item:Mime")
                                    GCItem.Mime = tfsxml_decode(v);
                                XML_ATTRIBUTE("Item:Semantic")
                                    GCItem.Semantic = tfsxml_decode(v);
                                XML_ATTRIBUTE("Item:Length")
                                    GCItem.Length = Ztring(tfsxml_decode(v).c_str()).To_int32u();
                                XML_ATTRIBUTE("Item:Label")
                                    GCItem.Label = tfsxml_decode(v);
                                XML_ATTRIBUTE("Item:Padding")
                                    GCItem.Padding = Ztring(tfsxml_decode(v).c_str()).To_int32u();
                                XML_ATTRIBUTE("Item:URI")
                                    GCItem.URI = tfsxml_decode(v);
                                XML_ATTRIBUTE_END
                                GContainerItems->push_back(GCItem);
                        XML_ELEMENT_LIST_END
                        XML_ELEMENT_END
                    }
                XML_ELEMENT("Device:Container")
                    if (GContainerItems) {
                        XML_ELEMENT_START
                        XML_ELEMENT("Container_1_:Directory")
                            XML_ELEMENT_START
                            XML_ELEMENT_LIST("rdf:Seq")
                                XML_ELEMENT("rdf:value")
                                    gc_item GCItem{};
                                    XML_ELEMENT_START
                                    XML_ELEMENT("Item_1_:Mime")
                                        XML_VALUE
                                        GCItem.Mime = tfsxml_decode(v);
                                    XML_ELEMENT("Item_1_:Length")
                                        XML_VALUE
                                        GCItem.Length = Ztring(tfsxml_decode(v).c_str()).To_int32u();
                                    XML_ELEMENT("Item_1_:DataURI")
                                        XML_VALUE
                                        auto DataURI = tfsxml_decode(v);
                                        if (DataURI == "primary_image")
                                            GCItem.Semantic = "Primary";
                                        if (DataURI == "android/original_image")
                                            GCItem.Semantic = "Original";
                                        if (DataURI == "android/depthmap")
                                            GCItem.Semantic = "Depth";
                                        if (DataURI == "android/confidencemap")
                                            GCItem.Semantic = "Confidence";
                                    XML_ELEMENT_END
                                    GContainerItems->push_back(GCItem);
                            XML_ELEMENT_LIST_END
                            XML_ELEMENT_END
                        XML_ELEMENT("Container:Directory")
                            XML_ELEMENT_START
                            XML_ELEMENT_LIST("rdf:Seq")
                                XML_ELEMENT("Container:Item")
                                    gc_item GCItem{};
                                    XML_ATTRIBUTE_START
                                    XML_ATTRIBUTE("Item:Mime")
                                        GCItem.Mime = tfsxml_decode(v);
                                    XML_ATTRIBUTE("Item:Length")
                                        GCItem.Length = Ztring(tfsxml_decode(v).c_str()).To_int32u();
                                    XML_ATTRIBUTE("Item:DataURI")
                                        auto DataURI = tfsxml_decode(v);
                                        if (DataURI == "primary_image")
                                            GCItem.Semantic = "Primary";
                                        if (DataURI == "android/original_image")
                                            GCItem.Semantic = "Original";
                                        if (DataURI == "android/depthmap")
                                            GCItem.Semantic = "Depth";
                                        if (DataURI == "android/confidencemap")
                                            GCItem.Semantic = "Confidence";
                                    XML_ATTRIBUTE_END
                                    GContainerItems->push_back(GCItem);
                            XML_ELEMENT_LIST_END
                            XML_ELEMENT_END
                        XML_ELEMENT_END
                    }
                XML_ELEMENT("pdfaid:part")
                    XML_VALUE
                    pdfaid_part = tfsxml_decode(v);
                XML_ELEMENT("pdfaid:conformance")
                    XML_VALUE
                    pdfaid_conformance = tfsxml_decode(v);
                XML_ELEMENT_NAMESPACE(photoshop)
                XML_ELEMENT_NAMESPACE(xmp)
                XML_ELEMENT_NAMESPACE(Iptc4xmpExt)
                XML_ELEMENT_END
            XML_ELEMENT_END
        XML_ELSE_REJECT
        XML_ELEMENT_END
    XML_ELSE_REJECT
    XML_XMP_END

    if (!pdfaid.empty()) {
        string Profile;
        if (pdfaid != "http://www.aiim.org/pdfa/ns/id/")
            Profile = pdfaid;
        else {
            Profile += "A";
            if (!pdfaid_part.empty()) {
                Profile += '-';
                Profile += pdfaid_part;
                if (!pdfaid_conformance.empty()) {
                    string Conformance{ pdfaid_conformance };
                    if (Conformance.size() == 1 && Conformance[0] >= 'A' && Conformance[0] <= 'Z')
                        Conformance[0] += 0x20; // From "A" to "a"
                    Profile += Conformance;
                }
            }
        }
        Fill(Stream_General, 0, General_Format_Profile, Profile);
    }

    Finish();
    return true;
}

//---------------------------------------------------------------------------
void File_Xmp::dc(const string &name, const string &value)
{
    size_t parameter{};
    if (name == "dc:title")
        parameter = General_Title;
    if (name == "dc:description")
        parameter = General_Description;
    if (name == "dc:subject")
        parameter = General_Subject;
    if (name == "dc:creator")
        parameter = General_Producer;
    if (name == "dc:rights")
        parameter = General_Copyright;
    if (parameter)
        Fill(Stream_General, 0, parameter, value);
}

//---------------------------------------------------------------------------
void File_Xmp::exif(const string& name, const string& value)
{
    if (name == "exif:DateTimeOriginal")
        Fill(Stream_General, 0, General_Recorded_Date, value);
    if (name == "exif:DateTimeDigitized")
        Fill(Stream_General, 0, General_Mastered_Date, value);
}

//---------------------------------------------------------------------------
void File_Xmp::pdf(const string& name, const string& value)
{
    if (name == "pdf:Description" && Retrieve_Const(Stream_General, 0, General_Description).empty())
        Fill(Stream_General, 0, General_Description, value);
    if (name == "pdf:Keywords" && Retrieve_Const(Stream_General, 0, General_Keywords).empty())
        Fill(Stream_General, 0, General_Keywords, value);
    if (name == "pdf:Producer" && Retrieve_Const(Stream_General, 0, General_Encoded_Library).empty())
        Fill(Stream_General, 0, General_Encoded_Library, value);
}

//---------------------------------------------------------------------------
void File_Xmp::photoshop(const string& name, const string& value)
{
    if (name == "photoshop:Credit")
        Fill(Stream_General, 0, General_Copyright, value);
    if (name == "photoshop:DateCreated") {
        CreateDate = Ztring().From_UTF8(value);
        if (CreateDate > ModifyDate)
            Fill(Stream_General, 0, General_Encoded_Date, CreateDate, true);
    }
}

//---------------------------------------------------------------------------
void File_Xmp::xmp(const string& name, const string& value)
{
    if (name == "xmp:CreateDate") {
        CreateDate = Ztring().From_UTF8(value);
        if (CreateDate > ModifyDate)
            Fill(Stream_General, 0, General_Encoded_Date, CreateDate, true);
    }
    if (name == "xmp:ModifyDate") {
        ModifyDate = Ztring().From_UTF8(value);
        if (ModifyDate > CreateDate)
            Fill(Stream_General, 0, General_Encoded_Date, ModifyDate, true);
    }
    if (name == "xmp:CreatorTool")
        Fill(Stream_General, 0, General_Encoded_Application, value);
    if (name == "xmp:Description")
        Fill(Stream_General, 0, General_Description, value, true, true);
    if (name == "xmp:Keywords")
        Fill(Stream_General, 0, General_Keywords, value, true, true);
    if (name == "xmp:Producer")
        Fill(Stream_General, 0, General_Encoded_Library, value, true, true);
}

//---------------------------------------------------------------------------
void File_Xmp::Iptc4xmpExt(const string& name, const string& value)
{
    if (name == "Iptc4xmpExt:DigitalSourceType") {
        string URI{ value };
        string::size_type pos = URI.find("https://");
        if (pos != std::string::npos) URI.replace(pos, 5, "http"); // Some Google generated files have https instead of http
        if (!strcmp(URI.c_str(), "http://cv.iptc.org/newscodes/digitalsourcetype/trainedAlgorithmicMedia"))
            Fill(Stream_General, 0, General_Copyright, "Created using generative AI");
        if (!strcmp(URI.c_str(), "http://cv.iptc.org/newscodes/digitalsourcetype/compositeWithTrainedAlgorithmicMedia"))
            Fill(Stream_General, 0, General_Copyright, "Edited using generative AI");
    }
}

} //NameSpace

#endif //MEDIAINFO_XMP_YES
