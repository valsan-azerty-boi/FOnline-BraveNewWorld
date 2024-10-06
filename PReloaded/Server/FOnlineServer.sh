#!/bin/bash

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# FOnlineServer executable
EXECUTABLE="Server.exe"
# MAX_RAM_MB=2560

function start_server() {
    echo "FOnlineServer: Start"
    echo "Stopping FOnlineServer."
    pkill -f "$SCRIPT_DIR/$EXECUTABLE"
    echo "Starting FOnlineServer."
    # ulimit -v $((MAX_RAM_MB))
    nohup wine explorer /desktop=FOOnline,1024x768 "$SCRIPT_DIR/$EXECUTABLE" --Start > /dev/null 2>&1 &
    disown
}

function stop_server() {
    echo "FOnlineServer: Stop"
    echo "Stopping FOnlineServer."
    pkill -f "$SCRIPT_DIR/$EXECUTABLE"
}

function restart_server() {
	echo "FOnlineServer: Restart"
    stop_server
    sleep 1
    start_server
}

function monitor_server() {
	echo "FOnlineServer: Monitor"
    if pgrep -f "$SCRIPT_DIR/$EXECUTABLE" >/dev/null; then
        if nc -z localhost 2238; then
            echo "FOnlineServer seems to be running."
        else
            echo "FOnlineServer process is running, but port 2238 is not in use."
            restart_server
        fi
    else
        echo "FOnlineServer is not running."
        restart_server
    fi
}

case "$1" in
    start)
        start_server
        ;;
    stop)
        stop_server
        ;;
    restart)
        restart_server
        ;;
    monitor)
        monitor_server
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|monitor}"
        exit 1
        ;;
esac
