#!/bin/bash

# statusBar Environment
source $HOME/.dwm/status-env.sh

# colorscheme
black=#1e222a
green=#7eca9c
white=#abb2bf
grey=#282c34
blue=#7aa2f7
red=#d47d85
darkblue=#668ee3

# icons initial
if [[ $(getConfProp showIcon) -eq 1 ]]; then
    declare -A icons
    icons["disk"]="﫭 "
    icons["memory"]=" "
    icons["temp"]=" "
    icons["cpu"]=" "
    icons["netSpeed"]=" "
fi

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

#  Output the current memory usage
print_mem() {
    printf '\x03'
    printf "^c$blue^^b$black^${icons["memory"]}$(free -h | awk '/^内存/ { print $3 }' | sed s/i//g)"
}

# Output the current cpu load
print_cpu() {
    printf '\x04'

    cpu_val=$(grep -o "^[^ ]*" /proc/loadavg)

    printf "^c$red^^b$grey^${icons[cpu]}$cpu_val"

    if [[ $(getConfProp showTemp) -eq 1 ]]; then
        printf "  $(print_temp)"
    fi
}

# Output current temperature of cpu
print_temp() {
    test -f /sys/class/thermal/thermal_zone0/temp || return 0
    temp=$(head -c 2 /sys/class/thermal/thermal_zone0/temp)

    printf "${icons[temp]}${temp}°C"
}

# Output Disk free space size
# disk path in variable $disk_root
print_disk() {
    printf '\x02'
    disk_root=$(df -h | grep /dev/sda2 | awk '{print $4}')
    printf "^c$grey^^b$white^${icons[disk]}$disk_root"
}

# Output current datetime
print_date() {
    printf '\x01'
    if [[ $(getConfProp dateExp) -eq 1 ]]; then
        printf "^c$black^^b$blue^$(date '+ %m-%d(%a)  %H:%M')"
    else
        printf "^c$black^^b$blue^$(date '+ %H:%M')"
    fi
}

get_bytes

# Calculates speeds
vel_recv="$(get_velocity $received_bytes $old_received_bytes $now)"
vel_trans="$(get_velocity $transmitted_bytes $old_transmitted_bytes $now)"

# Output network velocity
print_speed() {
    printf '\x05'
    recvIcon=" "
    transIcon=" "
    if [ $received_bytes -ne $old_received_bytes ]; then
        recvIcon=""
    fi
    if [ $transmitted_bytes -ne $old_transmitted_bytes ]; then
        transIcon=""
    fi

    if [[ $(getConfProp netSpeedExp) -eq 1 ]]; then
        echo -e "^c$black^^b$blue^${icons[netSpeed]}${recvIcon} $vel_recv ${transIcon} $vel_trans"
    else
        echo -e "^c$black^^b$blue^${icons[netSpeed]}${recvIcon} ${transIcon}"
    fi
}

xsetroot -name "$(print_speed)$(print_cpu)$(print_mem)$(print_disk)$(print_date)"

# Update old values to perform new calculation
old_received_bytes=$received_bytes
old_transmitted_bytes=$transmitted_bytes
old_time=$now

exit 0
