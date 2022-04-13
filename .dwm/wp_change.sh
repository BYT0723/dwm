#!/bin/bash

dir=~/Wallpapers/Dynamic_Wallpapers/

if [ $# -ge 1 ]; then
    if [ "$1" == "light" ]; then
        dir=~/Wallpapers/common/light
    elif [ "$1" == "dark" ]; then
        dir=~/Wallpapers/common/dark
    elif [ "$1" == "cos" ]; then
        dir=~/Wallpapers/mirrorCosplay
    elif [ "$1" == "mirror" ]; then
        dir=~/Wallpapers/mirrorEasterEggs
    elif [ "$1" == "dynamic" ]; then
        dir=~/Wallpapers/Dynamic_Wallpapers
    fi
fi

feh --recursive --randomize --bg-scale $dir
