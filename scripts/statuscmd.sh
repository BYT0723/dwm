#!/bin/bash

#  Handle the statusBar click event
#  see file config.h variable statuscmds

cmdType=$1
# 1 left button
# 2 middle button
# 3 right button
buttonType=$2

dateHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        notify-send -a "Datetime" -h string:x-dunst-stack-tag:datetime "$(cal -s)"
        ;;
    2) ;;
    3) ;;
    esac
}

batteryHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        notify-send -a "BatteryInformation" -h string:x-dunst-stack-tag:batteryInformation "$(acpi -i)"
        ;;
    2)
        echo 2 or 3
        ;;
    3)
        echo default
        ;;
    esac
}

diskHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        notify-send -a "DiskInformation" -h string:x-dunst-stack-tag:diskInformation "$(df -h)"
        ;;
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
    1) ;;
    2) ;;
    3)
        alacritty -e htop
        ;;
    esac
}

netSpeedHandler() {
    buttonType=$1
    case "$buttonType" in
    1) ;;
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
        mpd --kill
        ;;
    3)
        $HOME/.dwm/rofi/bin/mpd.sh
        ;;
    esac
}

weatherHandler() {
    buttonType=$1
    local language=$(echo $LANG | awk -F '_' '{print $1}')
    case "$buttonType" in
    1)
        notify-send -a "CurrentWeather" -h string:x-dunst-stack-tag:currentWeather "$(curl -H 'Accept-Language:'$language 'wttr.in/?T0')"
        ;;
    2)
        xdg-open https://wttr.in/?T
        ;;
    3) ;;
    esac
}

# route by $cmdType
case "$cmdType" in
date)
    dateHandler $2
    ;;
battery)
    batteryHandler $2
    ;;
volume)
    volumeHandler $2
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
