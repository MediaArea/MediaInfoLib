## Copyright (c) MediaArea.net SARL. All Rights Reserved.
 #
 # Use of this source code is governed by a BSD-style license that can
 # be found in the License.html file in the root of the source tree.
 ##

# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
#  Public DLL interface implementation
#  Wrapper for MediaInfo Library
#  Please see MediaInfo.h for help
#
# Converted to python module by Petr Kaderabek
# Modifications by Jerome Martinez
# Python 3 update by Jerome Martinez
# Mac OSX support, Python 2/3 merge and ctypes fixes by Miguel Grinberg
#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
# MediaInfoDLL.py and MediaInfoDLL3.py are same
# but all files are kept in order to not break programs calling them.
#
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

import os
import sys
import locale
from ctypes import *
if os.name == "nt" or os.name == "dos" or os.name == "os2" or os.name == "ce":
    MediaInfoDLL_Handler = windll.MediaInfo
    MustUseAnsi = 0
elif sys.platform == "darwin":
    MediaInfoDLL_Handler = CDLL("libmediainfo.0.dylib")
    MustUseAnsi = 1
else:
    MediaInfoDLL_Handler = CDLL("libmediainfo.so.0")
    MustUseAnsi = 1


# types --> C Python:
# size_t            c_size_t
# unsigned char*    c_char_p
# enum              c_size_t
# const wchar_t*    c_wchar_p,
# NULL              None,
# these functions need strings in unicode format




class Stream:
    General, Video, Audio, Text, Other, Image, Menu, Max = list(range(8))

class Info:
    Name, Text, Measure, Options, Name_Text, Measure_Text, Info, HowTo, Max = list(range(9))

class InfoOption:
    ShowInInform, Reserved, ShowInSupported, TypeOfValue, Max = list(range(5))

class FileOptions:
    Nothing, Recursive, CloseAll, xxNonexx_3, Max = list(range(5))



