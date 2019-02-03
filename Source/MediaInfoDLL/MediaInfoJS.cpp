/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include <MediaInfo/MediaInfo_Internal.h>
#include <MediaInfo/MediaInfo.h>
#include <emscripten/bind.h>
#include <string>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
class MediaInfoJs
{
private:
    MediaInfoLib::MediaInfo_Internal* Internal;
    MediaInfoJs(const MediaInfoJs&);
	MediaInfoJs& operator=(const MediaInfoJs&);  
public:
    // Constructor/Destructor
    MediaInfoJs()
    {
        Internal = new MediaInfoLib::MediaInfo_Internal;
    }

    ~MediaInfoJs()
    {
        delete Internal;
    }

    // Handling
    size_t Open_Buffer_Init(double File_Size, double File_Offset)
    {
        return Internal->Open_Buffer_Init(File_Size==-1?(ZenLib::int64u)-1:(ZenLib::int64u)File_Size, (ZenLib::int64u)File_Offset);
    }

    size_t Open_Buffer_Continue(const std::string& Buffer)
    {
        return Internal->Open_Buffer_Continue((ZenLib::int8u*)Buffer.data(), Buffer.size()).to_ulong();
    }

    double Open_Buffer_Continue_Goto_Get()
    {
        ZenLib::int64u ToReturn = Internal->Open_Buffer_Continue_GoTo_Get();
        if (ToReturn==(ZenLib::int64u)-1)
            return -1;

        return (double)ToReturn;
    }

    size_t Open_Buffer_Finalize()
    {
        return Internal->Open_Buffer_Finalize();
    }

    void Close()
    {
        Internal->Close();
    }

    // General information
    MediaInfoLib::String Inform()
    {
        return MediaInfoLib::MediaInfo_Internal::Inform(Internal);
    }

    MediaInfoLib::String GetI_3(MediaInfoLib::stream_t StreamKind, double StreamNumber, double Parameter)
    {
        return Internal->Get(StreamKind, (size_t)StreamNumber, (size_t)Parameter, MediaInfoLib::Info_Text);
    }

    MediaInfoLib::String GetI_4(MediaInfoLib::stream_t StreamKind, double StreamNumber, double Parameter, MediaInfoLib::info_t InfoKind)
    {
        return Internal->Get(StreamKind, (size_t)StreamNumber, (size_t)Parameter, InfoKind);
    }

    MediaInfoLib::String Get_3(MediaInfoLib::stream_t StreamKind, double StreamNumber, const MediaInfoLib::String &Parameter)
    {
        return Internal->Get(StreamKind, (size_t)StreamNumber, Parameter, MediaInfoLib::Info_Text, MediaInfoLib::Info_Name);
    }

    MediaInfoLib::String Get_4(MediaInfoLib::stream_t StreamKind, double StreamNumber, const MediaInfoLib::String &Parameter, MediaInfoLib::info_t InfoKind)
    {
        return Internal->Get(StreamKind, (size_t)StreamNumber, Parameter, InfoKind, MediaInfoLib::Info_Name);
    }

    MediaInfoLib::String Get_5(MediaInfoLib::stream_t StreamKind, double StreamNumber, const MediaInfoLib::String &Parameter, MediaInfoLib::info_t InfoKind, MediaInfoLib::info_t SearchKind)
    {
        return Internal->Get(StreamKind, (size_t)StreamNumber, Parameter, InfoKind, SearchKind);
    }

    // Options
    MediaInfoLib::String Option_1(const MediaInfoLib::String &Option)
    {
        return Internal->Option(Option, MediaInfoLib::String());
    }

    MediaInfoLib::String Option_2(const MediaInfoLib::String &Option, const MediaInfoLib::String &Value)
    {
        return Internal->Option(Option, Value);
    }

    static MediaInfoLib::String Option_Static_1(const MediaInfoLib::String &Option)
    {
        return MediaInfoLib::MediaInfo::Option_Static(Option,  MediaInfoLib::String());
    }

    static MediaInfoLib::String Option_Static_2(const MediaInfoLib::String &Option, const MediaInfoLib::String &Value)
    {
        return MediaInfoLib::MediaInfo::Option_Static(Option, Value);
    }

    double State_Get()
    {
        return Internal->State_Get();
    }

    double Count_Get_1(MediaInfoLib::stream_t StreamKind)
    {
        return Internal->Count_Get(StreamKind, (size_t)-1);
    }

    double Count_Get_2(MediaInfoLib::stream_t StreamKind, double StreamNumber)
    {
        return Internal->Count_Get(StreamKind, (size_t)StreamNumber);
    }
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
EMSCRIPTEN_BINDINGS(mediainfojs) {
    emscripten::class_<MediaInfoJs>("MediaInfo")
        .constructor()
        .function("Open_Buffer_Init", &MediaInfoJs::Open_Buffer_Init)
        .function("Open_Buffer_Continue", &MediaInfoJs::Open_Buffer_Continue)
        .function("Open_Buffer_Continue_Goto_Get", &MediaInfoJs::Open_Buffer_Continue_Goto_Get)
        .function("Open_Buffer_Finalize", &MediaInfoJs::Open_Buffer_Finalize)
        .function("Close", &MediaInfoJs::Close)
        .function("Inform", &MediaInfoJs::Inform)
        .function("GetI", &MediaInfoJs::GetI_3)
        .function("GetI", &MediaInfoJs::GetI_4)
        .function("Get", &MediaInfoJs::Get_3)
        .function("Get", &MediaInfoJs::Get_4)
        .function("Get", &MediaInfoJs::Get_5)
        .function("Option", &MediaInfoJs::Option_1)
        .function("Option", &MediaInfoJs::Option_2)
        .class_function("Option_Static", &MediaInfoJs::Option_Static_1)
        .class_function("Option_Static", &MediaInfoJs::Option_Static_2)
        .function("State_Get", &MediaInfoJs::State_Get)
        .function("Count_Get", &MediaInfoJs::Count_Get_1)
        .function("Count_Get", &MediaInfoJs::Count_Get_2)
    ;

    emscripten::enum_<MediaInfoLib::stream_t>("Stream")
        .value("General", MediaInfoLib::Stream_General)
        .value("Video", MediaInfoLib::Stream_Video)
        .value("Audio", MediaInfoLib::Stream_Audio)
        .value("Text", MediaInfoLib::Stream_Text)
        .value("Other", MediaInfoLib::Stream_Other)
        .value("Image", MediaInfoLib::Stream_Image)
        .value("Menu", MediaInfoLib::Stream_Menu)
        .value("Max", MediaInfoLib::Stream_Max)
    ;

    emscripten::enum_<MediaInfoLib::info_t>("Info")
        .value("Name", MediaInfoLib::Info_Name)
        .value("Text", MediaInfoLib::Info_Text)
        .value("Measure", MediaInfoLib::Info_Measure)
        .value("Options", MediaInfoLib::Info_Options)
        .value("Name_Text", MediaInfoLib::Info_Name_Text)
        .value("Measure_Text", MediaInfoLib::Info_Measure_Text)
        .value("Info", MediaInfoLib::Info_Info)
        .value("HowTo", MediaInfoLib::Info_HowTo)
        .value("Domain", MediaInfoLib::Info_Domain)
        .value("Max", MediaInfoLib::Info_Max)
    ;
}
//---------------------------------------------------------------------------
