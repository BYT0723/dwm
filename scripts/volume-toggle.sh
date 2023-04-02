#!/bin/bash

msgTag="3"
volume=$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)%\].*/\1/')
status=$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)\].*/\1/')

amixer sset Master toggle
if [ "$status" == "off" ]; then
    notify-send -a "changeVolume" -i audio-volume-high-symbolic -h string:x-dunst-stack-tag:$msgTag -h int:value:"$volume" "${volume}"
else
    notify-send -a "changeVolume" -i audio-volume-muted-symbolic -h string:x-dunst-stack-tag:$msgTag "muted"
fi
