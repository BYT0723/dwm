#!/bin/bash
# Screenshot: http://s.natalian.org/2013-08-17/dwm_status.png
# Network speed stuff stolen from http://linuxclues.blogspot.sg/2009/11/shell-script-show-network-speed.html

# 
# 
#  
# 
splitChar="  "

# status colors
c1=`printf "%x" 1`
c2=`printf "%x" 2`
c3=`printf "%x" 3`
c4=`printf "%x" 4`
c5=`printf "%x" 5`

# This function parses /proc/net/dev file searching for a line containing $interface data.
# Within that line, the first and ninth numbers after ':' are respectively the received and transmited bytes.
function get_bytes {
	# Find active network interface
	interface=$(ip route get 8.8.8.8 2>/dev/null| awk '{print $5}')
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
	if [ $velKB -gt 1000 ]
	then
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

print_net(){
    lan=$(ip addr | grep 'enp0s20f0u4u3'| grep 'inet'| awk '{print $2}')
    if [[ "$lan" != "" ]]; then 
        echo -e "\x$c1${lan} "
    fi
}

print_wlan(){
    wlan=$(ip addr | grep 'wlp2s0'| grep 'inet'| awk '{print $2}')
    if [[ "$wlan" != "" ]]; then
        echo -e "\x$c1直${wlan} "
    fi
}

print_mem(){
	memtotal=$(($(grep -m1 'MemTotal:' /proc/meminfo | awk '{print $2}') / 1024))
	memavailable=$(($(grep -m1 'MemAvailable:' /proc/meminfo | awk '{print $2}') / 1024))

    mem=$( echo "scale=2;${memtotal}-${memavailable}" | bc )

    if [ $mem -gt 1024 ]; then
        # memdec=$(( mem % 1024 * 976 / 10000 ))
        # mem=$(( mem / 1024 ))
        # echo -e "$mem.${memdec}G"

        mem=$( echo "scale=1;${mem}/1024" | bc )
        echo -e "\x$c2 ${mem}G"
    else
        echo -e "\x$c2 ${mem}M"
    fi
}

# CPU
print_cpu(){
    a=(`cat /proc/stat | grep -E "cpu\b" | awk -v total=0 '{$1="";for(i=2;i<=NF;i++){total+=$i};used=$2+$3+$4+$7+$8 }END{print total,used}'`)
    sleep 1
    b=(`cat /proc/stat | grep -E "cpu\b" | awk -v total=0 '{$1="";for(i=2;i<=NF;i++){total+=$i};used=$2+$3+$4+$7+$8 }END{print total,used}'`)
    cpuload=(`cat /proc/loadavg | awk '{print $1}'`)
    cpu_usage=$(((${b[1]}-${a[1]})*100/(${b[0]}-${a[0]})))
    echo -e "${cpu_usage}%, $cpuload, $(print_temp)"
}

print_temp(){
    icons=("" "" "" "" "")

	test -f /sys/class/thermal/thermal_zone0/temp || return 0
    temp=$(head -c 2 /sys/class/thermal/thermal_zone0/temp)

    index=$(( temp / 20 ))
    icon=${icons[$index]}
    echo -e "\x$c4 ${icon}${temp}°C"
}

print_mpd() {
    result=$(ps ax|grep -v grep|grep mpd)
    stat=$(mpc | grep "#" | awk '{print $1}')
    # icon=""

    if [[ "$result" != "" ]]; then
        if [[ "$stat" == "[playing]" ]]; then
            echo "$(songName)"
        else
            echo "$(songName)"
        fi
    fi
}

songName() {
    time=$(date "+%s")
    song=$(mpc current | awk -F . '{print $1}')
    sl=${#song}
    cap=10

    board="                      "
    index=$((time % (cap + sl)))

    if [ $index -le $cap ]; then
        n=$((cap-index))
        if [ $index -ge $sl ]; then
            m=$((index-sl))
        fi
        song="${board:0:$n}${song:0:$index}${board:0:$m}"
    else
        index=$((index-cap))
        if [ $((sl-index)) -le $cap ]; then
            n=$((cap-sl+index))
        fi
        song="${song:$index:$cap}${board:0:$n}"
    fi
    echo "$song"
}



print_disk(){
    disk_root=$(df -h | grep /dev/sda2 |awk '{print $4}');
    disk_home=$(df -h | grep /dev/sda4 |awk '{print $4}');
    echo -e "$disk_root $disk_home"
}


get_time_until_charged() {
    # parses acpitool's battery info for the remaining charge of all batteries and sums them up
    # battery_remaining_time=$(acpi -b | grep 'remaining' | awk '{print $5}' | awk -F: '{print $1*60+$2}')
    # printf "%.1fh" $(echo "scale=1; $battery_remaining_time / 60" | bc)

    battery_remaining_hour=$(acpi -b | grep 'remaining' | awk '{print $5}' | awk -F: '{print $1}')
    battery_remaining_minute=$(acpi -b | grep 'remaining' | awk '{print $5}' | awk -F: '{print $2}')

    if [ $battery_remaining_hour -gt 0 ]; then
        printf "%.1fh" $(echo "scale=1; $battery_remaining_hour + $battery_remaining_minute / 60" | bc)
    else
        printf "%dm" ${battery_remaining_minute}
    fi
}

get_battery_combined_percent() {
	charge=$(expr $(acpi -b | awk -F , '{print $2}' | grep -Eo "[0-9]+" | paste -sd+ | bc));
	percent=${charge};
	echo $percent;
}

print_bat(){
    icons=("" "" "" "" "" "" "" "" "" "" "")
    chargIcons=("" "" "" "" "" "" "" "" "" "" "")
    remain=$(get_battery_combined_percent)

    index=$(( $remain / 10 ))
	if $(acpi -b | grep "Battery 0"| grep --quiet Charging);then
        echo -e "\x$c4 ${chargIcons[$index]}${remain} "
    else
        echo -e "\x$c4 ${icons[$index]}${remain}($(get_time_until_charged)) "
    fi
}


print_bright(){
    icons=("" "" "")
    maxBright=$(cat /sys/class/backlight/intel_backlight/max_brightness)
    now=$(cat /sys/class/backlight/intel_backlight/brightness)
    percent=$(( (now * 100) / maxBright))
    index=$(( percent / 34 ))
    icon=${icons[$index]}
    echo -e "\x$c2 ${icon}${percent} "
}

print_vol(){
    icons=("奄" "奔" "墳")
    volume="$(amixer get Master | tail -n1 | sed -r 's/.*\[(.*)%\].*/\1/')"
    flag="$(amixer get Master | tail -n1 |awk -F "[" '{print $3}'|awk -F "]" '{print $1}')"

    index=$(( $volume / 33 ))
    icon=${icons[$index]}
    if test "$flag" = "off"
    then
        icon="ﱝ"
    fi
    echo -e "\x$c1 ${icon}${volume} "
}

print_date(){
  printf "\\x$c1"
   date '+ %m-%d(%a) %H:%M'
    # date '+%m/%d %H:%M'
    # date '+[%B.%d %H:%M]'
    # date '+%H:%M'
}

print_note(){
    top=$(cat ~/.note | grep \# | head -n 1 | awk '{ print $2 }')
    echo -e "\x$c3${top}"
}

LOC=$(readlink -f "$0")
DIR=$(dirname "$LOC")
export IDENTIFIER="unicode"

get_bytes

# Calculates speeds
vel_recv="$(get_velocity $received_bytes $old_received_bytes $now)"
vel_trans="$(get_velocity $transmitted_bytes $old_transmitted_bytes $now)"

print_speed(){
    recvIcon="  "
    transIcon="  "
    if [ $received_bytes -ne $old_received_bytes ]; then
        recvIcon=""
    fi
    if [ $transmitted_bytes -ne $old_transmitted_bytes ]; then
        transIcon=""
    fi
    echo -e "\x$c2 龍${recvIcon}$vel_recv ${transIcon}$vel_trans "
}

# xsetroot -name "$(print_mpd)$(print_temp)$vel_recv $vel_trans$(print_net)$(print_wlan)$(print_cpu)$(print_mem)$(print_vol)$(print_bat)$(print_date)"
xsetroot -name "$(print_temp) $(print_mem) $(print_date);$(print_note);$(print_wlan)$(print_net)$(print_speed)$(print_vol)$(print_bright)$(print_bat)"
# Update old values to perform new calculation
old_received_bytes=$received_bytes
old_transmitted_bytes=$transmitted_bytes
old_time=$now

exit 0
