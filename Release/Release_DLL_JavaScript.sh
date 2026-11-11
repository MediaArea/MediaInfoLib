#!/bin/bash

##  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 #
 #  Use of this source code is governed by a BSD-style license that can
 #  be found in the License.html file in the root of the source tree.
 ##

set -e # fail on any error

#-----------------------------------------------------------------------
# Setup
release_directory="$(realpath -- "$(dirname -- "${BASH_SOURCE[0]}")")"
version="$(<"${release_directory}/../Project/version.txt")"

emsdk_asmjs="${EMSDK_ASMJS-1.38.48}"
emsdk_wasm="${EMSDK_WASM-2.0.18}"

#-----------------------------------------------------------------------
# Clean
rm -fr "${release_directory}/build"
rm -fr "${release_directory}/MediaInfo_JavaScript"
rm -fr "${release_directory}/MediaInfo_DLL_JavaScript"
rm -f "${release_directory}"/MediaInfo_DLL_*_JavaScript.*

mkdir "${release_directory}/build"
mkdir "${release_directory}/MediaInfo_JavaScript"
mkdir "${release_directory}/MediaInfo_DLL_JavaScript"

#-----------------------------------------------------------------------
# Install emsdk
pushd "${release_directory}/"
    git clone --depth=1 "https://github.com/emscripten-core/emsdk.git" build/emsdk
popd

#-----------------------------------------------------------------------
# Build
pushd "${release_directory}/"
    ./build/emsdk/emsdk install "${emsdk_asmjs}"
    ./build/emsdk/emsdk activate "${emsdk_asmjs}"
    (
        source "${release_directory}/build/emsdk/emsdk_env.sh"
        pushd "${release_directory}/../.."
            ./SO_Compile.sh --emscripten-lib
        popd
    )

    cp -a "${release_directory}/../Project/GNU/Library/MediaInfo.js.mem" "MediaInfo_JavaScript/MediaInfo-${version}.js.mem"
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfo.js" "MediaInfo_JavaScript/MediaInfo-${version}.js"
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfo.js.mem" "MediaInfo_DLL_JavaScript"
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfo.js" "MediaInfo_DLL_JavaScript"

    ./build/emsdk/emsdk install "${emsdk_wasm}"
    ./build/emsdk/emsdk activate "${emsdk_wasm}"
    (
        source "${release_directory}/build/emsdk/emsdk_env.sh"
        pushd "${release_directory}/../.."
            ./SO_Compile.sh --emscripten-lib
        popd
    )

    cp -a "${release_directory}/../Project/GNU/Library/MediaInfoWasm.wasm" "MediaInfo_JavaScript/MediaInfoWasm-${version}.wasm"
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfoWasm.js" "MediaInfo_JavaScript/MediaInfoWasm-${version}.js"
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfoWasm.wasm" "MediaInfo_DLL_JavaScript"
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfoWasm.js" "MediaInfo_DLL_JavaScript"

    # MediaInfoLite version
    sed -i.bak -e 's/MediaInfo\.js/MediaInfoLite.js/g' -e 's/MediaInfoWasm\.js/MediaInfoLiteWasm.js/g' "../../SO_Compile.sh"

    ./build/emsdk/emsdk install "${emsdk_asmjs}"
    ./build/emsdk/emsdk activate "${emsdk_asmjs}"
    (
        source "${release_directory}/build/emsdk/emsdk_env.sh"
        export CXXFLAGS="-DMEDIAINFO_ALL_NO -DMEDIAINFO_MIXML_YES"
        pushd "${release_directory}/../.."
            ./SO_Compile.sh --emscripten-lib
        popd
    )
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfoLite.js.mem" "MediaInfo_JavaScript/MediaInfoLite-${version}.js.mem"
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfoLite.js" "MediaInfo_JavaScript/MediaInfoLite-${version}.js"

    ./build/emsdk/emsdk install "${emsdk_wasm}"
    ./build/emsdk/emsdk activate "${emsdk_wasm}"
    (
        source "${release_directory}/build/emsdk/emsdk_env.sh"
        export CXXFLAGS="-DMEDIAINFO_ALL_NO -DMEDIAINFO_MIXML_YES"
        pushd "${release_directory}/../.."
            ./SO_Compile.sh --emscripten-lib
        popd
    )
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfoLiteWasm.wasm" "MediaInfo_JavaScript/MediaInfoLiteWasm-${version}.wasm"
    cp -a "${release_directory}/../Project/GNU/Library/MediaInfoLiteWasm.js" "MediaInfo_JavaScript/MediaInfoLiteWasm-${version}.js"

    mv -f ../../SO_Compile.sh.bak ../../SO_Compile.sh
popd

#-----------------------------------------------------------------------
# Package
pushd "${release_directory}/"
    cp -a "${release_directory}/../Source/Example/HowToUse_Dll.html" "MediaInfo_DLL_JavaScript"
    cp -a "${release_directory}/../History_DLL.txt" "MediaInfo_DLL_JavaScript"
    cp -a "${release_directory}/../Changes.txt" "MediaInfo_DLL_JavaScript"
    cp -a "${release_directory}/../License.html" "MediaInfo_DLL_JavaScript"
    cp -a "${release_directory}/../Project/JavaScript/package.json" "MediaInfo_DLL_JavaScript"
    cp -a "${release_directory}/../Project/JavaScript/ReadMe.txt" "MediaInfo_DLL_JavaScript"

    zip -q -r "MediaInfo_DLL_${version}_JavaScript.zip" "MediaInfo_DLL_JavaScript"
    GZIP=-9 tar -cz --owner=root --group=root -f "MediaInfo_DLL_${version}_JavaScript.tar.gz" "MediaInfo_DLL_JavaScript"
    BZIP=-9 tar -cj --owner=root --group=root -f "MediaInfo_DLL_${version}_JavaScript.tar.bz2" "MediaInfo_DLL_JavaScript"
    XZ_OPT=-9e tar -cJ --owner=root --group=root -f "MediaInfo_DLL_${version}_JavaScript.tar.xz" "MediaInfo_DLL_JavaScript"
popd