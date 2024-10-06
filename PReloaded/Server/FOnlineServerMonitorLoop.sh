#!/bin/bash

./FOnlineServer.sh start

while true; do
	sleep 180
    ./FOnlineServer.sh monitor
done
