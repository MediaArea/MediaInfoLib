#!/bin/sh

# MediaInfoLib/Project/Mac/mktarball.sh
# Make the archive of a MediaInfoLib release under Mac

# Copyright (c) MediaArea.net SARL. All Rights Reserved.
# Use of this source code is governed by a BSD-style license that
# can be found in the License.html file in the root of the source
# tree.

# $1 : version of the archive

echo
echo ========== Create the folder ==========
echo

# Clean up
rm -fr MediaInfoLib

mkdir MediaInfoLib
cp ../GNU/Library/.libs/libmediainfo.0.dylib MediaInfoLib
cd MediaInfoLib
ln -s libmediainfo.0.dylib libmediainfo.dylib
codesign -f -s "Developer ID Application: MediaArea.net" --verbose libmediainfo.dylib
cd ..

cp ../../License.html MediaInfoLib
cp ../../History_DLL.txt MediaInfoLib/History.txt
cp ../../Changes.txt MediaInfoLib
cp ../../Release/ReadMe_DLL_Mac.txt MediaInfoLib/ReadMe.txt
mkdir MediaInfoLib/Developpers
cp ../../Source/Doc/Documentation.html MediaInfoLib/Developpers
cp -r ../../Doc MediaInfoLib/Developpers

mkdir MediaInfoLib/Developpers/Source
cp -r ../../Source/Example MediaInfoLib/Developpers/Source
mkdir -p MediaInfoLib/Developpers/Include/MediaInfo
cp ../../Source/MediaInfo/MediaInfo.h MediaInfoLib/Developpers/Include/MediaInfo
cp ../../Source/MediaInfo/MediaInfo_Const.h MediaInfoLib/Developpers/Include/MediaInfo
cp ../../Source/MediaInfo/MediaInfo_Events.h MediaInfoLib/Developpers/Include/MediaInfo
cp ../../Source/MediaInfo/MediaInfoList.h MediaInfoLib/Developpers/Include/MediaInfo
mkdir -p MediaInfoLib/Developpers/Include/MediaInfoDLL
cp ../../Source/MediaInfoDLL/MediaInfoDLL.h MediaInfoLib/Developpers/Include/MediaInfoDLL
cp ../../Source/MediaInfoDLL/MediaInfoDLL_Static.h MediaInfoLib/Developpers/Include/MediaInfoDLL
cp ../../Source/MediaInfoDLL/MediaInfoDLL.cs MediaInfoLib/Developpers/Include/MediaInfoDLL
cp ../../Source/MediaInfoDLL/MediaInfoDLL.JNA.java MediaInfoLib/Developpers/Include/MediaInfoDLL
cp ../../Source/MediaInfoDLL/MediaInfoDLL.JNI.java MediaInfoLib/Developpers/Include/MediaInfoDLL
cp ../../Source/MediaInfoDLL/MediaInfoDLL.JNative.java MediaInfoLib/Developpers/Include/MediaInfoDLL
cp ../../Source/MediaInfoDLL/MediaInfoDLL.py MediaInfoLib/Developpers/Include/MediaInfoDLL
cp ../../Source/MediaInfoDLL/MediaInfoDLL3.py MediaInfoLib/Developpers/Include/MediaInfoDLL

echo
echo ========== Create the archive ==========
echo

(BZIP=-9 tar -cjf MediaInfo_DLL_$1_Mac_x86_64+arm64.tar.bz2 MediaInfoLib)
(XZ_OPT=-9e tar -cJf MediaInfo_DLL_$1_Mac_x86_64+arm64.tar.xz MediaInfoLib)

# Clean up
rm -fr MediaInfoLib
