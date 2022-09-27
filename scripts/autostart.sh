#!/bin/bash

# nohup go-status &
/bin/bash ~/.dwm/dwm-status.sh &
/bin/bash ~/.dwm/background.sh &
xautolock -time 30 -locker slock -detectsleep &
picom --config ~/.dwm/configs/picom.conf -b

# 系统操作
mate-power-manager &
# usb mountion manager
udiskie -t &

sleep 1

nm-applet &
volumeicon &

sleep 1

# 其他工具
fcitx5 -d
trojan -c ~/.dwm/configs/trojan-cli.json &
