#!/bin/sh

# PNG files with XMP metadata (zTXt/iTXt) must not hang when fully parsed (ParseSpeed=1)

PATH_SCRIPT=$(dirname "$0")
PATH_SAMPLES=$PATH_SCRIPT/Samples
TIMEOUT=30

RESULT=0
for FILE_NAME in png_ztxt_xmp.png png_itxt_xmp.png; do
    for PARSE_SPEED in 0.5 1; do
        OUTPUT_NAME="/tmp/$FILE_NAME.$PARSE_SPEED.xml"
        rm -f "$OUTPUT_NAME"

        # Portable timeout, the timeout command is not available everywhere
        "$PATH_SCRIPT/mil_analyze" -f XML -e "ParseSpeed:$PARSE_SPEED" "$PATH_SAMPLES/$FILE_NAME" "$OUTPUT_NAME" &
        PID=$!
        ELAPSED=0
        while kill -0 $PID 2>/dev/null && [ $ELAPSED -lt $TIMEOUT ]; do
            sleep 1
            ELAPSED=$((ELAPSED + 1))
        done
        if kill -0 $PID 2>/dev/null; then
            kill -9 $PID 2>/dev/null
            wait $PID 2>/dev/null
            echo "NOK: $FILE_NAME, ParseSpeed=$PARSE_SPEED, timeout after $TIMEOUT seconds" >&9
            RESULT=1
            continue
        fi
        if ! wait $PID; then
            echo "NOK: $FILE_NAME, ParseSpeed=$PARSE_SPEED, mil_analyze failed" >&9
            RESULT=1
            continue
        fi

        if ! grep -q "<Encoded_Application>MediaInfoLib test</Encoded_Application>" "$OUTPUT_NAME"; then
            echo "NOK: $FILE_NAME, ParseSpeed=$PARSE_SPEED, XMP metadata is missing" >&9
            RESULT=1
        fi
        rm -f "$OUTPUT_NAME"
    done
done

exit $RESULT
