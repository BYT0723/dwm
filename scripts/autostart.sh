#!/bin/bash

# statusBar
/bin/bash ~/.dwm/dwm-status.sh &

# wallpaper
if [[ -f ~/.dwm/background.sh ]]; then
    /bin/bash ~/.dwm/background.sh &
fi

# picom (window composer)
picom --config ~/.dwm/configs/picom.conf -b --experimental-backends

# autolock (screen locker)
xautolock -time 30 -locker slock -detectsleep &

# power manager
mate-power-manager &

sleep 1

# network manager tray
nm-applet &
# volume tray
volumeicon &

sleep 1

# usb device manager
udiskie -tn &
# input method engine
fcitx5 -d
# proxy
trojan -c ~/.dwm/configs/trojan-cli.json &