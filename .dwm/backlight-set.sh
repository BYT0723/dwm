#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Error : At least one parameter is required "
    exit 1
fi

max=$(cat /sys/class/backlight/intel_backlight/max_brightness)
step=$(( max / 100 ))
pre=$1

echo $(( pre*step )) > /sys/class/backlight/intel_backlight/brightness