class MediaInfo:

    #MEDIAINFO_EXP void*         __stdcall MediaInfo_New (); /*you must ALWAYS call MediaInfo_Delete(Handle) in order to free memory*/
    #/** @brief A 'new' MediaInfo interface (with a quick init of useful options : "**VERSION**;**APP_NAME**;**APP_VERSION**", but without debug information, use it only if you know what you do), return a Handle, don't forget to delete it after using it*/
    MediaInfo_New = MediaInfoDLL_Handler.MediaInfo_New
    MediaInfo_New.argtypes = []
    MediaInfo_New.restype  = c_void_p

    #MEDIAINFO_EXP void*         __stdcall MediaInfo_New_Quick (const wchar_t* File, const wchar_t* Options); /*you must ALWAYS call MediaInfo_Delete(Handle) in order to free memory*/
    MediaInfo_New_Quick = MediaInfoDLL_Handler.MediaInfo_New_Quick
    MediaInfo_New_Quick.argtypes = [c_wchar_p, c_wchar_p]
    MediaInfo_New_Quick.restype  = c_void_p
    MediaInfoA_New_Quick = MediaInfoDLL_Handler.MediaInfoA_New_Quick
    MediaInfoA_New_Quick.argtypes = [c_char_p, c_char_p]
    MediaInfoA_New_Quick.restype  = c_void_p

    #/** @brief Delete a MediaInfo interface*/
    #MEDIAINFO_EXP void       __stdcall MediaInfo_Delete (void* Handle);
    MediaInfo_Delete = MediaInfoDLL_Handler.MediaInfo_Delete
    MediaInfo_Delete.argtypes = [c_void_p]
    MediaInfo_Delete.restype  = None

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a filename)*/
    #MEDIAINFO_EXP size_t       __stdcall MediaInfo_Open (void* Handle, const wchar_t* File);
    MediaInfo_Open = MediaInfoDLL_Handler.MediaInfo_Open
    MediaInfo_Open.argtypes = [c_void_p, c_wchar_p]
    MediaInfo_Open.restype = c_size_t
    MediaInfoA_Open = MediaInfoDLL_Handler.MediaInfoA_Open
    MediaInfoA_Open.argtypes = [c_void_p, c_char_p]
    MediaInfoA_Open.restype = c_size_t

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Open (with a buffer) */
    #MEDIAINFO_EXP size_t       __stdcall MediaInfo_Open_Buffer (void* Handle, const unsigned char* Begin, size_t Begin_Size, const unsigned char* End, size_t End_Size); /*return Handle*/
    MediaInfo_Open_Buffer = MediaInfoDLL_Handler.MediaInfo_Open_Buffer
    MediaInfo_Open_Buffer.argtypes = [c_void_p, c_void_p, c_size_t, c_void_p, c_size_t]
    MediaInfo_Open_Buffer.restype = c_size_t

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Save */
    #MEDIAINFO_EXP size_t       __stdcall MediaInfo_Save (void* Handle);
    MediaInfo_Save = MediaInfoDLL_Handler.MediaInfo_Save
    MediaInfo_Save.argtypes = [c_void_p]
    MediaInfo_Save.restype = c_size_t

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Close */
    #MEDIAINFO_EXP void       __stdcall MediaInfo_Close (void* Handle);
    MediaInfo_Close = MediaInfoDLL_Handler.MediaInfo_Close
    MediaInfo_Close.argtypes = [c_void_p]
    MediaInfo_Close.restype = None

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Inform */
    #MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfo_Inform (void* Handle, size_t Reserved); /*Default : Reserved=0*/
    MediaInfo_Inform = MediaInfoDLL_Handler.MediaInfo_Inform
    MediaInfo_Inform.argtypes = [c_void_p, c_size_t]
    MediaInfo_Inform.restype = c_wchar_p
    MediaInfoA_Inform = MediaInfoDLL_Handler.MediaInfoA_Inform
    MediaInfoA_Inform.argtypes = [c_void_p, c_size_t]
    MediaInfoA_Inform.restype = c_char_p

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Get */
    #MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfo_GetI (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, MediaInfo_info_C InfoKind); /*Default : InfoKind=Info_Text*/
    MediaInfo_GetI = MediaInfoDLL_Handler.MediaInfo_GetI
    MediaInfo_GetI.argtypes = [c_void_p, c_size_t, c_size_t, c_size_t, c_size_t]
    MediaInfo_GetI.restype = c_wchar_p
    MediaInfoA_GetI = MediaInfoDLL_Handler.MediaInfoA_GetI
    MediaInfoA_GetI.argtypes = [c_void_p, c_size_t, c_size_t, c_size_t, c_size_t]
    MediaInfoA_GetI.restype = c_char_p

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Get */
    #MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfo_Get (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, MediaInfo_info_C InfoKind, MediaInfo_info_C SearchKind); /*Default : InfoKind=Info_Text, SearchKind=Info_Name*/
    MediaInfo_Get = MediaInfoDLL_Handler.MediaInfo_Get
    MediaInfo_Get.argtypes = [c_void_p, c_size_t, c_size_t, c_wchar_p, c_size_t, c_size_t]
    MediaInfo_Get.restype = c_wchar_p
    MediaInfoA_Get = MediaInfoDLL_Handler.MediaInfoA_Get
    MediaInfoA_Get.argtypes = [c_void_p, c_size_t, c_size_t, c_wchar_p, c_size_t, c_size_t]
    MediaInfoA_Get.restype = c_char_p

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Set */
    #MEDIAINFO_EXP size_t       __stdcall MediaInfo_SetI (void* Handle, const wchar_t* ToSet, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, const wchar_t* OldParameter);
    MediaInfo_SetI = MediaInfoDLL_Handler.MediaInfo_SetI
    MediaInfo_SetI.argtypes = [c_void_p, c_wchar_p, c_size_t, c_size_t, c_size_t, c_wchar_p]
    MediaInfo_SetI.restype = c_void_p
    MediaInfoA_SetI = MediaInfoDLL_Handler.MediaInfoA_SetI
    MediaInfoA_SetI.argtypes = [c_void_p, c_char_p, c_size_t, c_size_t, c_size_t, c_wchar_p]
    MediaInfoA_SetI.restype = c_void_p

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Set */
    #MEDIAINFO_EXP size_t       __stdcall MediaInfo_Set (void* Handle, const wchar_t* ToSet, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, const wchar_t* OldParameter);
    MediaInfo_Set = MediaInfoDLL_Handler.MediaInfo_Set
    MediaInfo_Set.argtypes = [c_void_p, c_wchar_p, c_size_t, c_size_t, c_wchar_p, c_wchar_p]
    MediaInfo_Set.restype = c_size_t
    MediaInfoA_Set = MediaInfoDLL_Handler.MediaInfoA_Set
    MediaInfoA_Set.argtypes = [c_void_p, c_char_p, c_size_t, c_size_t, c_wchar_p, c_wchar_p]
    MediaInfoA_Set.restype = c_size_t

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Option */
    #MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfo_Option (void* Handle, const wchar_t* Option, const wchar_t* Value);
    MediaInfo_Option = MediaInfoDLL_Handler.MediaInfo_Option
    MediaInfo_Option.argtypes = [c_void_p, c_wchar_p, c_wchar_p]
    MediaInfo_Option.restype = c_wchar_p
    MediaInfoA_Option = MediaInfoDLL_Handler.MediaInfoA_Option
    MediaInfoA_Option.argtypes = [c_void_p, c_char_p, c_char_p]
    MediaInfoA_Option.restype = c_char_p

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::State_Get */
    #MEDIAINFO_EXP size_t       __stdcall MediaInfo_State_Get (void* Handle);
    MediaInfo_State_Get = MediaInfoDLL_Handler.MediaInfo_State_Get
    MediaInfo_State_Get.argtypes = [c_void_p]
    MediaInfo_State_Get.restype = c_size_t

    #/** @brief Wrapper for MediaInfoLib::MediaInfo::Count_Get */
    #MEDIAINFO_EXP size_t       __stdcall MediaInfo_Count_Get (void* Handle, MediaInfo_stream_C StreamKind, size_t StreamNumber); /*Default : StreamNumber=-1*/
    MediaInfo_Count_Get = MediaInfoDLL_Handler.MediaInfo_Count_Get
    MediaInfo_Count_Get.argtypes = [c_void_p, c_size_t, c_size_t]
    MediaInfo_Count_Get.restype = c_size_t

    Handle = c_void_p(0)

    #Handling
    def __init__(self):
        self.Handle=self.MediaInfo_New()
        self.MediaInfo_Option(self.Handle, "CharSet", "UTF-8")
        if sys.version_info.major < 3 and os.name != "nt" and os.name != "dos" and os.name != "os2" and os.name != "ce" and locale.getlocale() == (None, None):
            locale.setlocale(locale.LC_CTYPE, locale.getdefaultlocale())
    def __del__(self):
        self.MediaInfo_Delete(self.Handle)
    def Open(self, File):
        if MustUseAnsi:
            return self.MediaInfoA_Open (self.Handle, File.encode("utf-8"));
        else:
            return self.MediaInfo_Open (self.Handle, File);
    def Open_Buffer(self, Begin, Begin_Size, End=None, End_Size=0):
        return self.MediaInfo_Open_Buffer(self.Handle, Begin, Begin_Size, End, End_Size)
    def Save(self):
        return self.MediaInfo_Save(self.Handle)
    def Close(self):
        return self.MediaInfo_Close(self.Handle)

    #General information
    def Inform(self):
        if MustUseAnsi:
            return self.MediaInfoA_Inform(self.Handle, 0).decode("utf_8", 'ignore')
        else:
            return self.MediaInfo_Inform(self.Handle, 0)
    def Get(self, StreamKind, StreamNumber, Parameter, InfoKind=Info.Text, SearchKind=Info.Name):
        if MustUseAnsi:
            return self.MediaInfoA_Get(self.Handle, StreamKind, StreamNumber, Parameter.encode("utf-8"), InfoKind, SearchKind).decode("utf_8", 'ignore')
        else:
            return self.MediaInfo_Get(self.Handle, StreamKind, StreamNumber, Parameter, InfoKind, SearchKind)
    def GetI(self, StreamKind, StreamNumber, Parameter, InfoKind=Info.Text):
        if MustUseAnsi:
            return self.MediaInfoA_GetI(self.Handle, StreamKind, StreamNumber, Parameter, InfoKind).decode("utf_8", 'ignore')
        else:
            return self.MediaInfo_GetI(self.Handle, StreamKind, StreamNumber, Parameter, InfoKind)
    def Set(self, ToSet, StreamKind, StreamNumber, Parameter, OldParameter=""):
        if MustUseAnsi:
            return self.MediaInfoA_Set(self.Handle, ToSet, StreamKind, StreamNumber, Parameter.encode("utf-8"), OldParameter.encode("utf-8"))
        else:
            return self.MediaInfo_Set(self.Handle, ToSet, StreamKind, StreamNumber, Parameter, OldParameter)
    def SetI(self, ToSet, StreamKind, StreamNumber, Parameter, OldValue):
        if MustUseAnsi:
            return self.MediaInfoA_SetI(self.Handle, ToSet, StreamKind, StreamNumber, Parameter, OldValue.encode("utf-8"))
        else:
            return self.MediaInfo_SetI(self.Handle, ToSet, StreamKind, StreamNumber, Parameter, OldValue)

    #Options
    def Option(self, Option, Value=""):
        if MustUseAnsi:
            return self.MediaInfoA_Option(self.Handle, Option.encode("utf-8"), Value.encode("utf-8")).decode("utf_8", 'ignore')
        else:
            return self.MediaInfo_Option(self.Handle, Option, Value)
    def Option_Static(self, Option, Value=""):
        if MustUseAnsi:
            return self.MediaInfoA_Option(None, Option.encode("utf-8"), Value.encode("utf-8")).decode("utf_8", 'ignore')
        else:
            return self.MediaInfo_Option(None, Option, Value)
    def State_Get(self):
        return self.MediaInfo_State_Get(self.Handle)
    def Count_Get(self, StreamKind, StreamNumber=-1):
        return self.MediaInfo_Count_Get(self.Handle, StreamKind, StreamNumber)


