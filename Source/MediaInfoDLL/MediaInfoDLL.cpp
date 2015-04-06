/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Public DLL interface implementation
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
// For user: you can disable or enable it
//#define MEDIAINFO_DEBUG
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Conf.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifdef UNICODE //DLL C Interface is currently done only in UNICODE mode
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//#if defined(MUST_INCLUDE_STDAFX) //Windows ATL for Shell Extension request stdafx.h
//    #include "stdafx.h"
//#endif //MUST_INCLUDE_STDAFX
#include "MediaInfo/MediaInfoList.h"
#include "MediaInfo/MediaInfo.h"
#define MEDIAINFO_DLL_EXPORT
#include "MediaInfoDLL_Static.h"
#include "ZenLib/Ztring.h"
#include "ZenLib/CriticalSection.h"
#include <map>
#include <vector>
#include <clocale>
using namespace MediaInfoLib;
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
struct mi_output
{
    std::string  Ansi;    //One-Byte-sized characters
    std::wstring Unicode; //Unicode characters
};
typedef std::map<void*, mi_output*> mi_outputs;

struct mi_input
{
    Ztring Unicode[8];  //Unicode characters multiple times
};
typedef std::map<void*, mi_input*> mi_inputs;

mi_outputs MI_Outputs;
mi_inputs MI_Inputs;

static CriticalSection Critical;
static bool utf8=false;

//---------------------------------------------------------------------------
const char* WC2MB(void* Handle, const wchar_t* Text)
{
    //Coherancy
    Critical.Enter();
    mi_outputs::iterator MI_Output=MI_Outputs.find(Handle);
    if (MI_Outputs.find(Handle)==MI_Outputs.end())
    {
        MI_Outputs[Handle]=new mi_output; //Generic Handle
        MI_Output=MI_Outputs.find(Handle);
    }
    Critical.Leave();

    //Adaptation
    if (utf8)
        MI_Output->second->Ansi=Ztring(Text).To_UTF8();
    else
        MI_Output->second->Ansi=Ztring(Text).To_Local();
    return  MI_Output->second->Ansi.c_str();
}

//---------------------------------------------------------------------------
const wchar_t* MB2WC(void* Handle, size_t Pos, const char* Text)
{
    //Coherancy
    Critical.Enter();
    mi_inputs::iterator MI_Input=MI_Inputs.find(Handle);
    if (MI_Input==MI_Inputs.end())
    {
        MI_Inputs[Handle]=new mi_input; //Generic Handle
        MI_Input=MI_Inputs.find(Handle);
    }
    Critical.Leave();

    //Adaptation
    if (utf8)
        return MI_Input->second->Unicode[Pos].From_UTF8(Text).c_str();
    else
        return MI_Input->second->Unicode[Pos].From_Local(Text).c_str();
}

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
// For Widgets with DEBUG in BCB
// Here only because this is useful in all MediaInfo
#ifdef _DEBUG
    void wxOnAssert(const wchar_t*, int, const char*, const wchar_t*, const wchar_t*) {}
#endif //_DEBUG

//---------------------------------------------------------------------------
// Debug
#ifdef MEDIAINFO_DEBUG
    #include <stdio.h>
    #include <windows.h>
    namespace MediaInfoDLL_Debug
    {
        FILE* F;
        std::string Debug;
        SYSTEMTIME st_In;

        void Debug_Open(bool Out)
        {
            F=fopen("C:\\Temp\\MediaInfo_Debug.txt", "a+t");
            Debug.clear();
            SYSTEMTIME st;
            GetLocalTime( &st );

            char Duration[100];
            if (Out)
            {
                FILETIME ft_In;
                if (SystemTimeToFileTime(&st_In, &ft_In))
                {
                    FILETIME ft_Out;
                    if (SystemTimeToFileTime(&st, &ft_Out))
                    {
                        ULARGE_INTEGER UI_In;
                        UI_In.HighPart=ft_In.dwHighDateTime;
                        UI_In.LowPart=ft_In.dwLowDateTime;

                        ULARGE_INTEGER UI_Out;
                        UI_Out.HighPart=ft_Out.dwHighDateTime;
                        UI_Out.LowPart=ft_Out.dwLowDateTime;

                        ULARGE_INTEGER UI_Diff;
                        UI_Diff.QuadPart=UI_Out.QuadPart-UI_In.QuadPart;

                        FILETIME ft_Diff;
                        ft_Diff.dwHighDateTime=UI_Diff.HighPart;
                        ft_Diff.dwLowDateTime=UI_Diff.LowPart;

                        SYSTEMTIME st_Diff;
                        if (FileTimeToSystemTime(&ft_Diff, &st_Diff))
                        {
                            sprintf(Duration, "%02hd:%02hd:%02hd.%03hd", st_Diff.wHour, st_Diff.wMinute, st_Diff.wSecond, st_Diff.wMilliseconds);
                        }
                        else
                            strcpy(Duration, "            ");
                    }
                    else
                        strcpy(Duration, "            ");

                }
                else
                    strcpy(Duration, "            ");
            }
            else
            {
                st_In=st;
                strcpy(Duration, "            ");
            }

            fprintf(F,"%02hd:%02hd:%02hd.%03hd %s", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, Duration);
        }

        void Debug_Close()
        {
            Debug += "\r\n";
            fwrite(Debug.c_str(), Debug.size(), 1, F); \
            fclose(F);
        }
    }
    using namespace MediaInfoDLL_Debug;

    #define MEDIAINFO_DEBUG1(_NAME,_TOAPPEND) \
        Debug_Open(false); \
        { \
        Ztring HandleTemp=Ztring::ToZtring((size_t)Handle, 16); \
        while (HandleTemp.size()<8) \
            HandleTemp.insert(0, 1, __T('0')); \
        Debug+=", H=";Debug+=HandleTemp.To_UTF8(); \
        } \
        Debug+=", ";Debug+=_NAME; \
        _TOAPPEND; \
        Debug_Close();

    #define MEDIAINFO_DEBUG2(_NAME,_TOAPPEND) \
        Debug_Open(true); \
        { \
        Ztring HandleTemp=Ztring::ToZtring((size_t)Handle, 16); \
        while (HandleTemp.size()<8) \
            HandleTemp.insert(0, 1, __T('0')); \
        Debug+=", H=";Debug+=HandleTemp.To_UTF8(); \
        } \
        Debug+=", ";Debug+=_NAME; \
        _TOAPPEND; \
        Debug_Close();
