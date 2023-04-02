#!/bin/bash

xbacklight -dec 4
notify-send -t 1000 -r 2 "󰃟 "$(xbacklight -get)
