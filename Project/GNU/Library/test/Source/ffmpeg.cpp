/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

// Search of FFmpeg on Windows with a PATH set by this program (Git Bash changes the PATH of the programs it starts), for ffmpeg.sh
// Usage: mil_ffmpeg PATH|unset file
// Opens file with the FFmpeg option and PATH set to PATH ("unset": removed), exit code 0 if the file is opened
// Copies named ffmpeg-ma.exe, started by the library as FFmpeg: write their name in a file "run" next to them

#include <cstdio>
#include <cwchar>
#include <string>
#include <windows.h>
#include "MediaInfoDLL/MediaInfoDLL.h" // MediaInfo.dll loaded when used, the copies run without it

int wmain(int argc, wchar_t* argv[])
{
    // Copy started as FFmpeg
    std::wstring Name(32768, L'\0');
    Name.resize(GetModuleFileNameW(NULL, &Name[0], (DWORD)Name.size()));
    size_t Name_Start=Name.find_last_of(L'\\')+1;
    if (!_wcsicmp(Name.c_str()+Name_Start, L"ffmpeg-ma.exe"))
    {
        FILE* Run=NULL;
        if (_wfopen_s(&Run, (Name.substr(0, Name_Start)+L"run").c_str(), L"w"))
            return 1;
        fputws(argv[0], Run);
        return fclose(Run)?1:0;
    }

    if (argc!=3)
    {
        fputws(L"Usage: mil_ffmpeg PATH|unset file\n", stderr);
        return 1;
    }

    // MediaInfo.dll loaded with the PATH of the environment, then the PATH of the test for the search of FFmpeg
    MediaInfoDLL::MediaInfo MI;
    if (!MI.IsReady())
    {
        fputws(L"MediaInfo.dll not loaded\n", stderr);
        return 1;
    }
    MI.Option_Static(L"Enable_FFmpeg", L"1");
    if (!SetEnvironmentVariableW(L"PATH", wcscmp(argv[1], L"unset")?argv[1]:NULL))
    {
        fputws(L"PATH not changed\n", stderr);
        return 1;
    }
    if (!MI.Open(argv[2]))
    {
        fputws(L"File not opened\n", stderr);
        return 1;
    }
    return 0;
}
