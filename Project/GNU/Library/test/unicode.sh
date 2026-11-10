#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
PATH_FILES=$PATH_SCRIPT/Files
. "$PATH_SCRIPT/utils.sh"

if ! enabled unicode ; then
    exit 77 # Skip test
fi

FILE="$PATH_SCRIPT/../../../../Release/Example.ogg"
FILE_NAME="$(basename $FILE)"
OUTPUT_XML_NAME="/tmp/$FILE_NAME.xml"
analyze -f XML "$FILE" "$OUTPUT_XML_NAME"
xml_is_correct "$OUTPUT_XML_NAME"
file_contains "$OUTPUT_XML_NAME" "ĀāĂăĄąĆćĈĉĊċČčĎďĐđĒēĔĕĖėĘęĚěĜĝĞğĠġĢģĤĥĦħĨĩĪīĬĭĮįİıĲĳĴĵĶķĸĹĺĻļĽľĿŀŁłŃńŅņ"
file_contains "$OUTPUT_XML_NAME" "aàäâåáãæÅÆ cçÇ eéèëê uùüû"
file_contains "$OUTPUT_XML_NAME" "αβγδεζηθικλμνξοπρςστυφχψω"
file_contains "$OUTPUT_XML_NAME" "ئابةتثجحخدذرزسشصضطظعغـف"
file_contains "$OUTPUT_XML_NAME" "אבגדהוזחטיךכלםמןנסעףפץצְֱ"
file_contains "$OUTPUT_XML_NAME" "ЁЂЃЄЅІЇЈЉЊЋЌЎЏ"
file_contains "$OUTPUT_XML_NAME" "ˆˇˉ˘˙˚˛˜̣̀́̃̉"