#else // MEDIAINFO_DEBUG
    #define MEDIAINFO_DEBUG1(_NAME,_TOAPPEND)
    #define MEDIAINFO_DEBUG2(_NAME,_TOAPPEND)
#endif // MEDIAINFO_DEBUG

//---------------------------------------------------------------------------
//To clarify the code
#define INTEGRITY_VOID(_NAME,_DEBUGA) \
    MEDIAINFO_DEBUG1(_NAME,_DEBUGA) \
    Critical.Enter(); \
    mi_outputs::iterator MI_Output=MI_Outputs.find(Handle); \
    bool MI_Output_IsOk=MI_Outputs.find(Handle)!=MI_Outputs.end(); \
    Critical.Leave(); \
    if (Handle==NULL || !MI_Output_IsOk) \
    { \
        MEDIAINFO_DEBUG2(_NAME,Debug+="Handle error") \
        return; \
    } \

#define INTEGRITY_SIZE_T(_NAME,_DEBUGA) \
    MEDIAINFO_DEBUG1(_NAME,_DEBUGA) \
    Critical.Enter(); \
    mi_outputs::iterator MI_Output=MI_Outputs.find(Handle); \
    bool MI_Output_IsOk=MI_Outputs.find(Handle)!=MI_Outputs.end(); \
    Critical.Leave(); \
    if (Handle==NULL || !MI_Output_IsOk) \
    { \
        MEDIAINFO_DEBUG2(_NAME, Debug+="Handle error") \
        return 0; \
    } \

#define INTEGRITY_INT64U(_NAME,_DEBUGA) \
    MEDIAINFO_DEBUG1(_NAME,_DEBUGA) \
    Critical.Enter(); \
    mi_outputs::iterator MI_Output=MI_Outputs.find(Handle); \
    bool MI_Output_IsOk=MI_Outputs.find(Handle)!=MI_Outputs.end(); \
    Critical.Leave(); \
    if (Handle==NULL || !MI_Output_IsOk) \
    { \
        MEDIAINFO_DEBUG2(_NAME, Debug+="Handle error") \
        return 0; \
    } \

#define INTEGRITY_STRING(_NAME,_DEBUGA) \
    MEDIAINFO_DEBUG1(_NAME,_DEBUGA) \
    Critical.Enter(); \
    mi_outputs::iterator MI_Output=MI_Outputs.find(Handle); \
    bool MI_Output_IsOk=MI_Outputs.find(Handle)!=MI_Outputs.end(); \
    Critical.Leave(); \
    if (Handle==NULL || !MI_Output_IsOk) \
    { \
        MEDIAINFO_DEBUG2(_NAME, Debug+="Handle error") \
        Critical.Enter(); \
        MI_Output=MI_Outputs.find(NULL); \
        if (MI_Output==MI_Outputs.end()) \
        { \
            MI_Outputs[NULL]=new mi_output; \
            MI_Output=MI_Outputs.find(NULL); \
        } \
        Critical.Leave(); \
        MI_Output->second->Unicode=L"Note to developer : you must create an object before"; \
        return MI_Output->second->Unicode.c_str(); \
    } \

#ifndef MEDIAINFO_DEBUG
#define EXECUTE_VOID(_NAME,_CLASS,_METHOD) \
    try \
    { \
        ((_CLASS*)Handle)->_METHOD; \
    } catch (...) {return;}
#else //MEDIAINFO_DEBUG
#define EXECUTE_VOID(_NAME,_CLASS,_METHOD) \
    try \
    { \
        ((_CLASS*)Handle)->_METHOD; \
        MEDIAINFO_DEBUG2(_NAME, "") \
    } catch (...) {return;}
#endif //MEDIAINFO_DEBUG

#ifndef MEDIAINFO_DEBUG
#define EXECUTE_SIZE_T(_NAME,_CLASS,_METHOD) \
    try \
    { \
        return ((_CLASS*)Handle)->_METHOD; \
    } catch (...) {return (size_t)-1;}
#else //MEDIAINFO_DEBUG
#define EXECUTE_SIZE_T(_NAME,_CLASS,_METHOD) \
    try \
    { \
        size_t ToReturn=((_CLASS*)Handle)->_METHOD; \
        MEDIAINFO_DEBUG2(_NAME, Debug+=", returns ";Debug+=Ztring::ToZtring((size_t)ToReturn).To_UTF8();) \
        return ToReturn; \
    } catch (...) {MEDIAINFO_DEBUG2(_NAME, Debug+="!!!Exception thrown!!!";) return (size_t)-1;}
#endif //MEDIAINFO_DEBUG

#ifndef MEDIAINFO_DEBUG
#define EXECUTE_INT64U(_NAME,_CLASS,_METHOD) \
    try \
    { \
        return ((_CLASS*)Handle)->_METHOD; \
    } catch (...) {return (size_t)-1;}
