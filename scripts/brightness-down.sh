#!/bin/bash

msgTag="brightness"

xbacklight -dec 2

notify-send -c tools -i display-brightness-symbolic -h string:x-dunst-stack-tag:$msgTag "$(xbacklight -get)"
