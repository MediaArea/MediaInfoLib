/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

// File names with non-ASCII characters with the C API (narrow and wide functions, C++ classes without Unicode) and the locale of the environment, for filename.sh
// Usage: mil_filename setlocale(0|1) charset_utf8(0|1) directory
// Prints "NOK: check, message" lines or "OK: count checks", exit code 0 (OK) or 1 (NOK)

#include <clocale>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
    #include <direct.h>
    #include <windows.h>
#else
    #include <langinfo.h>
#endif

#if defined(UNICODE)

#if !defined(_WIN32)
    #define __stdcall
#endif

// C API, narrow and wide functions (MediaInfoDLL_Static.h declares only one of them, depending on UNICODE)
extern "C"
{
    void*           __stdcall MediaInfoA_New();
    void*           __stdcall MediaInfoA_New_Quick(const char* File, const char* Options);
    void            __stdcall MediaInfoA_Delete(void* Handle);
    size_t          __stdcall MediaInfoA_Open(void* Handle, const char* File);
    const char*     __stdcall MediaInfoA_Inform(void* Handle, size_t Reserved);
    const char*     __stdcall MediaInfoA_Get(void* Handle, int StreamKind, size_t StreamNumber, const char* Parameter, int KindOfInfo, int KindOfSearch);
    const char*     __stdcall MediaInfoA_Option(void* Handle, const char* Option, const char* Value);
    void*           __stdcall MediaInfoListA_New();
    void*           __stdcall MediaInfoListA_New_Quick(const char* File, const char* Options);
    void            __stdcall MediaInfoListA_Delete(void* Handle);
    size_t          __stdcall MediaInfoListA_Open(void* Handle, const char* File, int Options);
    const char*     __stdcall MediaInfoListA_Get(void* Handle, size_t FilePos, int StreamKind, size_t StreamNumber, const char* Parameter, int KindOfInfo, int KindOfSearch);
    size_t          __stdcall MediaInfoListA_Count_Get_Files(void* Handle);
    void*           __stdcall MediaInfo_New();
    void            __stdcall MediaInfo_Delete(void* Handle);
    size_t          __stdcall MediaInfo_Open(void* Handle, const wchar_t* File);
    const wchar_t*  __stdcall MediaInfo_Get(void* Handle, int StreamKind, size_t StreamNumber, const wchar_t* Parameter, int KindOfInfo, int KindOfSearch);
}

#else //defined(UNICODE)

// The C API is only in the Unicode build: same names for the C++ classes, strings are the bytes of the names
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfoList.h"
typedef MediaInfoLib::MediaInfo     MediaInfo_Class;
typedef MediaInfoLib::MediaInfoList MediaInfoList_Class;

static std::string Result; // Valid until the next call, as the strings of the C API

static const char* Result_Set(const std::string& Value)
{
    Result=Value;
    return Result.c_str();
}

static void* MediaInfoA_New()
{
    return new MediaInfo_Class;
}

static void* MediaInfoA_New_Quick(const char* File, const char*)
{
    MediaInfo_Class* MI=new MediaInfo_Class;
    if (MI->Open(File))
        return MI;
    delete MI;
    return NULL;
}

static void MediaInfoA_Delete(void* Handle)
{
    delete (MediaInfo_Class*)Handle;
}

static size_t MediaInfoA_Open(void* Handle, const char* File)
{
    return ((MediaInfo_Class*)Handle)->Open(File);
}

static const char* MediaInfoA_Inform(void* Handle, size_t)
{
    return Result_Set(((MediaInfo_Class*)Handle)->Inform());
}

static const char* MediaInfoA_Get(void* Handle, int StreamKind, size_t StreamNumber, const char* Parameter, int KindOfInfo, int KindOfSearch)
{
    return Result_Set(((MediaInfo_Class*)Handle)->Get((MediaInfoLib::stream_t)StreamKind, StreamNumber, Parameter, (MediaInfoLib::info_t)KindOfInfo, (MediaInfoLib::info_t)KindOfSearch));
}

static const char* MediaInfoA_Option(void* Handle, const char* Option, const char* Value)
{
    return Result_Set(Handle?((MediaInfo_Class*)Handle)->Option(Option, Value):MediaInfo_Class::Option_Static(Option, Value));
}

static void* MediaInfoListA_New()
{
    return new MediaInfoList_Class;
}

static void* MediaInfoListA_New_Quick(const char* File, const char*)
{
    MediaInfoList_Class* List=new MediaInfoList_Class;
    List->Open(File);
    return List;
}