#else //MEDIAINFO_DEBUG
#define EXECUTE_INT64U(_NAME,_CLASS,_METHOD) \
    try \
    { \
        int64u ToReturn=((_CLASS*)Handle)->_METHOD; \
        MEDIAINFO_DEBUG2(_NAME, Debug+=", returns ";Debug+=Ztring::ToZtring((size_t)ToReturn).To_UTF8();) \
        return ToReturn; \
    } catch (...) {MEDIAINFO_DEBUG2(_NAME, Debug+="!!!Exception thrown!!!";) return (size_t)-1;}
#endif //MEDIAINFO_DEBUG

#ifndef MEDIAINFO_DEBUG
#define EXECUTE_STRING(_NAME,_CLASS,_METHOD) \
    try \
    { \
        MI_Output->second->Unicode=((_CLASS*)Handle)->_METHOD; \
    } catch (...) {MI_Output->second->Unicode.clear();} \
    return MI_Output->second->Unicode.c_str();
#else //MEDIAINFO_DEBUG
#define EXECUTE_STRING(_NAME,_CLASS,_METHOD) \
    try \
    { \
        MI_Output->second->Unicode=((_CLASS*)Handle)->_METHOD; \
    } catch (...) {MEDIAINFO_DEBUG2(_NAME, Debug+="!!!Exception thrown!!!";) MI_Output->second->Unicode.clear();} \
    Ztring ToReturn=MI_Output->second->Unicode; \
    MEDIAINFO_DEBUG2(_NAME, Debug+=", returns ";Debug+=ToReturn.To_UTF8();) \
    return MI_Output->second->Unicode.c_str();
#endif //MEDIAINFO_DEBUG

#define MANAGE_VOID(_NAME,_CLASS,_METHOD,_DEBUGA) \
    INTEGRITY_VOID(_NAME,_DEBUGA)       EXECUTE_VOID(_NAME,_CLASS,_METHOD)

#define MANAGE_SIZE_T(_NAME,_CLASS,_METHOD,_DEBUGA) \
    INTEGRITY_SIZE_T(_NAME,_DEBUGA)     EXECUTE_SIZE_T(_NAME,_CLASS,_METHOD)

#define MANAGE_INT64U(_NAME,_CLASS,_METHOD,_DEBUGA) \
    INTEGRITY_INT64U(_NAME,_DEBUGA)     EXECUTE_INT64U(_NAME,_CLASS,_METHOD)

#define MANAGE_STRING(_NAME,_CLASS,_METHOD,_DEBUGA) \
    INTEGRITY_STRING(_NAME,_DEBUGA)     EXECUTE_STRING(_NAME,_CLASS,_METHOD)

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------

void*           __stdcall MediaInfoA_New ()
{
    return MediaInfo_New();
}

void*           __stdcall MediaInfoA_New_Quick (const char* File, const char* Options)
{
    return MediaInfo_New_Quick(MB2WC(NULL, 0, File), MB2WC(NULL, 1, Options));
}

void            __stdcall MediaInfoA_Delete (void* Handle)
{
    MediaInfo_Delete(Handle);
}

size_t          __stdcall MediaInfoA_Open (void* Handle, const char* File)
{
    return MediaInfo_Open(Handle, MB2WC(Handle, 0, File));
}

size_t          __stdcall MediaInfoA_Open_Buffer (void* Handle, const unsigned char* Begin, size_t  Begin_Size, const unsigned char* End, size_t  End_Size)
{
    return MediaInfo_Open_Buffer(Handle, Begin, Begin_Size, End, End_Size);
}

size_t          __stdcall MediaInfoA_Open_Buffer_Init (void* Handle, MediaInfo_int64u File_Size, MediaInfo_int64u File_Offset)
{
    return MediaInfo_Open_Buffer_Init(Handle, File_Size, File_Offset);
}

size_t          __stdcall MediaInfoA_Open_Buffer_Continue (void* Handle, MediaInfo_int8u* Buffer, size_t Buffer_Size)
{
    return MediaInfo_Open_Buffer_Continue(Handle, Buffer, Buffer_Size);
}

MediaInfo_int64u __stdcall MediaInfoA_Open_Buffer_Continue_GoTo_Get (void* Handle)
{
    return MediaInfo_Open_Buffer_Continue_GoTo_Get(Handle);
}

size_t          __stdcall MediaInfoA_Open_Buffer_Finalize (void* Handle)
{
    return MediaInfo_Open_Buffer_Finalize(Handle);
}

size_t          __stdcall MediaInfoA_Open_NextPacket (void* Handle)
{
    return MediaInfo_Open_NextPacket(Handle);
}

size_t          __stdcall MediaInfoA_Save (void* Handle)
{
    return MediaInfo_Save(Handle);
}

void            __stdcall MediaInfoA_Close (void* Handle)
{
    MediaInfo_Close(Handle);
}

const char*     __stdcall MediaInfoA_Inform (void* Handle, size_t Reserved)
{
        return WC2MB(Handle, MediaInfo_Inform(Handle, 0));
}

const char*     __stdcall MediaInfoA_GetI (void* Handle, MediaInfo_stream_t StreamKind, size_t StreamNumber, size_t  Parameter, MediaInfo_info_C KindOfInfo)
{
    return WC2MB(Handle, MediaInfo_GetI(Handle, StreamKind, StreamNumber, Parameter, KindOfInfo));
}

const char*     __stdcall MediaInfoA_Get (void* Handle, MediaInfo_stream_t StreamKind, size_t StreamNumber, const char* Parameter, MediaInfo_info_C KindOfInfo, MediaInfo_info_C KindOfSearch)
{
    return WC2MB(Handle, MediaInfo_Get(Handle, StreamKind, StreamNumber, MB2WC(Handle, 0, Parameter), KindOfInfo, KindOfSearch));
}

