/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

// File and directory names longer than MAX_PATH with the C API (on Windows, the DLL used by a program without longPathAware in its manifest,
// or with it), for longpath.sh
// Usage: mil_longpath directory [network path of the root of the drive of directory, e.g. //localhost/C$ for C:/...]
// LONGPATHS_EXPECTED=enabled|disabled: support of long paths by the process expected on Windows (not checked if not set)
// Prints "NOK: check, message" lines or "OK: count checks", exit code 0 (OK), 1 (NOK) or 77 (skipped, build without Unicode)

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#if defined(UNICODE)

#if !defined(_WIN32)
    #define __stdcall
#endif

// C API, wide functions
extern "C"
{
    void*           __stdcall MediaInfo_New();
    void            __stdcall MediaInfo_Delete(void* Handle);
    size_t          __stdcall MediaInfo_Open(void* Handle, const wchar_t* File);
    const wchar_t*  __stdcall MediaInfo_Get(void* Handle, int StreamKind, size_t StreamNumber, const wchar_t* Parameter, int KindOfInfo, int KindOfSearch);
    void*           __stdcall MediaInfoList_New();
    void            __stdcall MediaInfoList_Delete(void* Handle);
    size_t          __stdcall MediaInfoList_Open(void* Handle, const wchar_t* File, int Options);
    const wchar_t*  __stdcall MediaInfoList_Get(void* Handle, size_t FilePos, int StreamKind, size_t StreamNumber, const wchar_t* Parameter, int KindOfInfo, int KindOfSearch);
    size_t          __stdcall MediaInfoList_Count_Get_Files(void* Handle);
}

static const int Stream_General=0;
static const int Info_Name=0;
static const int Info_Text=1;

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

// Names of the test are ASCII
static std::wstring W(const std::string& Value)
{
    return std::wstring(Value.begin(), Value.end());
}

static std::string N(const wchar_t* Value)
{
    std::string ToReturn;
    for (; Value && *Value; Value++)
        ToReturn+=(*Value<0x80)?(char)*Value:'?';
    return ToReturn;
}

// Name with "/" as separator, for the comparisons (the library gives "\" on Windows in the names of a listing)
static std::string Slashes(const std::string& Value)
{
    std::string ToReturn(Value);
    #if defined(_WIN32)
        std::replace(ToReturn.begin(), ToReturn.end(), '\\', '/');
    #endif
    return ToReturn;
}

// Value of a field of the general stream
static std::string General(void* MI, const char* Parameter)
{
    return N(MediaInfo_Get(MI, Stream_General, 0, W(Parameter).c_str(), Info_Text, Info_Name));
}

// Complete names of the files of a list, with "/" as separator, sorted
static std::string Listed(void* List)
{
    std::vector<std::string> Names;
    size_t Count=MediaInfoList_Count_Get_Files(List);
    for (size_t Pos=0; Pos<Count; Pos++)
        Names.push_back(Slashes(N(MediaInfoList_Get(List, Pos, Stream_General, 0, L"CompleteName", Info_Text, Info_Name))));
    std::sort(Names.begin(), Names.end());
    std::string ToReturn;
    for (size_t Pos=0; Pos<Names.size(); Pos++)
        ToReturn+=(Pos?" | ":"")+Names[Pos];
    return ToReturn;
}

// File system functions independent from the library, with the extended-length prefix on Windows (full paths)
#if defined(_WIN32)
static std::wstring Raw_Name(const std::string& Path)
{
    std::wstring ToReturn(L"\\\\?\\");
    size_t Start=0;
    if (Path.size()>=2 && (Path[0]=='/' || Path[0]=='\\') && (Path[1]=='/' || Path[1]=='\\'))
    {
        ToReturn+=L"UNC\\";
        Start=2;
    }
    for (size_t Pos=Start; Pos<Path.size(); Pos++)
        ToReturn+=Path[Pos]=='/'?L'\\':(wchar_t)(unsigned char)Path[Pos];
    return ToReturn;
}

static bool Create_Directory(const std::string& Path)
{
    return CreateDirectoryW(Raw_Name(Path).c_str(), NULL)!=0;
}

static FILE* Open_Raw(const std::string& Path)
{
    FILE* F=NULL;
    return _wfopen_s(&F, Raw_Name(Path).c_str(), L"wb")?NULL:F;
}

static bool Change_Directory(const std::string& Path)
{
    return SetCurrentDirectoryW(Raw_Name(Path).c_str()+4)!=0; // Short directory
}
#else //defined(_WIN32)
static bool Create_Directory(const std::string& Path)
{
    return !mkdir(Path.c_str(), 0700);
}

static FILE* Open_Raw(const std::string& Path)
{
    return fopen(Path.c_str(), "wb");
}

