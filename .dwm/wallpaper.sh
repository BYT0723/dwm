#!/bin/sh
[ -z "$1" ] && exit

killall xwinwrap

sleep 0.3

# no-osc 关闭进度条等
# loop-file 循环播放
# panscan 裁剪图像比例
# no-keepaspect 强制拉伸为屏幕大小
# no-input-default-bindings 关闭键位响应
# input-conf 指定键位配置

Type=$(echo "${1#*.}")

case "$Type" in
  mp4 | mkv | avi ) Type="video"
  ;;
  jpg | png ) Type="img"
  ;;
  html | htm ) Type="page"
  ;;
  *) Type="video"
  ;;
esac


if [[ $Type == "video" ]]; then
  nohup xwinwrap -ov -g 1920x1080+0+0 -- mpv -wid WID "$1" --no-osc --no-osd-bar --loop-file --player-operation-mode=cplayer --no-input-default-bindings --input-conf=/home/tao/.dwm/wallpaperKeyMap.conf --hwdec&
  echo "xwinwrap -ov -g 1920x1080+0+0 -- mpv -wid WID "$1" --no-osc --no-osd-bar --loop-file --player-operation-mode=cplayer --no-input-default-bindings --input-conf=/home/tao/.dwm/wallpaperKeyMap.conf --hwdec" > ~/.dwm/background.sh
elif [[ $Type == "img" ]]; then
  feh --bg-scale --no-fehbg "$1" 
  echo "feh --no-fehbg --bg-scale "$1 > ~/.dwm/background.sh
elif [[ $Type == "page" ]]; then
  xwinwrap -ov -g 1920x1080+0+0 -- firefox --safe-model --kiosk --new-window "$1" &
fi
