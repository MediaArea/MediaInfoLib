#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
PATH_FILES=$PATH_SCRIPT/Files
. "$PATH_SCRIPT/utils.sh"

if ! enabled trace ; then
    exit 77 # Skip test
fi

unset args
while IFS= read -r i; do
    FILE_NAME=$(basename "$i")
    OUTPUT_XML_NAME="/tmp/$FILE_NAME.xml"
    `$PATH_SCRIPT/mil_analyze -d -h -f XML "$PATH_FILES/$i" "$OUTPUT_XML_NAME"`
    cmd_is_ok
    xml_is_correct "$OUTPUT_XML_NAME"
    output_xml_is_a_valid_mt "$OUTPUT_XML_NAME"
    OUTPUT_MICRO_XML_NAME="/tmp/$FILE_NAME.micro.xml"
    `$PATH_SCRIPT/mil_analyze -d -h -f MICRO_XML "$PATH_FILES/$i" "$OUTPUT_MICRO_XML_NAME"`
    cmd_is_ok
    xml_is_correct "$OUTPUT_MICRO_XML_NAME"
    output_xml_is_a_valid_mmt "$OUTPUT_MICRO_XML_NAME"
done < "$PATH_FILES/files.txt"
