#!/usr/bin/env bash

## Author  : Aditya Shakya (adi1090x)
## Github  : @adi1090x
#
## Applets : MPD (music)

# Import Current Theme
type="$HOME/.dwm/rofi/applets/type-2"
style='style-3.rasi'
theme="$type/$style"

# Theme Elements
status="$(mpc status)"
textboxPromptColon="  | "
if [[ -z "$status" ]]; then
    prompt='Offline'
    mesg="MPD is Offline"
else
    prompt="$(mpc -f "%title% - %artist%" current)"
    mesg="$(mpc status | grep "#" | awk '{print $3}')  墳 $(mpc volume | awk -F ':' '{print $2}')"
fi

if [[ ("$theme" == *'type-1'*) || ("$theme" == *'type-3'*) || ("$theme" == *'type-5'*) ]]; then
    list_col='1'
    list_row='9'
elif [[ ("$theme" == *'type-2'*) || ("$theme" == *'type-4'*) ]]; then
    list_col='9'
    list_row='1'
fi

# Options
layout=$(cat ${theme} | grep 'USE_ICON' | cut -d'=' -f2)
if [[ "$layout" == 'NO' ]]; then
    option_power="⏻ Start Local MPD"
    if [[ ${status} == *"[playing]"* ]]; then
        option_1=" Pause"
    else
        option_1=" Play"
    fi
    option_2=" Stop"
    option_3=" Previous"
    option_4=" Next"
    option_5="ﱜ Down"
    option_6="ﱛ Up"
    option_7=" Repeat"
    option_8=" Random"
else
    option_power="⏻"
    if [[ ${status} == *"[playing]"* ]]; then
        option_1=""
    else
        option_1=""
    fi
    option_2=""
    option_3=""
    option_4=""
    option_5="ﱜ"
    option_6="ﱛ"
    option_7=""
    option_8=""
fi

# Toggle Actions
active=''
urgent=''
# Repeat
if [[ ${status} == *"repeat: on"* ]]; then
    active="-a 7"
elif [[ ${status} == *"repeat: off"* ]]; then
    urgent="-u 7"
else
    option_7=" Parsing Error"
fi
# Random
if [[ ${status} == *"random: on"* ]]; then
    [ -n "$active" ] && active+=",8" || active="-a 8"
elif [[ ${status} == *"random: off"* ]]; then
    [ -n "$urgent" ] && urgent+=",8" || urgent="-u 8"
else
    option_8=" Parsing Error"
fi

# Rofi CMD
rofi_cmd() {
    rofi -theme-str "listview {columns: $list_col; lines: $list_row;}" \
        -theme-str 'textbox-prompt-colon {str: " ";}' \
        -dmenu \
        -p "$prompt" \
        -mesg "$mesg" \
        ${active} ${urgent} \
        -markup-rows \
        -theme ${theme}
}

# Pass variables to rofi dmenu
run_rofi() {
    if [[ -z "$status" ]]; then
        echo -e "$option_power" | rofi_cmd
    else
        echo -e "$option_1\n$option_2\n$option_3\n$option_4\n$option_5\n$option_6\n$option_7\n$option_8" | rofi_cmd
    fi
}

# Execute Command
run_cmd() {
    if [[ "$1" == '--on' ]]; then
        mpd
    elif [[ "$1" == '--opt1' ]]; then
        mpc -q toggle && notify-send -u low "$prompt $(mpc status | awk 'NR==2 {print $1}')"
    elif [[ "$1" == '--opt2' ]]; then
        mpc -q stop
    elif [[ "$1" == '--opt3' ]]; then
        mpc -q prev && notify-send -u low " $prompt"
    elif [[ "$1" == '--opt4' ]]; then
        mpc -q next && notify-send -u low " $prompt"
    elif [[ "$1" == '--opt5' ]]; then
        mpc volume -20 && notify-send -u low " $(mpc volume)"
    elif [[ "$1" == '--opt6' ]]; then
        mpc volume +20 && notify-send -u low " $(mpc volume)"
    elif [[ "$1" == '--opt7' ]]; then
        mpc -q repeat
    elif [[ "$1" == '--opt8' ]]; then
        mpc -q random
    fi
}

# Actions
chosen="$(run_rofi)"
case ${chosen} in
$option_power)
    run_cmd --on
    ;;
$option_1)
    run_cmd --opt1
    ;;
$option_2)
    run_cmd --opt2
    ;;
$option_3)
    run_cmd --opt3
    ;;
$option_4)
    run_cmd --opt4
    ;;
$option_5)
    run_cmd --opt5
    ;;
$option_6)
    run_cmd --opt6
    ;;
$option_7)
    run_cmd --opt7
    ;;
$option_8)
    run_cmd --opt8
    ;;
esac
