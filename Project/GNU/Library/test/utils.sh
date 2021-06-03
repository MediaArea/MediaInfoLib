#!/bin/sh
UTILS_PATH=$(dirname "$0")

if [ ! -f "$UTILS_PATH/mediatrace.xsd" ]
then
    wget https://github.com/MediaArea/MediaAreaXml/raw/master/mediatrace.xsd -O "$UTILS_PATH/mediatrace.xsd"
fi

if [ ! -f "$UTILS_PATH/micromediatrace.xsd" ]
then
    wget https://github.com/MediaArea/MediaAreaXml/raw/master/micromediatrace.xsd -O "$UTILS_PATH/micromediatrace.xsd"
fi

if [ ! -f "$UTILS_PATH/mediainfo.xsd" ]
then
    wget https://mediaarea.net/mediainfo/mediainfo_2_0.xsd -O "$UTILS_PATH/mediainfo.xsd"
fi

if [ ! -f "$UTILS_PATH/mediaarea.xsd" ]
then
    wget https://github.com/MediaArea/MediaAreaXml/raw/master/mediaarea.xsd -O "$UTILS_PATH/mediaarea.xsd"
fi

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

cmd_is_ok()
{
    if test $? -ne 0
    then
        exit 1;
    fi
}

xml_is_correct()
{
    $(xmllint --noout "$1" 2> /dev/null)
    if test $? -ne 0
    then
        exit 1;
    fi
}

json_is_correct()
{
    $(jsonlint -q "$1" 2> /dev/null)
    if test $? -ne 0
    then
        exit 1;
    fi
}

output_xml_is_a_valid_mi()
{
    $(xmllint --noout --schema "$UTILS_PATH/mediainfo.xsd" "$1" 2> /dev/null)
    if test $? -ne 0
    then
        exit 1;
    fi
}

output_xml_is_a_valid_mt()
{
    $(xmllint --noout --schema "$UTILS_PATH/mediatrace.xsd" "$1" 2> /dev/null)
    if test $? -ne 0
    then
        exit 1;
    fi
}

output_xml_is_a_valid_mmt()
{
    $(xmllint --noout --schema "$UTILS_PATH/micromediatrace.xsd" "$1" 2> /dev/null)
    if test $? -ne 0
    then
        exit 1;
    fi
}

output_xml_is_a_valid_ma()
{
    $(xmllint --noout --schema "$UTILS_PATH/mediaarea.xsd" "$1" 2> /dev/null)
    if test $? -ne 0
    then
        exit 1;
    fi
}
