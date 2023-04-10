#!/bin/bash

msgTag="brightness"

xbacklight -inc 2

notify-send -t 1000 -a "changeBrightness" -i display-brightness-symbolic -h string:x-dunst-stack-tag:$msgTag "$(xbacklight -get)"
