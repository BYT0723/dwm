#!/bin/bash

# nohup go-status &
/bin/bash ~/.dwm/dwm-status.sh &
/bin/bash ~/.dwm/background.sh &
xautolock -time 30 -locker betterlockscreen &
picom -i 0.95 -b --corner-radius 5

# 电池管理
mate-power-manager &

sleep 1

# 系统操作
nm-applet &
volumeicon &

sleep 1

# 其他工具
fcitx5 -d &
trojan -c ~/APP/config.json &
