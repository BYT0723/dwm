#!/bin/bash

/usr/bin/amixer -qM set Master 2%+ umute
notify-send -t 1000 -r 3 "墳 "$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)%\].*/\1/')
