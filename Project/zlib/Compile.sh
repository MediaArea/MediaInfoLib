#! /bin/sh

# Initialization
test -d ../../Source || mkdir -p ../../Source
zlib_source=../../Source/zlib

##########################################################################
# Fetch if necessary
if test -e $zlib_source/configure; then
    echo
    echo The source of zlib are presents
    echo
else
    echo
    echo Downloading zlib...
    echo
    rm -fr $zlib_source
    git clone -b "v1.2.8" https://github.com/madler/zlib $zlib_source
    if test -e $zlib_source/configure; then
        echo
        echo zlib downloaded, compiling it
        echo
    else
        echo
        echo Error while downloading zlib
        echo
        exit 1
    fi
fi

##########################################################################
# Already compiled
if test -e $zlib_source/zlib/zlib.a || test -e $zlib_source/zlib/zlib.la; then
    echo
    echo zlib is already compiled, recompiling it
    echo
fi

##########################################################################
# Compile
cd $zlib_source
echo
echo Compiling zlib...
echo
./configure $*
make clean
make
# In the previous version:
    mkdir zlib
    cp zlib.h zlib
    cp zconf.h zlib

unset -v zlib_source
