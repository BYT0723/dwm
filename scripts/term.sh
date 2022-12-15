#!/bin/bash

# Termina Manager
# launch command in file config.h varialbe termcmd

Type=$1

# new different termina by $Type
case "$Type" in
float)
    st -i -g 90x25+480+200
    ;;
*)
    # st
    alacritty
    ;;
esac
