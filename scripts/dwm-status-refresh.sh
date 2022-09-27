#!/bin/bash
# Screenshot: http://s.natalian.org/2013-08-17/dwm_status.png
# Network speed stuff stolen from http://linuxclues.blogspot.sg/2009/11/shell-script-show-network-speed.html

# This function parses /proc/net/dev file searching for a line containing $interface data.
# Within that line, the first and ninth numbers after ':' are respectively the received and transmited bytes.
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

print_net() {
    lan=$(ip addr | grep 'enp0s20f0u4u3' | grep 'inet' | awk '{print $2}')
    if [[ "$lan" != "" ]]; then
        echo -e "${lan} "
    fi
}

print_wlan() {
    wlan=$(ip addr | grep 'wlp2s0' | grep 'inet' | awk '{print $2}')
    if [[ "$wlan" != "" ]]; then
        echo -e "直${wlan} "
    fi
}

print_mem() {
    memtotal=$(($(grep -m1 'MemTotal:' /proc/meminfo | awk '{print $2}') / 1024))
    memavailable=$(($(grep -m1 'MemAvailable:' /proc/meminfo | awk '{print $2}') / 1024))

    mem=$(echo "scale=2;${memtotal}-${memavailable}" | bc)

    printf '\x03'
    if [ $mem -gt 1024 ]; then
        # memdec=$(( mem % 1024 * 976 / 10000 ))
        # mem=$(( mem / 1024 ))
        # echo -e "$mem.${memdec}G"

        mem=$(echo "scale=1;${mem}/1024" | bc)
        echo -e " ${mem}G"
    else
        echo -e " ${mem}M"
    fi
}

# CPU
print_cpu() {
    a=($(cat /proc/stat | grep -E "cpu\b" | awk -v total=0 '{$1="";for(i=2;i<=NF;i++){total+=$i};used=$2+$3+$4+$7+$8 }END{print total,used}'))
    sleep 1
    b=($(cat /proc/stat | grep -E "cpu\b" | awk -v total=0 '{$1="";for(i=2;i<=NF;i++){total+=$i};used=$2+$3+$4+$7+$8 }END{print total,used}'))
    # cpuload=(`cat /proc/loadavg | awk '{print $1}'`)
    cpu_usage=$(((${b[1]} - ${a[1]}) * 100 / (${b[0]} - ${a[0]})))

    isShowTemp=$(cat ~/.dwm/configs/statusConf | grep "show_temp" | tail -n 1 | awk -F '=' '{print $2}')
    if [[ $isShowTemp -eq 1 ]]; then
        printf "\x04 %2d%% $(print_temp)" ${cpu_usage}
    else
        printf "\x04 %2d%%" ${cpu_usage}
    fi
}

print_temp() {
    icons=("" "" "" "" "")

    test -f /sys/class/thermal/thermal_zone0/temp || return 0
    temp=$(head -c 2 /sys/class/thermal/thermal_zone0/temp)

    index=$((temp / 20))
    icon=${icons[$index]}
    echo -e "${icon} ${temp}°C"
}

print_disk() {
    disk_root=$(df -h | grep /dev/sda2 | awk '{print $4}')
    icon="﫭"
    if [[ $(echo $disk_root | awk -F 'G' '{print $1}') -le 10 ]]; then
        icon=""
    fi
    echo -e "\x02$icon $disk_root "
}

print_date() {
    printf '\x01'
    isExp=$(cat ~/.dwm/configs/statusConf | grep "date_exp" | tail -n 1 | awk -F '=' '{print $2}')

    # '+ %x(%a) %H:%M'
    # '+%H:%M'
    if [[ $isExp -eq 1 ]]; then
        date '+%x(%a) %H:%M '
    else
        date '+%H:%M '
    fi
}

# LOC=$(readlink -f "$0")
# DIR=$(dirname "$LOC")
# export IDENTIFIER="unicode"

get_bytes

# Calculates speeds
vel_recv="$(get_velocity $received_bytes $old_received_bytes $now)"
vel_trans="$(get_velocity $transmitted_bytes $old_transmitted_bytes $now)"

print_speed() {
    recvIcon=" "
    transIcon=" "
    if [ $received_bytes -ne $old_received_bytes ]; then
        recvIcon="ﰬ"
    fi
    if [ $transmitted_bytes -ne $old_transmitted_bytes ]; then
        transIcon="ﰵ"
    fi
    isExp=$(cat ~/.dwm/configs/statusConf | grep "net_speed_exp" | tail -n 1 | awk -F '=' '{print $2}')

    if [[ $isExp -eq 1 ]]; then
        echo -e "\x05  ${recvIcon} $vel_recv ${transIcon} $vel_trans "
    else
        echo -e "\x05   ${recvIcon} ${transIcon} "
    fi
}

xsetroot -name "$(print_speed) $(print_cpu)  $(print_mem)  $(print_disk) $(print_date)"
# Update old values to perform new calculation
old_received_bytes=$received_bytes
old_transmitted_bytes=$transmitted_bytes
old_time=$now

exit 0
