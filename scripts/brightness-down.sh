#!/bin/bash

xbacklight -dec 4
dunstify -r 2 "󰃟 "$(xbacklight -get)
