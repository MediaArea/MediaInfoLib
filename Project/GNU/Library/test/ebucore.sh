#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
PATH_FILES=$PATH_SCRIPT/Files
. "$PATH_SCRIPT/utils.sh"

if ! enabled ebucore ; then
    exit 77 # Skip test
fi

unset args
while IFS= read -r i; do
    for VERSION in EBUCore_1.5 EBUCore_1.6 EBUCore_1.8_ps EBUCore_1.8_sp; do
        FILE_NAME=$(basename "$i")
        OUTPUT_XML_NAME="/tmp/$FILE_NAME.ebucore.$VERSION.xml"
        OUTPUT_JSON_NAME="/tmp/$FILE_NAME.ebucore.$VERSION.json"
        `$PATH_SCRIPT/mil_analyze -f $VERSION "$PATH_FILES/$i" "$OUTPUT_XML_NAME"`
        cmd_is_ok
        xml_is_correct "$OUTPUT_XML_NAME"
        `$PATH_SCRIPT/mil_analyze -f ${VERSION}_JSON "$PATH_FILES/$i" "$OUTPUT_JSON_NAME"`
        cmd_is_ok
        json_is_correct "$OUTPUT_JSON_NAME"
    done
done < "$PATH_FILES/files.txt"

grep -q 'writingLibraryVersion=\"\([0-9]\+\.\)\{1,3\}[0-9]\+\"' "$OUTPUT_XML_NAME"
cmd_is_ok
