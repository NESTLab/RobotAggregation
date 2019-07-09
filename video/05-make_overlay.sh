#!/bin/sh

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <base_name> <up_to>"
    exit 1
fi

FBASE=$1
UPTO=$2
TOT=$((`ls -1 ${FBASE}?????.png | wc -l`-1))

function do_overlay {
    local START=$1
    local END=$2
    local FOVERLAY=$3
    for FRAME in `seq $START $END`; do
	IFNAME=`printf "%s%05d.png" $FBASE $FRAME`
	OFNAME=`printf "%s_overlay_%05d.png" $FBASE $FRAME`
	echo $IFNAME $FOVERLAY $OFNAME
	magick composite -gravity center $FOVERLAY $IFNAME $OFNAME
    done
}

do_overlay 0 $(($UPTO-1)) normalspeed.png
do_overlay $UPTO $TOT x4speed.png
