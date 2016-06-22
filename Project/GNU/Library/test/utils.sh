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
