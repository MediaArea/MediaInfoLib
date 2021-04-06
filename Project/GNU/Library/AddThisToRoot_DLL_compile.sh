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
    CFLAGS="$CFLAGS -Oz -s EMBIND_STD_STRING_IS_UTF8=1"
    CXXFLAGS="$CXXFLAGS -Oz -s EMBIND_STD_STRING_IS_UTF8=1 -fno-exceptions"
    MediaInfoLib_CXXFLAGS="-I ../../../Source -I ../../../../ZenLib/Source -s USE_ZLIB=1 \
                           -DMEDIAINFO_ADVANCED_NO \
                           -DMEDIAINFO_MINIMAL_YES \
                           -DMEDIAINFO_EXPORT_YES \
                           -DMEDIAINFO_SEEK_YES \
                           -DMEDIAINFO_READER_NO \
                           -DMEDIAINFO_REFERENCES_NO"
fi

##################################################################
# ZenLib

if test -e ZenLib/Project/GNU/Library/configure; then
    cd ZenLib/Project/GNU/Library/
    test -e Makefile && rm Makefile
    chmod +x configure

    if [ "$OS" = "emscripten" ]; then
        emconfigure ./configure --host=le32-unknown-nacl --disable-unicode --enable-static --disable-shared CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS"
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
        emconfigure ./configure --host=le32-unknown-nacl --enable-static --disable-shared --disable-dll $* CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS $MediaInfoLib_CXXFLAGS"
    else
        ./configure --enable-staticlibs --enable-shared --disable-static --with-libcurl=runtime --with-graphviz=runtime $MediaInfoLib_Options $*
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
if [ "$OS" = "emscripten" ]; then
    cd MediaInfoLib/Project/GNU/Library/
    em++ $CXXFLAGS $MediaInfoLib_CXXFLAGS --bind -c ../../../Source/MediaInfoDLL/MediaInfoJS.cpp -o MediaInfoJS.o

    em++ -s WASM=0 $CXXFLAGS $MediaInfoLib_CXXFLAGS -s TOTAL_MEMORY=134217728 -s NO_FILESYSTEM=1 -s MODULARIZE=1 --closure 0 \
         --bind MediaInfoJS.o .libs/libmediainfo.a ../../../../ZenLib/Project/GNU/Library/.libs/libzen.a \
         --post-js ../../../Source/Resource/JavaScript/Post.js \
         -s EXPORT_NAME="'MediaInfoLib'" \
         -o MediaInfo.js

    em++ -s WASM=1 $CXXFLAGS $MediaInfoLib_CXXFLAGS -s TOTAL_MEMORY=33554432 -s ALLOW_MEMORY_GROWTH=1 -s NO_FILESYSTEM=1 -s MODULARIZE=1 --closure 0 \
         --bind MediaInfoJS.o .libs/libmediainfo.a ../../../../ZenLib/Project/GNU/Library/.libs/libzen.a \
         --post-js ../../../Source/Resource/JavaScript/Post.js \
         -s EXPORT_NAME="'MediaInfoLib'" \
         -o MediaInfoWasm.js

    if test -e MediaInfo.js; then
        echo MediaInfoLib JavaScript interface compiled
    else
        echo Problem while compiling MediaInfoLib JavaScript interface
        exit
    fi

    echo "MediaInfoLib JavaScript interface is in MediaInfoLib/Project/GNU/Library"
    cd $Home
    exit
fi

##################################################################

echo "MediaInfo shared object is in MediaInfoLib/Project/GNU/Library/.libs"
echo "For installing ZenLib, cd ZenLib/Project/GNU/Library && make install"
echo "For installing MediaInfoLib, cd MediaInfoLib/Project/GNU/Library && make install"

unset -v Home ZenLib_Options JsOptions OS
