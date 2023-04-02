#!/bin/bash

msgTag=2

xbacklight -dec 4

notify-send -a "changeBrightness" -i display-brightness-symbolic -h string:x-dunst-stack-tag:$msgTag "$(xbacklight -get)"
