/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

 // audioProgramme
 // - audioContent
 // - - audioObject
 // - - - audioPackFormat
 // - - - - audioChannelFormat
 // - - - audioTrackUID
 // - - - - audioChannelFormat +
 // - - - - audioPackFormat
 // - - - - - audioChannelFormat +
 // - - - - audioTrackFormat
 // - - - - - audioStreamFormat
 // - - - - - - audioChannelFormat +
 // - - - - - - audioPackFormat +
 // - - - - - - audioTrackFormat +
 
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
#if defined(MEDIAINFO_ADM_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Adm.h"
#include "ThirdParty/tfsxml/tfsxml.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{


enum items {
    item_audioProgramme,
    item_audioContent,
    item_audioObject,
    item_audioPackFormat,
    item_audioChannelFormat,
    item_audioTrackUID,
    item_audioTrackFormat,
    item_audioStreamFormat,
    item_Max
};

enum audioProgramme_String {
    audioProgramme_audioProgrammeID,
    audioProgramme_audioProgrammeName,
    audioProgramme_audioProgrammeLanguage,
    audioProgramme_start,
    audioProgramme_end,
    audioProgramme_typeLabel,
    audioProgramme_String_Max
};

enum audioProgramme_StringVector {
    audioProgramme_audioProgrammeLabel,
    audioProgramme_audioContentIDRef,
    audioProgramme_StringVector_Max
};

enum audioContent_String {
    audioContent_audioContentID,
    audioContent_audioContentName,
    audioContent_audioContentLanguage,
    audioContent_typeLabel,
    audioContent_String_Max
};

enum audioContent_StringVector {
    audioContent_dialogue,
    audioContent_audioContentLabel,
    audioContent_integratedLoudness,
    audioContent_audioObjectIDRef,
    audioContent_StringVector_Max
};

enum audioObject_String {
    audioObject_audioObjectID,
    audioObject_audioObjectName,
    audioObject_startTime,
    audioObject_duration,
    audioObject_typeLabel,
    audioObject_String_Max
};

enum audioObject_StringVector {
    audioObject_audioPackFormatIDRef,
    audioObject_audioTrackUIDRef,
    audioObject_StringVector_Max
};

enum audioPackFormat_String {
    audioPackFormat_audioPackFormatID,
    audioPackFormat_audioPackFormatName,
    audioPackFormat_typeDefinition,
    audioPackFormat_typeLabel,
    audioPackFormat_String_Max
};

enum audioPackFormat_StringVector {
    audioPackFormat_audioChannelFormatIDRef,
    audioPackFormat_StringVector_Max
};

enum audioChannelFormat_String {
    audioChannelFormat_audioChannelFormatID,
    audioChannelFormat_audioChannelFormatName,
    audioChannelFormat_typeDefinition,
    audioChannelFormat_typeLabel,
    audioChannelFormat_String_Max
};

enum audioChannelFormat_StringVector {
    audioChannelFormat_StringVector_Max
};

enum audioTrackUID_String {
    audioTrackUID_UID,
    audioTrackUID_bitDepth,
    audioTrackUID_sampleRate,
    audioTrackUID_typeLabel,
    audioTrackUID_String_Max
};

enum audioTrackUID_StringVector {
    audioTrackUID_audioChannelFormatIDRef,
    audioTrackUID_audioPackFormatIDRef,
    audioTrackUID_audioTrackFormatIDRef,
    audioTrackUID_StringVector_Max
};

enum audioTrackFormat_String {
    audioTrackFormat_audioTrackFormatID,
    audioTrackFormat_audioTrackFormatName,
    audioTrackFormat_formatDefinition,
    audioTrackFormat_typeDefinition,
    audioTrackFormat_typeLabel,
    audioTrackFormat_String_Max
};

enum audioTrackFormat_StringVector {
    audioTrackFormat_audioStreamFormatIDRef,
    audioTrackFormat_StringVector_Max
};

enum audioStreamFormat_String {
    audioStreamFormat_audioStreamFormatID,
    audioStreamFormat_audioStreamFormatName,
    audioStreamFormat_formatDefinition,
    audioStreamFormat_formatLabel,
    audioStreamFormat_typeDefinition,
    audioStreamFormat_typeLabel,
    audioStreamFormat_String_Max
};

