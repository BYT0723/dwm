#!/bin/bash

/bin/bash ~/.dwm/dwm-status.sh &
/bin/bash ~/.fehbg &
/bin/bash ~/.dwm/autolock.sh &
fcitx5 -d

picom -i 0.95 -b --corner-radius 10 &
nm-applet &
