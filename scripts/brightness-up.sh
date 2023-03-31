#!/bin/bash

xbacklight -inc 4
dunstify -r 2 "󰃟 "$(xbacklight -get)
