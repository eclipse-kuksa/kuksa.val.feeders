#!/bin/sh

# ENV GPSD_ARGS="-S ${GPSD_LISTENER_PORT} ${GPSD_SOURCE}"
# CMD gpsd GPSD_ARGS; ./gpsd_feeder.py

[ -z "$GPSD_OPTIONS" ] && GPSD_OPTIONS="-S 2948 udp://0.0.0.0:29998"

gpsd $GPSD_OPTIONS -N -D 2 &
if [ $? -eq 0 ]; then
    export GPSD_PID=$$
    echo "# gpsd started, PID: $GPSD_PID"
else
    echo "ERROR: Failed to start gpsd $GPSD_OPTIONS -N -D 2"
fi

trap cleanup 15

cleanup() {
    echo "# cleanup"
    if [ -n "$GPSD_PID" ]; then
        echo "# killng gpsd PID: $GPSD_PID"
        kill -9 $GPSD_PID
        unset GPSD_PID
    fi
}

echo "# Launching: ./gpsd_feeder.py $*"
python -u ./gpsd_feeder.py $*

cleanup

# check for remaining gpsd processes
pidof gpsd