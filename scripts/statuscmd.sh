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
    1) ;;
    2) ;;
    3)
        toggleConfProp ${confProperty["dateExp"]}
        ;;
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
        alacritty -e htop
        ;;
    2) ;;
    3)
        toggleConfProp ${confProperty["showTemp"]}
        ;;
    esac
}

netSpeedHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        alacritty -e speedtest
        ;;
    2) ;;
    3)
        toggleConfProp ${confProperty["netSpeedExp"]}
        ;;
    esac
}

mpdHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        mpc toggle
        ;;
    2) ;;
    3)
        $HOME/.dwm/rofi/bin/mpd.sh
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
cpuInfo)
    cpuHandler $2
    ;;
netSpeed)
    netSpeedHandler $2
    ;;
mpd)
    mpdHandler $2
    ;;
esac
