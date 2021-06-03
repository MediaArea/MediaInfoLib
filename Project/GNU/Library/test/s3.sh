#!/bin/sh

PATH_SCRIPT=$(dirname "$0")
. "$PATH_SCRIPT/utils.sh"

RCODE=0

if [ -z $S3_KEY ] || [ -z $S3_PASS ] || ! enabled s3 ; then
    exit 77 # Skip test
fi

SERVERS="travis-standard.s3.amazonaws.com"
SERVERS="$SERVERS s3.amazonaws.com/travis-standard"
SERVERS="$SERVERS travis-oregon.s3.amazonaws.com"
SERVERS="$SERVERS travis-oregon.s3-us-west-2.amazonaws.com"
SERVERS="$SERVERS s3-us-west-2.amazonaws.com/travis-oregon"
SERVERS="$SERVERS travis-frankfurt.s3.amazonaws.com"
SERVERS="$SERVERS s3-eu-central-1.amazonaws.com/travis-frankfurt"
SERVERS="$SERVERS travis-frankfurt.s3.eu-central-1.amazonaws.com"
SERVERS="$SERVERS s3-eu-central-1.amazonaws.com/travis-frankfurt"

for SERVER in $SERVERS; do
    FILE=$(mktemp)

    "$PATH_SCRIPT"/mil_analyze -f "General;%Format%" -l raw  "https://$S3_KEY:$S3_PASS@$SERVER/Example.ogg" $FILE
    if [ "$(cat $FILE)" = "Ogg" ] ; then
        echo "OK: $SERVER" >&9
    else
        RCODE=1
        echo "NOK: $SERVER" >&9
    fi

    rm $FILE
done

exit $RCODE