class MediaInfoList:
    #/** @brief A 'new' MediaInfoList interface, return a Handle, don't forget to delete it after using it*/
    #MEDIAINFO_EXP void*             __stdcall MediaInfoList_New (); /*you must ALWAYS call MediaInfoList_Delete(Handle) in order to free memory*/
    MediaInfoList_New = MediaInfoDLL_Handler.MediaInfoList_New
    MediaInfoList_New.argtypes = []
    MediaInfoList_New.restype = c_void_p

    #/** @brief A 'new' MediaInfoList interface (with a quick init of useful options : "**VERSION**;**APP_NAME**;**APP_VERSION**", but without debug information, use it only if you know what you do), return a Handle, don't forget to delete it after using it*/
    #MEDIAINFO_EXP void*             __stdcall MediaInfoList_New_Quick (const wchar_t* Files, const wchar_t* Config); /*you must ALWAYS call MediaInfoList_Delete(Handle) in order to free memory*/
    MediaInfoList_New_Quick = MediaInfoDLL_Handler.MediaInfoList_New_Quick
    MediaInfoList_New_Quick.argtypes = [c_wchar_p, c_wchar_p]
    MediaInfoList_New_Quick.restype = c_void_p

    #/** @brief Delete a MediaInfoList interface*/
    #MEDIAINFO_EXP void           __stdcall MediaInfoList_Delete (void* Handle);
    MediaInfoList_Delete = MediaInfoDLL_Handler.MediaInfoList_Delete
    MediaInfoList_Delete.argtypes = [c_void_p]

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Open (with a filename)*/
    #MEDIAINFO_EXP size_t           __stdcall MediaInfoList_Open (void* Handle, const wchar_t* Files, const MediaInfo_fileoptions_C Options); /*Default : Options=MediaInfo_FileOption_Nothing*/
    MediaInfoList_Open = MediaInfoDLL_Handler.MediaInfoList_Open
    MediaInfoList_Open.argtypes = [c_void_p, c_wchar_p, c_void_p]
    MediaInfoList_Open.restype = c_void_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Open (with a buffer) */
    #MEDIAINFO_EXP size_t           __stdcall MediaInfoList_Open_Buffer (void* Handle, const unsigned char* Begin, size_t Begin_Size, const unsigned char* End, size_t End_Size); /*return Handle*/
    MediaInfoList_Open_Buffer = MediaInfoDLL_Handler.MediaInfoList_Open_Buffer
    MediaInfoList_Open_Buffer.argtypes = [c_void_p, c_void_p, c_void_p, c_void_p, c_void_p]
    MediaInfoList_Open_Buffer.restype = c_void_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Save */
    #MEDIAINFO_EXP size_t           __stdcall MediaInfoList_Save (void* Handle, size_t FilePos);
    MediaInfoList_Save = MediaInfoDLL_Handler.MediaInfoList_Save
    MediaInfoList_Save.argtypes = [c_void_p, c_void_p]
    MediaInfoList_Save.restype = c_void_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Close */
    #MEDIAINFO_EXP void           __stdcall MediaInfoList_Close (void* Handle, size_t FilePos);
    MediaInfoList_Close = MediaInfoDLL_Handler.MediaInfoList_Close
    MediaInfoList_Close.argtypes = [c_void_p, c_void_p]

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Inform */
    #MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfoList_Inform (void* Handle, size_t FilePos, size_t Reserved); /*Default : Reserved=0*/
    MediaInfoList_Inform = MediaInfoDLL_Handler.MediaInfoList_Inform
    MediaInfoList_Inform.argtypes = [c_void_p, c_void_p, c_void_p]
    MediaInfoList_Inform.restype = c_wchar_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Get */
    #MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfoList_GetI (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, MediaInfo_info_C InfoKind); /*Default : InfoKind=Info_Text*/
    MediaInfoList_GetI = MediaInfoDLL_Handler.MediaInfoList_GetI
    MediaInfoList_GetI.argtypes = [c_void_p, c_void_p, c_void_p, c_void_p, c_void_p, c_void_p]
    MediaInfoList_GetI.restype = c_wchar_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Get */
    #MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfoList_Get (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, MediaInfo_info_C InfoKind, MediaInfo_info_C SearchKind); /*Default : InfoKind=Info_Text, SearchKind=Info_Name*/
    MediaInfoList_Get = MediaInfoDLL_Handler.MediaInfoList_Get
    MediaInfoList_Get.argtypes = [c_void_p, c_void_p, c_void_p, c_void_p, c_wchar_p, c_void_p, c_void_p]
    MediaInfoList_Get.restype = c_wchar_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Set */
    #MEDIAINFO_EXP size_t           __stdcall MediaInfoList_SetI (void* Handle, const wchar_t* ToSet, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, size_t Parameter, const wchar_t* OldParameter);
    MediaInfoList_SetI = MediaInfoDLL_Handler.MediaInfoList_SetI
    MediaInfoList_SetI.argtypes = [c_void_p, c_wchar_p, c_void_p, c_void_p, c_void_p, c_void_p, c_wchar_p]
    MediaInfoList_SetI.restype = c_void_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Set */
    #MEDIAINFO_EXP size_t           __stdcall MediaInfoList_Set (void* Handle, const wchar_t* ToSet, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber, const wchar_t* Parameter, const wchar_t* OldParameter);
    MediaInfoList_Set = MediaInfoDLL_Handler.MediaInfoList_Set
    MediaInfoList_Set.argtypes = [c_void_p, c_wchar_p, c_void_p, c_void_p, c_void_p, c_wchar_p, c_wchar_p]
    MediaInfoList_Set.restype = c_void_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Option */
    #MEDIAINFO_EXP const wchar_t*    __stdcall MediaInfoList_Option (void* Handle, const wchar_t* Option, const wchar_t* Value);
    MediaInfoList_Option = MediaInfoDLL_Handler.MediaInfoList_Option
    MediaInfoList_Option.argtypes = [c_void_p, c_wchar_p, c_wchar_p]
    MediaInfoList_Option.restype = c_wchar_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::State_Get */
    #MEDIAINFO_EXP size_t           __stdcall MediaInfoList_State_Get (void* Handle);
    MediaInfoList_State_Get = MediaInfoDLL_Handler.MediaInfoList_State_Get
    MediaInfoList_State_Get.argtypes = [c_void_p]
    MediaInfoList_State_Get.restype = c_void_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Count_Get */
    #MEDIAINFO_EXP size_t           __stdcall MediaInfoList_Count_Get (void* Handle, size_t FilePos, MediaInfo_stream_C StreamKind, size_t StreamNumber); /*Default : StreamNumber=-1*/
    MediaInfoList_Count_Get = MediaInfoDLL_Handler.MediaInfoList_Count_Get
    MediaInfoList_Count_Get.argtypes = [c_void_p, c_void_p, c_void_p, c_void_p]
    MediaInfoList_Count_Get.restype = c_void_p

    #/** @brief Wrapper for MediaInfoListLib::MediaInfoList::Count_Get */
    #MEDIAINFO_EXP size_t           __stdcall MediaInfoList_Count_Get_Files (void* Handle);
    MediaInfoList_Count_Get_Files = MediaInfoDLL_Handler.MediaInfoList_Count_Get_Files
    MediaInfoList_Count_Get_Files.argtypes = [c_void_p]
    MediaInfoList_Count_Get_Files.restype = c_void_p

    Handle = c_void_p(0)

    #Handling
    def __init__(self):
        self.Handle=MediaInfoList_New()
        self.MediaInfo_Option(self.Handle, "CharSet", "UTF-8")
        if sys.version_info.major < 3 and os.name != "nt" and os.name != "dos" and os.name != "os2" and os.name != "ce" and sys.platform != "darwin" and locale.getlocale() == (None, None):
            locale.setlocale(locale.LC_CTYPE, locale.getdefaultlocale())
    def __del__(self):
        MediaInfoList_Delete(self.Handle)
    def Open(self, Files, Options=FileOptions.Nothing):
        return MediaInfoList_Open(self.Handle, Files, Options)
    def Open_Buffer(self, Begin, Begin_Size, End=None, End_Size=0):
        return MediaInfoList_Open_Buffer (self.Handle, Begin, Begin_Size, End, End_Size)
    def Save(self, FilePos):
        return MediaInfoList_Save(self.Handle, FilePos)
    def Close(self, FilePos):
        MediaInfoList_Close (self.Handle, FilePos)

    #General information
    def Inform(self, FilePos, Reserved=0):
        return MediaInfoList_Inform (self.Handle, FilePos, Reserved)
    def GetI(self, FilePos, StreamKind, StreamNumber, Parameter, InfoKind=Info.Text):
        return MediaInfoList_GetI (self.Handle, FilePos, StreamKind, StreamNumber, Parameter, InfoKind)
    def Get(self, FilePos, StreamKind, StreamNumber, Parameter, InfoKind=Info.Text, SearchKind=Info.Name):
        return MediaInfoList_Get (self.Handle, FilePos, StreamKind, StreamNumber, (Parameter), InfoKind, SearchKind)
    def SetI(self, ToSet, FilePos, StreamKind, StreamNumber, Parameter, OldParameter=""):
        return MediaInfoList_SetI (self, Handle, ToSet, FilePos, StreamKind, StreamNumber, Parameter, OldParameter)
    def Set(self, ToSet, FilePos, StreamKind, StreamNumber, Parameter, OldParameter=""):
        return MediaInfoList_Set (self.Handle, ToSet, FilePos, StreamKind, StreamNumber, Parameter, OldParameter)

    #Options
    def Option(self, Option, Value=""):
        return MediaInfoList_Option (self.Handle, Option, Value)
    def Option_Static(self, Option, Value=""):
        return MediaInfoList_Option(None, Option, Value)
    def State_Get(self):
        return MediaInfoList_State_Get (self.Handle)
    def Count_Get(self, FilePos, StreamKind, StreamNumber):
        return MediaInfoList_Count_Get (self.Handle, FilePos, StreamKind, StreamNumber=-1)
    def Count_Get_Files(self):
        return MediaInfoList_Count_Get_Files (self.Handle)