enum audioStreamFormat_StringVector {
    audioStreamFormat_audioChannelFormatIDRef,
    audioStreamFormat_audioPackFormatIDRef,
    audioStreamFormat_audioTrackFormatIDRef,
    audioStreamFormat_StringVector_Max
};

struct Item_Struct {
    vector<string> Strings;
    vector<vector<string> > StringVectors;
    map<string, string> Extra;
};

struct Items_Struct {
    void Init(size_t Strings_Size_, size_t StringVectors_Size_) {
        Strings_Size = Strings_Size_;
        StringVectors_Size =StringVectors_Size_;
    }

    Item_Struct& New()
    {
        Items.resize(Items.size() + 1);
        Item_Struct& Item = Items[Items.size() - 1];
        Item.Strings.resize(Strings_Size);
        Item.StringVectors.resize(StringVectors_Size);
        return Item;
    }

    Item_Struct& Last()
    {
        Item_Struct& Item = Items[Items.size() - 1];
        return Item;
    }

    vector<Item_Struct> Items;
    size_t Strings_Size;
    size_t StringVectors_Size;
};

static string Apply_Init(File__Analyze& F, const Char* Name, size_t i, const Items_Struct& audioProgramme_List, Ztring Summary) {
    const Item_Struct& audioProgramme = audioProgramme_List.Items[i];
    string P = Ztring(Name + Ztring::ToZtring(i)).To_UTF8();
    F.Fill(Stream_Audio, 0, P.c_str(), Summary.empty() ? __T("Yes") : Summary);
    F.Fill(Stream_Audio, 0, (P + " Pos").c_str(), i);
    F.Fill_SetOptions(Stream_Audio, 0, (P + " Pos").c_str(), "N NIY");
    return P;
}

static void Apply_SubStreams(File__Analyze& F, const string& P_And_LinkedTo, const Item_Struct& Source, size_t i, const Items_Struct& Dest) {
    ZtringList SubstreamPos, SubstreamNum;
    for (size_t j = 0; j < Source.StringVectors[i].size(); j++) {
        const string& ID = Source.StringVectors[i][j];
        size_t Pos = -1;
        for (size_t k = 0; k < Dest.Items.size(); k++) {
            if (Dest.Items[k].Strings[0] == ID) {
                Pos = k;
                break;
            }
        }
        if (Pos == -1) {
            continue;
        }
        SubstreamPos.push_back(Ztring::ToZtring(Pos));
        SubstreamNum.push_back(Ztring::ToZtring(Pos + 1));
    }
    if (SubstreamPos.empty())
        return;
    SubstreamPos.Separator_Set(0, __T(" + "));
    F.Fill(Stream_Audio, 0, P_And_LinkedTo.c_str(), SubstreamPos.Read());
    F.Fill_SetOptions(Stream_Audio, 0, P_And_LinkedTo.c_str(), "N NIY");
    SubstreamNum.Separator_Set(0, __T(" + "));
    F.Fill(Stream_Audio, 0, (P_And_LinkedTo + "/String").c_str(), SubstreamNum.Read());
    F.Fill_SetOptions(Stream_Audio, 0, (P_And_LinkedTo + "/String").c_str(), "Y NIN");
}

class file_adm_private
{
public:
    tfsxml_string p;
    Items_Struct Items[item_Max];

    void parse();
    void coreMetadata();
    void format();
    void audioFormatExtended();
};

