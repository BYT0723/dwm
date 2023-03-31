#!/bin/bash

amixer sset Master toggle
if [ "$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)\].*/\1/')" == "off" ]; then
  dunstify -r 3 "婢 0"
else
  dunstify -r 3 "墳 "$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)%\].*/\1/')
fi
