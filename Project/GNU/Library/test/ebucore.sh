#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
PATH_FILES=$PATH_SCRIPT/Files
. "$PATH_SCRIPT/utils.sh"

unset args
while IFS= read -r i; do
    for VERSION in EBUCore_1.5 EBUCore_1.6 EBUCore_1.8_ps EBUCore_1.8_sp; do
        FILE_NAME=$(basename "$i")
        OUTPUT_XML_NAME="/tmp/$FILE_NAME.ebucore.$VERSION.xml"
        `$PATH_SCRIPT/mil_analyze -f $VERSION "$PATH_FILES/$i" "$OUTPUT_XML_NAME"`
        cmd_is_ok
        xml_is_correct "$OUTPUT_XML_NAME"
    done
done < "$PATH_FILES/files.txt"
