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
`$PATH_SCRIPT/mil_analyze -f XML "$FILE" "$OUTPUT_XML_NAME"`
cmd_is_ok
xml_is_correct "$OUTPUT_XML_NAME"
grep -q "ĀāĂăĄąĆćĈĉĊċČčĎďĐđĒēĔĕĖėĘęĚěĜĝĞğĠġĢģĤĥĦħĨĩĪīĬĭĮįİıĲĳĴĵĶķĸĹĺĻļĽľĿŀŁłŃńŅņ" "$OUTPUT_XML_NAME" || exit 1
grep -q "aàäâåáãæÅÆ cçÇ eéèëê uùüû" "$OUTPUT_XML_NAME" || exit 1
grep -q "αβγδεζηθικλμνξοπρςστυφχψω" "$OUTPUT_XML_NAME" || exit 1
grep -q "ئابةتثجحخدذرزسشصضطظعغـف" "$OUTPUT_XML_NAME" || exit 1
grep -q "אבגדהוזחטיךכלםמןנסעףפץצְֱ" "$OUTPUT_XML_NAME" || exit 1
grep -q "ЁЂЃЄЅІЇЈЉЊЋЌЎЏ" "$OUTPUT_XML_NAME" || exit 1
grep -q "ˆˇˉ˘˙˚˛˜̣̀́̃̉" "$OUTPUT_XML_NAME" || exit 1
