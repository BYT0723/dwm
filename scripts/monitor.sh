#!/bin/bash

filepath="$HOME/.schedule"

while true; do
    sleep 60
    # 获取文件的最后修改时间
    timestamp=$(stat --format=%Y $filepath)

    # 如果最后修改时间与当前时间的差值小于60
    if [[ $(($(date +%s) - $timestamp)) -le 60 ]]; then
        # 关闭 clock.sh 进程
        id=$(ps ax | grep $HOME/.dwm/alarm.sh | awk 'NR==1 {print $1}')
        kill -9 $id
        # 重新启动 alarm.sh
        /bin/bash ~/.dwm/alarm.sh &
    fi
done
