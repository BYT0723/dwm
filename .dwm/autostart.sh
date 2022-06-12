#!/bin/bash

~/.dwm/go-status &
/bin/bash ~/.fehbg &
xautolock -time 30 -locker slock &
picom -i 0.95 -b --corner-radius 10

mate-power-manager &

sleep 0.5

nm-applet &
volumeicon &

sleep 0.5

fcitx5 -d
qv2ray &
