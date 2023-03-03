#!/usr/bin/env bash

# Import Current Theme
type="$HOME/.dwm/rofi/applets/type-1"
style='style-3.rasi'
theme="$type/$style"

source ~/.dwm/rofi/bin/util.sh
source ~/.dwm/status-env.sh

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

# Options
layout=$(cat ${theme} | grep 'USE_ICON' | cut -d'=' -f2)

if [[ "$layout" == 'NO' ]]; then
    firstOpt=(
        " StatusBar"
        " Picom                         $(icon active app picom)"
        " Network                       $(icon active app NetworkManager)"
        " Bluetooth                     $(icon active service bluetooth)"
        " Notification                  $(icon active app dunst)"
    )
    barOpt=(
        " ShowIcon                      $(icon toggle conf statusBar ^${confProperty[showIcon]} number)"
        " ShowMpd                       $(icon toggle conf statusBar ^${confProperty[showMpd]} number)"
        " NetSpeed                      $(icon toggle conf statusBar ^${confProperty[netSpeedExp]} number)"
        " Template                      $(icon toggle conf statusBar ^${confProperty[showTemp]} number)"
        " DateTime                      $(icon toggle conf statusBar ^${confProperty[dateExp]} number)"
    )
    picomOpt=(
        "蘒 Toggle                       $(icon toggle app picom)"
        "𧻓 Animation                    $(icon toggle conf picom ^animations bool)"
    )
    notificationOpt=(
        "Pop                            $(dunstctl count history)"
        "CloseAll                       $(dunstctl count displayed)"
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
        " $(icon toggle conf statusBar ^${confProperty[showIcon]} number)"
        " $(icon toggle conf statusBar ^${confProperty[showMpd]} number)"
        " $(icon toggle conf statusBar ^${confProperty[netSpeedExp]} number)"
        " $(icon toggle conf statusBar ^${confProperty[showTemp]} number)"
        " $(icon toggle conf statusBar ^${confProperty[dateExp]} number)"
    )
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
optId[${barOpt[3]}]="--barOpt4"

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
        prompt='StatusBar'
        mesg="Dwm Status Bar"
        opts=("${barOpt[@]}")
    elif [[ "$1" == ${optId[${firstOpt[1]}]} ]]; then
        prompt='Picom'
        mesg="Windows Composer"
        opts=("${picomOpt[@]}")
    elif [[ "$1" == ${optId[${firstOpt[2]}]} ]]; then
        prompt='Network'
        mesg=""
        eth="$(nmcli connection show -active | grep -E 'eth' | awk '{print $1}')"
        wifi="$(nmcli connection show -active | grep -E 'wifi' | awk '{print $1}')"
        if [ "$eth" != "" ]; then
            mesg="  $eth"
        fi
        if [[ "$wifi" != "" ]]; then
            if [ "$mesg" != "" ]; then
                mesg="$mesg
  $wifi [Connected]"
            else
                mesg="  $wifi [Connected]"
            fi
        fi
        opts=$(nmcli device wifi list --rescan auto | awk 'NR!=1 {print substr($0,9)}' | awk '{print $8," ",$2}' | awk '!a[$0]++')
    elif [[ "$1" == ${optId[${firstOpt[3]}]} ]]; then
        prompt='Bluetooth'
        connected_device=$(bluetoothctl devices Connected | awk '{print substr($0,25)}')
        if [ "$connected_device" != "" ]; then
            mesg="Connected:
$connected_device"
        else
            mesg="No device connected"
        fi
        opts=$(bluetoothctl devices | awk '{print substr($0,26)}')
    elif [[ "$1" == ${optId[${firstOpt[4]}]} ]]; then
        prompt='Notification'
        mesg="Dunst Notification Manager"
        opts=("${notificationOpt[@]}")
    else
        prompt='Module'
        mesg="Manage Module Of System"
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
        toggleConf statusBar ^${confProperty[showIcon]} number
        ;;
    ${optId[${barOpt[1]}]})
        toggleConf statusBar ^${confProperty[showMpd]} number
        ;;
    ${optId[${barOpt[2]}]})
        toggleConf statusBar ^${confProperty[netSpeedExp]} number
        ;;
    ${optId[${barOpt[3]}]})
        toggleConf statusBar ^${confProperty[showTemp]} number
        ;;
    ${optId[${barOpt[4]}]})
        toggleConf statusBar ^${confProperty[dateExp]} number
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
    ${optId[${firstOpt[2]}]})
        chosen="$(run_rofi $1)"
        if [[ "$chosen" == "" || "$chosen" == "$(nmcli connection show -active | grep -E 'wifi' | awk '{print $1}')" ]]; then
            exit
        fi
        nmcli device wifi connect $(echo $chosen | awk '{print $2}')
        ;;
    ${optId[${firstOpt[3]}]})
        chosen="$(run_rofi $1)"
        if [[ "$chosen" == "" ]]; then
            exit
        fi
        bluetoothctl disconnect $(bluetoothctl devices Connected | grep "$chosen" | awk '{print $2}')
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

exit
