#!/bin/bash

/bin/bash ~/.dwm/dwm-status.sh &
/bin/bash ~/.fehbg &
xautolock -time 30 -locker slock &
picom -i 0.95 -b --corner-radius 10 &

nm-applet &
fcitx5 -d
qv2ray
