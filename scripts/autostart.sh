#!/bin/bash

# status
/bin/bash ~/.dwm/dwm-status.sh &

# wallpaper
if [[ -f ~/.dwm/background.sh ]]; then
  /bin/bash ~/.dwm/background.sh &
fi

# picom
picom --config ~/.dwm/configs/picom.conf -b --experimental-backends

# autolock
xautolock -time 30 -locker slock -detectsleep &

# 系统操作
mate-power-manager &

sleep 1

nm-applet &
volumeicon &

sleep 1

# usb mountion manager
udiskie -tn &
# input method engine
fcitx5 -d
# proxy
trojan -c ~/.dwm/configs/trojan-cli.json &
