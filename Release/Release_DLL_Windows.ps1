##  Copyright (c) MediaArea.net SARL. All Rights Reserved.
##
##  Use of this source code is governed by a BSD-style license that can
##  be found in the License.html file in the root of the source tree.
##

Param([parameter(Mandatory=$true)][String]$arch)

$ErrorActionPreference = "Stop"

#-----------------------------------------------------------------------
# Setup
$release_directory = Split-Path -Parent $MyInvocation.MyCommand.Path
$version = (Get-Content "${release_directory}\..\Project\version.txt" -Raw).Trim()
$arch_alt="${arch}"
if ("${arch}" -eq "Win32" ) {
    $arch_alt="i386"
}

#-----------------------------------------------------------------------
# Cleanup
$artifact = "${release_directory}\MediaInfo_DLL_${version}_Windows_${arch_alt}_WithoutInstaller"
if (Test-Path "${artifact}") {
    Remove-Item -Force -Recurse "${artifact}"
}

$artifact = "${release_directory}\MediaInfo_DLL_${version}_Windows_${arch_alt}_WithoutInstaller.zip"
if (Test-Path "${artifact}") {
    Remove-Item -Force "${artifact}"
}

$artifact = "${release_directory}\MediaInfo_DLL_${version}_Windows_${arch_alt}_WithoutInstaller.7z"
if (Test-Path "${artifact}") {
    Remove-Item -Force "${artifact}"
}

$artifact = "${release_directory}\MediaInfo_DLL_${version}_Windows_${arch_alt}.exe"
if (Test-Path "${artifact}") {
    Remove-Item -Force "${artifact}"
}

#-----------------------------------------------------------------------
# Generate documentation
Push-Location "${release_directory}\..\Source\Doc"
    doxygen.exe
Pop-Location

