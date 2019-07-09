#!/bin/sh

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <base_name>"
    exit 1
fi

FBASE=$1

for FNAME in ${FBASE}*.pov; do
    povray 05-behavior.ini +I$FNAME
done
