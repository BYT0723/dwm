#!/bin/bash

max=$(cat /sys/class/backlight/intel_backlight/max_brightness)
step=$(( (max+100) / 100 ))
now=$(cat /sys/class/backlight/intel_backlight/brightness)
new=$(( now + step*2 ))
echo $new > /sys/class/backlight/intel_backlight/brightness

bash ./dwm-status-refresh.sh
