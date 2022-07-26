#!/bin/bash

nohup go-status &
/bin/bash ~/.fehbg &
xautolock -time 30 -locker slock &
picom -i 0.95 -b --corner-radius 5

mate-power-manager &
fcitx5 -d &

sleep 1

nm-applet &
volumeicon &

sleep 1

qv2ray &
