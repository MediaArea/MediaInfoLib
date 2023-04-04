#!/bin/bash

PATH_SCRIPT=$(dirname "$0")
. "$PATH_SCRIPT/utils.sh"

FD=1
if (command exec >&9) ; then
    FD=9
fi >/dev/null 2>&1

RCODE=0

if [ -z $S3_KEY ] || [ -z $S3_PASS ] || ! enabled s3 ; then
    exit 77 # Skip test
fi

# Test server

INDEX=0
SERVERS[((INDEX++))]="travis-standard.s3.amazonaws.com"
SERVERS[((INDEX++))]="s3.amazonaws.com/travis-standard"
SERVERS[((INDEX++))]="travis-oregon.s3.amazonaws.com"
SERVERS[((INDEX++))]="travis-oregon.s3-us-west-2.amazonaws.com"
SERVERS[((INDEX++))]="s3-us-west-2.amazonaws.com/travis-oregon"
SERVERS[((INDEX++))]="travis-frankfurt.s3.amazonaws.com"
SERVERS[((INDEX++))]="s3-eu-central-1.amazonaws.com/travis-frankfurt"
SERVERS[((INDEX++))]="travis-frankfurt.s3.eu-central-1.amazonaws.com"
SERVERS[((INDEX++))]="s3-eu-central-1.amazonaws.com/travis-frankfurt"

for SERVER in "${SERVERS[@]}"; do
    OUTPUT=$(mktemp)

    "$PATH_SCRIPT"/mil_analyze -f "General;%Format%" -l raw  "https://$S3_KEY:$S3_PASS@$SERVER/Example.ogg" $OUTPUT
    if [ "$(cat $OUTPUT)" = "Ogg" ] ; then
        echo "OK: $SERVER" >&$FD
    else
        RCODE=1
        echo "NOK: $SERVER" >&$FD
    fi

    rm $OUTPUT
done

# Test URLs (Guess)

INDEX=0
FILES[((INDEX++))]="Example/Example.ogg"
FILES[((INDEX++))]="Example +Ÿ/Example +Ÿ.ogg"
FILES[((INDEX++))]="Example with spaces & special chars éà.ogg"
FILES[((INDEX++))]="Example with spaces & special chars éà@.ogg"
FILES[((INDEX++))]="Example.dxw"
FILES[((INDEX++))]="Example+.ogg"
FILES[((INDEX++))]="Example+/Example+.ogg"
FILES[((INDEX++))]="Example+%2B%C5%B8/Example+%2B%C5%B8.ogg"

for FILE in "${FILES[@]}"; do
    OUTPUT=$(mktemp)

    if [ "${FILE##*.}" == "dxw" ] ; then
        "$PATH_SCRIPT"/mil_analyze -f "Audio;%MuxingMode%" -l raw  "https://$S3_KEY:$S3_PASS@travis-frankfurt.s3.amazonaws.com/$FILE" $OUTPUT
    else
        "$PATH_SCRIPT"/mil_analyze -f "General;%Format%" -l raw  "https://$S3_KEY:$S3_PASS@travis-frankfurt.s3.amazonaws.com/$FILE" $OUTPUT
    fi
    if [ "$(cat $OUTPUT)" = "Ogg" ] ; then
        echo "OK: $FILE" >&$FD
    else
        RCODE=1
        echo "NOK: $FILE" >&$FD
    fi

    rm $OUTPUT
done

unset FILES

# Test URLs (Not)Encoded

INDEX=0
FILES[((INDEX++))]="1:Example+%2B%C5%B8/Example+%2B%C5%B8.ogg"
FILES[((INDEX++))]="0:Example+%2B%C5%B8/Example+%2B%C5%B8.ogg"
FILES[((INDEX++))]="0:Example +Ÿ/Example+%2B%C5%B8.ogg"
FILES[((INDEX++))]="0:Example+%2B%C5%B8/Example +Ÿ.ogg"

for FILE in "${FILES[@]}"; do
    OUTPUT=$(mktemp)

    "$PATH_SCRIPT"/mil_analyze -f "General;%Format%" -e "URLEncode:${FILE%%:*}" -l raw  "https://$S3_KEY:$S3_PASS@travis-frankfurt.s3.amazonaws.com/${FILE#*:}" $OUTPUT
    if [ "$(cat $OUTPUT)" = "Ogg" ] ; then
        echo "OK: ${FILE#*:}" >&$FD
    else
        RCODE=1
        echo "NOK: ${FILE#*:}" >&$FD
    fi

    rm $OUTPUT
done

exit $RCODE
