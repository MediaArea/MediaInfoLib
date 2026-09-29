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

if [ -z "$JAVA_HOME" ] ; then
    echo "error: missing mandatory environment \$JAVA_HOME" >&2
    exit 1
fi

if [ -z "$ANDROID_HOME" ] ; then
    echo "error: missing mandatory environment \$ANDROID_HOME" >&2
    exit 1
fi

#-----------------------------------------------------------------------
# Clean
rm -fr "$releasedir"/../Source/AndroidLib/mediainfolib/.cxx
rm -fr "$releasedir"/../Source/AndroidLib/mediainfolib/build
rm -f "$releasedir"/MediaInfo_DLL_*_Android.aar

#-----------------------------------------------------------------------
# Build
pushd "$releasedir"/../Source/AndroidLib
    chmod +x ./gradlew
    ./gradlew assembleRelease
popd

#-----------------------------------------------------------------------
# Package
cp "$releasedir"/../Source/AndroidLib/mediainfolib/build/outputs/aar/mediainfolib-release.aar "$releasedir"/MediaInfo_DLL_${VERSION}_Android.aar
