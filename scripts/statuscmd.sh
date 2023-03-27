#!/bin/bash

#  Handle the statusBar click event
#  see file config.h variable statuscmds

cmdType=$1
# 1 left button
# 2 middle button
# 3 right button
buttonType=$2

source $HOME/.dwm/status-env.sh

dateHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        toggleConfProp ${confProperty["dateExp"]}
        ;;
    2) ;;
    3) ;;
    esac
}

diskHandler() {
    buttonType=$1
    case "$buttonType" in
    1) ;;
    2) ;;
    3) ;;
    esac
}

memoryHandler() {
    buttonType=$1
    case "$buttonType" in
    1) ;;
    2) ;;
    3) ;;
    esac
}

cpuHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        toggleConfProp ${confProperty["showTemp"]}
        ;;
    2) ;;
    3)
        alacritty -e htop
        ;;
    esac
}

netSpeedHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        toggleConfProp ${confProperty["netSpeedExp"]}
        ;;
    2) ;;
    3)
        alacritty -e speedtest
        ;;
    esac
}

mpdHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        mpc toggle
        ;;
    2)
        killall mpd
        ;;
    3)
        $HOME/.dwm/rofi/bin/mpd.sh
        ;;
    esac
}

weatherHandler() {
    buttonType=$1
    case "$buttonType" in
    1) ;;
    2) ;;
    3)
        # st -i -g 130x40+480+200 -e curl -H "Accept-Language:zh" -s --retry 2 --connect-timeout 2 wttr.in
        xdg-open https://wttr.in/
        ;;
    esac
}

# route by $cmdType
case "$cmdType" in
date)
    dateHandler $2
    ;;
disk-root)
    diskHandler $2
    ;;
memory)
    memoryHandler $2
    ;;
cpu)
    cpuHandler $2
    ;;
netSpeed)
    netSpeedHandler $2
    ;;
mpd)
    mpdHandler $2
    ;;
weather)
    weatherHandler $2
    ;;
esac
