#! /bin/sh

##################################################################

Parallel_Make () {
    local numprocs=1
    case $OS in
    'linux'|'emscripten')
        numprocs=`nproc`
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
Emscripten_LDFLAGS=""
Emscripten_Bind_LDFLAGS=""
Emscripten_CXX_STANDARD=""
emscripten_target="both"

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
    case "$1" in
    --asmjs|--wasm)
        emscripten_target="$1"
        shift
        ;;
    esac
    OS="emscripten"
    Make="emmake make"
    CFLAGS="$CFLAGS -Oz"
    CXXFLAGS_JavaScript="$CXXFLAGS -fno-exceptions"
    CXXFLAGS="$CXXFLAGS_JavaScript -Oz"
    Emscripten_LDFLAGS="-s EMBIND_STD_STRING_IS_UTF8=1 -s DYNAMIC_EXECUTION=0"
    case "$(em++ --version | head -n 1)" in
    *" 1."*)
        Emscripten_Bind_LDFLAGS="--bind"
        Emscripten_CXX_STANDARD="-std=c++11"
        ;;
    *)
        Emscripten_Bind_LDFLAGS="-lembind"
        Emscripten_CXX_STANDARD="-std=c++17"
        ;;
    esac
    MediaInfoLib_CXXFLAGS="-I ../../../Source -I ../../../../ZenLib/Source -s USE_ZLIB=1 \
                           -DMEDIAINFO_ADVANCED_NO \
                           -DMEDIAINFO_MINIMAL_YES \
                           -DMEDIAINFO_EXPORT_YES \
                           -DMEDIAINFO_SEEK_YES \
                           -DMEDIAINFO_CONFORMANCE_YES \
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
            exit 1
        fi
    else
        echo Problem while configuring ZenLib
        exit 1
    fi
else
    echo ZenLib directory is not found
    exit 1
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
            exit 1
        fi
    else
        echo Problem while configuring MediaInfoLib
        exit 1
    fi
else
    echo MediaInfoLib directory is not found
    exit 1
fi
cd $Home

##################################################################
# JavaScript Interface
if [ "$OS" = "emscripten" ]; then
    cd MediaInfoLib/Project/GNU/Library/
    if [ "$emscripten_target" != "--wasm" ]; then
        rm -f MediaInfo.js MediaInfo.js.mem
    fi
    if [ "$emscripten_target" != "--asmjs" ]; then
        rm -f MediaInfoWasm.js MediaInfoWasm.wasm
    fi
    rm -f MediaInfoJS.o

    em++ $Emscripten_CXX_STANDARD $CXXFLAGS $MediaInfoLib_CXXFLAGS -c ../../../Source/MediaInfoDLL/MediaInfoJS.cpp -o MediaInfoJS.o
    if test -e MediaInfoJS.o; then
        echo MediaInfoLib JavaScript binding compiled
    else
        echo Problem while compiling MediaInfoLib JavaScript binding
        exit 1
    fi

    if [ "$emscripten_target" != "--wasm" ]; then
        # asm.js: -O2, as with -Os or -Oz Emscripten 1.x removes the imports which are not used by the WebAssembly code before converting it to
        # asm.js, which then needs getTempRet0()
        if ! em++ -s WASM=0 $CXXFLAGS_JavaScript $Emscripten_LDFLAGS -O2 $MediaInfoLib_CXXFLAGS -s TOTAL_MEMORY=134217728 -s NO_FILESYSTEM=1 -s MODULARIZE=1 --closure 0 \
             $Emscripten_Bind_LDFLAGS MediaInfoJS.o .libs/libmediainfo.a ../../../../ZenLib/Project/GNU/Library/.libs/libzen.a ../../../../Shared/Source/zlib/libz.a \
             --post-js ../../../Source/Resource/JavaScript/Post.js \
             -s EXPORT_NAME="'MediaInfoLib'" \
             -o MediaInfo.js; then
            echo Problem while compiling MediaInfoLib asm.js interface
            exit 1
        fi
    fi

    if [ "$emscripten_target" != "--asmjs" ]; then
        if ! em++ -s WASM=1 $CXXFLAGS $Emscripten_LDFLAGS $MediaInfoLib_CXXFLAGS -s TOTAL_MEMORY=33554432 -s ALLOW_MEMORY_GROWTH=1 -s NO_FILESYSTEM=1 -s MODULARIZE=1 --closure 0 \
             $Emscripten_Bind_LDFLAGS MediaInfoJS.o .libs/libmediainfo.a ../../../../ZenLib/Project/GNU/Library/.libs/libzen.a ../../../../Shared/Source/zlib/libz.a \
             --post-js ../../../Source/Resource/JavaScript/Post.js \
             -s EXPORT_NAME="'MediaInfoLib'" \
             -o MediaInfoWasm.js; then
            echo Problem while compiling MediaInfoLib WebAssembly interface
            exit 1
        fi
    fi

    if { [ "$emscripten_target" = "--wasm" ] && test -e MediaInfoWasm.js; } || \
       { [ "$emscripten_target" = "--asmjs" ] && test -e MediaInfo.js; } || \
       { [ "$emscripten_target" = "both" ] && test -e MediaInfo.js && test -e MediaInfoWasm.js; }; then
        echo MediaInfoLib JavaScript interface compiled
    else
        echo Problem while compiling MediaInfoLib JavaScript interface
        exit 1
    fi

    echo "MediaInfoLib JavaScript interface is in MediaInfoLib/Project/GNU/Library"
    cd $Home
    exit
fi

##################################################################

echo "MediaInfo shared object is in MediaInfoLib/Project/GNU/Library/.libs"
echo "For installing ZenLib, cd ZenLib/Project/GNU/Library && make install"
echo "For installing MediaInfoLib, cd MediaInfoLib/Project/GNU/Library && make install"

unset -v Home ZenLib_Options JsOptions OS Emscripten_LDFLAGS Emscripten_Bind_LDFLAGS Emscripten_CXX_STANDARD emscripten_target
