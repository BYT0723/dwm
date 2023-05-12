#!/bin/bash

msgTag="brightness"

case "$1" in
'up')
  xbacklight -inc 2
  ;;
'down')
  xbacklight -dec 2
  ;;
esac

notify-send -c tools -i display-brightness-symbolic -h string:x-dunst-stack-tag:$msgTag "$(xbacklight -get)"
