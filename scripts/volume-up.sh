#!/bin/bash

msgTag="3"
volume=$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)%\].*/\1/')

/usr/bin/amixer -qM set Master 2%+ umute

notify-send -a "changeVolume" -i audio-volume-high-symbolic -h string:x-dunst-stack-tag:$msgTag -h int:value:"$volume" "${volume}"
