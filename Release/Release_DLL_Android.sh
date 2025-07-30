#!/bin/bash

##  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 #
 #  Use of this source code is governed by a BSD-style license that can
 #  be found in the License.html file in the root of the source tree.
 ##

#-----------------------------------------------------------------------
# Helpers
set -e # fail on any error
releasedir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

#-----------------------------------------------------------------------
# Used variables
if [ -z "$VERSION" ] ; then
    VERSION=$(<"$releasedir"/../Project/version.txt)
fi

if [ -z "$NDK" ] ; then
    echo "error: missing mandatory environment \$NDK" >&2
    exit 1
fi

if [ -z "$MINSDKVERSION" ] ; then
    MINSDKVERSION=24 # Android 7.0 needed for -D_FILE_OFFSET_BITS=64
fi

#-----------------------------------------------------------------------
# Clean
rm -fr "$releasedir"/build
rm -fr "$releasedir"/MediaInfo_DLL_Android
rm -f "$releasedir"/MediaInfo_DLL_*_Android.zip

#-----------------------------------------------------------------------
# Build
abis=(arm64-v8a armeabi-v7a x86_64 x86)
for abi in "${abis[@]}" ; do
    mkdir -p "$releasedir"/build/$abi
    pushd "$releasedir"/build/$abi
        cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$NDK"/build/cmake/android.toolchain.cmake -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-$MINSDKVERSION -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON -DBUILD_SHARED_LIBS=1 -DBUILD_ZENLIB=1 "$releasedir"/../Project/CMake
        ninja
        mkdir -p "$releasedir"/MediaInfo_DLL_Android/lib/$abi
        cp libmediainfo.so "$releasedir"/MediaInfo_DLL_Android/lib/$abi
    popd
done

mkdir -p "$releasedir"/MediaInfo_DLL_Android/include/MediaInfo
cp -f  "$releasedir"/../Source/MediaInfo/MediaInfo.h "$releasedir"/MediaInfo_DLL_Android/include/MediaInfo
cp -f  "$releasedir"/../Source/MediaInfo/MediaInfoList.h "$releasedir"/MediaInfo_DLL_Android/include/MediaInfo
cp -f  "$releasedir"/../Source/MediaInfo/MediaInfo_Const.h "$releasedir"/MediaInfo_DLL_Android/include/MediaInfo
cp -f  "$releasedir"/../Source/MediaInfo/MediaInfo_Events.h "$releasedir"/MediaInfo_DLL_Android/include/MediaInfo
mkdir -p "$releasedir"/MediaInfo_DLL_Android/include/MediaInfoDLL
cp -f  "$releasedir"/../Source/MediaInfoDLL/MediaInfoDLL.h "$releasedir"/MediaInfo_DLL_Android/include/MediaInfoDLL
cp -f  "$releasedir"/../Source/MediaInfoDLL/MediaInfoDLL_Static.h "$releasedir"/MediaInfo_DLL_Android/include/MediaInfoDLL

#-----------------------------------------------------------------------
# Package
zip -r MediaInfo_DLL_${VERSION}_Android.zip MediaInfo_DLL_Android

