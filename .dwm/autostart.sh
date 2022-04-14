#!/bin/bash

/bin/bash ~/.dwm/dwm-status.sh &
/bin/bash ~/.fehbg &
/bin/bash ~/.dwm/autolock.sh &
fcitx5 -d

picom -i 0.8 -b &
nm-applet &
