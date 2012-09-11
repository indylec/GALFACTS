#!/bin/bash
#clear

# Executable directory
BIN=/n/ras/people/mircea/map/bin

#Field
FIELD="N1"

#Band
BAND="1"

#Central frequency
CNTRFRQ="1300.018"

#RA min & max
RAMIN="20.0"
RAMAX="107.0"

#DEC min & max
DECMIN="19.7"
DECMAX="37.6"

#CELLSIZE max resolution is 3.5
CELLSIZE="1.0"
#CELLSIZE="0.5"

#radius of spread, multiple of resolution 3.5, recommended 1.5 - 2.0
PATCH="2.0"

# 1 upscan, 2 downscan, 3 both, for creating images
GRIDTYPE="3"

DAYFIT="100"

SCANFIT="100"

BALGAIN="0.142"

BALEPSILON="0.000001"

BWORDER="6"

DECORDER="12"

SOURCE="GALFACTS_N1"

# AVG=0 for average image, AVG=1,2,... for cube
AVG="1"

# AVG first channel
AVG_LOWCHAN="50"

# AVG last channel
AVG_HIGHCHAN="4051"

# First channel
LOWCHAN="690"
# Last channel
HIGHCHAN="770"
$BIN/map4096 multibeam $CNTRFRQ $LOWCHAN $HIGHCHAN $RAMIN $RAMAX $DECMIN $DECMAX $CELLSIZE $PATCH $GRIDTYPE $BALGAIN $BALEPSILON $BWORDER $DECORDER $AVG $AVG_LOWCHAN $AVG_HIGHCHAN "$SOURCE" $FIELD $BAND &

# First channel
LOWCHAN="770"
# Last channel
HIGHCHAN="850"
$BIN/map4096 multibeam $CNTRFRQ $LOWCHAN $HIGHCHAN $RAMIN $RAMAX $DECMIN $DECMAX $CELLSIZE $PATCH $GRIDTYPE $BALGAIN $BALEPSILON $BWORDER $DECORDER $AVG $AVG_LOWCHAN $AVG_HIGHCHAN "$SOURCE" $FIELD $BAND &

# First channel
LOWCHAN="850"
# Last channel
HIGHCHAN="930"
$BIN/map4096 multibeam $CNTRFRQ $LOWCHAN $HIGHCHAN $RAMIN $RAMAX $DECMIN $DECMAX $CELLSIZE $PATCH $GRIDTYPE $BALGAIN $BALEPSILON $BWORDER $DECORDER $AVG $AVG_LOWCHAN $AVG_HIGHCHAN "$SOURCE" $FIELD $BAND &

# First channel
LOWCHAN="930"
# Last channel
HIGHCHAN="1010"
$BIN/map4096 multibeam $CNTRFRQ $LOWCHAN $HIGHCHAN $RAMIN $RAMAX $DECMIN $DECMAX $CELLSIZE $PATCH $GRIDTYPE $BALGAIN $BALEPSILON $BWORDER $DECORDER $AVG $AVG_LOWCHAN $AVG_HIGHCHAN "$SOURCE" $FIELD $BAND &

