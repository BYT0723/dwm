#!/bin/bash

# statusBar
/bin/bash ~/.dwm/dwm-status.sh &
# clock notification
/bin/bash ~/.dwm/alarm.sh &
# file change monitor
/bin/bash ~/.dwm/monitor.sh &

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

# proxy (privoxy with trojan [socks5])
#
# 1. cp ./configs/trojan.json /etc/trojan/config.json
# 2. systemctl start trojan
# 3. systemctl enable trojan
# trojan -c ~/.dwm/configs/trojan-cli.json &