void file_adm_private::parse()
{
    tfsxml_string b, v;

    # define STRUCTS(NAME) \
        Items[item_##NAME].Init(NAME##_String_Max, NAME##_StringVector_Max);

    STRUCTS(audioProgramme);
    STRUCTS(audioContent);
    STRUCTS(audioObject);
    STRUCTS(audioPackFormat);
    STRUCTS(audioChannelFormat);
    STRUCTS(audioTrackUID);
    STRUCTS(audioTrackFormat);
    STRUCTS(audioStreamFormat);

    
    #define ELEMENT_START(NAME) \
        if (!tfsxml_cmp_charp(b, #NAME)) \
        { \
            Item_Struct& NAME##_Content = Items[item_##NAME].New(); \
            for (;;) { \
                if (tfsxml_attr(&p, &b, &v)) \
                    break; \
            if (false) { \
            } \

    #define ELEMENT_MIDDLE() \
            } \
        tfsxml_enter(&p, &b); \
        for (;;) { \
            if (tfsxml_next(&p, &b)) \
                break; \
        if (false) { \
        } \

    #define ELEMENT_END() \
            } \
        } \

    #define ATTRIBUTE(NAME,ATTR) \
        if (!tfsxml_cmp_charp(b, #ATTR)) { \
            NAME##_Content.Strings[NAME##_##ATTR].assign(v.buf, v.len); \
        } \

    #define ELEMENT(NAME,ELEM) \
        if (!tfsxml_cmp_charp(b, #ELEM)) { \
            tfsxml_value(&p, &b); \
            NAME##_Content.StringVectors[NAME##_##ELEM].push_back(string(b.buf, b.len)); \
        } \

    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        if (!tfsxml_cmp_charp(b, "ebuCoreMain"))
        {
            tfsxml_enter(&p, &b);
            for (;;) {
                if (tfsxml_next(&p, &b))
                    break;
                if (!tfsxml_cmp_charp(b, "coreMetadata"))
                {
                    coreMetadata();
                }
            }
        }
        if (!tfsxml_cmp_charp(b, "frame"))
        {
            format();
        }
        if (!tfsxml_cmp_charp(b, "format"))
        {
            format();
        }
    }
}

void file_adm_private::coreMetadata()
{
    tfsxml_string b;

    tfsxml_enter(&p, &b);
    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        if (!tfsxml_cmp_charp(b, "format"))
        {
            format();
        }
    }
}

void file_adm_private::format()
{
    tfsxml_string b;

    tfsxml_enter(&p, &b);
    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        if (!tfsxml_cmp_charp(b, "audioFormatExtended"))
        {
            audioFormatExtended();
        }
    }
}

void file_adm_private::audioFormatExtended()
{
    tfsxml_string b, v;

    tfsxml_enter(&p, &b);
    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        ELEMENT_START(audioProgramme)
            ATTRIBUTE(audioProgramme, audioProgrammeID);
            ATTRIBUTE(audioProgramme, audioProgrammeName);
            ATTRIBUTE(audioProgramme, audioProgrammeLanguage);
            ATTRIBUTE(audioProgramme, start);
            ATTRIBUTE(audioProgramme, end);
        ELEMENT_MIDDLE()
            ELEMENT(audioProgramme, audioContentIDRef);
            if (!tfsxml_cmp_charp(b, "audioProgrammeLabel")) {
                string Language;
                for (;;) {
                    if (tfsxml_attr(&p, &b, &v))
                        break;
                    if (!tfsxml_cmp_charp(b, "language")) {
                        Language += string(v.buf, v.len);
                    }
                }
                tfsxml_value(&p, &b);
                string Value = string(b.buf, b.len);
                if (!Value.empty() && !Language.empty()) {
                    Value.insert(0, '(' + Language + ')');
                }
                audioProgramme_Content.StringVectors[audioProgramme_audioProgrammeLabel].push_back(Value);
            }
        ELEMENT_END()
        ELEMENT_START(audioContent)
            ATTRIBUTE(audioContent, audioContentID);
            ATTRIBUTE(audioContent, audioContentName);
            ATTRIBUTE(audioContent, audioContentLanguage);
            ATTRIBUTE(audioContent, typeLabel);
        ELEMENT_MIDDLE()
            ELEMENT(audioContent, audioObjectIDRef);
            if (!tfsxml_cmp_charp(b, "dialogue")) {
                string Type;
                for (;;) {
                    if (tfsxml_attr(&p, &b, &v))
                        break;
                    if (!tfsxml_cmp_charp(b, "nonDialogueContentKind")
                        || !tfsxml_cmp_charp(b, "dialogueContentKind")
                        || !tfsxml_cmp_charp(b, "mixedContentKind")) {
                        Type += string(v.buf, v.len);
                    }
                }
                tfsxml_value(&p, &b);
                string Value;
                if (!tfsxml_cmp_charp(b, "0")) {
                    if (Type == "1") {
                        Value = "Music";
                    }
                    else if (Type == "2") {
                        Value = "Effect";
                    }
                    else {
                        Value = "No Dialogue";
                        if (!Type.empty() && Type != "0") {
                            Value += " (" + Type + ')';
                        }
                    }
                }
                else if (!tfsxml_cmp_charp(b, "1")) {
                    if (Type == "1") {
                        Value = "Music";
                    }
                    else if (Type == "2") {
                        Value = "Effect";
                    }
                    else if (Type == "3") {
                        Value = "Spoken Subtitle";
                    }
                    else if (Type == "4") {
                        Value = "Visually Impaired";
                    }
                    else if (Type == "5") {
                        Value = "Commentary";
                    }
                    else if (Type == "6") {
                        Value = "Emergency";
                    }
                    else {
                        Value = "Dialogue";
                        if (!Type.empty() && Type != "0") {
                            Value += " (" + Type + ')';
                        }
                    }
                }
                else if (!tfsxml_cmp_charp(b, "2")) {
                    if (Type == "1") {
                        Value = "Complete Main";
                    }
                    else if (Type == "2") {
                        Value = "Mixed (Mixed)";
                    }
                    else if (Type == "3") {
                        Value = "Hearing Impaired";
                    }
                    else {
                        Value = "Mixed";
                        if (!Type.empty() && Type != "0") {
                            Value += " (" + Type + ')';
                        }
                    }
                }
                else {
                    Value = string(b.buf, b.len);
                    if (!Type.empty()) {
                        Value += " (" + Type + ')';
                    }
                }
                audioContent_Content.StringVectors[audioContent_dialogue].push_back(Value);
            }
            if (!tfsxml_cmp_charp(b, "audioContentLabel")) {
                string Language;
                for (;;) {
                    if (tfsxml_attr(&p, &b, &v))
                        break;
                    if (!tfsxml_cmp_charp(b, "language")) {
                        Language += string(v.buf, v.len);
                    }
                }
                tfsxml_value(&p, &b);
                string Value = string(b.buf, b.len);
                if (!Value.empty() && !Language.empty()) {
                    Value.insert(0, '(' + Language + ')');
                }
                audioContent_Content.StringVectors[audioContent_audioContentLabel].push_back(Value);
            }
            if (!tfsxml_cmp_charp(b, "loudnessMetadata")) {
                tfsxml_enter(&p, &b);
                for (;;) {
                    if (tfsxml_next(&p, &b))
                        break;
                    if (!tfsxml_cmp_charp(b, "integratedLoudness")) {
                        tfsxml_value(&p, &b);
                        audioContent_Content.StringVectors[audioContent_integratedLoudness].push_back(string(b.buf, b.len));
                    }
                }
            }
            ELEMENT(audioContent, dialogue);
        ELEMENT_END()
        ELEMENT_START(audioObject)
            ATTRIBUTE(audioObject, audioObjectID);
            ATTRIBUTE(audioObject, audioObjectName);
            ATTRIBUTE(audioObject, duration);
            ATTRIBUTE(audioObject, startTime);
            ATTRIBUTE(audioObject, typeLabel);
        ELEMENT_MIDDLE()
            ELEMENT(audioObject, audioPackFormatIDRef);
            ELEMENT(audioObject, audioTrackUIDRef);
        ELEMENT_END()
        ELEMENT_START(audioPackFormat)
            ATTRIBUTE(audioPackFormat, audioPackFormatID);
            ATTRIBUTE(audioPackFormat, audioPackFormatName);
            ATTRIBUTE(audioPackFormat, typeDefinition);
            ATTRIBUTE(audioPackFormat, typeLabel);
        ELEMENT_MIDDLE()
            ELEMENT(audioPackFormat, audioChannelFormatIDRef);
        ELEMENT_END()
        ELEMENT_START(audioChannelFormat)
            ATTRIBUTE(audioChannelFormat, audioChannelFormatID);
            ATTRIBUTE(audioChannelFormat, audioChannelFormatName);
            ATTRIBUTE(audioChannelFormat, typeDefinition);
            ATTRIBUTE(audioChannelFormat, typeLabel);
        ELEMENT_MIDDLE()
            if (!tfsxml_cmp_charp(b, "audioBlockFormat")) {
                map<string, string>::iterator Extra_Type = audioChannelFormat_Content.Extra.find("Type");
                if (Extra_Type != audioChannelFormat_Content.Extra.end()) {
                    audioChannelFormat_Content.Extra.clear();
                    audioChannelFormat_Content.Extra["Type"] = "Dynamic";
                    tfsxml_leave(&p, &b); // audioBlockFormat
                    tfsxml_leave(&p, &b); // audioChannelFormat
                    break;
                }
                else {
                    audioChannelFormat_Content.Extra["Type"] = "Static";
                    tfsxml_enter(&p, &b);
                    for (;;) {
                        if (tfsxml_next(&p, &b))
                            break;
                        if (!tfsxml_cmp_charp(b, "speakerLabel")) {
                            tfsxml_value(&p, &b);
                            audioChannelFormat_Content.Extra["SpeakerLabel"] = string(b.buf, b.len);
                        }
                    }
                }
            }
        ELEMENT_END()
        ELEMENT_START(audioTrackUID)
            ATTRIBUTE(audioTrackUID, UID);
            ATTRIBUTE(audioTrackUID, bitDepth);
            ATTRIBUTE(audioTrackUID, sampleRate);
            ATTRIBUTE(audioTrackUID, typeLabel);
        ELEMENT_MIDDLE()
            ELEMENT(audioTrackUID, audioChannelFormatIDRef);
            ELEMENT(audioTrackUID, audioPackFormatIDRef);
            ELEMENT(audioTrackUID, audioTrackFormatIDRef);
        ELEMENT_END()
        ELEMENT_START(audioTrackFormat)
            ATTRIBUTE(audioTrackFormat, audioTrackFormatID);
            ATTRIBUTE(audioTrackFormat, audioTrackFormatName);
            ATTRIBUTE(audioTrackFormat, formatDefinition);
            ATTRIBUTE(audioTrackFormat, typeDefinition);
            ATTRIBUTE(audioTrackFormat, typeLabel);
        ELEMENT_MIDDLE()
            ELEMENT(audioTrackFormat, audioStreamFormatIDRef);
        ELEMENT_END()
        ELEMENT_START(audioStreamFormat)
            ATTRIBUTE(audioStreamFormat, audioStreamFormatID);
            ATTRIBUTE(audioStreamFormat, audioStreamFormatName);
            ATTRIBUTE(audioStreamFormat, formatDefinition);
            ATTRIBUTE(audioStreamFormat, formatLabel);
            ATTRIBUTE(audioStreamFormat, typeDefinition);
            ATTRIBUTE(audioStreamFormat, typeLabel);
        ELEMENT_MIDDLE()
            ELEMENT(audioStreamFormat, audioChannelFormatIDRef);
            ELEMENT(audioStreamFormat, audioPackFormatIDRef);
            ELEMENT(audioStreamFormat, audioTrackFormatIDRef);
        ELEMENT_END()
    }
}


//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Adm::File_Adm()
:File__Analyze()
{
    //Configuration
    Buffer_MaximumSize = 256 * 1024 * 1024;

    File_Adm_Private = new file_adm_private();
}

//---------------------------------------------------------------------------
File_Adm::~File_Adm()
{
    delete File_Adm_Private;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Adm::FileHeader_Begin()
{
    // File must be fully loaded
    if (!IsSub && Buffer_Size < File_Size)
    {
        Element_WaitForMoreData();
        return false; //Must wait for more data
    }

    if (tfsxml_init(&File_Adm_Private->p, (void*)(Buffer), Buffer_Size))
        return true;
    File_Adm_Private->parse();

    #define FILL_COUNT(NAME,FIELD) \
        if (!File_Adm_Private->Items[item_##NAME].Items.empty()) \
            Fill(Stream_Audio, 0, "NumberOf"FIELD"s", File_Adm_Private->Items[item_##NAME].Items.size());

    #define FILL_START(NAME,ATTRIBUTE,FIELD) \
        for (size_t i = 0; i < File_Adm_Private->Items[item_##NAME].Items.size(); i++) { \
            Ztring Summary = Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].Strings[NAME##_##ATTRIBUTE]); \
            string P = Apply_Init(*this, __T(FIELD), i, File_Adm_Private->Items[item_##NAME], Summary); \

    #define FILL_A(NAME,ATTRIBUTE,FIELD) \
        Fill(Stream_Audio, StreamPos_Last, (P + ' ' + FIELD).c_str(), File_Adm_Private->Items[item_##NAME].Items[i].Strings[NAME##_##ATTRIBUTE].c_str(), Unlimited, true); \

    #define FILL_E(NAME,ATTRIBUTE,FIELD) \
        for (size_t j = 0; j < File_Adm_Private->Items[item_##NAME].Items[i].StringVectors[NAME##_##ATTRIBUTE].size(); j++) { \
            Fill(Stream_Audio, StreamPos_Last, (P + ' ' + FIELD).c_str(), File_Adm_Private->Items[item_##NAME].Items[i].StringVectors[NAME##_##ATTRIBUTE][j].c_str(), Unlimited, true); \
        } \

    #define LINK(NAME,FIELD,VECTOR,TARGET) \
        Apply_SubStreams(*this, P + " LinkedTo_"FIELD"_Pos", File_Adm_Private->Items[item_##NAME].Items[i], NAME##_##VECTOR, File_Adm_Private->Items[item_##TARGET]); \

    //Filling
    Accept("ADM");
    Stream_Prepare(Stream_Audio);
    if (!IsSub)
        Fill(Stream_Audio, StreamPos_Last, Audio_Format, "ADM");
    FILL_COUNT(audioProgramme, "Programme");
    FILL_COUNT(audioContent, "Content");
    FILL_COUNT(audioObject, "Object");
    FILL_COUNT(audioPackFormat, "PackFormat");
    FILL_COUNT(audioChannelFormat, "ChannelFormat");
    FILL_COUNT(audioTrackUID, "TrackUID");
    FILL_COUNT(audioTrackFormat, "TrackFormat");
    FILL_COUNT(audioStreamFormat, "StreamFormat");
    size_t TotalCount = 0;
    for (size_t i = 0; i < item_Max; i++)
        TotalCount += File_Adm_Private->Items[i].Items.size();
    bool Full = TotalCount < 100 ? true : false;

    FILL_START(audioProgramme, audioProgrammeName, "Programme")
        if (Full)
            FILL_A(audioProgramme, audioProgrammeID, "ID");
        FILL_A(audioProgramme, audioProgrammeName, "Title");
        FILL_E(audioProgramme, audioProgrammeLabel, "Label");
        FILL_A(audioProgramme, audioProgrammeLanguage, "Language");
        FILL_A(audioProgramme, start, "Start");
        FILL_A(audioProgramme, end, "End");
        LINK(audioProgramme, "Content", audioContentIDRef, audioContent);
        const Ztring& Label = Retrieve_Const(StreamKind_Last, StreamPos_Last, (P + " Label").c_str());
        if (!Label.empty()) {
            Summary += __T(' ');
            Summary += __T('(');
            Summary += Label;
            Summary += __T(')');
            Fill(StreamKind_Last, StreamPos_Last, P.c_str(), Summary, true);
        }
    }

    FILL_START(audioContent, audioContentName, "Content")
        if (Full)
            FILL_A(audioContent, audioContentID, "ID");
        FILL_A(audioContent, audioContentName, "Title");
        FILL_E(audioContent, audioContentLabel, "Label");
        FILL_A(audioContent, audioContentLanguage, "Language");
        FILL_E(audioContent, dialogue, "Mode");
        FILL_E(audioContent, integratedLoudness, "IntegratedLoudness");
        LINK(audioContent, "Object", audioObjectIDRef, audioObject);
        const Ztring& Label = Retrieve_Const(StreamKind_Last, StreamPos_Last, (P + " Label").c_str());
        if (!Label.empty()) {
            Summary += __T(' ');
            Summary += __T('(');
            Summary += Label;
            Summary += __T(')');
            Fill(StreamKind_Last, StreamPos_Last, P.c_str(), Summary, true);
        }
}

    FILL_START(audioObject, audioObjectName, "Object")
        if (Full)
            FILL_A(audioObject, audioObjectID, "ID");
        FILL_A(audioObject, audioObjectName, "Title");
        FILL_A(audioObject, startTime, "Start");
        FILL_A(audioObject, duration, "Duration");
        LINK(audioObject, "PackFormat", audioPackFormatIDRef, audioPackFormat);
        if (Full)
            LINK(audioObject, "TrackUID", audioTrackUIDRef, audioTrackUID);
    }

    FILL_START(audioPackFormat, audioPackFormatName, "PackFormat")
        if (Full)
            FILL_A(audioPackFormat, audioPackFormatID, "ID");
        FILL_A(audioPackFormat, audioPackFormatName, "Title");
        FILL_A(audioPackFormat, typeDefinition, "TypeDefinition");
        LINK(audioPackFormat, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
    }

    FILL_START(audioChannelFormat, audioChannelFormatName, "ChannelFormat")
        if (Full)
            FILL_A(audioChannelFormat, audioChannelFormatID, "ID");
        FILL_A(audioChannelFormat, audioChannelFormatName, "Title");
        FILL_A(audioChannelFormat, typeDefinition, "TypeDefinition");
        for (map<string, string>::iterator Extra = File_Adm_Private->Items[item_audioChannelFormat].Items[i].Extra.begin(); Extra != File_Adm_Private->Items[item_audioChannelFormat].Items[i].Extra.end(); ++Extra) {
            Fill(Stream_Audio, StreamPos_Last, (P + ' ' + Extra->first).c_str(), Extra->second);
        }
    }

    if (Full) {
            FILL_START(audioTrackUID, UID, "TrackUID")
            FILL_A(audioTrackUID, UID, "ID");
            FILL_A(audioTrackUID, bitDepth, "BitDepth");
            FILL_A(audioTrackUID, sampleRate, "SamplingRate");
            LINK(audioTrackUID, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
            LINK(audioTrackUID, "PackFormat", audioPackFormatIDRef, audioPackFormat);
            LINK(audioTrackUID, "TrackFormat", audioTrackFormatIDRef, audioTrackFormat);
        }

        FILL_START(audioTrackFormat, audioTrackFormatName, "TrackFormat")
            FILL_A(audioTrackFormat, audioTrackFormatID, "ID");
            FILL_A(audioTrackFormat, audioTrackFormatName, "Title");
            FILL_A(audioTrackFormat, formatDefinition, "FormatDefinition");
            FILL_A(audioTrackFormat, typeDefinition, "TypeDefinition");
            LINK(audioTrackFormat, "StreamFormat", audioStreamFormatIDRef, audioStreamFormat);
        }

        FILL_START(audioStreamFormat, audioStreamFormatName, "StreamFormat")
            FILL_A(audioStreamFormat, audioStreamFormatID, "ID");
            FILL_A(audioStreamFormat, audioStreamFormatName, "Title");
            FILL_A(audioStreamFormat, formatDefinition, "Format");
            FILL_A(audioStreamFormat, typeDefinition, "TypeDefinition");
            LINK(audioStreamFormat, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
            LINK(audioStreamFormat, "PackFormat", audioPackFormatIDRef, audioPackFormat);
            LINK(audioStreamFormat, "TrackFormat", audioTrackFormatIDRef, audioTrackFormat);
        }
    }
    else
        Fill(Stream_Audio, 0, "PartialDisplay", "Yes");


    Element_Offset=File_Size;
    delete File_Adm_Private; File_Adm_Private = NULL;


    //All should be OK...
    Fill("ADM");
    return true;
}

} //NameSpace

#endif //MEDIAINFO_ADM_YES

