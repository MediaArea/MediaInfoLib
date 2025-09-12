#!/bin/bash

PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"

export IPHONEOS_DEPLOYMENT_TARGET="12.0"

build_lib()
{
    target=$1
    platform=$2
    prefix=$3

    host=$target

    export CC="$(xcrun -sdk iphoneos -find clang)"
    export CXX="$(xcrun -sdk iphoneos -find clang++)"
    export AR="$(xcrun -sdk iphoneos -find ar)"
    export RANLIB="$(xcrun -sdk iphoneos -find ranlib)"
    export CFLAGS="-stdlib=libc++ -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform.sdk -miphoneos-version-min=$IPHONEOS_DEPLOYMENT_TARGET"
    export CXXFLAGS="-stdlib=libc++ -arch ${target}  -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform.sdk -miphoneos-version-min=$IPHONEOS_DEPLOYMENT_TARGET"
    export LDFLAGS="-stdlib=libc++ -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform.sdk"

    pushd "$(dirname "${BASH_SOURCE[0]}")/../GNU/Library"
        ./configure --prefix="$prefix" --disable-shared --enable-static --host=$host-apple-darwin
        make clean
        make
        make install
    popd
}

build_lib arm64 iPhoneOS "$PWD/native/arm64"
build_lib x86_64 iPhoneSimulator "$PWD/native/x86_64"

LIPO=$(xcrun -sdk iphoneos -find lipo)

$LIPO -create $PWD/native/arm64/lib/libmediainfo.a $PWD/native/x86_64/lib/libmediainfo.a -output $PWD/libmediainfo.a
