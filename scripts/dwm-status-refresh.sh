#!/bin/bash

# statusBar Environment
source ~/.dwm/status-env.sh

# colorscheme
black=#1e222a
green=#7eca9c
white=#abb2bf
grey=#282c34
blue=#7aa2f7
red=#d47d85
darkblue=#668ee3

# Icons initial
if [[ $(getConfProp showIcon) -eq 1 ]]; then
    declare -A icons
    icons["disk"]="﫭 "
    icons["memory"]=" "
    icons["temp"]=" "
    icons["cpu"]=" "
    icons["netSpeed"]=" "
    icons["mpd"]=" "
fi

weather_update_duration=60
weather_path="/tmp/.weather"

function get_bytes {
    # Find active network interface
    interface=$(ip route get 8.8.8.8 2>/dev/null | awk '{print $5}')
    line=$(grep $interface /proc/net/dev | cut -d ':' -f 2 | awk '{print "received_bytes="$1, "transmitted_bytes="$9}')
    eval $line
    now=$(date +%s%N)
}

# Function which calculates the speed using actual and old byte number.
# Speed is shown in KByte per second when greater or equal than 1 KByte per second.
# This function should be called each second.

function get_velocity {
    value=$1
    old_value=$2
    now=$3

    timediff=$(($now - $old_time))
    velKB=$(echo "1000000000*($value-$old_value)/1024/$timediff" | bc)
    if [ $velKB -gt 1000 ]; then
        # echo $(echo "scale=2; $velKB/1024" | bc)M/s
        printf "%.1fM/s" $(echo "scale=1; $velKB/1024" | bc)
    else
        # echo ${velKB}K/s
        printf "%3dK/s" ${velKB}
    fi
}

# Get initial values
get_bytes
old_received_bytes=$received_bytes
old_transmitted_bytes=$transmitted_bytes
old_time=$now

# Memory usage
print_mem() {
    # memory value
    local mem_val=$(free -h | awk '/^内存/ { print $3 }' | sed s/i//g)
    # colorscheme
    printf "\x03^c$blue^^b$black^"
    # output
    printf "${icons[memory]}$mem_val"
}

# CPU average load
print_cpu() {
    # cpu load value
    local cpu_val=$(grep -o "^[^ ]*" /proc/loadavg)
    # colorscheme
    printf "\x04^c$red^^b$grey^"
    # output
    printf "${icons[cpu]}$cpu_val"

    # append cpu temperature
    if [[ $(getConfProp showTemp) -eq 1 ]]; then
        printf "  $(print_temp)"
    fi
}

# Temperature of CPU
print_temp() {
    # test get temperature file
    test -f /sys/class/thermal/thermal_zone0/temp || return 0
    # temperature value
    local temp=$(head -c 2 /sys/class/thermal/thermal_zone0/temp)

    # output
    printf "${icons[temp]}${temp}°C"
}

# Disk free space size
# disk path in variable $disk_root
print_disk() {
    # root disk space value
    local disk_root=$(df -h | grep /dev/sda2 | awk '{print $4}')
    # colorscheme
    printf "\x02^c$grey^^b$white^"
    # output
    printf "${icons[disk]}$disk_root"
}

# Datetime
print_date() {
    # colorscheme
    printf "\x01^c$white^^b$black^"
    # output
    if [[ $(getConfProp dateExp) -eq 1 ]]; then
        printf "$(date '+ %m-%d(%a)  %H:%M')"
    else
        printf "$(date '+ %H:%M')"
    fi
}

# Music Player Daemon
print_mpd() {
    # determine whether showMpd property is on
    if [[ $(getConfProp showMpd) -eq 0 ]]; then
        return
    fi
    # determine whether mpd is started
    if [[ -z "$(mpc status)" ]]; then
        return
    fi

    # task max length
    maxLen=16

    songName=$(mpc -f "%title% - %artist%" current)

    # to fill task
    spaceStr="                              "
    # empty task string
    spaces=${spaceStr:1:$maxLen}

    # like this
    # "      songname        "
    songName=$spaces$songName$spaces

    offset=$(($(date +%s) % $((${#songName} - $maxLen))))

    songName=${songName:$offset:$maxLen}

    # calculate mpd play status
    if [[ $(mpc status) == *"[playing]"* ]]; then
        icon=""
    else
        icon=""
    fi
    # colorscheme
    printf "\x06^c$green^^b$black^"
    # output
    printf "${icons[mpd]} $songName $icon"
}

# Update weather to $weather_path
function update_weather() {
    # more look at: https://github.com/chubin/wttr.in
    # 获取主机使用语言
    # local language="en"
    local language=$(cat /etc/locale.conf | awk -F '=' '{print $2}' | awk -F '_' '{print $1}')
    weather=$(curl -H "Accept-Language:"$language -s -m 1 "wttr.in?format=%c\[%C\]+%t\n")
    if [[ $weather != "" ]]; then
        echo $weather'?'$(date +'%Y-%m-%d %H:%M') >$weather_path
    fi
}

print_weather() {
    if [ ! -f $weather_path ]; then
        touch $weather_path
    fi
    local date=$(cat $weather_path | awk -F '?' '{print $2}')
    if [[ $date == "" ]]; then
        update_weather &
    fi
    # 计算两次请求时间间隔
    # 如果时间间隔超过$weather_update_duration秒,则更新天气状态
    local duration=$(($(date +%s) - $(date -d "$date" +%s)))
    if [[ $duration > $weather_update_duration ]]; then
        update_weather &
    fi

    # colorscheme
    printf "\x07^c$darkblue^^b$grey^$(cat $weather_path | awk -F '?' '{print $1}')"
}

get_bytes

# Calculates velocity
vel_recv="$(get_velocity $received_bytes $old_received_bytes $now)"
vel_trans="$(get_velocity $transmitted_bytes $old_transmitted_bytes $now)"

# Network velocity
print_speed() {
    # define the calculated upper and lower symbols
    local recvIcon=" "
    local transIcon=" "
    if [ $received_bytes -ne $old_received_bytes ]; then
        recvIcon=""
    fi
    if [ $transmitted_bytes -ne $old_transmitted_bytes ]; then
        transIcon=""
    fi
    # colorscheme
    printf "\x05^c$black^^b$blue^"
    # output
    if [[ $(getConfProp netSpeedExp) -eq 1 ]]; then
        printf "${icons[netSpeed]}${recvIcon} $vel_recv ${transIcon} $vel_trans"
    else
        printf "${icons[netSpeed]}${recvIcon} ${transIcon}"
    fi
}

xsetroot -name "$(print_mpd)$(print_weather)$(print_speed)$(print_cpu)$(print_mem)$(print_disk)$(print_date)"

# Update old values to perform new calculation
old_received_bytes=$received_bytes
old_transmitted_bytes=$transmitted_bytes
old_time=$now

exit 0