size_t          __stdcall MediaInfoA_SetI (void* Handle, const char* ToSet, MediaInfo_stream_t StreamKind, size_t StreamNumber, size_t  Parameter, const char* OldParameter)
{
    return MediaInfo_SetI(Handle, MB2WC(Handle, 0, ToSet), StreamKind, StreamNumber, Parameter, MB2WC(Handle, 1, OldParameter));
}

size_t          __stdcall MediaInfoA_Set (void* Handle, const char* ToSet, MediaInfo_stream_t StreamKind, size_t StreamNumber, const char* Parameter, const char* OldParameter)
{
    return MediaInfo_Set(Handle, MB2WC(Handle, 0, ToSet), StreamKind, StreamNumber, MB2WC(Handle, 1, Parameter), MB2WC(Handle, 2, OldParameter));
}

size_t          __stdcall MediaInfoA_Output_Buffer_Get (void* Handle, const char* Value)
{
    return MediaInfo_Output_Buffer_Get(Handle, MB2WC(Handle, 0, Value));
}

size_t          __stdcall MediaInfoA_Output_Buffer_GetI (void* Handle, size_t Pos)
{
    return MediaInfo_Output_Buffer_GetI(Handle, Pos);
}

const char*     __stdcall MediaInfoA_Option (void* Handle, const char* Option, const char* Value)
{
    return WC2MB(Handle, MediaInfo_Option(Handle, MB2WC(Handle, 0, Option), MB2WC(Handle, 1, Value)));
}

size_t          __stdcall MediaInfoA_State_Get(void* Handle)
{
    return MediaInfo_State_Get(Handle);
}

size_t          __stdcall MediaInfoA_Count_Get(void* Handle, MediaInfo_stream_t StreamKind, size_t StreamNumber)
{
    return MediaInfo_Count_Get(Handle, StreamKind, StreamNumber);
}

//---------------------------------------------------------------------------

void*           __stdcall MediaInfoListA_New ()
{
    return MediaInfoList_New();
}

void*           __stdcall MediaInfoListA_New_Quick (const char* File, const char* Options)
{
    return MediaInfoList_New_Quick(MB2WC(NULL, 0, File), MB2WC(NULL, 1, Options));
}

void            __stdcall MediaInfoListA_Delete (void* Handle)
{
    MediaInfoList_Delete(Handle);
}

size_t          __stdcall MediaInfoListA_Open (void* Handle, const char* File, const MediaInfo_fileoptions_C Options)
{
    return MediaInfoList_Open(Handle, MB2WC(Handle, 0, File), Options);
}

size_t          __stdcall MediaInfoListA_Open_Buffer (void* Handle, const unsigned char* Begin, size_t  Begin_Size, const unsigned char* End, size_t  End_Size)
{
    return MediaInfoList_Open_Buffer(Handle, Begin, Begin_Size, End, End_Size);
}

size_t          __stdcall MediaInfoListA_Save (void* Handle, size_t  FilePos)
{
    return MediaInfoList_Save(Handle, FilePos);
}

void            __stdcall MediaInfoListA_Close (void* Handle, size_t  FilePos)
{
    MediaInfoList_Close(Handle, FilePos);
}

const char*     __stdcall MediaInfoListA_Inform (void* Handle, size_t  FilePos, size_t Reserved)
{
    return WC2MB(Handle, MediaInfoList_Inform(Handle, FilePos, 0));
}

const char*     __stdcall MediaInfoListA_GetI (void* Handle, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber, size_t  Parameter, MediaInfo_info_C KindOfInfo)
{
    return WC2MB(Handle, MediaInfoList_GetI(Handle, FilePos, StreamKind, StreamNumber, Parameter, KindOfInfo));
}

const char*     __stdcall MediaInfoListA_Get (void* Handle, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber, const char* Parameter, MediaInfo_info_C KindOfInfo, MediaInfo_info_C KindOfSearch)
{
    return WC2MB(Handle, MediaInfoList_Get(Handle, FilePos, StreamKind, StreamNumber, MB2WC(Handle, 1, Parameter), KindOfInfo, KindOfSearch));
}

size_t          __stdcall MediaInfoListA_SetI (void* Handle, const char* ToSet, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber, size_t  Parameter, const char* OldParameter)
{
    return MediaInfoList_SetI(Handle, MB2WC(Handle, 0, ToSet), FilePos, StreamKind, StreamNumber, Parameter, MB2WC(Handle, 1, OldParameter));
}

size_t          __stdcall MediaInfoListA_Set (void* Handle, const char* ToSet, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber, const char* Parameter, const char* OldParameter)
{
    return MediaInfoList_Set(Handle, MB2WC(Handle, 0, ToSet), FilePos, StreamKind, StreamNumber, MB2WC(Handle, 1, Parameter), MB2WC(Handle, 2, OldParameter));
}

const char*     __stdcall MediaInfoListA_Option (void* Handle, const char* Option, const char* Value)
{
    return WC2MB(Handle, MediaInfoList_Option(Handle, MB2WC(Handle, 0, Option), MB2WC(Handle, 1, Value)));
}

size_t          __stdcall MediaInfoListA_State_Get(void* Handle)
{
    return MediaInfoList_State_Get(Handle);
}

size_t          __stdcall MediaInfoListA_Count_Get(void* Handle, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber)
{
    return MediaInfoList_Count_Get(Handle, FilePos, StreamKind, StreamNumber);
}

size_t          __stdcall MediaInfoListA_Count_Get_Files(void* Handle)
{
    return MediaInfoList_Count_Get_Files(Handle);
}

//---------------------------------------------------------------------------

