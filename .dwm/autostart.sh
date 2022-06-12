#!/bin/bash

~/.dwm/go-status &
/bin/bash ~/.fehbg &
xautolock -time 30 -locker slock &
picom -i 0.95 -b --corner-radius 10

mate-power-manager &
nm-applet &
volumeicon &
fcitx5 -d
qv2ray &
