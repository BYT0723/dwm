#!/bin/bash

# status
/bin/bash ~/.dwm/dwm-status.sh &

# wallpaper
/bin/bash ~/.dwm/background.sh &

# picom
picom --config ~/.dwm/configs/picom.conf -b

# 系统操作
mate-power-manager &

sleep 1

nm-applet &
volumeicon &

sleep 1

# input method engine
fcitx5 -d
# usb mountion manager
udiskie -tN &
# proxy
trojan -c ~/.dwm/configs/trojan-cli.json &

# autolock
xautolock -time 30 -locker slock -detectsleep &
