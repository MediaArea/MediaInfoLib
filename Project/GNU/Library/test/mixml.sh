#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
PATH_FILES=$PATH_SCRIPT/Files
. "$PATH_SCRIPT/utils.sh"

if ! enabled mixml ; then
    exit 77 # Skip test
fi

unset args
while IFS= read -r i; do
    FILE_NAME=$(basename "$i")
    OUTPUT_XML_NAME="/tmp/$FILE_NAME.xml.gz"
    `$PATH_SCRIPT/mil_analyze -e Inform_Compress:zlib+base64 -f MIXML "$PATH_FILES/$i" "$OUTPUT_XML_NAME"`
    cmd_is_ok
    [ "$(stat -c%s $OUTPUT_XML_NAME 2>/dev/null || stat -f%z $OUTPUT_XML_NAME 2>/dev/null)" -gt 400 ]  || exit 1

    `$PATH_SCRIPT/mil_analyze -e Inform_Compress:zlib+base64,File_ForceParser:MiXml,Input_Compressed:zlib+base64 -f MIXML "$OUTPUT_XML_NAME" "$OUTPUT_XML_NAME.2"`
    cmd_is_ok
    [ "$(stat -c%s $OUTPUT_XML_NAME.2 2>/dev/null || stat -f%z $OUTPUT_XML_NAME.2 2>/dev/null)" -gt 400 ] || exit 1
    
done < "$PATH_FILES/files.txt"
