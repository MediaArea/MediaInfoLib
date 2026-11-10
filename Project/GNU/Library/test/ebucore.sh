#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
PATH_FILES=$PATH_SCRIPT/Files
. "$PATH_SCRIPT/utils.sh"

if ! enabled ebucore ; then
    exit 77 # Skip test
fi

unset args
while IFS= read -r i; do
    mkdir -p "$TEMP_PATH/$(dirname "$i")"
    for VERSION in EBUCore_1.5 EBUCore_1.6 EBUCore_1.8_ps EBUCore_1.8_sp; do
        OUTPUT_XML_NAME="$TEMP_PATH/$i.ebucore.$VERSION.xml"
        OUTPUT_JSON_NAME="$TEMP_PATH/$i.ebucore.$VERSION.json"
        analyze -f $VERSION "$PATH_FILES/$i" "$OUTPUT_XML_NAME"
        xml_is_correct "$OUTPUT_XML_NAME"
        analyze -f ${VERSION}_JSON "$PATH_FILES/$i" "$OUTPUT_JSON_NAME"
        json_is_correct "$OUTPUT_JSON_NAME"
    done
done < "$PATH_FILES/files.txt"

file_contains "$OUTPUT_XML_NAME" 'writingLibraryVersion=\"\([0-9]\+\.\)\{1,3\}[0-9]\+\"'