void*           __stdcall MediaInfo_New ()
{
    #ifdef MEDIAINFO_DEBUG
        Debug_Open(false);
        Debug+=",             New, Build="; Debug+=__DATE__; Debug+=' '; Debug+=__TIME__;
        Debug_Close();
    #endif //MEDIAINFO_DEBUG

    //First init
    Critical.Enter();
    if (MI_Outputs.find(NULL)==MI_Outputs.end())
    {
        MI_Outputs[NULL]=new mi_output; //Generic Handle
    }
    Critical.Leave();

    //New
    MediaInfo* Handle=NULL;
    try
    {
        Handle=new MediaInfo;
    }
    catch(...)
    {
        MEDIAINFO_DEBUG2(   "New",
                            Debug+="!!!Exception thrown!!!";)

        delete Handle;
        return NULL;
    }

    Critical.Enter();
    MI_Outputs[Handle]=new mi_output;
    Critical.Leave();

    MEDIAINFO_DEBUG2(   "New",
                        Debug+=", returns ";Debug+=Ztring::ToZtring((size_t)Handle).To_UTF8();)

    return Handle;
}

void*           __stdcall MediaInfo_New_Quick (const wchar_t* File, const wchar_t* Options)
{
    MediaInfo_Option(NULL, L"QuickInit", Options);
    void* Handle=MediaInfo_New();
    if (MediaInfo_Open(Handle, File)==0)
    {
        //No valid files, return NULL
        delete (MediaInfo*)Handle;
        return NULL;
    }
    return Handle;
}

void            __stdcall MediaInfo_Delete (void* Handle)
{
    INTEGRITY_VOID(     "Delete"
                        ,)

    //Delete the object
    delete (MediaInfo*)Handle;

    //Delete strings
    Critical.Enter();
    delete MI_Outputs[Handle];
    MI_Outputs.erase(Handle);
    if (MI_Outputs.size()==1 && MI_Outputs.find(NULL)!=MI_Outputs.end()) //In case of the last object : delete the NULL object, no more need
    {
        delete MI_Outputs[NULL];
        MI_Outputs.erase(NULL);
    }
    Critical.Leave();

    MEDIAINFO_DEBUG2(   "Delete",
                        )
}

size_t          __stdcall MediaInfo_Open (void* Handle, const wchar_t* File)
{
    MANAGE_SIZE_T(  "Open",
                    MediaInfo,
                    Open(File),
                    Debug+=", File=";Debug+=Ztring(File).To_UTF8();)
}

size_t          __stdcall MediaInfo_Open_Buffer (void* Handle, const unsigned char* Begin, size_t  Begin_Size, const unsigned char* End, size_t  End_Size)
{
    MANAGE_SIZE_T(  "Open_Buffer",
                    MediaInfo,
                    Open(Begin, Begin_Size, End, End_Size),
                    Debug+=", Begin_Size=";Debug+=Ztring::ToZtring(Begin_Size).To_UTF8();Debug+=", End_Size=";Debug+=Ztring::ToZtring(End_Size).To_UTF8();)
}

size_t          __stdcall MediaInfo_Open_Buffer_Init (void* Handle, MediaInfo_int64u File_Size, MediaInfo_int64u File_Offset)
{
    MANAGE_SIZE_T(  "Open_Buffer_Init",
                    MediaInfo,
                    Open_Buffer_Init(File_Size, File_Offset),
                    Debug+=", File_Size=";Debug+=Ztring::ToZtring(File_Size).To_UTF8();Debug+=", File_Offset=";Debug+=Ztring::ToZtring(File_Offset).To_UTF8();)
}

size_t          __stdcall MediaInfo_Open_Buffer_Continue (void* Handle, MediaInfo_int8u* Buffer, size_t Buffer_Size)
{
    MANAGE_SIZE_T(  "Open_Buffer_Continue",
                    MediaInfo,
                    Open_Buffer_Continue(Buffer, Buffer_Size),
                    Debug+=", Buffer_Size=";Debug+=Ztring::ToZtring(Buffer_Size).To_UTF8();)
}

MediaInfo_int64u __stdcall MediaInfo_Open_Buffer_Continue_GoTo_Get (void* Handle)
{
    MANAGE_INT64U(  "Open_Buffer_Continue_GoTo_Get",
                    MediaInfo,
                    Open_Buffer_Continue_GoTo_Get(),
                    )
}

size_t          __stdcall MediaInfo_Open_Buffer_Finalize (void* Handle)
{
    MANAGE_SIZE_T(  "Open_Buffer_Finalize",
                    MediaInfo,
                    Open_Buffer_Finalize(),
                    )
}

size_t          __stdcall MediaInfo_Open_NextPacket (void* Handle)
{
    MANAGE_SIZE_T(  "Open_NextPacket",
                    MediaInfo,
                    Open_NextPacket(),
                    )
}

size_t          __stdcall MediaInfo_Save (void* Handle)
{
    MANAGE_SIZE_T(  "Save",
                    MediaInfo,
                    Save(),
                    )
}

void            __stdcall MediaInfo_Close (void* Handle)
{
    MANAGE_VOID(    "Close",
                    MediaInfo,
                    Close(),
                    )
}

const wchar_t*  __stdcall MediaInfo_Inform (void* Handle, size_t Reserved)
{
    MANAGE_STRING(  "Inform",
                    MediaInfo,
                    Inform(),
                    )
}

const wchar_t*  __stdcall MediaInfo_GetI (void* Handle, MediaInfo_stream_t StreamKind, size_t StreamNumber, size_t  Parameter, MediaInfo_info_C KindOfInfo)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    KindOfInfo=(MediaInfo_info_C)  (((size_t)KindOfInfo)&0xFF);
    MANAGE_STRING(  "GetI",
                    MediaInfo,
                    Get((stream_t)StreamKind, StreamNumber, Parameter, (info_t)KindOfInfo),
                    Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring(StreamNumber).To_UTF8();Debug+=", Parameter=";Debug+=Ztring::ToZtring(Parameter).To_UTF8();Debug+=", KindOfInfo=";Debug+=Ztring::ToZtring(KindOfInfo).To_UTF8();)
}

