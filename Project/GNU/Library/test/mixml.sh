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
    analyze -e Inform_Compress:zlib+base64 -f MIXML "$PATH_FILES/$i" "$OUTPUT_XML_NAME"
    [ "$(stat -c%s $OUTPUT_XML_NAME 2>/dev/null || stat -f%z $OUTPUT_XML_NAME 2>/dev/null)" -gt 400 ] || fail "$OUTPUT_XML_NAME has 400 bytes or less"

    analyze -e Inform_Compress:zlib+base64,File_ForceParser:MiXml,Input_Compressed:zlib+base64 -f MIXML "$OUTPUT_XML_NAME" "$OUTPUT_XML_NAME.2"
    [ "$(stat -c%s $OUTPUT_XML_NAME.2 2>/dev/null || stat -f%z $OUTPUT_XML_NAME.2 2>/dev/null)" -gt 400 ] || fail "$OUTPUT_XML_NAME.2 has 400 bytes or less"
    
done < "$PATH_FILES/files.txt"
