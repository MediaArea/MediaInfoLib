#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
PATH_FILES=$PATH_SCRIPT/Files
. "$PATH_SCRIPT/utils.sh"

unset args
while IFS= read -r i; do
    FILE_NAME=$(basename "$i")
    OUTPUT_NAME="/tmp/$FILE_NAME.xml"
    `$PATH_SCRIPT/mil_analyze "$PATH_FILES/$i" "$OUTPUT_NAME"`
    cmd_is_ok
    xml_is_correct "$OUTPUT_NAME"
    output_xml_is_a_valid_mt "$OUTPUT_NAME"
done < "$PATH_FILES/files.txt"
