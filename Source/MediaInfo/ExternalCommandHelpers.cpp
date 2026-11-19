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
#include "MediaInfo/ExternalCommandHelpers.h"
#include "ZenLib/OS_Utils.h"

#ifdef WINDOWS
#include <windows.h>
#include <process.h>
#include <wchar.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#endif //WINDOWS
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>

using namespace std;
using namespace ZenLib;

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
const ZtringList ffmpeg_PossibleNames= // Hack, ZtringList needs to handle C++11 style static initialization
{
    #ifdef WINDOWS
    __T("ffmpeg-ma.exe;ffmpeg.exe")
    #else
    __T("ffmpeg-ma;ffmpeg")
    #endif //WINDOWS
};

//---------------------------------------------------------------------------
Ztring External_Command_Exists(const ZtringList& PossibleNames)
{
    #ifndef WINDOWS_UWP
    Ztring Path;

    #ifdef WINDOWS
    Ztring ExeDirectory;

    DWORD EnvPathSize=GetEnvironmentVariableW(L"PATH", nullptr, 0);
    while (EnvPathSize)
    {
        std::wstring EnvPath(EnvPathSize, L'\0');
        DWORD EnvPathLen=GetEnvironmentVariableW(L"PATH", &EnvPath[0], EnvPathSize);
        if (EnvPathLen<EnvPathSize)
        {
            EnvPath.resize(EnvPathLen);
            Path.From_Unicode(EnvPath);
            break;
        }
        EnvPathSize=EnvPathLen; //PATH was made longer in the meantime, this is the size it needs now
    }

    std::wstring ExePath(256, L'\0'); //Path of any length, the buffer is extended while the name is truncated
    for (;;)
    {
        DWORD ExePathLen=GetModuleFileNameW(NULL, &ExePath[0], (DWORD)ExePath.size());
        if (ExePathLen>0 && ExePathLen<ExePath.size())
        {
            ExePath.resize(ExePathLen);
            ExeDirectory=Ztring().From_Unicode(ExePath);
            size_t Pos=ExeDirectory.find_last_of(__T("\\"));
            if (Pos!=string::npos)
                ExeDirectory=ExeDirectory.substr(0, Pos);
            break;
        }
        if (!ExePathLen || ExePath.size()>=0x10000)
            break;
        ExePath.resize(ExePath.size()*2);
    }

    const Ztring EnvSep = __T(";");
    const Ztring PathSep = __T("\\");
    #else
    const char* EnvPath=getenv("PATH");
    if (EnvPath)
        Path=FileName_From_Native(EnvPath);
    const Ztring EnvSep=__T(":");
    const Ztring PathSep=__T("/");
    #endif //WINDOWS

    for (const Ztring& Command : PossibleNames)
    {
        #ifdef WINDOWS
        if (!ExeDirectory.empty()) // Search in program directory
        {
            Ztring FullPath=ExeDirectory+PathSep+Command;
            struct _stat64 Buf;
            if (_wstat64(FileName_To_Wide(FullPath).c_str(), &Buf)==0 && Buf.st_mode&S_IFREG && Buf.st_mode&S_IEXEC)
                return FullPath;
        }
        #endif //WINDOWS

        size_t Start=0;
        while (!Path.empty() && Start<=Path.size()) //PATH not set or empty: no search (implementation-defined for POSIX)
        {
            size_t End=Path.find(EnvSep, Start);
            #ifdef WINDOWS
            size_t Quote=Path.find(__T('"'), Start);
            while (Quote<End) //Separators between quotes are in the entry, as for cmd.exe (e.g. "C:\a;b")
            {
                size_t Quote_End=Path.find(__T('"'), Quote+1);
                if (Quote_End==string::npos)
                {
                    End=string::npos;
                    break;
                }
                if (Quote_End>End)
                    End=Path.find(EnvSep, Quote_End);
                Quote=Path.find(__T('"'), Quote_End+1);
            }
            #endif //WINDOWS
            if (End==string::npos)
            {
                #ifdef WINDOWS
                if (Start==Path.size())
                    break; //Separator at the end, not an entry for SearchPath()
                #endif //WINDOWS
                End=Path.size();
            }
            Ztring Directory=Path.substr(Start, End-Start);
            #ifdef WINDOWS
            Directory.FindAndReplace(__T("\""), Ztring(), 0, Ztring_Recursive); //Quotes are not in the names, as for cmd.exe
            #endif //WINDOWS
            if (Directory.empty())
                Directory=__T("."); //Empty entry, the current directory as for POSIX shells and SearchPath()

            Ztring FullPath=Directory+PathSep+Command;

            #ifdef WINDOWS
            struct _stat64 Buf;
            if (_wstat64(FileName_To_Wide(FullPath).c_str(), &Buf)==0 && Buf.st_mode&S_IFREG && Buf.st_mode&S_IEXEC)
            #else
            struct stat Buf;
            if (stat(FileName_To_Native(FullPath).c_str(), &Buf)==0 && Buf.st_mode&S_IFREG && Buf.st_mode&S_IXUSR)
            #endif //WINDOWS
                return FullPath;

            Start=End+1;
        }
    }
    #endif //WINDOWS_UWP

    return Ztring();
}