static void MediaInfoListA_Delete(void* Handle)
{
    delete (MediaInfoList_Class*)Handle;
}

static size_t MediaInfoListA_Open(void* Handle, const char* File, int)
{
    return ((MediaInfoList_Class*)Handle)->Open(File);
}

static const char* MediaInfoListA_Get(void* Handle, size_t FilePos, int StreamKind, size_t StreamNumber, const char* Parameter, int KindOfInfo, int KindOfSearch)
{
    return Result_Set(((MediaInfoList_Class*)Handle)->Get(FilePos, (MediaInfoLib::stream_t)StreamKind, StreamNumber, Parameter, (MediaInfoLib::info_t)KindOfInfo, (MediaInfoLib::info_t)KindOfSearch));
}

static size_t MediaInfoListA_Count_Get_Files(void* Handle)
{
    return ((MediaInfoList_Class*)Handle)->Count_Get();
}

#endif //defined(UNICODE)

static const int Stream_General=0;
static const int Info_Name=0;
static const int Info_Text=1;

// UTF-8 names: Latin-1 range, CJK, outside of the BMP
static const char* const Names[]=
{
    "caf\xC3\xA9",
    "\xC3\x85snan \xC3\xA9 \xE6\xB5\x8B\xE8\xAF\x95",
    "\xF0\x9F\x8E\xB5",
};
static const size_t Names_Size=sizeof(Names)/sizeof(*Names);
static const char* const Title="caf\xC3\xA9 \xE6\xB5\x8B\xE8\xAF\x95 \xF0\x9F\x8E\xB5";

// Names which are not valid UTF-8: Latin-1, EUC-JP, overlong sequences of "/" and ".." (they can't be created on macOS and Windows)
static const char* const Names_NotUTF8[]=
{
    "caf\xE9.flac",
    "\xA5\xC6\xA5\xB9\xA5\xC8.flac",
    "\xC0\xAF",
    "a\xC0\257b.flac",
    "\xC0\xAE\xC0\xAE",
};
static const size_t Names_NotUTF8_Size=sizeof(Names_NotUTF8)/sizeof(*Names_NotUTF8);

static size_t Checks=0;
static int Status=0;

static void Check(bool OK, const std::string& Name, const std::string& Message)
{
    Checks++;
    if (!OK)
    {
        printf("NOK: %s, %s\n", Name.c_str(), Message.c_str());
        Status=1;
    }
}

// Bytes which are not printable ASCII as \xHH, for the messages
static std::string Printable(const char* Value)
{
    if (!Value)
        return "(null)";
    std::string ToReturn;
    for (const unsigned char* Char=(const unsigned char*)Value; *Char; Char++)
    {
        if (*Char<0x20 || *Char>=0x7F)
        {
            char Hex[5];
            snprintf(Hex, sizeof(Hex), "\\x%02X", *Char);
            ToReturn+=Hex;
        }
        else
            ToReturn+=(char)*Char;
    }
    return ToReturn;
}

static std::string Printable(const std::string& Value)
{
    return Printable(Value.c_str());
}

// Strict UTF-8 decoding (no overlong sequence, no surrogate), false if not valid
static bool UTF8_To_Wide(const std::string& Value, std::wstring& ToReturn)
{
    ToReturn.clear();
    for (size_t Pos=0; Pos<Value.size();)
    {
        unsigned char Char=(unsigned char)Value[Pos++];
        unsigned long Code;
        int More;
        if (Char<0x80)
            { Code=Char; More=0; }
        else if ((Char&0xE0)==0xC0)
            { Code=Char&0x1F; More=1; }
        else if ((Char&0xF0)==0xE0)
            { Code=Char&0x0F; More=2; }
        else if ((Char&0xF8)==0xF0)
            { Code=Char&0x07; More=3; }
        else
            return false;
        for (int i=0; i<More; i++)
        {
            if (Pos>=Value.size() || (Value[Pos]&0xC0)!=0x80)
                return false;
            Code=(Code<<6)|(Value[Pos++]&0x3F);
        }
        if ((More==1 && Code<0x80) || (More==2 && Code<0x800) || (More==3 && Code<0x10000) || Code>0x10FFFF || (Code>=0xD800 && Code<0xE000))
            return false;
        if (sizeof(wchar_t)==2 && Code>=0x10000) // UTF-16
        {
            ToReturn+=(wchar_t)(0xD800|((Code-0x10000)>>10));
            ToReturn+=(wchar_t)(0xDC00|((Code-0x10000)&0x3FF));
        }
        else
            ToReturn+=(wchar_t)Code;
    }
    return true;
}

