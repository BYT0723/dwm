#!/bin/bash

/usr/bin/amixer -qM set Master 2%+ umute
dunstify -r 3 "墳 "$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)%\].*/\1/')
