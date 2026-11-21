#!/bin/sh

# CLI of MediaInfo (branch of the same name if any) with this library, its file name test and files which can not be opened, for the modes ${1}...:
# - shared: C++ API of the installed library (same Unicode mode)
# - dll: C API of the installed library (with or without Unicode, the same results), loaded at run time, by a CLI with Unicode
# - dll_without_unicode: same, by a CLI without Unicode
# (for the dll modes, ZenLib is built again for the CLI if it has not the Unicode mode of the CLI, so after shared)
# The libraries are installed before (the CLI and the test load them from the system directories), MAKE and SUDO may be set

set -e
make="${MAKE:-make}"

# CLI of the current directory with a file which does not exist and a file without read permission (if the permission applies,
# not for root): exit code 1 and error message of the library on the error output
check_not_opened() {
    temp_path="$(mktemp -d)"
    : >"${temp_path}/unreadable.mp4"
    chmod 000 "${temp_path}/unreadable.mp4"
    result=0
    for file in missing unreadable ; do
        if [ "${file}" = "missing" ] ; then
            message="E: File not found: ${temp_path}/missing.mp4"
        elif [ -r "${temp_path}/unreadable.mp4" ] ; then
            echo "SKIP: file without read permission, readable anyway"
            continue
        else
            message="E: File can not be opened: ${temp_path}/unreadable.mp4"
        fi
        status=0
        ./mediainfo "${temp_path}/${file}.mp4" >/dev/null 2>"${temp_path}/stderr" || status=$?
        if [ "${status}" = "1" ] && grep -F -x -q "${message}" "${temp_path}/stderr" ; then
            echo "OK: ${file} file"
        else
            echo "NOK: ${file} file, exit code ${status}, error output: $(cat "${temp_path}/stderr")"
            result=1
        fi
    done
    chmod 600 "${temp_path}/unreadable.mp4"
    rm -rf "${temp_path:?}"
    return ${result}
}

if [ ! -d ../MediaInfo ] ; then
    sh .github/clone_branch.sh MediaInfo ../MediaInfo
fi

for mode in "${@}" ; do
    echo "CLI, ${mode}"
    case "${mode}" in
        shared)
            options=
            ;;
        dll|dll_without_unicode)
            unicode=yes
            if [ "${mode}" = dll_without_unicode ] ; then
                unicode=no
            fi
            options=--with-dll
            if [ "$(../ZenLib/Project/GNU/Library/libzen-config Unicode)" != "${unicode}" ] ; then
                options="--with-dll --enable-staticlibs" # ZenLib built again only as a static library
                git -C ../ZenLib clean -fdxq
                ( cd ../ZenLib/Project/GNU/Library && autoreconf -if && ./configure --enable-silent-rules --enable-unicode=${unicode} --enable-static --disable-shared && ${make} -j4 )
            fi
            ;;
        *)
            echo "unknown mode ${mode}"
            exit 1
            ;;
    esac
    ( cd ../MediaInfo/Project/GNU/CLI && autoreconf -if && ./configure --enable-silent-rules ${options} && ${make} -j4 && ${make} check )
    ( cd ../MediaInfo/Project/GNU/CLI && check_not_opened )
    git -C ../MediaInfo clean -fdxq
done
