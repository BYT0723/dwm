#!/bin/bash

msgTag="3"

/usr/bin/amixer -qM set Master 2%+ umute

volume=$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)%\].*/\1/')

notify-send -a "changeVolume" -i audio-volume-high-symbolic -h string:x-dunst-stack-tag:$msgTag "${volume}"
