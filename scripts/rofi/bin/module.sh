#!/usr/bin/env bash

## Author  : Aditya Shakya (adi1090x)
## Github  : @adi1090x
#
## Applets : Favorite Applications

# Import Current Theme
type="$HOME/.dwm/rofi/applets/type-1"
style='style-2.rasi'
theme="$type/$style"

# Theme Elements
prompt='Module'
mesg="Manage Module Of System"

HistoryPopCount=10

if [[ ("$theme" == *'type-1'*) || ("$theme" == *'type-3'*) || ("$theme" == *'type-5'*) ]]; then
    list_col='1'
    list_row='6'
elif [[ ("$theme" == *'type-2'*) || ("$theme" == *'type-4'*) ]]; then
    list_col='6'
    list_row='1'
fi

# 定义配置文件位置
declare -A confPath
confPath["picom"]="$HOME/.dwm/configs/picom.conf"
confPath["statusBar"]="$HOME/.dwm/configs/statusConf"

# 定义运行命令的Map
declare -A applicationCmd
applicationCmd["picom"]="picom --config $HOME/.dwm/configs/picom.conf -b --experimental-backends"

source $HOME/.dwm/rofi/bin/util.sh

# Options
layout=$(cat ${theme} | grep 'USE_ICON' | cut -d'=' -f2)

if [[ "$layout" == 'NO' ]]; then
    firstOpt=(
        " Notification             $(icon active app dunst)"
        " Network                  $(icon active app NetworkManager)"
        " Bluetooth                $(icon active service bluetooth)"
        " Picom                    $(icon active app picom)"
        " StatusBar"
    )
    barOpt=(
        " NetSpeed                $(icon toggle conf statusBar ^net_speed_exp number)"
        " Template                $(icon toggle conf statusBar ^show_temp number)"
        " DateTime                $(icon toggle conf statusBar ^date_exp number)"
    )
    networkOpt=$(nmcli connection show | grep -E 'wifi' | awk '{print $1}')
    bluetoothOpt=$(bluetoothctl devices Trusted | awk '{print substr($0,26,20)}')
    picomOpt=(
        "蘒 Toggle                  $(icon toggle app picom)"
        "𧻓 Animation               $(icon toggle conf picom ^animations bool)"
    )
    notificationOpt=(
        "Pop                      $(dunstctl count history)"
        "CloseAll                 $(dunstctl count displayed)"
    )
else
    firstOpt=(
        " $(icon active app dunst)"
        " $(icon active app NetworkManager)"
        " $(icon active service bluetooth)"
        " $(icon active app picom)"
        " "
    )
    barOpt=(
        " $(icon toggle conf statusBar ^net_speed_exp number)"
        " $(icon toggle conf statusBar ^show_temp number)"
        " $(icon toggle conf statusBar ^date_exp number)"
    )
    networkOpt=$(nmcli connection show | grep -E 'wifi' | awk '{print $1}')
    bluetoothOpt=$(bluetoothctl devices Trusted | awk '{print substr($0,26,20)}')
    picomOpt=(
        "蘒$(icon toggle app picom)"
        "𧻓$(icon toggle conf picom ^animations bool)"
    )
    notificationOpt=(
        "P$(dunstctl count history)"
        "C$(dunstctl count displayed)"
    )
fi

declare -A optId
optId[${firstOpt[0]}]="--opt1"
optId[${firstOpt[1]}]="--opt2"
optId[${firstOpt[2]}]="--opt3"
optId[${firstOpt[3]}]="--opt4"
optId[${firstOpt[4]}]="--opt5"

optId[${barOpt[0]}]="--barOpt1"
optId[${barOpt[1]}]="--barOpt2"
optId[${barOpt[2]}]="--barOpt3"

optId[${picomOpt[0]}]="--picomOpt1"
optId[${picomOpt[1]}]="--picomOpt2"

optId[${notificationOpt[0]}]="--notificationOpt1"
optId[${notificationOpt[1]}]="--notificationOpt2"

# Rofi CMD
rofi_cmd() {
    rofi -theme-str "listview {columns: $list_col; lines: $list_row;}" \
        -theme-str 'textbox-prompt-colon {str: " ";}' \
        -dmenu \
        -p "$prompt" \
        -mesg "$mesg" \
        -markup-rows \
        -theme ${theme}
}

# Pass variables to rofi dmenu
run_rofi() {
    if [[ "$1" == ${optId[${firstOpt[0]}]} ]]; then
        prompt='Notification'
        mesg="Dunst Notification Manager"
        opts=("${notificationOpt[@]}")
    elif [[ "$1" == ${optId[${firstOpt[1]}]} ]]; then
        prompt='Network'
        mesg="Eth: $(nmcli connection show -active | grep -E 'eth' | awk '{print $1}')
Wifi: $(nmcli connection show -active | grep -E 'wifi' | awk '{print $1}')"
        opts=("${networkOpt[@]}")
    elif [[ "$1" == ${optId[${firstOpt[2]}]} ]]; then
        prompt='Bluetooth'
        mesg="Connected: $(bluetoothctl devices Connected | awk '{print substr($0,26,20)}')"
        opts=("${bluetoothOpt[@]}")
    elif [[ "$1" == ${optId[${firstOpt[3]}]} ]]; then
        prompt='Picom'
        opts=("${picomOpt[@]}")
    elif [[ "$1" == ${optId[${firstOpt[4]}]} ]]; then
        prompt='StatusBar'
        mesg="Dwm Status Bar"
        opts=("${barOpt[@]}")
    else
        opts=("${firstOpt[@]}")
    fi

    for ((i = 0; i < ${#opts[@]}; i++)); do
        if [[ $i > 0 ]]; then
            msg=$msg"\n"
        fi
        msg=$msg${opts[$i]}
    done
    echo -e "$msg" | rofi_cmd
}

# Execute Command
run_cmd() {
    case "$1" in
    ${optId[${barOpt[0]}]})
        toggleConf statusBar ^net_speed_exp number
        ;;
    ${optId[${barOpt[1]}]})
        toggleConf statusBar ^show_temp number
        ;;
    ${optId[${barOpt[2]}]})
        toggleConf statusBar ^date_exp number
        ;;
    ${optId[${picomOpt[0]}]})
        toggleApplication picom
        ;;
    ${optId[${picomOpt[1]}]})
        toggleConf picom ^animations bool
        ;;
    ${optId[${notificationOpt[0]}]})
        for ((i = 0; i < $HistoryPopCount; i++)); do
            dunstctl history-pop
        done
        ;;
    ${optId[${notificationOpt[1]}]})
        dunstctl close-all
        ;;
    ${optId[${firstOpt[1]}]})
        chosen="$(run_rofi $1)"
        if [[ "$chosen" == "" || "$chosen" == "$(nmcli connection show -active | grep -E 'wifi' | awk '{print $1}')" ]]; then
            exit
        fi
        nmcli connection up $chosen
        ;;
    ${optId[${firstOpt[2]}]})
        chosen="$(run_rofi $1)"
        if [[ "$chosen" == "" ]]; then
            exit
        fi
        bluetoothctl connect
        nmcli connection up $chosen
        ;;
    *)
        chosen="$(run_rofi $1)"
        if [[ "$chosen" == "" ]]; then
            exit
        fi
        run_cmd ${optId[$chosen]}
        ;;
    esac
}

# Actions
chosen="$(run_rofi)"
if [[ "$chosen" == "" ]]; then
    exit
fi
run_cmd ${optId[$chosen]}
