#!/bin/sh
UTILS_PATH=$(dirname "$0")

# messages on the console of make check (file descriptor 9, see AM_TESTS_FD_REDIRECT), else on the standard output
FD=1
if (command exec >&9) ; then
    FD=9
fi >/dev/null 2>&1

# directory for the outputs of the test, removed at its end except if the test failed
TEMP_PATH=$(mktemp -d -t 'mediainfolib_testsuite.XXXXXX') || exit 99
trap 'rm -rf "$TEMP_PATH"' EXIT

enabled()
{
    if test -z "${RUN_TESTS+x}" ; then
        return 0
    fi

    local test="$1"

    set -- $RUN_TESTS
    while test $# -gt 0 ; do
        if test "$1" = "$test" || test "$1" = "all" ; then
            return 0
        fi
        shift
    done

    return 1
}

# end of the test with the reason ${1} of its failure, followed by the first lines of the messages ${2} of the failed tool
fail()
{
    printf '%s\n' "NOK: $(basename "$0" .sh), $1" >&$FD
    if test -n "$2"
    then
        printf '%s\n' "$2" | head -n 20 >&$FD
    fi
    trap - EXIT
    printf '%s\n' "outputs of the test kept in $TEMP_PATH" >&$FD
    exit 1
}

analyze()
{
    local output
    output=$("$UTILS_PATH/mil_analyze" "$@" 2>&1)
    local status=$?
    if test $status -ne 0
    then
        fail "mil_analyze $*, exit code $status" "$output"
    fi
}

file_contains()
{
    grep -q "$2" "$1" || fail "$1 has no line matching $2"
}

xml_is_correct()
{
    local output
    output=$(xmllint --noout "$1" 2>&1) || fail "$1 is not well-formed XML" "$output"
}

json_is_correct()
{
    local output
    output=$(jsonlint -q "$1" 2>&1) || fail "$1 is not correct JSON" "$output"
}

# schema ${2} downloaded from ${3} at its first use, not kept if its download failed
xml_is_valid()
{
    if [ ! -f "$UTILS_PATH/$2" ]
    then
        local download
        if ! download=$(wget "$3" -O "$UTILS_PATH/$2" 2>&1)
        then
            rm -f "$UTILS_PATH/$2"
            fail "download of $3 failed" "$download"
        fi
    fi
    local output
    output=$(xmllint --noout --schema "$UTILS_PATH/$2" "$1" 2>&1) || fail "$1 is not valid against $2" "$output"
}

output_xml_is_a_valid_mi()
{
    xml_is_valid "$1" mediainfo.xsd https://mediaarea.net/mediainfo/mediainfo_2_0.xsd
}

output_xml_is_a_valid_mt()
{
    xml_is_valid "$1" mediatrace.xsd https://github.com/MediaArea/MediaAreaXml/raw/master/mediatrace.xsd
}

output_xml_is_a_valid_mmt()
{
    xml_is_valid "$1" micromediatrace.xsd https://github.com/MediaArea/MediaAreaXml/raw/master/micromediatrace.xsd
}

output_xml_is_a_valid_ma()
{
    xml_is_valid "$1" mediaarea.xsd https://github.com/MediaArea/MediaAreaXml/raw/master/mediaarea.xsd
}
