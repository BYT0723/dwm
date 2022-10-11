#!/bin/bash

# color
black=#1e222a
green=#7eca9c
white=#abb2bf
grey=#282c34
blue=#7aa2f7
red=#d47d85
darkblue=#668ee3

# Screenshot: http://s.natalian.org/2013-08-17/dwm_status.png
# Network speed stuff stolen from http://linuxclues.blogspot.sg/2009/11/shell-script-show-network-speed.html

# This function parses /proc/net/dev file searching for a line containing $interface data.

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

print_mem() {
  printf '\x03'
  printf "^c$blue^^b$black^  $(free -h | awk '/^内存/ { print $3 }' | sed s/i//g) "
}

# CPU
print_cpu() {
  printf '\x04'

  cpu_val=$(grep -o "^[^ ]*" /proc/loadavg)

  isShowTemp=$(cat ~/.dwm/configs/statusConf | grep "show_temp" | tail -n 1 | awk -F '=' '{print $2}')
  printf "^c$white^^b$grey^  $cpu_val "

  if [[ $isShowTemp -eq 1 ]]; then
    printf "$(print_temp)"
  fi
}

print_temp() {
  icons=("" "" "" "" "")

  test -f /sys/class/thermal/thermal_zone0/temp || return 0
  temp=$(head -c 2 /sys/class/thermal/thermal_zone0/temp)

  index=$((temp / 20))
  icon=${icons[$index]}
  printf "^c$black^^b$red^ ${icon} ${temp}°C "
}

print_disk() {
  printf '\x02'
  disk_root=$(df -h | grep /dev/sda2 | awk '{print $4}')
  icon="﫭"
  if [[ $(echo $disk_root | awk -F 'G' '{print $1}') -le 10 ]]; then
    icon=""
  fi
  printf "^c$grey^^b$white^ $icon $disk_root "
}

print_date() {
  printf '\x01'
  isExp=$(cat ~/.dwm/configs/statusConf | grep "date_exp" | tail -n 1 | awk -F '=' '{print $2}')

  if [[ $isExp -eq 1 ]]; then
    printf "^c$black^^b$blue^  $(date '+%x(%a) %H:%M') "
  else
    printf "^c$black^^b$blue^  $(date '+%H:%M') "
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
  printf '\x05'
  printf "^c$black^^b$blue^   "
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
    echo -e "${recvIcon} $vel_recv ${transIcon} $vel_trans "
  else
    echo -e "${recvIcon} ${transIcon} "
  fi
}

xsetroot -name "$(print_speed)$(print_cpu)$(print_mem)$(print_disk)$(print_date)"
# Update old values to perform new calculation
old_received_bytes=$received_bytes
old_transmitted_bytes=$transmitted_bytes
old_time=$now

exit 0
