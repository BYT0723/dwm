#!/bin/bash

# Because this script is called by the cronie service, it needs to export DBUS_SESSION_BUS_ADDRESS
DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus
export DBUS_SESSION_BUS_ADDRESS

# Displayed screen
export DISPLAY=:0

# send notification
notify-send -i preferences-system-time-symbolic -u critical "$(date +"%H:%M") $1\n $2"
