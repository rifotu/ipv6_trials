#!/bin/sh
### BEGIN INIT INFO
# Provides:          gpsdproxy
# Required-Start:    gpsd
# Required-Stop:     gpsd
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: GPSDproxy daemon
# Description:       Start/Stop script for the gpsd proxy daemon,
#                    GPS position read from gpsd daemon is forwarded
#                    to a remote host using UDP packets.
### END INIT INFO

PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
DESC='GPSD Proxy client'
NAME=gpsdproxy
DAEMON=/usr/local/sbin/$NAME
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME

client_id='gpsdproxy'
gpsd_host=127.0.0.1
gpsd_port=2947
remote_host=''
remote_port=''
interval=10
debug=3

test -f $DAEMON || exit 0
test -f /etc/default/gpsdproxy && . /etc/default/gpsdproxy
test -z "$remote_host" && exit 0

case "$1" in
    start)
        echo -n "Starting $DESC: "
        start-stop-daemon --start --quiet --pidfile $PIDFILE \
          --exec $DAEMON -- \
          -i "$client_id" -t $interval \
          -h $remote_host -p $remote_port -s $gpsd_host -r $gpsd_port \
          -f $PIDFILE -d $debug -b
        if [ $? = 0 ]; then
            echo "$NAME."
        else
            echo "(failed.)"
        fi
        ;;
    stop)
        echo -n "Stopping $DESC: "
        start-stop-daemon --oknodo --stop --quiet --pidfile $PIDFILE
        rm -f $PIDFILE
        echo "gpsdproxy."
        ;;
    *)
        echo "Usage: $SCRIPTNAME {start|stop}"
        exit 1
        ;;
esac

exit 0
