#!/usr/bin/env bash

PATH_SCRIPT=$(dirname "$0")
. "$PATH_SCRIPT/utils.sh"
. "$PATH_SCRIPT/helpers.sh"

if ! enabled filename ; then
    exit 77 # Skip test
fi

test="filename"

# file names with non-ASCII characters with the narrow (e.g. a C program which doesn't call setlocale()) and wide
# (e.g. pymediainfo) C API, with an ASCII locale, a UTF-8 locale and other locales, with the UTF-8 option of the narrow API or not

for config in ${locale_configs} ; do
    for set_locale in 0 1 ; do
        for charset in 0 1 ; do
            directory="${temp_path}/${config}_${set_locale}_${charset}"
            mkdir "${directory}" || fatal "internal" "mkdir command failed"
            run_command "${config}" "${PATH_SCRIPT}/mil_filename" "${set_locale}" "${charset}" "${directory}"
            report "${config}/setlocale=${set_locale}/charset_utf8=${charset}"
        done
    done
done

# pymediainfo if installed (its Python process calls setlocale()), with the library of this build and the files of the
# program above: pymediainfo uses the wide API and the names can be encoded with these locales

python=
for command in python3 python ; do
    if command -v "${command}" >/dev/null 2>&1 && "${command}" -c 'import pymediainfo' >/dev/null 2>&1 ; then
        python="${command}"
        break
    fi
done

library=
for file in "${PATH_SCRIPT}/../.libs/libmediainfo.so" "${PATH_SCRIPT}/../.libs/libmediainfo.dylib" "${PATH_SCRIPT}/MediaInfo.dll" "${PATH_SCRIPT}/mediainfo.dll" ; do
    if [ -e "${file}" ] ; then
        library="$(cd "$(dirname "${file}")" && pwd)/$(basename "${file}")"
        if test -n "${WINDOWS}" ; then
            library="$(cygpath -m "${library}")"
        fi
        break
    fi
done

if [ -e "${PATH_SCRIPT}/../Makefile" ] && ! grep -q -e '-DUNICODE' "${PATH_SCRIPT}/../Makefile" ; then
    library= # pymediainfo uses the C API, which is only in the Unicode build
fi

if [ -n "${python}" ] && [ -n "${library}" ] ; then
    title=$'caf\xC3\xA9 \xE6\xB5\x8B\xE8\xAF\x95 \xF0\x9F\x8E\xB5'
    code='import sys; from pymediainfo import MediaInfo; Track=MediaInfo.parse(sys.argv[2], library_file=sys.argv[1]).general_tracks[0]; sys.exit(0 if Track.complete_name==sys.argv[2] and Track.title==sys.argv[3] else "wrong name or title: %a, %a" % (Track.complete_name, Track.title))'
    for config in ${locale_configs} ; do
        if [ "${config}" == "latin1" ] || [ "${config}" == "eucjp" ] || [ "${config}" == "gb18030" ] ; then
            continue # files with UTF-8 names only, not with these locales
        fi
        for path in "${temp_path}/${config}_1_0/files"/*.flac ; do
            printf -v shown '%q' "${path##*/}"
            expected_title="${title}"
            if [[ "${path}" == *_ascii_title.flac ]] ; then
                expected_title="Plain title"
            fi
            run_command "${config}" "${python}" -c "${code}" "${library}" "${path}" "${expected_title}"
            if [ "${cmd_status}" == "0" ] ; then
                echo "OK: ${test}/${config}/pymediainfo, ${shown}" >&${fd}
            else
                echo "NOK: ${test}/${config}/pymediainfo, ${shown}, exit code ${cmd_status}, ${cmd_stderr}" >&${fd}
                status=1
            fi
        done
    done

    # names which are not valid in the encoding of the locale, given by Python with the bytes kept (os.fsdecode() with "surrogateescape"),
    # with all the locales (the name in the documents of the library is valid UTF-8, so not checked, the title is in the code, as the
    # codecs of Python do not give all the bytes back, e.g. a byte 0x8B decoded as U+008B with EUC-JP)
    code='import sys; from pymediainfo import MediaInfo; Track=MediaInfo.parse(sys.argv[2], library_file=sys.argv[1]).general_tracks[0]; sys.exit(0 if Track.title=="caf\u00e9 \u6d4b\u8bd5 \U0001f3b5" else "wrong title: %a" % (Track.title,))'
    for config in ${locale_configs} ; do
        for path in "${temp_path}/${config}_1_0/list"/*.flac ; do
            printf -v shown '%q' "${path##*/}"
            run_command "${config}" "${python}" -c "${code}" "${library}" "${path}"
            if [ "${cmd_status}" == "0" ] ; then
                echo "OK: ${test}/${config}/pymediainfo, list/${shown}" >&${fd}
            else
                echo "NOK: ${test}/${config}/pymediainfo, list/${shown}, exit code ${cmd_status}, ${cmd_stderr}" >&${fd}
                status=1
            fi
        done
    done
fi

exit ${status}
