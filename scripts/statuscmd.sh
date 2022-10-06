#!/bin/bash

cmdType=$1
buttonType=$2

propToggle() {
  now=$(cat ~/.dwm/configs/statusConf | grep $1 | tail -n 1 | awk -F '=' '{print $2}')
  if [[ $now -eq 1 ]]; then
    sed -i "s/$1=1/$1=0/g" ~/.dwm/configs/statusConf
  else
    sed -i "s/$1=0/$1=1/g" ~/.dwm/configs/statusConf
  fi
}

dateHandler() {
  buttonType=$1
  case "$buttonType" in
  1)
    propToggle date_exp
    ;;
  2) ;;

  3) ;;

  esac
}

diskHandler() {
  buttonType=$1
  case "$buttonType" in
  1) ;;

  2) ;;

  3) ;;

  esac
}

memoryHandler() {
  buttonType=$1
  case "$buttonType" in
  1) ;;

  2) ;;

  3) ;;

  esac
}

cpuHandler() {
  buttonType=$1
  case "$buttonType" in
  1)
    propToggle show_temp
    ;;
  2) ;;

  3) ;;

  esac
}

netSpeedHandler() {
  buttonType=$1
  case "$buttonType" in
  1)
    propToggle net_speed_exp
    ;;
  2) ;;

  3) ;;

  esac
}

case "$cmdType" in
date)
  dateHandler $2
  ;;
disk-root)
  diskHandler $2
  ;;
memory)
  memoryHandler $2
  ;;
cpuInfo)
  cpuHandler $2
  ;;
netSpeed)
  netSpeedHandler $2
  ;;
esac