static bool Is_UTF8(const char* Value)
{
    std::wstring Wide;
    return Value && UTF8_To_Wide(Value, Wide);
}

// Latin-1 bytes of a UTF-8 text, false if it can't be encoded
static bool UTF8_To_Latin1(const std::string& Value, std::string& ToReturn)
{
    std::wstring Wide;
    if (!UTF8_To_Wide(Value, Wide))
        return false;
    ToReturn.clear();
    for (size_t Pos=0; Pos<Wide.size(); Pos++)
    {
        if ((unsigned long)Wide[Pos]>0xFF)
            return false;
        ToReturn+=(char)Wide[Pos];
    }
    return true;
}

// Encoding of the strings of the narrow API
static bool Narrow_IsUTF8=false;
static bool Narrow_IsLatin1=false; // or cp1252, same bytes for the tested characters

// Bytes of a UTF-8 text for the narrow API, false if it can't be encoded
static bool To_Narrow(const std::string& Value, std::string& ToReturn)
{
    if (Narrow_IsUTF8)
    {
        ToReturn=Value;
        return true;
    }
    return Narrow_IsLatin1 && UTF8_To_Latin1(Value, ToReturn);
}

// Name of a directory listing as the names of the tested files
static std::string Listed_Path(const char* Value)
{
    std::string ToReturn(Value?Value:"");
    #if defined(_WIN32)
        for (size_t Pos=0; Pos<ToReturn.size(); Pos++)
            if (ToReturn[Pos]=='\\')
                ToReturn[Pos]='/';
    #endif
    return ToReturn;
}

// File system functions with the bytes of the names (UTF-8 on Windows with Unicode), independent from the library and from the locale
#if defined(_WIN32) && defined(UNICODE)
static FILE* Open_Raw(const std::string& Path)
{
    std::wstring Wide;
    FILE* F=NULL;
    if (!UTF8_To_Wide(Path, Wide) || _wfopen_s(&F, Wide.c_str(), L"wb"))
        return NULL;
    return F;
}

static bool Create_Directory(const std::string& Path)
{
    std::wstring Wide;
    return UTF8_To_Wide(Path, Wide) && !_wmkdir(Wide.c_str());
}
#elif defined(_WIN32)
static FILE* Open_Raw(const std::string& Path)
{
    FILE* F=NULL;
    return fopen_s(&F, Path.c_str(), "wb")?NULL:F;
}

static bool Create_Directory(const std::string& Path)
{
    return !_mkdir(Path.c_str());
}
#else //defined(_WIN32)
static FILE* Open_Raw(const std::string& Path)
{
    return fopen(Path.c_str(), "wb");
}

static bool Create_Directory(const std::string& Path)
{
    return !mkdir(Path.c_str(), 0700);
}
#endif //defined(_WIN32)

// FLAC file with a title (Vorbis comment, UTF-8)
static void Append_LE32(std::string& Data, size_t Value)
{
    for (int i=0; i<4; i++)
        Data+=(char)((Value>>(i*8))&0xFF);
}

static bool Create_FLAC(const std::string& Path, const std::string& Title)
{
    static const unsigned char StreamInfo[]=
    {
        'f', 'L', 'a', 'C',
        0x00, 0x00, 0x00, 0x22,                                     // STREAMINFO, 34 bytes
        0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Block and frame sizes
        0x0A, 0xC4, 0x42, 0xF0, 0x00, 0x00, 0x00, 0x00,             // 44100 Hz, 2 channels, 16 bits, unknown duration
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MD5
    };
    std::string Comment="TITLE="+Title;
    std::string Block;
    Append_LE32(Block, 4);
    Block+="test";
    Append_LE32(Block, 1);
    Append_LE32(Block, Comment.size());
    Block+=Comment;
    std::string Data((const char*)StreamInfo, sizeof(StreamInfo));
    Data+=(char)0x84; // Last block, VORBIS_COMMENT
    Data+=(char)((Block.size()>>16)&0xFF);
    Data+=(char)((Block.size()>>8)&0xFF);
    Data+=(char)(Block.size()&0xFF);
    Data+=Block;

    FILE* F=Open_Raw(Path);
    if (!F)
        return false;
    bool Written=fwrite(Data.data(), 1, Data.size(), F)==Data.size();
    return !fclose(F) && Written;
}

