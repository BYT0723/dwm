#!/bin/bash

cmdType=$1
buttonType=$2

source $HOME/.dwm/status-env.sh

propToggle() {
    now=$(cat $conf | grep $1 | awk -F '=' '{print $2}')
    if [[ $now -eq 1 ]]; then
        sed -i "s/$1=1/$1=0/g" $conf
    else
        sed -i "s/$1=0/$1=1/g" $conf
    fi
}

dateHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        propToggle ${confProperty["dateExp"]}
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
        propToggle ${confProperty["showTemp"]}
        ;;
    2) ;;

    3) ;;

    esac
}

netSpeedHandler() {
    buttonType=$1
    case "$buttonType" in
    1)
        propToggle ${confProperty["netSpeedExp"]}
        ;;
    2) ;;

    3) ;;

    esac
}

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
