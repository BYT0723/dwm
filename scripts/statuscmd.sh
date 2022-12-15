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

    3) ;;

    esac
}

netSpeedHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        toggleConfProp ${confProperty["netSpeedExp"]}
        ;;
    2) ;;

    3) ;;

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
esac