int main(int argc, char* argv[])
{
    if (argc<4)
    {
        fprintf(stderr, "Usage: mil_filename setlocale(0|1) charset_utf8(0|1) directory\n");
        return 2;
    }
    setvbuf(stdout, NULL, _IONBF, 0); // Lines already written if the program is stopped, e.g. endless directory listing
    if (!strcmp(argv[1], "1"))
        setlocale(LC_ALL, "");
    bool CharSet_UTF8=!strcmp(argv[2], "1");
    if (CharSet_UTF8)
        MediaInfoA_Option(NULL, "CharSet", "UTF-8");
    std::string Directory(argv[3]);

    // Encoding of the locale: ASCII (converted to UTF-8 by ZenLib), UTF-8, or another one (e.g. Latin-1, only a part of the names can be converted)
    #if defined(_WIN32)
        bool IsASCII=false;
        bool IsUnicode=false; // Code page of the system for the narrow strings, with or without setlocale()
        bool IsLatin1=GetACP()==1252;
        bool FileNames_IsUnicode=true; // Wide functions of the system
    #else
        const char* CodeSet=nl_langinfo(CODESET);
        bool IsASCII=!strcmp(CodeSet, "ANSI_X3.4-1968") || !strcmp(CodeSet, "ASCII") || !strcmp(CodeSet, "US-ASCII") || !strcmp(CodeSet, "646");
        bool IsUnicode=IsASCII || !strcmp(CodeSet, "UTF-8") || !strcmp(CodeSet, "utf8");
        bool IsLatin1=!strcmp(CodeSet, "ISO-8859-1") || !strcmp(CodeSet, "ISO8859-1");
        #if defined(__APPLE__)
            bool FileNames_IsUnicode=true; // UTF-8 file names whatever the locale
        #else
            bool FileNames_IsUnicode=IsUnicode;
        #endif
    #endif
    #if !defined(UNICODE)
        // Without Unicode, strings are the bytes of the names, no conversion whatever the locale: same results as with the bytes kept with an ASCII locale
        IsASCII=true;
        IsUnicode=true;
        IsLatin1=false;
        FileNames_IsUnicode=true;
    #endif
    Narrow_IsUTF8=IsUnicode || CharSet_UTF8; // The option has the priority over the locale
    Narrow_IsLatin1=!Narrow_IsUTF8 && IsLatin1;

    // Files, with a name in the encoding of the locale if file names use it
    std::string Files_Path=Directory+"/files";
    struct file
    {
        std::string Path;
        std::string Path_System;
        std::string Title;
        bool        Exists;
    };
    file Files[Names_Size+2];
    Files[0].Path=Files_Path+"/ascii.flac";
    Files[0].Title=Title;
    for (size_t i=0; i<Names_Size; i++)
    {
        Files[1+i].Path=Files_Path+"/"+Names[i]+".flac";
        Files[1+i].Title=Title;
    }
    Files[Names_Size+1].Path=Files_Path+"/"+Names[0]+"_ascii_title.flac"; // Only the file name is not ASCII
    Files[Names_Size+1].Title="Plain title";
    bool Created=Create_Directory(Files_Path);
    for (size_t i=0; i<Names_Size+2; i++)
    {
        Files[i].Exists=true;
        if (FileNames_IsUnicode)
            Files[i].Path_System=Files[i].Path;
        else if (!IsLatin1)
            Files[i].Exists=i==0; // e.g. EUC-JP, ASCII name only
        else
            Files[i].Exists=UTF8_To_Latin1(Files[i].Path, Files[i].Path_System);
        if (i==0 && !FileNames_IsUnicode)
            Files[i].Path_System=Files[i].Path;
        if (Files[i].Exists)
            Created=Create_FLAC(Files[i].Path_System, Files[i].Title) && Created;
    }

    // Directory with UTF-8 names, and names which are not valid UTF-8
    std::string List_Path=Directory+"/list";
    std::string SubDirectory=std::string("sub_")+Names[0];
    std::set<std::string> Expected;
    Created=Create_Directory(List_Path) && Create_Directory(List_Path+"/"+SubDirectory) && Created;
    for (size_t i=0; i<Names_Size; i++)
    {
        std::string Path=List_Path+"/"+Names[i]+".flac";
        Created=Create_FLAC(Path, Title) && Created;
        Expected.insert(Path);
    }
    Created=Create_FLAC(List_Path+"/ascii.flac", Title) && Create_FLAC(List_Path+"/"+SubDirectory+"/inner.flac", Title) && Created;
    Expected.insert(List_Path+"/ascii.flac");
    Expected.insert(List_Path+"/"+SubDirectory+"/inner.flac");
    std::set<std::string> Expected_All=Expected; // With the names which are not valid UTF-8
    size_t NotUTF8_Count=0;
    for (size_t i=0; i<Names_NotUTF8_Size; i++)
        if (Create_FLAC(List_Path+"/"+Names_NotUTF8[i], Title)) // Not on macOS and Windows
        {
            NotUTF8_Count++;
            Expected_All.insert(List_Path+"/"+Names_NotUTF8[i]);
        }
    if (!Created)
    {
        printf("NOK: internal, can't create the files in %s\n", Printable(Directory).c_str());
        return 1;
    }

    for (size_t i=0; i<Names_Size+2; i++)
    {
        if (!Files[i].Exists)
            continue;
        const std::string& Path=Files[i].Path;
        std::string Shown=Printable(Path.substr(Files_Path.size()+1));

        // Narrow API, e.g. a C program which doesn't call setlocale(): file opened, name and title in UTF-8 (the locale is ASCII or UTF-8)
        // or in the encoding of the locale or of the code page, with the names and titles which can be encoded
        std::string Path_Narrow, Title_Narrow;
        if (To_Narrow(Path, Path_Narrow))
        {
            void* MI=MediaInfoA_New();
            size_t Opened=MediaInfoA_Open(MI, Path_Narrow.c_str());
            Check(Opened, "MediaInfoA_Open("+Shown+")", "file not opened");
            if (Opened)
            {
                const char* Value=MediaInfoA_Get(MI, Stream_General, 0, "CompleteName", Info_Text, Info_Name);
                Check(Value && Path_Narrow==Value, "MediaInfoA_Get("+Shown+", CompleteName)", "result is "+Printable(Value));
                if (To_Narrow(Files[i].Title, Title_Narrow))
                {
                    Value=MediaInfoA_Get(MI, Stream_General, 0, "Title", Info_Text, Info_Name);
                    Check(Value && Title_Narrow==Value, "MediaInfoA_Get("+Shown+", Title)", "result is "+Printable(Value));
                }
                Value=MediaInfoA_Inform(MI, 0);
                Check(Value && (!Narrow_IsUTF8 || Is_UTF8(Value)) && strstr(Value, Path_Narrow.c_str()), "MediaInfoA_Inform("+Shown+")", "not UTF-8 or without the file name");
            }
            MediaInfoA_Delete(MI);

            MI=MediaInfoA_New_Quick(Path_Narrow.c_str(), "");
            const char* Value=MI?MediaInfoA_Get(MI, Stream_General, 0, "CompleteName", Info_Text, Info_Name):NULL;
            Check(Value && Path_Narrow==Value, "MediaInfoA_New_Quick("+Shown+")", "file not opened or name is "+Printable(Value));
            if (MI)
                MediaInfoA_Delete(MI);

            void* List=MediaInfoListA_New_Quick(Path_Narrow.c_str(), "");
            Value=(List && MediaInfoListA_Count_Get_Files(List)==1)?MediaInfoListA_Get(List, 0, Stream_General, 0, "CompleteName", Info_Text, Info_Name):NULL;
            Check(Value && Path_Narrow==Value, "MediaInfoListA_New_Quick("+Shown+")", "file not opened or name is "+Printable(Value));
            if (List)
                MediaInfoListA_Delete(List);
        }

        // Wide API, e.g. pymediainfo
        #if defined(UNICODE)
            std::wstring Path_Wide, Title_Wide;
            UTF8_To_Wide(Path, Path_Wide);
            UTF8_To_Wide(Files[i].Title, Title_Wide);
            void* MI=MediaInfo_New();
            size_t Opened=MediaInfo_Open(MI, Path_Wide.c_str());
            Check(Opened, "MediaInfo_Open("+Shown+")", "file not opened");
            if (Opened)
            {
                const wchar_t* Value_Wide=MediaInfo_Get(MI, Stream_General, 0, L"CompleteName", Info_Text, Info_Name);
                Check(Value_Wide && Path_Wide==Value_Wide, "MediaInfo_Get("+Shown+", CompleteName)", "wrong name");
                Value_Wide=MediaInfo_Get(MI, Stream_General, 0, L"Title", Info_Text, Info_Name);
                Check(Value_Wide && Title_Wide==Value_Wide, "MediaInfo_Get("+Shown+", Title)", "wrong title");
            }
            MediaInfo_Delete(MI);
        #endif //defined(UNICODE)
    }

    // Narrow API, text which is not a file name: encoding of the locale, or UTF-8 with an ASCII locale or the UTF-8 option
    std::string Text_Narrow;
    if (To_Narrow(Names[0], Text_Narrow))
    {
        void* MI=MediaInfoA_New();
        MediaInfoA_Option(MI, "Inform", ("General;"+Text_Narrow+"|%FileExtension%").c_str());
        std::string Value;
        if (MediaInfoA_Open(MI, Files[0].Path.c_str()))
        {
            const char* Inform=MediaInfoA_Inform(MI, 0);
            Value=Inform?Inform:"";
        }
        while (!Value.empty() && (Value[Value.size()-1]=='\r' || Value[Value.size()-1]=='\n'))
            Value.resize(Value.size()-1);
        Check(Value==Text_Narrow+"|flac", "MediaInfoA_Option(Inform)", "result is "+Printable(Value));
        MediaInfoA_Option(MI, "Inform", "");
        MediaInfoA_Delete(MI);
    }

    // Narrow API, name which is not valid UTF-8 with an ASCII locale, next to the UTF-8 name of the same characters: the file with this name is opened,
    // with the same bytes as name (not with the UTF-8 option, which needs valid UTF-8, these names can't be created on macOS)
    if (IsASCII && !CharSet_UTF8)
    {
        std::string Path=Directory+"/"+Names_NotUTF8[0];
        if (Create_FLAC(Path, "Plain title") && Create_FLAC(Directory+"/"+Names[0]+".flac", Title))
        {
            void* MI=MediaInfoA_New();
            size_t Opened=MediaInfoA_Open(MI, Path.c_str());
            Check(Opened, "MediaInfoA_Open("+Printable(Names_NotUTF8[0])+")", "file not opened");
            if (Opened)
            {
                const char* Value=MediaInfoA_Get(MI, Stream_General, 0, "CompleteName", Info_Text, Info_Name);
                Check(Value && Path==Value, "MediaInfoA_Get("+Printable(Names_NotUTF8[0])+", CompleteName)", "result is "+Printable(Value));
                Value=MediaInfoA_Get(MI, Stream_General, 0, "Title", Info_Text, Info_Name);
                Check(Value && !strcmp(Value, "Plain title"), "MediaInfoA_Get("+Printable(Names_NotUTF8[0])+", Title)", "result is "+Printable(Value));
            }
            MediaInfoA_Delete(MI);
        }
    }

    // Narrow API, directory listing: it must end, with all names with an ASCII locale, else with the UTF-8 names only if file names are UTF-8
    std::string List_Path_Narrow;
    if (!To_Narrow(List_Path, List_Path_Narrow))
        List_Path_Narrow=List_Path; // e.g. EUC-JP, ASCII name
    void* List=MediaInfoListA_New();
    MediaInfoListA_Open(List, List_Path_Narrow.c_str(), 0);
    std::set<std::string> Listed;
    std::string Listed_Shown;
    size_t Count=MediaInfoListA_Count_Get_Files(List);
    for (size_t i=0; i<Count; i++)
    {
        const char* Value=MediaInfoListA_Get(List, i, Stream_General, 0, "CompleteName", Info_Text, Info_Name);
        Listed.insert(Listed_Path(Value));
        Listed_Shown+=" "+Printable(Value);
    }
    MediaInfoListA_Delete(List);
    if (IsASCII && CharSet_UTF8)
        Check(Count==Expected_All.size(), "MediaInfoListA_Open(list)", "listed:"+Listed_Shown); // The option is for valid UTF-8, so not the bytes of all names
    else if (IsASCII)
        Check(Listed==Expected_All, "MediaInfoListA_Open(list)", "listed:"+Listed_Shown);
    else if (FileNames_IsUnicode && Narrow_IsUTF8)
        Check(Listed==Expected, "MediaInfoListA_Open(list)", "listed:"+Listed_Shown);
    else if (FileNames_IsUnicode)
        Check(Count==Expected.size(), "MediaInfoListA_Open(list)", "listed:"+Listed_Shown); // Names not all encodable
    else if (IsLatin1)
        Check(Count==Expected.size()+NotUTF8_Count, "MediaInfoListA_Open(list)", "listed:"+Listed_Shown); // Each byte is a character
    else
        Check(Count>=1, "MediaInfoListA_Open(list)", "listed:"+Listed_Shown); // e.g. EUC-JP, names which can be decoded

    if (!Status)
        printf("OK: %u checks\n", (unsigned)Checks);
    return Status;
}
