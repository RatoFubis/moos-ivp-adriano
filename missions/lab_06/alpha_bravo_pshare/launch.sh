#!/bin/bash -e
#----------------------------------------------------------
#  Script: launch.sh
#----------------------------------------------------------

TIME_WARP=1
GUI="yes"

#----------------------------------------------------------
# Handle command line args
#----------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ] ; then
        echo "launch.sh [SWITCHES] [time_warp]"
        echo "  --help, -h      Show help"
        echo "  --nogui         Launch without pMarineViewer"
        exit 0;

    elif [ "${ARGI}" = "--nogui" ] ; then
        GUI="no"

    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI

    else
        echo "Bad Arg: $ARGI"
        exit 1
    fi
done

#----------------------------------------------------------
# Optionally disable GUI
#----------------------------------------------------------
if [ "${GUI}" = "no" ] ; then
    IX_GUI=" --nogui "
fi

#----------------------------------------------------------
# Launch Shoreside
#----------------------------------------------------------
echo "Launching shoreside MOOS Community"
pAntler shoreside.moos --MOOSTimeWarp=$TIME_WARP >& /dev/null &

#----------------------------------------------------------
# Launch Alpha
#----------------------------------------------------------
echo "Launching alpha MOOS Community"
pAntler alpha.moos --MOOSTimeWarp=$TIME_WARP >& /dev/null &

#----------------------------------------------------------
# Launch Bravo
#----------------------------------------------------------
echo "Launching bravo MOOS Community"
pAntler bravo.moos --MOOSTimeWarp=$TIME_WARP >& /dev/null &

#----------------------------------------------------------
# Launch uMAC on shoreside
#----------------------------------------------------------
uMAC shoreside.moos

#----------------------------------------------------------
# Kill all spawned processes on exit
#----------------------------------------------------------
kill -- -$$
