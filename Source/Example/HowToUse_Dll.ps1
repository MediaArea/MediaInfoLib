#  Copyright (c) MediaArea.net SARL. All Rights Reserved.
#
#  Use of this source code is governed by a BSD-style license that can
#  be found in the License.html file in the root of the source tree.

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
# PowerShell example
#
# To make this example working, you must put Example.ogg
# in the working directory or pass a file via command line argument
# when executing this script.
# 
# Change $MediaInfoLibDllWrapperPathto the actual path to MediaInfoDLL.cs
#
# Additionally,
# on Windows, MediaInfo.dll must be in the working directory or PATH while
# on Linux, libmediainfo-dev must be installed.
#
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

$MediaInfoLibDllWrapperPath = "$PSScriptRoot\Developers\Source\MediaInfoDLL\MediaInfoDLL.cs"

# Load and compile the C# MediaInfoLib wrapper
try {
    Add-Type -LiteralPath $MediaInfoLibDllWrapperPath
} catch {
    Write-Error "Failed to load MediaInfo DLL wrapper. Please check the path and dependencies."
    Write-Error $_.Exception.Message
    exit 1
}

# Define a variable for the StreamKind enumeration so that it can be used with a shorter name
$StreamKind = [MediaInfoLib.StreamKind]

# Get command line argument (if provided)
if ($Args.Count -gt 0) {
    $FileName = $Args[0]
} else {
    # Replace 'Example.ogg' with a file path you want to test if no argument is provided.
    $FileName = "Example.ogg"
}

# Initialize the MediaInfo object (similar to C# 'MediaInfo MI = new MediaInfo();')
# '$MI = [MediaInfoLib.MediaInfo]::new()' is an alternative method
$MI = New-Object -TypeName MediaInfoLib.MediaInfo

# Set encoding to UTF-8
$MI.Option("CharSet", "UTF-8")

# Variable to accumulate output
$ToDisplay = ""

# Display MediaInfoLib version
$ToDisplay = $MI.Option("Info_Version");

# Information about MediaInfo
$ToDisplay += "`r`n`r`nInfo_Parameters`r`n"
$ToDisplay += $MI.Option("Info_Parameters")

$ToDisplay += "`r`n`r`nInfo_Capacities`r`n"
$ToDisplay += $MI.Option("Info_Capacities")

$ToDisplay += "`r`n`r`nInfo_Codecs`r`n"
$ToDisplay += $MI.Option("Info_Codecs")

# An example of how to use the library
$ToDisplay += "`r`n`r`nOpen`r`n"
$FilesOpened = $MI.Open($FileName)

# Check if file was opened successfully
if ($FilesOpened -ne 1) {
    Write-Error "Error opening file" -ErrorAction Stop
}

$ToDisplay += "`r`n`r`nInform with Complete=false`r`n"
$MI.Option("Complete")
$ToDisplay += $MI.Inform()

$ToDisplay += "`r`n`r`nInform with Complete=true`r`n"
$MI.Option("Complete", "1")
$ToDisplay += $MI.Inform()

$ToDisplay += "`r`n`r`nCustom Inform`r`n"
$MI.Option("Inform", "General;File size is %FileSize% bytes")
$ToDisplay += $MI.Inform()

$ToDisplay += "`r`n`r`nGet with Stream=General and Parameter='FileSize'`r`n"
$ToDisplay += $MI.Get($StreamKind::General, 0, "FileSize")

$ToDisplay += "`r`n`r`nGet with Stream=General and Parameter=90`r`n"
$ToDisplay += $MI.Get($StreamKind::General, 0, 90)

$ToDisplay += "`r`n`r`nCount_Get with StreamKind=Stream_Audio`r`n"
$ToDisplay += $MI.Count_Get($StreamKind::Audio)

$ToDisplay += "`r`n`r`nGet with Stream=General and Parameter='AudioCount'`r`n"
$ToDisplay += $MI.Get($StreamKind::General, 0, "AudioCount")

$ToDisplay += "`r`n`r`nGet with Stream=Audio and Parameter='StreamCount'`r`n"
$ToDisplay += $MI.Get($StreamKind::Audio, 0, "StreamCount")

$ToDisplay += "`r`n`r`nClose`r`n"
$MI.Close()

# Displaying the text
Write-Output $ToDisplay