static bool Change_Directory(const std::string& Path)
{
    return !chdir(Path.c_str());
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

// File opened with its name as given, and the fields from it
static void Check_File(const std::string& Path, const std::string& Folder, const std::string& Label)
{
    void* MI=MediaInfo_New();
    Check(MediaInfo_Open(MI, W(Path).c_str())==1, "MediaInfo_Open() of "+Label, "not opened");
    std::string Value=General(MI, "CompleteName");
    Check(Value==Path, "CompleteName of "+Label, "result is "+Value);
    Value=General(MI, "FolderName");
    Check(Slashes(Value)==Folder, "FolderName of "+Label, "result is "+Value);
    Value=General(MI, "FileNameExtension");
    Check(Value=="file.flac", "FileNameExtension of "+Label, "result is "+Value);
    Value=General(MI, "Format")+"|"+General(MI, "Title");
    Check(Value=="FLAC|Long path", "Format and title of "+Label, "result is "+Value);
    MediaInfo_Delete(MI);
}

// Directory listing, through the directories longer than MAX_PATH, with the names as given
static void Check_List(const std::string& Path, const std::string& Expected, const std::string& Label)
{
    void* List=MediaInfoList_New();
    size_t Count=MediaInfoList_Open(List, W(Path).c_str(), 0);
    std::string Value=Listed(List);
    Check(Count==2 && Value==Expected, "MediaInfoList_Open() of "+Label, "listed "+Value);
    MediaInfoList_Delete(List);
}

int main(int argc, char* argv[])
{
    if (argc<2)
    {
        fprintf(stderr, "Usage: mil_longpath directory [network path of the root of the drive of directory]\n");
        return 2;
    }
    setvbuf(stdout, NULL, _IONBF, 0); // Lines already written if the program is stopped
    std::string Directory(argv[1]);
    std::string Network_Root(argc>=3?argv[2]:"");

    // Support of long paths by the process (Windows 10 version 1607 and more, LongPathsEnabled in the registry and longPathAware in the
    // manifest of the program), else the library adds the extended-length prefix
    #if defined(_WIN32)
        typedef BOOLEAN (WINAPI *RtlAreLongPathsEnabled_Type)();
        RtlAreLongPathsEnabled_Type RtlAreLongPathsEnabled=(RtlAreLongPathsEnabled_Type)(void(*)())GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlAreLongPathsEnabled");
        bool LongPaths=RtlAreLongPathsEnabled && RtlAreLongPathsEnabled();
        printf("OK: long paths of the process: %s\n", LongPaths?"enabled":"disabled");
        char Expected[16];
        DWORD Expected_Size=GetEnvironmentVariableA("LONGPATHS_EXPECTED", Expected, sizeof(Expected));
        if (Expected_Size && Expected_Size<sizeof(Expected))
            Check(!strcmp(Expected, LongPaths?"enabled":"disabled"), "support of long paths by the process", std::string("not ")+Expected+" (registry or manifest not taken into account)");
    #endif

    // Directory tree longer than MAX_PATH also from the directory, each part shorter than the limit of a name (255)
    std::string Relative_Directory;
    std::string Long_Directory=Directory;
    for (int i=1; i<=6; i++)
    {
        Relative_Directory+=(i==1?"":"/")+std::string("directory")+(char)('0'+i)+std::string(40, 'x');
        Long_Directory=Directory+"/"+Relative_Directory;
        if (!Create_Directory(Long_Directory))
            Check(false, "internal", "directory not created");
    }
    std::string First_Directory=Directory+"/"+Relative_Directory.substr(0, Relative_Directory.find('/'));
    std::string Long_File=Long_Directory+"/file.flac";
    std::string Inner_File=Long_Directory+"/sub/inner.flac";
    if (!Create_FLAC(Long_File, "Long path") || !Create_Directory(Long_Directory+"/sub") || !Create_FLAC(Inner_File, "Long path"))
        Check(false, "internal", "file not created");
    printf("OK: name of %u characters\n", (unsigned)Long_File.size());

    // File and directory listing
    Check_File(Long_File, Long_Directory, "a long path");
    Check_List(First_Directory, Long_File+" | "+Inner_File, "a directory");

    // Network path (Windows)
    if (!Network_Root.empty())
    {
        size_t Colon=Long_Directory.find(':');
        std::string Network_Directory=Network_Root+Long_Directory.substr(Colon+1);
        Check_File(Network_Directory+"/file.flac", Network_Directory, "a network path");
        Check_List(Network_Directory, Network_Directory+"/file.flac | "+Network_Directory+"/sub/inner.flac", "a network directory");
    }

    // Relative path longer than MAX_PATH, from a short current directory
    if (!Change_Directory(Directory))
        Check(false, "internal", "current directory not changed");
    Check_File(Relative_Directory+"/file.flac", Relative_Directory, "a relative path");
    Check_List(Relative_Directory.substr(0, Relative_Directory.find('/')), Relative_Directory+"/file.flac | "+Relative_Directory+"/sub/inner.flac", "a relative directory");

    printf("OK: %u checks\n", (unsigned)Checks);
    return Status;
}

#else //defined(UNICODE)

int main()
{
    return 77; // The C API is only in the Unicode build
}

#endif //defined(UNICODE)
