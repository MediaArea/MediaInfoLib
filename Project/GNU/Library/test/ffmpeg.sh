#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
. "$PATH_SCRIPT/utils.sh"

if ! enabled ffmpeg ; then
    exit 77 # Skip test
fi

# FFmpeg run with the FFmpeg option for the images and videos: ffmpeg-ma, else ffmpeg, of the first entry of PATH which
# has it, an empty entry being the current directory, here programs which write their name in a file "run" next to them,
# in "bin" and in the current directory "cwd" (Windows: also in "bin;quoted")
# Windows (Git Bash, which changes the PATH of the programs it starts): PATH set by mil_ffmpeg of the MSBuild job, whose
# copies are the FFmpeg programs, entries separated by ";"

case "$(uname -s)" in
    CYGWIN*|MINGW*|MSYS*)
        windows=1
        analyzer="$(cd "$PATH_SCRIPT" && pwd)/mil_ffmpeg.exe"
        test -f "$analyzer" || exit 77 # Skip test, program of the MSBuild job only
        temp=$(cygpath -m "$TEMP_PATH") # same names for Git Bash and the programs
        sep=';'
        ;;
    *)
        windows=
        analyzer="$(cd "$PATH_SCRIPT" && pwd)/mil_analyze"
        temp="$TEMP_PATH"
        sep=:
        ;;
esac
dirs="bin cwd"
if test -n "$windows" ; then
    dirs="$dirs bin;quoted" # name with the separator, for the entries in quotes
fi
for dir in $dirs ; do
    mkdir "$temp/$dir" || exit 99
    if test -n "$windows" ; then
        cp "$analyzer" "$temp/$dir/ffmpeg-ma.exe" || exit 99
    else
        printf '#!/bin/sh\necho "$0" > "${0%%/*}/run"\n' > "$temp/$dir/ffmpeg-ma" || exit 99
        chmod +x "$temp/$dir/ffmpeg-ma" || exit 99
    fi
done

# BMP image of 1x1 pixel
{
    printf 'BM\072\000\000\000\000\000\000\000\066\000\000\000' # file header: 58 bytes, pixels at 54
    printf '\050\000\000\000\001\000\000\000\001\000\000\000\001\000\030\000' # header of 40 bytes: 1x1, 1 plane, 24 bits
    printf '\000\000\000\000\004\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000' # 4 bytes of pixels
    printf '\000\000\000\000' # pixel and padding
} > "$temp/image.bmp" || exit 99

# PATH ${1} ("unset" to remove it), ffmpeg-ma of ${2} run ("none" for none of them)
check()
{
    for dir in $dirs ; do
        rm -f "$temp/$dir/run"
    done
    if test -n "$windows" ; then
        output=$(cd "$temp/cwd" && "$analyzer" "$1" ../image.bmp 2>&1)
    elif test "$1" = unset ; then
        output=$(cd "$temp/cwd" && unset PATH && "$analyzer" -e Enable_FFmpeg:1 ../image.bmp ../output.xml 2>&1)
    else
        output=$(cd "$temp/cwd" && PATH="$1" "$analyzer" -e Enable_FFmpeg:1 ../image.bmp ../output.xml 2>&1)
    fi
    status=$?
    test $status -eq 0 || fail "PATH $1, ${analyzer##*/} exit code $status" "$output"
    for dir in $dirs ; do
        if test -f "$temp/$dir/run" ; then
            test "$dir" = "$2" || fail "PATH $1, $dir/ffmpeg-ma run"
        else
            test "$dir" != "$2" || fail "PATH $1, $dir/ffmpeg-ma not run" "$output"
        fi
    done
}

check "$temp/none$sep$temp/bin" bin # last entry, without separator after it
check "$temp/none" none # no empty entry
check "$temp/none$sep$sep$temp/bin" cwd # empty entries
check "$sep$temp/bin" cwd
if test -n "$windows" ; then
    check "$temp/none$sep" none # separator at the end, ignored as by SearchPath()
else
    check "$temp/none$sep" cwd
fi
check "" none # PATH empty or not set
check unset none

# Windows: entries in quotes, as for cmd.exe (separators between quotes in the entry, quotes removed)
if test -n "$windows" ; then
    check "$temp/none;\"$temp/bin\"" bin
    check "$temp/none;\"$temp/bin;quoted\";$temp/none" "bin;quoted"
    check "$temp/none;$temp/\"bin;quoted\"" "bin;quoted" # part of an entry
    check "$temp/none;\"$temp/bin" bin # quote not closed
fi