const wchar_t*  __stdcall MediaInfo_Get (void* Handle, MediaInfo_stream_t StreamKind, size_t StreamNumber, const wchar_t* Parameter, MediaInfo_info_C KindOfInfo, MediaInfo_info_C KindOfSearch)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    KindOfInfo=(MediaInfo_info_C)  (((size_t)KindOfInfo)&0xFF);
    KindOfSearch=(MediaInfo_info_C)(((size_t)KindOfSearch)&0xFF);
    MANAGE_STRING(  "Get",
                    MediaInfo,
                    Get((stream_t)StreamKind, StreamNumber, Parameter, (info_t)KindOfInfo, (info_t)KindOfSearch),
                    Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring(StreamNumber).To_UTF8();Debug+=", Parameter=";Debug+=Ztring(Parameter).To_UTF8();Debug+=", KindOfInfo=";Debug+=Ztring::ToZtring(KindOfInfo).To_UTF8();Debug+=", KindOfSearch=";Debug+=Ztring::ToZtring(KindOfSearch).To_UTF8();)
}

size_t          __stdcall MediaInfo_SetI (void* Handle, const wchar_t* ToSet, MediaInfo_stream_t StreamKind, size_t StreamNumber, size_t  Parameter, const wchar_t* OldParameter)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    MANAGE_SIZE_T(  "SetI",
                    MediaInfo,
                    Set(ToSet, (stream_t)StreamKind, StreamNumber, Parameter, OldParameter),
                    Debug+=", ToSet=";Debug+=Ztring(ToSet).To_UTF8();Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring(StreamNumber).To_UTF8();Debug+=", Parameter=";Debug+=Ztring::ToZtring(Parameter).To_UTF8();)
}

size_t          __stdcall MediaInfo_Set (void* Handle, const wchar_t* ToSet, MediaInfo_stream_t StreamKind, size_t StreamNumber, const wchar_t* Parameter, const wchar_t* OldParameter)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    MANAGE_SIZE_T(  "Set",
                    MediaInfo,
                    Set(ToSet, (stream_t)StreamKind, StreamNumber, Parameter, OldParameter),
                    Debug+=", ToSet=";Debug+=Ztring(ToSet).To_UTF8();Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring(StreamNumber).To_UTF8();Debug+=", Parameter=";Debug+=Ztring(Parameter).To_UTF8();)
}

size_t          __stdcall MediaInfo_Output_Buffer_Get (void* Handle, const wchar_t* Value)
{
    MANAGE_SIZE_T(  "Output_Buffer_Get",
                    MediaInfo,
                    Output_Buffer_Get(Value),
                    Debug+=", Value=";Debug+=Ztring(Value).To_UTF8();)
}

size_t          __stdcall MediaInfo_Output_Buffer_GetI (void* Handle, size_t Pos)
{
    MANAGE_SIZE_T(  "Output_Buffer_GetI",
                    MediaInfo,
                    Output_Buffer_Get(Pos),
                    Debug+=", Pos=";Debug+=Ztring::ToZtring(Pos).To_UTF8();)
}

const wchar_t*     __stdcall MediaInfo_Option (void* Handle, const wchar_t* Option, const wchar_t* Value)
{
    //DLL only option
    if (Ztring(Option).Compare(L"CharSet", L"=="))
    {
        MEDIAINFO_DEBUG1(   "Option",
                            Debug+=", Option=";Debug+=Ztring(Option).To_UTF8();Debug+=", Value=";Debug+=Ztring(Value).To_UTF8();)

        //Coherancy
        Critical.Enter();
        mi_outputs::iterator MI_Output=MI_Outputs.find(NULL);
        if (MI_Outputs.find(NULL)==MI_Outputs.end())
        {
            MI_Outputs[NULL]=new mi_output; //Generic Handle
            MI_Output=MI_Outputs.find(NULL);
        }
        Critical.Leave();

        if (Ztring(Value).Compare(L"UTF-8", L"=="))
            utf8=true;
        else
            utf8=false;
        MI_Output->second->Unicode.clear();

        MEDIAINFO_DEBUG2(   "CharSet",
                            )

       return MI_Output->second->Unicode.c_str();
    }
    if (Ztring(Option).Compare(L"setlocale_LC_CTYPE", L"=="))
    {
        //Coherancy
        Critical.Enter();
        mi_outputs::iterator MI_Output=MI_Outputs.find(NULL);
        if (MI_Outputs.find(NULL)==MI_Outputs.end())
        {
            MI_Outputs[NULL]=new mi_output; //Generic Handle
            MI_Output=MI_Outputs.find(NULL);
        }
        Critical.Leave();

        setlocale(LC_CTYPE, utf8?Ztring(Value).To_UTF8().c_str():Ztring(Value).To_Local().c_str());
        MI_Output->second->Unicode.clear();

        MEDIAINFO_DEBUG2(   "setlocale_LC_CTYPE",
                            )

        return MI_Output->second->Unicode.c_str();
    }

    if (Handle)
    {
        MANAGE_STRING(  "Option",
                        MediaInfo,
                        Option(Option, Value),
                        )
    }
    else
    {
        //MANAGE_STRING
        Critical.Enter();
        mi_outputs::iterator MI_Output=MI_Outputs.find(NULL);
        if (MI_Output==MI_Outputs.end())
        {
            MI_Outputs[NULL]=new mi_output;
            MI_Output=MI_Outputs.find(NULL);
        }
        Critical.Leave();

        EXECUTE_STRING( "Option_Static",
                        MediaInfo,
                        Option_Static(Option, Value));
    }
}

