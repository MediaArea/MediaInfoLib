#! /bin/sh

##################################################################

Parallel_Make () {
    local numprocs=1
    case $OS in
    'linux')
        numprocs=`grep -c ^processor /proc/cpuinfo 2>/dev/null`
        ;;
    'mac')
        if type sysctl &> /dev/null; then
            numprocs=`sysctl -n hw.ncpu`
        fi
        ;;
    #"solaris')
    #    on Solaris you need to use psrinfo -p instead
    #    ;;
    #'freebsd')
    #    ;;
    *) ;;
    esac
    if [ "$numprocs" = "" ] || [ "$numprocs" = "0" ]; then
        numprocs=1
    fi
   $Make -s -j$numprocs
}

##################################################################
# Init

Home=`pwd`
Make="make"
ZenLib_Options=""
MacOptions="--with-macosx-version-min=10.5"
JsOptions="--host=le32-unknown-nacl"

OS=$(uname -s)
# expr isn’t available on mac
if [ "$OS" = "Darwin" ]; then
    OS="mac"
# if the 5 first caracters of $OS equal "Linux"
elif [ "$(expr substr $OS 1 5)" = "Linux" ]; then
    OS="linux"
#elif [ "$(expr substr $OS 1 5)" = "SunOS" ]; then
#    OS="solaris"
#elif [ "$(expr substr $OS 1 7)" = "FreeBSD" ]; then
#    OS="freebsd"
fi

if [ "$1" = "--emscripten-lib" ]; then
    shift
    OS="emscripten"
    Make="emmake make"
    CFLAGS="$CFLAGS -Oz -DUNICODE"
    CXXFLAGS="$CXXFLAGS -Oz -DUNICODE -fno-exceptions"
    MediaInfoLib_CXXFLAGS="-I ../../../Source -I ../../../../ZenLib/Source -s USE_ZLIB=1 \
    -DMEDIAINFO_ADVANCED_NO -DMEDIAINFO_REFERENCES_NO -DMEDIAINFO_FILTER_NO -DMEDIAINFO_DUPLICATE_NO -DMEDIAINFO_MACROBLOCKS_NO \
    -DMEDIAINFO_TRACE_NO -DMEDIAINFO_TRACE_FFV1CONTENT_NO -DMEDIAINFO_IBI_NO -DMEDIAINFO_DIRECTORY_NO -DMEDIAINFO_JNI_NO\
    -DMEDIAINFO_LIBCURL_NO -DMEDIAINFO_LIBMMS_NO -DMEDIAINFO_DVDIF_ANALYZE_NO -DMEDIAINFO_MPEGTS_DUPLICATE_NO \
    -DMEDIAINFO_READTHREAD_NO -DMEDIAINFO_MD5_NO -DMEDIAINFO_SHA1_NO -DMEDIAINFO_SHA2_NO -DMEDIAINFO_EVENTS_NO \
    -DMEDIAINFO_DEMUX_NO -DMEDIAINFO_AES_NO -DMEDIAINFO_FIXITY_NO -DMEDIAINFO_READER_NO -DMEDIAINFO_NEXTPACKET_NO"
fi

##################################################################
# ZenLib

if test -e ZenLib/Project/GNU/Library/configure; then
    cd ZenLib/Project/GNU/Library/
    test -e Makefile && rm Makefile
    chmod +x configure

    if [ "$OS" = "emscripten" ]; then
        emconfigure ./configure --enable-static --disable-shared $JsOptions $ZenLib_Options $* CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS"
    elif [ "$OS" = "mac" ]; then
        ./configure --enable-static --disable-shared $MacOptions $ZenLib_Options $*
    else
        ./configure --enable-static --disable-shared $ZenLib_Options $*
    fi
    if test -e Makefile; then
        make clean
        Parallel_Make
        if test -e libzen.la; then
            echo ZenLib compiled
        else
            echo Problem while compiling ZenLib
            exit
        fi
    else
        echo Problem while configuring ZenLib
        exit
    fi
else
    echo ZenLib directory is not found
    exit
fi
cd $Home

##################################################################
# MediaInfoLib

if test -e MediaInfoLib/Project/GNU/Library/configure; then
    cd MediaInfoLib/Project/GNU/Library/
    test -e Makefile && rm Makefile
    chmod +x configure
    if [ "$OS" = "emscripten" ]; then
        emconfigure ./configure --enable-static --disable-shared --disable-dll $JsOptions $* CFLAGS="$CFLAGS -s USE_ZLIB=1" CXXFLAGS="$CXXFLAGS $MediaInfoLib_CXXFLAGS"
    elif [ "$OS" = "mac" ]; then
        ./configure $MacOptions --enable-staticlibs --enable-shared --disable-static --with-libcurl=runtime $MediaInfoLib_Options $*
    else
        ./configure --enable-staticlibs --enable-shared --disable-static --with-libcurl=runtime $MediaInfoLib_Options $*
    fi
    if test -e Makefile; then
        make clean
        Parallel_Make
        if test -e libmediainfo.la; then
            echo MediaInfoLib compiled
        else
            echo Problem while compiling MediaInfoLib
            exit
        fi
    else
        echo Problem while configuring MediaInfoLib
        exit
    fi
else
    echo MediaInfoLib directory is not found
    exit
fi
cd $Home

##################################################################
# JavaScript Interface
cd MediaInfoLib/Project/GNU/Library/
if [ "$OS" = "emscripten" ]; then
    em++ $CXXFLAGS $MediaInfoLib_CXXFLAGS --bind -c ../../../Source/MediaInfoDLL/MediaInfoJS.cpp
    em++ $CXXFLAGS $MediaInfoLib_CXXFLAGS -s TOTAL_MEMORY=134217728 -s NO_FILESYSTEM=1 -s MODULARIZE=1 \
    --llvm-lto 0 --closure 1 --bind ../../../Source/MediaInfoDLL/MediaInfoJS.o .libs/libmediainfo.a \
    ../../../../ZenLib/Project/GNU/Library/.libs/libzen.a -o MediaInfo.js

    if test -e MediaInfo.js; then
        echo MediaInfoLib JavaScript interface compiled
    else
        echo Problem while compiling MediaInfoLib JavaScript interface
        exit
    fi

    mv MediaInfo.js MediaInfo_.js
    cat ../../../Source/Resource/JavaScript/Pre.js MediaInfo_.js ../../../Source/Resource/JavaScript/Post.js > MediaInfo.js
    rm MediaInfo_.js

    echo "MediaInfoLib JavaScript interface is in MediaInfoLib/Project/GNU/Library"
fi
cd $Home

##################################################################

echo "MediaInfo shared object is in MediaInfoLib/Project/GNU/Library/.libs"
echo "For installing ZenLib, cd ZenLib/Project/GNU/Library && make install"
echo "For installing MediaInfoLib, cd MediaInfoLib/Project/GNU/Library && make install"

unset -v Home ZenLib_Options MacOptions OS
