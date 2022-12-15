#!/bin/bash

conf="$HOME/.dwm/configs/statusConf"

declare -A confProperty
confProperty["showIcon"]="showIcon"
confProperty["dateExp"]="dateExp"
confProperty["showTemp"]="showTemp"
confProperty["netSpeedExp"]="netSpeedExp"

# 获取property的value
getConf() {
    echo $(cat $conf | grep "${confProperty[$1]}" | tail -n 1 | awk -F '=' '{print $2}')
}
