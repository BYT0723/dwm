#!/bin/bash

nohup go-status &
/bin/bash ~/.fehbg &
xautolock -time 30 -locker slock &
picom -i 0.95 -b --corner-radius 5

# 电池管理
mate-power-manager &

sleep 1

# 系统操作
nm-applet &
volumeicon &

sleep 1

# 其他工具
qv2ray &
fcitx5 -d &
