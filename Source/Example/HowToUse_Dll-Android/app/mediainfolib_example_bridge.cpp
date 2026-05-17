#include <jni.h>
#include <string>
#include <MediaInfoDLL/MediaInfoDLL_Static.h> // Header supplied automatically by Prefab!

// This can be your custom native logic to do whatever processing before passing the result to your Kotlin side

extern "C"
JNIEXPORT jstring JNICALL
Java_net_mediaarea_mediainfo_example_Bridge_Version(JNIEnv *env, jobject thiz) {
    MediaInfoDLL::MediaInfo MI;
    std::string MediaInfo_Output = MI.Option(__T("Info_Version"));
    return env->NewStringUTF(MediaInfo_Output.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_net_mediaarea_mediainfo_example_Bridge_Info(JNIEnv *env, jobject thiz) {
    MediaInfoDLL::MediaInfo MI;
    std::string MediaInfo_Output;
    MediaInfo_Output += __T("\n\nInfo_Parameters\n");
    MediaInfo_Output += __T("====================\n");
    MediaInfo_Output += MI.Option(__T("Info_Parameters"));
    MediaInfo_Output += __T("\n\nInfo_Codecs\n");
    MediaInfo_Output += __T("====================\n");
    MediaInfo_Output += MI.Option(__T("Info_Codecs"));
    return env->NewStringUTF(MediaInfo_Output.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_net_mediaarea_mediainfo_example_Bridge_Inform(JNIEnv *env, jobject thiz, jstring path) {
    const char *nativePath = env->GetStringUTFChars(path, nullptr);
    MediaInfoDLL::MediaInfo MI;
    MI.Option(__T("Language"), __T("  Config_Text_ColumnSize;20"));
    MI.Open(nativePath);
    std::string MediaInfo_Output = __T("\n\nInform\n");
    MediaInfo_Output += __T("====================\n");
    MediaInfo_Output += MI.Inform();
    return env->NewStringUTF(MediaInfo_Output.c_str());
}