size_t          __stdcall MediaInfo_State_Get(void* Handle)
{
    MANAGE_SIZE_T(  "State_Get",
                    MediaInfo,
                    State_Get(),
                    )
}

size_t          __stdcall MediaInfo_Count_Get(void* Handle, MediaInfo_stream_t StreamKind, size_t StreamNumber)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    MANAGE_SIZE_T(  "Count_Get",
                    MediaInfo,
                    Count_Get((stream_t)StreamKind, StreamNumber),
                    Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring((size_t)StreamNumber).To_UTF8();)
}

//---------------------------------------------------------------------------

void*           __stdcall MediaInfoList_New ()
{
    #ifdef MEDIAINFO_DEBUG
        Debug_Open(false);
        Debug+=",             New, Build="; Debug+=__DATE__; Debug+=' '; Debug+=__TIME__;
        Debug_Close();
    #endif //MEDIAINFO_DEBUG

    //First init
    Critical.Enter();
    if (MI_Outputs.find(NULL)==MI_Outputs.end())
    {
        MI_Outputs[NULL]=new mi_output; //Generic Handle
    }
    Critical.Leave();

    //New
    MediaInfoList* Handle=NULL;
    try
    {
        Handle=new MediaInfoList;
    }
    catch(...)
    {
        MEDIAINFO_DEBUG2(   "New",
                            Debug+="!!!Exception thrown!!!";)

        delete Handle;
        return NULL;
    }

    Critical.Enter();
    MI_Outputs[Handle]=new mi_output;
    Critical.Leave();

    MEDIAINFO_DEBUG2(   "New",
                        Debug+=", returns ";Debug+=Ztring::ToZtring((size_t)Handle).To_UTF8();)

    return Handle;
}

void*           __stdcall MediaInfoList_New_Quick (const wchar_t* File, const wchar_t* Options)
{
    MediaInfoList_Option(NULL, L"QuickInit", Options);
    void* Handle=MediaInfoList_New();
    if (MediaInfoList_Open(Handle, File, MediaInfo_FileOption_Nothing)==0)
    {
        //No valid files, return NULL
        delete (MediaInfoList*)Handle;
        return NULL;
    }
    return Handle;
}

void            __stdcall MediaInfoList_Delete (void* Handle)
{
    INTEGRITY_VOID(     "Delete"
                        ,)

    //Delete the object
    delete (MediaInfoList*)Handle;

    //Delete strings
    Critical.Enter();
    delete MI_Outputs[Handle];
    MI_Outputs.erase(Handle);
    if (MI_Outputs.size()==1 && MI_Outputs.find(NULL)!=MI_Outputs.end()) //In case of the last object : delete the NULL object, no more need
    {
        delete MI_Outputs[NULL];
        MI_Outputs.erase(NULL);
    }
    Critical.Leave();

    MEDIAINFO_DEBUG2(   "Delete",
                        )
}

size_t          __stdcall MediaInfoList_Open (void* Handle, const wchar_t* File, const MediaInfo_fileoptions_C Options)
{
    MANAGE_SIZE_T(  "Open",
                    MediaInfoList,
                    Open(File),
                    Debug+=", File=";Debug+=Ztring(File).To_UTF8();)
}

size_t          __stdcall MediaInfoList_Open_Buffer (void* Handle, const unsigned char* Begin, size_t  Begin_Size, const unsigned char* End, size_t  End_Size)
{
    return 0; // Not implemented
}

size_t          __stdcall MediaInfoList_Save (void* Handle, size_t  FilePos)
{
    MANAGE_SIZE_T(  "Save",
                    MediaInfoList,
                    Save(FilePos),
                    )
}

void            __stdcall MediaInfoList_Close (void* Handle, size_t  FilePos)
{
    MANAGE_VOID(    "Close",
                    MediaInfoList,
                    Close(FilePos),
                    )
}

const wchar_t*  __stdcall MediaInfoList_Inform (void* Handle, size_t  FilePos, size_t Reserved)
{
    MANAGE_STRING(  "Inform",
                    MediaInfoList,
                    Inform(FilePos),
                    )
}

const wchar_t*  __stdcall MediaInfoList_GetI (void* Handle, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber, size_t  Parameter, MediaInfo_info_C KindOfInfo)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    KindOfInfo=(MediaInfo_info_C)  (((size_t)KindOfInfo)&0xFF);
    MANAGE_STRING(  "GetI",
                    MediaInfoList,
                    Get(FilePos, (stream_t)StreamKind, StreamNumber, Parameter, (info_t)KindOfInfo),
                    Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring(StreamNumber).To_UTF8();Debug+=", Parameter=";Debug+=Ztring::ToZtring(Parameter).To_UTF8();Debug+=", KindOfInfo=";Debug+=Ztring::ToZtring(KindOfInfo).To_UTF8();)
}

const wchar_t*  __stdcall MediaInfoList_Get (void* Handle, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber, const wchar_t* Parameter, MediaInfo_info_C KindOfInfo, MediaInfo_info_C KindOfSearch)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    KindOfInfo=(MediaInfo_info_C)  (((size_t)KindOfInfo)&0xFF);
    MANAGE_STRING(  "Get",
                    MediaInfoList,
                    Get(FilePos, (stream_t)StreamKind, StreamNumber, Parameter, (info_t)KindOfInfo, (info_t)KindOfSearch),
                    Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring(StreamNumber).To_UTF8();Debug+=", Parameter=";Debug+=Ztring(Parameter).To_UTF8();Debug+=", KindOfInfo=";Debug+=Ztring::ToZtring(KindOfInfo).To_UTF8();Debug+=", KindOfSearch=";Debug+=Ztring::ToZtring(KindOfSearch).To_UTF8();)
}

