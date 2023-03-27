#!/bin/bash

/usr/bin/amixer -qM set Master 2%+ umute

bash ~/.dwm/dwm-status-refresh.sh
