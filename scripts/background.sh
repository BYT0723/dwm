#!/bin/bash

conf="$HOME/.dwm/configs/wallpaper.conf"

getWallpaperProp() {
    echo $(cat $conf | grep "$1=" | tail -n 1 | awk -F '=' '{print $2}')
}

next() {
    case "$(getWallpaperProp random_type)" in
    "video")
        if [[ -n $(pgrep xwinwrap) ]]; then
            killall xwinwrap
        fi

        sleep 0.3

        len=$(find $(getWallpaperProp random_video_dir) -type f | wc -l)
        filename=$(find $(getWallpaperProp random_video_dir) -type f | awk 'NR=='$(($RANDOM % $len + 1))' {print $0}')

        nohup xwinwrap -ov -fs -- mpv -wid WID "$filename" --mute --no-osc --no-osd-bar --loop-file --player-operation-mode=cplayer --no-input-default-bindings --input-conf=~/.dwm/configs/wallpaperKeyMap.conf >/dev/null 2>&1 &
        ;;
    "image")
        feh --bg-scale --no-fehbg -z $(getWallpaperProp random_image_dir)
        ;;
    esac
}

if [ -f $conf ]; then
    cmd=$(getWallpaperProp cmd)
    if [ $(getWallpaperProp random) -eq 1 ]; then
        while true; do
            next
            sleep $(($(getWallpaperProp duration) * 60))
        done
    else
        $cmd
    fi
fi
