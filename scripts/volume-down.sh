#!/bin/bash

msgTag="volume"

/usr/bin/amixer -qM set Master 2%- umute

volume=$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)%\].*/\1/')

notify-send -t 1000 -a "changeVolume" -i audio-volume-low-symbolic -h string:x-dunst-stack-tag:$msgTag "${volume}"
