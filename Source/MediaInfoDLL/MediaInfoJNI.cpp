/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_JNI_YES)
//---------------------------------------------------------------------------

#ifdef __ANDROID__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <ZenLib/Ztring.h>

#include "ThirdParty/jni/jni.h"
#include "MediaInfo/MediaInfo_Internal.h"

#define BUFFER_SIZE 1024 * 1024

using namespace MediaInfoLib;

/**
 * Helpers
 */

//------------------------------------------------------------------------------
static Ztring Jstring2Ztring(JNIEnv* _env, jstring jstr)
{
    const char* str = _env->GetStringUTFChars(jstr, JNI_FALSE);

    Ztring toReturn = Ztring().From_UTF8(str);

    _env->ReleaseStringUTFChars(jstr, str);

    return toReturn;
}

//------------------------------------------------------------------------------
static MediaInfo_Internal* GetMiObj(JNIEnv* _env, jobject _this)
{
    jclass cls = _env->GetObjectClass(_this);
    if (cls == NULL)
        return NULL;

    jfieldID miId = _env->GetFieldID(cls, "mi", "J");
    if (miId == NULL)
        return NULL;

    MediaInfo_Internal* mi = (MediaInfo_Internal*)_env->GetLongField(_this, miId);
    if (mi == NULL)
        return NULL;

    return mi;
}

//------------------------------------------------------------------------------
static jlong JNI_Init(JNIEnv*, jobject)
{
    return (jlong)new MediaInfo_Internal();
}


//------------------------------------------------------------------------------
static jint JNI_Destroy(JNIEnv* _env, jobject _this)
{
    delete GetMiObj(_env, _this);

    return 0;
}

/**
 * Interface
 */

//------------------------------------------------------------------------------
static jint JNI_Open(JNIEnv* _env, jobject _this, jstring name)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return 1;

    return (jint)mi->Open(Jstring2Ztring(_env, name));
}

//------------------------------------------------------------------------------
#ifdef __ANDROID__
static jint JNI_OpenFd(JNIEnv* _env, jobject _this, jint fd, jstring name)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return 1;

    int64u size = (int64u)-1;

    struct stat64 s;
    if (fstat64(fd, &s) != -1 && s.st_size > 0)
        size = (int64u)s.st_size;

    mi->Option(__T("File_FileName"), Jstring2Ztring(_env, name));

    int8u* buffer = new int8u[BUFFER_SIZE];

    mi->Open_Buffer_Init(size, 0);
    while (1) {

        ssize_t count = read(fd, (void*)buffer, BUFFER_SIZE);

        if (count < 0)
            break; // error

        bitset<32> state = mi->Open_Buffer_Continue(buffer, (size_t)count);

        // bit 3 set means finalized
        if (state.test(3))
            break;

        // test if there is a MediaInfo request to go elsewhere
        int64u seekTo = mi->Open_Buffer_Continue_GoTo_Get();
        if (seekTo != (int64u)-1)
        {
            off64_t offset = lseek64(fd, (off64_t)seekTo, SEEK_SET);
            if (offset >= 0) {
                mi->Open_Buffer_Init(size, (int64u)offset); // inform MediaInfo we have seek
                continue;
            }
        }

        // EOF and no seekTo request
        if (count == 0)
            break;
    }
    mi->Open_Buffer_Finalize();

    delete[] buffer;
    close(fd);

    return 0;
}
#endif //__ANDROID__

//------------------------------------------------------------------------------
static jint JNI_Open_Buffer_Init(JNIEnv* _env, jobject _this, jlong fileSize, jlong  fileOffset)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return (jint)-1;

    return mi->Open_Buffer_Init((int64u)fileSize, (int64u)fileOffset);
}

//------------------------------------------------------------------------------
static jint JNI_Open_Buffer_Continue(JNIEnv* _env, jobject _this, jbyteArray buffer, jlong  bufferSize)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return (jint)-1;

    int8u*  buff = (int8u*)_env->GetByteArrayElements(buffer, JNI_FALSE);
    jint toReturn = (jint)mi->Open_Buffer_Continue(buff, (size_t)bufferSize).to_ulong();

    _env->ReleaseByteArrayElements(buffer, (jbyte*)buff, 0);

    return toReturn;
}

//------------------------------------------------------------------------------
static jlong JNI_Open_Buffer_Continue_GoTo_Get(JNIEnv* _env, jobject _this)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return (jint)-1;

    return (jlong)mi->Open_Buffer_Continue_GoTo_Get();
}

