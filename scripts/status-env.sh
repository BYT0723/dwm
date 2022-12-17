#!/bin/bash

# Variables and functions required by statusBar

conf="$HOME/.dwm/configs/statusConf"

declare -A confProperty
confProperty["showIcon"]="showIcon"
confProperty["showMpd"]="showMpd"
confProperty["dateExp"]="dateExp"
confProperty["showTemp"]="showTemp"
confProperty["netSpeedExp"]="netSpeedExp"

# get property's value
getConfProp() {
    echo $(cat $conf | grep "${confProperty[$1]}" | tail -n 1 | awk -F '=' '{print $2}')
}

# toggle property's value
toggleConfProp() {
    now=$(cat $conf | grep $1 | awk -F '=' '{print $2}')
    if [[ $now -eq 1 ]]; then
        sed -i "s/$1=1/$1=0/g" $conf
    else
        sed -i "s/$1=0/$1=1/g" $conf
    fi
}
