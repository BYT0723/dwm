#!/bin/bash

# statusBar
/bin/bash ~/.dwm/dwm-status.sh &

# wallpaper
/bin/bash ~/.dwm/wallpaper.sh -r &

# picom (window composer)
if [ -z "$(pgrep picom)" ]; then
    # picom --config ~/.dwm/configs/picom.conf -b --experimental-backends
    picom --config ~/.dwm/configs/picom.conf -b
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

if [ -z "$(pgrep udiskie)" ]; then
    udiskie -sn &
fi

# mate-power-manager &
# volumeicon &

if [ -z "$(pgrep fcitx5)" ]; then
    fcitx5 -d
fi
