#!/bin/bash
/bin/bash ./dwm-status.sh &
/bin/bash ~/.fehbg &
picom -i 0.8 -b &
fcitx5 -d
nm-applet &
/bin/bash ./autolock.sh &
