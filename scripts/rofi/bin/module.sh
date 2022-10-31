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

# Options
layout=$(cat ${theme} | grep 'USE_ICON' | cut -d'=' -f2)
if [[ "$layout" == 'NO' ]]; then
  option_1=" Picom (compositor)    $(runIcon picom)"
else
  option_1=" $(runIcon picom)"
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
  echo -e "$option_1" | rofi_cmd
}

# Execute Command
run_cmd() {
  if [[ "$1" == '--opt1' ]]; then
    toggleApplication picom
  fi
}

# Actions
chosen="$(run_rofi)"
case ${chosen} in
$option_1)
  run_cmd --opt1
  ;;
esac