size_t          __stdcall MediaInfoList_SetI (void* Handle, const wchar_t* ToSet, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber, size_t  Parameter, const wchar_t* OldParameter)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    MANAGE_SIZE_T(  "SetI",
                    MediaInfoList,
                    Set(ToSet, FilePos, (stream_t)StreamKind, StreamNumber, Parameter),
                    Debug+=", ToSet=";Debug+=Ztring(ToSet).To_UTF8();Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring(StreamNumber).To_UTF8();Debug+=", Parameter=";Debug+=Ztring::ToZtring(Parameter).To_UTF8();)
}

size_t          __stdcall MediaInfoList_Set (void* Handle, const wchar_t* ToSet, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber, const wchar_t* Parameter, const wchar_t* OldParameter)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    MANAGE_SIZE_T(  "Set",
                    MediaInfoList,
                    Set(ToSet, FilePos, (stream_t)StreamKind, StreamNumber, Parameter, OldParameter),
                    Debug+=", ToSet=";Debug+=Ztring(ToSet).To_UTF8();Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring(StreamNumber).To_UTF8();Debug+=", Parameter=";Debug+=Ztring(Parameter).To_UTF8();)
}

const wchar_t*     __stdcall MediaInfoList_Option (void* Handle, const wchar_t* Option, const wchar_t* Value)
{
    //DLL only option
    if (Ztring(Option).Compare(L"CharSet", L"=="))
    {
        MEDIAINFO_DEBUG1(   "Option",
                            Debug+=", Option=";Debug+=Ztring(Option).To_UTF8();Debug+=", Value=";Debug+=Ztring(Value).To_UTF8();)

        //Coherancy
        Critical.Enter();
        mi_outputs::iterator MI_Output=MI_Outputs.find(NULL);
        if (MI_Outputs.find(NULL)==MI_Outputs.end())
        {
            MI_Outputs[NULL]=new mi_output; //Generic Handle
            MI_Output=MI_Outputs.find(NULL);
        }
        Critical.Leave();

        if (Ztring(Value).Compare(L"UTF-8", L"=="))
            utf8=true;
        else
            utf8=false;
        MI_Output->second->Unicode.clear();

        MEDIAINFO_DEBUG2(   "CharSet",
                            )

       return MI_Output->second->Unicode.c_str();
    }
    if (Ztring(Option).Compare(L"setlocale_LC_CTYPE", L"=="))
    {
        //Coherancy
        Critical.Enter();
        mi_outputs::iterator MI_Output=MI_Outputs.find(NULL);
        if (MI_Outputs.find(NULL)==MI_Outputs.end())
        {
            MI_Outputs[NULL]=new mi_output; //Generic Handle
            MI_Output=MI_Outputs.find(NULL);
        }
        Critical.Leave();

        setlocale(LC_CTYPE, utf8?Ztring(Value).To_UTF8().c_str():Ztring(Value).To_Local().c_str());
        MI_Output->second->Unicode.clear();

        MEDIAINFO_DEBUG2(   "setlocale_LC_CTYPE",
                            )

        return MI_Output->second->Unicode.c_str();
    }

    if (Handle)
    {
        MANAGE_STRING(  "Option",
                        MediaInfoList,
                        Option(Option, Value),
                        )
    }
    else
    {
        //MANAGE_STRING
        Critical.Enter();
        mi_outputs::iterator MI_Output=MI_Outputs.find(NULL);
        if (MI_Output==MI_Outputs.end())
        {
            MI_Outputs[NULL]=new mi_output;
            MI_Output=MI_Outputs.find(NULL);
        }
        Critical.Leave();

        EXECUTE_STRING( "Option_Static",
                        MediaInfoList,
                        Option_Static(Option, Value));
    }
}

size_t          __stdcall MediaInfoList_State_Get(void* Handle)
{
    MANAGE_SIZE_T(  "State_Get",
                    MediaInfoList,
                    State_Get(),
                    )
}

size_t          __stdcall MediaInfoList_Count_Get(void* Handle, size_t  FilePos, MediaInfo_stream_t StreamKind, size_t StreamNumber)
{
    StreamKind=(MediaInfo_stream_t)(((size_t)StreamKind)&0xFF);
    MANAGE_SIZE_T(  "Count_Get",
                    MediaInfoList,
                    Count_Get(FilePos, (stream_t)StreamKind, StreamNumber),
                    Debug+=", StreamKind=";Debug+=Ztring::ToZtring((size_t)StreamKind).To_UTF8();Debug+=", StreamNumber=";Debug+=Ztring::ToZtring((size_t)StreamNumber).To_UTF8();)
}

size_t          __stdcall MediaInfoList_Count_Get_Files(void* Handle)
{
    MANAGE_SIZE_T(  "Count_Get",
                    MediaInfoList,
                    Count_Get(),
                    )
}

//---------------------------------------------------------------------------

const char*     __stdcall MediaInfo_Info_Version()
{
    #ifdef MEDIAINFO_DEBUG
        Debug_Open(false);
        Debug+=",             MediaInfo_Info_Version";
        Debug_Close();
    #endif //MEDIAINFO_DEBUG

    #ifdef MEDIAINFO_DEBUG
        Debug_Open(true);
        Debug+=",             MediaInfo_Info_Version";
        Debug_Close();
    #endif //MEDIAINFO_DEBUG

    return "Your software uses an outdated interface, You must use MediaInfo.DLL 0.4.1.1 instead";
    //wchar_t* MediaInfo_wChar=new wchar_t[1000];
    //GetModuleFileNameW (NULL, MediaInfo_wChar, 1000);
    //return WC2MB(MediaInfo_wChar);
}

//***************************************************************************
//
//***************************************************************************

#endif //UNICODE
