#!/bin/bash

# statusBar
/bin/bash ~/.dwm/dwm-status.sh &

# wallpaper
/bin/bash ~/.dwm/background.sh &

# picom (window composer)
if [ -z "$(pgrep picom)" ]; then
    picom --config ~/.dwm/configs/picom.conf -b --experimental-backends
fi

# polkit (require lxsession or lxsession-gtk3)
if [ -z "$(pgrep lxpolkit)" ]; then
    lxpolkit &
fi

# autolock (screen locker)
if [ -z "$(pgrep xautolock)" ]; then
    xautolock -time 30 -locker betterlockscreen -detectsleep &
fi

if [ -z "$(pgrep nm-applet)" ]; then
    nm-applet &
fi

# mate-power-manager &
# volumeicon &

if [ -z "$(pgrep fcitx5)" ]; then
    fcitx5 -d
fi

sleep 3

# 设置风扇转速为0
if [ -x nbfc ]; then
    nbfc set -s 0
fi
