#!/bin/bash

# nohup go-status &
/bin/bash ~/.dwm/dwm-status.sh &
/bin/bash ~/.dwm/background.sh &
xautolock -time 30 -locker slock -detectsleep &
picom --config ~/.dwm/configs/picom.conf -b

# 系统操作
mate-power-manager &

sleep 1

nm-applet &
volumeicon &

sleep 1

# input method engine
fcitx5 -d
# proxy
trojan -c ~/.dwm/configs/trojan-cli.json &
# usb mountion manager
udiskie -t &