//---------------------------------------------------------------------------
#if defined(WINDOWS) && !defined(WINDOWS_UWP)
static Ztring External_Command_Quote(const Ztring& Argument)
{
    //Windows command line rules (CommandLineToArgvW)
    if (!Argument.empty() && Argument.find_first_of(__T(" \t\n\v\""))==string::npos)
        return Argument;

    Ztring Result(__T("\""));
    for (size_t i=0; ; i++)
    {
        size_t Backslashes=0;
        while (i<Argument.size() && Argument[i]==__T('\\'))
        {
            i++;
            Backslashes++;
        }
        if (i>=Argument.size())
        {
            Result.append(Backslashes*2, __T('\\')); //Escape all backslashes, the closing quote follows
            break;
        }
        if (Argument[i]==__T('"'))
            Result.append(Backslashes*2+1, __T('\\')); //Escape all backslashes and the quote
        else
            Result.append(Backslashes, __T('\\'));
        Result+=Argument[i];
    }
    Result+=__T('"');
    return Result;
}
#endif //defined(WINDOWS) && !defined(WINDOWS_UWP)

//---------------------------------------------------------------------------
int External_Command_Run(const Ztring& Command, const ZtringList& Arguments, Ztring* StdOut, Ztring* StdErr)
{
    #ifdef WINDOWS
    DWORD ExitCode=-1;
    #else
    int ExitCode=-1;
    #endif

    #ifndef WINDOWS_UWP
    #ifdef WINDOWS
    Ztring CommandLine=External_Command_Quote(Command);
    for (size_t Pos=0; Pos<Arguments.size(); Pos++)
    {
        CommandLine+=__T(' ');
        CommandLine+=External_Command_Quote(Arguments[Pos]);
    }
    std::wstring CommandLine_Unicode=CommandLine.To_Unicode();

    SECURITY_ATTRIBUTES Attrs;
    ZeroMemory(&Attrs, sizeof(SECURITY_ATTRIBUTES));
    Attrs.nLength=sizeof(SECURITY_ATTRIBUTES);
    Attrs.lpSecurityDescriptor=nullptr;
    Attrs.bInheritHandle=TRUE;

    HANDLE StdOutRead, StdOutWrite, StdErrRead, StdErrWrite;
    if (StdOut)
    {
        if (!CreatePipe(&StdOutRead, &StdOutWrite, &Attrs, 0))
            return -1;
    }
    else
    {
        StdOutRead=nullptr;
        StdOutWrite=nullptr;
    }

    if (StdErr)
    {
        if (!CreatePipe(&StdErrRead, &StdErrWrite, &Attrs, 0))
        {
            if (StdOutWrite) CloseHandle(StdOutWrite);
            if (StdOutRead)  CloseHandle(StdOutRead);
            return -1;
        }
    }
    else
    {
        StdErrRead=nullptr;
        StdErrWrite=nullptr;
    }

    STARTUPINFOW StartupInfo;
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb=sizeof(StartupInfo);
    StartupInfo.hStdError=StdErrWrite;
    StartupInfo.hStdOutput=StdOutWrite;
    StartupInfo.dwFlags|=STARTF_USESTDHANDLES;

    PROCESS_INFORMATION ProcessInfo;
    ZeroMemory(&ProcessInfo, sizeof(PROCESS_INFORMATION));

    std::wstring Command_Unicode=FileName_To_Wide(Command); //The program found, also with a long path (the name of the module in the command line is limited to MAX_PATH)
    if (!CreateProcessW(Command_Unicode.c_str(), &CommandLine_Unicode[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &StartupInfo, &ProcessInfo))
    {
        if (StdOutWrite) CloseHandle(StdOutWrite);
        if (StdOutRead)  CloseHandle(StdOutRead);
        if (StdErrWrite) CloseHandle(StdErrWrite);
        if (StdErrRead)  CloseHandle(StdErrRead);
        return -1;
    }

    if (StdOutWrite) CloseHandle(StdOutWrite);
    if (StdErrWrite) CloseHandle(StdErrWrite);

    char Buf[128];
    if (StdOut)
    {
        for (;;)
        {
            DWORD OutReadCount;
            if (!ReadFile(StdOutRead, Buf, sizeof(Buf), &OutReadCount, nullptr) || !OutReadCount)
                break;
            *StdOut+=Ztring().From_UTF8(Buf, OutReadCount);
        }
    }
    if (StdErr)
    {
        for (;;)
        {
            DWORD ErrReadCount;
            if (!ReadFile(StdErrRead, Buf, sizeof(Buf), &ErrReadCount, nullptr) || !ErrReadCount)
                break;
            *StdErr+=Ztring().From_UTF8(Buf, ErrReadCount);
        }
    }

    if (StdOutRead) CloseHandle(StdOutRead);
    if (StdErrRead) CloseHandle(StdErrRead);

    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);

    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);
    #else
    int StdOutPipe[2];
    int StdErrPipe[2];

    if (pipe(StdOutPipe)!=0)
        return -1;

    if (pipe(StdErrPipe)!=0)
    {
        close(StdOutPipe[0]);
        close(StdOutPipe[1]);
        return -1;
    }

    pid_t PID=fork();
    if (PID==-1)
    {
        close(StdOutPipe[0]);
        close(StdOutPipe[1]);
        close(StdErrPipe[0]);
        close(StdErrPipe[1]);
        return -1;
    }
    else if (PID==0)
    {
        close(StdOutPipe[0]);
        close(StdErrPipe[0]);
        dup2(StdOutPipe[1], STDOUT_FILENO);
        dup2(StdErrPipe[1], STDERR_FILENO);

        close(StdOutPipe[1]);
        close(StdErrPipe[1]);

        size_t Count=Arguments.size()+1; // Add Command as argv[0]
        char** ArgumentsArr=new char*[Count+1]; // Add final nullptr
        for (size_t Pos=0; Pos<Count; Pos++)
        {
            string Argument=FileName_To_Native((Pos==0)?Command:Arguments[Pos-1]);

            ArgumentsArr[Pos]=new char[Argument.size()+1];
            memcpy(ArgumentsArr[Pos], Argument.c_str(), Argument.size());
            ArgumentsArr[Pos][Argument.size()]=0;
        }
        ArgumentsArr[Count]=nullptr;

        execvp(ArgumentsArr[0], ArgumentsArr);

        for (size_t Pos=0; Pos<Count; Pos++)
            delete[] ArgumentsArr[Pos];
        delete[] ArgumentsArr;
        _exit(1);
    }
    else {
        close(StdOutPipe[1]);
        close(StdErrPipe[1]);

        char Buf[128];
        if (StdOut)
        {
            for (;;)
            {
                auto OutReadCount=read(StdOutPipe[0], Buf, sizeof(Buf));
                if (!OutReadCount)
                    break;
                *StdOut+=Ztring().From_UTF8(Buf, OutReadCount);
            }
        }
        if (StdErr)
        {
            for (;;)
            {
                auto ErrReadCount=read(StdErrPipe[0], Buf, sizeof(Buf));
                if (!ErrReadCount)
                    break;
                *StdErr+=Ztring().From_UTF8(Buf, ErrReadCount);
            }
        }

        close(StdOutPipe[0]);
        close(StdErrPipe[0]);

        waitpid(PID, &ExitCode, 0);
    }
    #endif //WINDOWS
    #endif //WINDOWS_UWP

    return ExitCode;
}

} //Namespace
