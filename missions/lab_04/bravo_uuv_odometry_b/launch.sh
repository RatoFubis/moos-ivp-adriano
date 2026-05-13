#!/bin/bash -e
#----------------------------------------------------------
#  Script: launch.sh
#  Author: Michael Benjamin
#  LastEd: May 20th 2019
#----------------------------------------------------------
#  Part 1: Set Exit actions and declare global var defaults
#----------------------------------------------------------
TIME_WARP=1
COMMUNITY="bravo"
GUI="yes"

# PARAMETERS
DIST_THRESH=200
DEPTH_THRESH=25

#----------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#----------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ] ; then
        echo "launch.sh [SWITCHES] [time_warp]"
        exit 0;

    elif [ "${ARGI}" = "--nogui" ] ; then
        GUI="no"

    # BONUS: distance threshold
    elif [[ "${ARGI}" == --dist=* ]]; then
        DIST_THRESH="${ARGI#--dist=}"

    # BONUS: depth threshold
    elif [[ "${ARGI}" == --depth=* ]]; then
        DEPTH_THRESH="${ARGI#--depth=}"

    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then 
        TIME_WARP=$ARGI

    else 
        echo "launch.sh Bad arg:" $ARGI " Exiting with code: 1"
        exit 1
    fi
done


#----------------------------------------------------------
#  Part 3: Launch the processes
#----------------------------------------------------------
echo "Launching $COMMUNITY MOOS Community with WARP:" $TIME_WARP

nsplug $COMMUNITY.moos targ_$COMMUNITY.moos -f \
    DEPTH_THRESH=$DEPTH_THRESH

nsplug $COMMUNITY.bhv targ_$COMMUNITY.bhv -f \
    DIST_THRESH=$DIST_THRESH

pAntler targ_$COMMUNITY.moos --MOOSTimeWarp=$TIME_WARP >& /dev/null &

uMAC -t $COMMUNITY.moos
kill -- -$$
