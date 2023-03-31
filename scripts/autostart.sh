#!/bin/bash

# statusBar
/bin/bash ~/.dwm/dwm-status.sh &

# wallpaper
/bin/bash ~/.dwm/background.sh &

# picom (window composer)
picom --config ~/.dwm/configs/picom.conf -b --experimental-backends

# autolock (screen locker)
xautolock -time 30 -locker betterlockscreen -detectsleep &

nm-applet &
# mate-power-manager &
# volumeicon &
# udiskie -tn &

fcitx5 -d