//------------------------------------------------------------------------------
static jlong JNI_Open_Buffer_Finalize(JNIEnv* _env, jobject _this)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return (jlong)-1;

    return (jlong)mi->Open_Buffer_Finalize();
}


//------------------------------------------------------------------------------
static jint JNI_Close(JNIEnv* _env, jobject _this)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return 1;

    mi->Close();

    return 0;
}

//------------------------------------------------------------------------------
static jstring JNI_Inform(JNIEnv* _env, jobject _this)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return _env->NewStringUTF("");

    return _env->NewStringUTF(Ztring(MediaInfo_Internal::Inform(mi)).To_UTF8().c_str());
}

//------------------------------------------------------------------------------
static jstring JNI_GetI(JNIEnv* _env, jobject _this, jint streamKind, jint streamNumber, jint parameter, jint infoKind)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return _env->NewStringUTF("");

    return _env->NewStringUTF(Ztring(mi->Get((stream_t) streamKind, (size_t)streamNumber, (size_t)parameter, (info_t)infoKind)).To_UTF8().c_str());
}

//------------------------------------------------------------------------------
static jstring JNI_GetS(JNIEnv* _env, jobject _this, jint streamKind, jint streamNumber, jstring parameter, jint infoKind, jint searchKind)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return _env->NewStringUTF("");

    return _env->NewStringUTF(Ztring(mi->Get((stream_t) streamKind, (size_t)streamNumber, Jstring2Ztring(_env, parameter), (info_t)infoKind, (info_t)searchKind)).To_UTF8().c_str());
}

//------------------------------------------------------------------------------
static jstring JNI_Option(JNIEnv* _env, jobject _this, jstring option, jstring value)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return _env->NewStringUTF("");

    return _env->NewStringUTF(Ztring(mi->Option(Jstring2Ztring(_env, option), Jstring2Ztring(_env, value))).To_UTF8().c_str());
}

//------------------------------------------------------------------------------
static jint JNI_State_Get(JNIEnv* _env, jobject _this)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return 0;

    return (jint)mi->State_Get();
}

//------------------------------------------------------------------------------
static jint JNI_Count_Get(JNIEnv* _env, jobject _this, jint streamKind, jint streamNumber)
{
    MediaInfo_Internal* mi = GetMiObj(_env, _this);
    if (mi == NULL)
        return 0;

    return (jint)mi->Count_Get((stream_t)streamKind, streamNumber==-1?(size_t)-1:(size_t)streamNumber);
}

/**
 * Initialize
 */

//------------------------------------------------------------------------------
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*)
{
    static JNINativeMethod methods[] = {
        {"Init", "()J", (void*)&JNI_Init},
        {"Destroy", "()I", (void*)&JNI_Destroy},
        {"Open", "(Ljava/lang/String;)I", (void*)&JNI_Open},
#ifdef __ANDROID__
        {"OpenFd", "(ILjava/lang/String;)I", (void*)&JNI_OpenFd},
#endif
        {"Open_Buffer_Init", "(JJ)I", (void*)&JNI_Open_Buffer_Init},
        {"Open_Buffer_Continue", "([BJ)I", (void*)&JNI_Open_Buffer_Continue},
        {"Open_Buffer_Continue_GoTo_Get", "()J", (void*)&JNI_Open_Buffer_Continue_GoTo_Get},
        {"Open_Buffer_Finalize", "()J", (void*)&JNI_Open_Buffer_Finalize},
        {"Close", "()I", (void*)&JNI_Close},
        {"Inform", "()Ljava/lang/String;", (void*)&JNI_Inform},
        {"GetI", "(IIII)Ljava/lang/String;", (void*)&JNI_GetI},
        {"GetS", "(IILjava/lang/String;II)Ljava/lang/String;", (void*)&JNI_GetS},
        {"Option", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void*)&JNI_Option},
        {"State_Get", "()I", (void*)&JNI_State_Get},
        {"Count_Get", "(II)I", (void*)&JNI_Count_Get}
    };

    JNIEnv* env = NULL;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
        return JNI_ERR;

    jclass cls = NULL;
#ifdef __ANDROID__
    cls = env->FindClass("net/mediaarea/mediainfo/MediaInfo");
#else
    cls = env->FindClass("MediaInfo");
#endif

    if (cls == NULL)
        return JNI_ERR;

    if ((env->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0]))) < 0)
        return JNI_ERR;

    return JNI_VERSION_1_4;
}

#endif //MEDIAINFO_JNI_YES