#-----------------------------------------------------------------------
# Package DLL
Push-Location "${release_directory}"
    New-Item -ItemType Directory -Path "MediaInfo_DLL_${version}_Windows_${arch_alt}_WithoutInstaller"
    Push-Location "MediaInfo_DLL_${version}_Windows_${arch_alt}_WithoutInstaller"
        ### Copying: Documentation ###
        New-Item -Force -ItemType Directory "Developers"
        Copy-Item -Force "..\..\Source\Doc\*.html" "Developers"
        New-Item -Force -ItemType Directory "Developers\Doc"
        Copy-Item -Force "..\..\Doc\*" "Developers\Doc"
        New-Item -Force -ItemType Directory "Developers\List_Of_Parameters"
        Copy-Item -Force "..\..\Source\Resource\Text\Stream\*.csv" "Developers\List_Of_Parameters"
        ### Copying: Include ###
        New-Item -Force -ItemType Directory "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL-rs" "Developers\Source\MediaInfoDLL" -Recurse
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.h" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL_Static.h" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.def" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.pas" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.cs" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.jsl" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.vb" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.JNA.java" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.JNI.java" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.JNative.java" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL.py" "Developers\Source\MediaInfoDLL"
        Copy-Item -Force "..\..\Source\MediaInfoDLL\MediaInfoDLL3.py" "Developers\Source\MediaInfoDLL"
        ### Copying: Projects ###
        New-Item -Force -ItemType Directory "Developers\Project\BCB\Example"
        Copy-Item -Force "..\..\Project\BCB\Example\*.bpf" "Developers\Project\BCB\Example"
        Copy-Item -Force "..\..\Project\BCB\Example\*.bpr" "Developers\Project\BCB\Example"
        Copy-Item -Force "..\..\Project\BCB\Example\*.res*" "Developers\Project\BCB\Example"
        Copy-Item -Force "..\..\Project\BCB\Example\*.dfm" "Developers\Project\BCB\Example"
        Copy-Item -Force "..\..\Project\BCB\Example\*.h" "Developers\Project\BCB\Example"
        Copy-Item -Force "..\..\Project\BCB\Example\*.cpp" "Developers\Project\BCB\Example"
        New-Item -Force -ItemType Directory "Developers\Project\Delphi\Example"
        Copy-Item -Force "..\..\Project\Delphi\Example\*.dpr" "Developers\Project\Delphi\Example"
        Copy-Item -Force "..\..\Project\Delphi\Example\*.dfm" "Developers\Project\Delphi\Example"
        Copy-Item -Force "..\..\Project\Delphi\Example\*.res" "Developers\Project\Delphi\Example"
        Copy-Item -Force "..\..\Project\Delphi\Example\*.pas" "Developers\Project\Delphi\Example"
        Copy-Item -Force "..\..\Project\Delphi\Example\*.bdsproj" "Developers\Project\Delphi\Example"
        Copy-Item -Force "..\..\Project\Delphi\Example\*.bdsgroup" "Developers\Project\Delphi\Example"
        New-Item -Force -ItemType Directory "Developers\Project\MSCS2008"
        Copy-Item -Force "..\..\Project\MSCS2008\*.sln" "Developers\Project\MSCS2008"
        New-Item -Force -ItemType Directory "Developers\Project\MSCS2008\Example"
        Copy-Item -Force "..\..\Project\MSCS2008\Example\*.cs" "Developers\Project\MSCS2008\Example"
        Copy-Item -Force "..\..\Project\MSCS2008\Example\*.csproj" "Developers\Project\MSCS2008\Example"
        Copy-Item -Force "..\..\Project\MSCS2008\Example\*.res*" "Developers\Project\MSCS2008\Example"
        Copy-Item -Force "..\..\Project\MSCS2008\Example\*.ico" "Developers\Project\MSCS2008\Example"
        New-Item -Force -ItemType Directory "Developers\Project\MSCS2008\asp_net_web_application"
        Copy-Item -Force "..\..\Project\MSCS2008\asp_net_web_application\*.cs" "Developers\Project\MSCS2008\asp_net_web_application"
        Copy-Item -Force "..\..\Project\MSCS2008\asp_net_web_application\*.csproj" "Developers\Project\MSCS2008\asp_net_web_application"
        Copy-Item -Force "..\..\Project\MSCS2008\asp_net_web_application\*.aspx" "Developers\Project\MSCS2008\asp_net_web_application"
        Copy-Item -Force "..\..\Project\MSCS2008\asp_net_web_application\*.config" "Developers\Project\MSCS2008\asp_net_web_application"
        New-Item -Force -ItemType Directory "Developers\Project\MSCS2010"
        Copy-Item -Force "..\..\Project\MSCS2010\*.sln" "Developers\Project\MSCS2010"
        New-Item -Force -ItemType Directory "Developers\Project\MSCS2010\Example"
        Copy-Item -Force "..\..\Project\MSCS2010\Example\*.cs" "Developers\Project\MSCS2010\Example"
        Copy-Item -Force "..\..\Project\MSCS2010\Example\*.csproj" "Developers\Project\MSCS2010\Example"
        Copy-Item -Force "..\..\Project\MSCS2010\Example\*.res*" "Developers\Project\MSCS2010\Example"
        Copy-Item -Force "..\..\Project\MSCS2010\Example\*.ico" "Developers\Project\MSCS2010\Example"
        New-Item -Force -ItemType Directory "Developers\Project\MSCS2010\asp_net_web_application"
        Copy-Item -Force "..\..\Project\MSCS2010\asp_net_web_application\*.cs" "Developers\Project\MSCS2010\asp_net_web_application"
        Copy-Item -Force "..\..\Project\MSCS2010\asp_net_web_application\*.csproj" "Developers\Project\MSCS2010\asp_net_web_application"
        Copy-Item -Force "..\..\Project\MSCS2010\asp_net_web_application\*.aspx" "Developers\Project\MSCS2010\asp_net_web_application"
        Copy-Item -Force "..\..\Project\MSCS2010\asp_net_web_application\*.config" "Developers\Project\MSCS2010\asp_net_web_application"
        New-Item -Force -ItemType Directory "Developers\Project\MSJS"
        Copy-Item -Force "..\..\Project\MSJS\*.sln" "Developers\Project\MSJS"
        New-Item -Force -ItemType Directory "Developers\Project\MSJS\Example"
        Copy-Item -Force "..\..\Project\MSJS\Example\*.jsl" "Developers\Project\MSJS\Example"
        Copy-Item -Force "..\..\Project\MSJS\Example\*.vjsproj" "Developers\Project\MSJS\Example"
        Copy-Item -Force "..\..\Project\MSJS\Example\*.res*" "Developers\Project\MSJS\Example"
        New-Item -Force -ItemType Directory "Developers\Project\MSVB"
        Copy-Item -Force "..\..\Project\MSVB\*.sln" "Developers\Project\MSVB"
        New-Item -Force -ItemType Directory "Developers\Project\MSVB\Example"
        Copy-Item -Force "..\..\Project\MSVB\Example\*.vb" "Developers\Project\MSVB\Example"
        Copy-Item -Force "..\..\Project\MSVB\Example\*.vbproj" "Developers\Project\MSVB\Example"
        Copy-Item -Force "..\..\Project\MSVB\Example\*.res*" "Developers\Project\MSVB\Example"
        New-Item -Force -ItemType Directory "Developers\Project\MSVB\Example\My Project"
        Copy-Item -Force "..\..\Project\MSVB\Example\My Project\*.*" "Developers\Project\MSVB\Example\My Project"
        New-Item -Force -ItemType Directory "Developers\Project\MSVB\Example VB6"
        Copy-Item -Force "..\..\Project\MSVB\Example VB6\*.*" "Developers\Project\MSVB\Example VB6"
        New-Item -Force -ItemType Directory "Developers\Project\MSVC2022"
        Copy-Item -Force "..\..\Project\MSVC2022\*.sln" "Developers\Project\MSVC2022"
        New-Item -Force -ItemType Directory "Developers\Project\MSVC2022\Example"
        Copy-Item -Force "..\..\Project\MSVC2022\Example\HowToUse_Dll.vcxproj" "Developers\Project\MSVC2022\Example"
        Copy-Item -Force "..\..\Project\MSVC2022\Example\HowToUse_Dll.vcxproj.filters" "Developers\Project\MSVC2022\Example"
        New-Item -Force -ItemType Directory "Developers\Project\Java\Example.JNA"
        Copy-Item -Force "..\..\Project\Java\Example.JNA\*.java" "Developers\Project\Java\Example.JNA"
        Copy-Item -Force "..\..\Project\Java\Example.JNA\*.txt" "Developers\Project\Java\Example.JNA"
        Copy-Item -Force "..\..\Project\Java\Example.JNA\*.bat" "Developers\Project\Java\Example.JNA"
        Copy-Item -Force "..\..\Project\Java\Example.JNA\*.sh" "Developers\Project\Java\Example.JNA"
        New-Item -Force -ItemType Directory "Developers\Project\Java\Example.JNI"
        Copy-Item -Force "..\..\Project\Java\Example.JNI\*.bat" "Developers\Project\Java\Example.JNI"
        Copy-Item -Force "..\..\Project\Java\Example.JNI\*.sh" "Developers\Project\Java\Example.JNI"
        New-Item -Force -ItemType Directory "Developers\Project\Java\Example.JNative"
        Copy-Item -Force "..\..\Project\Java\Example.JNative\*.java" "Developers\Project\Java\Example.JNative"
        Copy-Item -Force "..\..\Project\Java\Example.JNative\*.txt" "Developers\Project\Java\Example.JNative"
        Copy-Item -Force "..\..\Project\Java\Example.JNative\*.bat" "Developers\Project\Java\Example.JNative"
        Copy-Item -Force "..\..\Project\Java\Example.JNative\*.sh" "Developers\Project\Java\Example.JNative"
        New-Item -Force -ItemType Directory "Developers\Project\NetBeans\Example.JNA"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNA\*.xml" "Developers\Project\NetBeans\Example.JNA"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNA\*.properties" "Developers\Project\NetBeans\Example.JNA"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNA\*.mf" "Developers\Project\NetBeans\Example.JNA"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNA\*.txt" "Developers\Project\NetBeans\Example.JNA"
        New-Item -Force -ItemType Directory "Developers\Project\NetBeans\Example.JNA\src"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNA\src\*.java" "Developers\Project\NetBeans\Example.JNA\src"
        New-Item -Force -ItemType Directory "Developers\Project\NetBeans\Example.JNative"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNative\*.xml" "Developers\Project\NetBeans\Example.JNative"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNative\*.properties" "Developers\Project\NetBeans\Example.JNative"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNative\*.mf" "Developers\Project\NetBeans\Example.JNative"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNative\*.txt" "Developers\Project\NetBeans\Example.JNative"
        New-Item -Force -ItemType Directory "Developers\Project\NetBeans\Example.JNative\src"
        Copy-Item -Force "..\..\Project\NetBeans\Example.JNative\src\*.java" "Developers\Project\NetBeans\Example.JNative\src"
        ### Copying: Contrib ###
        New-Item -Force -ItemType Directory "Developers/Contrib"
        Copy-Item -Force -Recurse "..\..\Contrib\*" "Developers\Contrib"
        ### Copying: Libs ###
        New-Item -Force -ItemType Directory "Developers\Project\MSVC2022\${arch}\Release"
        Copy-Item -Force "..\..\Project\MSVC2022\${arch}\Release\MediaInfo.lib" "Developers\Project\MSVC2022\${arch}\Release"
        New-Item -Force -ItemType Directory "Developers\Project\MSVC2022\${arch}\Debug"
        Copy-Item -Force "..\..\Project\MSVC2022\${arch}\Debug\MediaInfo.lib" "Developers\Project\MSVC2022\${arch}\Debug"
        if ($arch -eq "ARM64") {
            New-Item -Force -ItemType Directory "Developers\Project\MSVC2022\${arch}EC\Release"
            Copy-Item -Force "..\..\Project\MSVC2022\${arch}EC\Release\MediaInfo.lib" "Developers\Project\MSVC2022\${arch}EC\Release"
            New-Item -Force -ItemType Directory "Developers\Project\MSVC2022\${arch}EC\Debug"
            Copy-Item -Force "..\..\Project\MSVC2022\${arch}EC\Debug\MediaInfo.lib" "Developers\Project\MSVC2022\${arch}EC\Debug"
        }
        New-Item -Force -ItemType Directory "Developers\Source\Example"
        Copy-Item -Force "..\..\Source\Example\HowToUse_Dll-rs" "Developers\Source\Example" -Recurse
        Copy-Item -Force "..\..\Source\Example\HowToUse_Dll*.*" "Developers\Source\Example"
        New-Item -Force -ItemType Directory "Developers"
        Copy-Item -Force "..\Example.ogg" "Developers"
        ### Copying: Information files ###
        Copy-Item -Force "..\..\History_DLL.txt" "Developers\History.txt"
        Copy-Item -Force "..\..\License.html" "Developers"
        Copy-Item -Force "..\..\Changes.txt" "Developers"
        Copy-Item -Force "..\ReadMe_DLL_Windows.txt" "ReadMe.txt"
        ### Copying: DLL ###
        if ($arch -eq "ARM64") {
            Copy-Item -Force "..\..\Project\MSVC2022\${arch}EC\Release\MediaInfo.dll" .
        }
        else {
            Copy-Item -Force "..\..\Project\MSVC2022\${arch}\Release\MediaInfo.dll" .
        }
        Copy-Item -Force "..\..\Project\MSVC2022\${arch}\Release\MediaInfo_InfoTip.dll" .
        Copy-Item -Force "..\..\Project\MSVC2022\ShellExtension\*.bat" .
        ### Archive
        7za.exe a -r -t7z -mx9 "..\MediaInfo_DLL_${version}_Windows_${arch_alt}_WithoutInstaller.7z" *
        7za.exe a -r -tzip -mx9 "..\MediaInfo_DLL_${version}_Windows_${arch_alt}_WithoutInstaller.zip" *
    Pop-Location
Pop-Location

#-----------------------------------------------------------------------
# Package installer
Push-Location "${release_directory}"
    makensis.exe "..\Source\Install\MediaInfo_DLL_Windows_${arch_alt}.nsi"
Pop-Location
