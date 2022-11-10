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

if [[ ("$theme" == *'type-1'*) || ("$theme" == *'type-3'*) || ("$theme" == *'type-5'*) ]]; then
  list_col='1'
  list_row='6'
elif [[ ("$theme" == *'type-2'*) || ("$theme" == *'type-4'*) ]]; then
  list_col='6'
  list_row='1'
fi

#
runIcon() {
  if [[ -n $(pgrep $1) ]]; then
    echo " "
  else
    echo " "
  fi
}

# 定义运行命令的Map
declare -A runcmd
runcmd["picom"]="picom --config $HOME/.dwm/configs/picom.conf -b --experimental-backends"

toggleApplication() {
  if [[ -n $(pgrep $1) ]]; then
    killall $1
  else
    ${runcmd[$1]}
  fi
}

declare -A confPath
confPath["picom"]="$HOME/.dwm/configs/picom.conf"

typeToValue() {
  case "$1" in
  bool)
    echo false
    ;;
  int)
    echo 0
    ;;
  esac
}

confIcon() {
  flag=$(typeToValue $3)
  if [[ -n $(cat ${confPath[$1]} | grep $2 | grep $flag) ]]; then
    echo " "
  else
    echo " "
  fi
}

toggleConf() {
  flag=$(typeToValue $3)
  line=$(cat ${confPath[$1]} | grep $2 -n | awk -F ':' '{print $1}')
  if [[ -n $(cat ${confPath[$1]} | grep $2 | grep $flag) ]]; then
    sed -i $line' s/false/true/' ${confPath[$1]}
  else
    sed -i $line' s/true/false/' ${confPath[$1]}
  fi
}

# Options
layout=$(cat ${theme} | grep 'USE_ICON' | cut -d'=' -f2)
if [[ "$layout" == 'NO' ]]; then
  option_1=" Picom                 $(runIcon picom)"
  option_2=" Picom Animation       $(confIcon picom ^animations bool)"
else
  option_1=" $(runIcon picom)"
  option_2=" $(confIcon picom ^animations bool)"
fi

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
  echo -e "$option_1\n$option_2" | rofi_cmd
}

# Execute Command
run_cmd() {
  if [[ "$1" == '--opt1' ]]; then
    toggleApplication picom
  elif [[ "$1" == '--opt2' ]]; then
    toggleConf picom ^animations bool
  fi
}

# Actions
chosen="$(run_rofi)"
case ${chosen} in
$option_1)
  run_cmd --opt1
  ;;
$option_2)
  run_cmd --opt2
  ;;
esac
