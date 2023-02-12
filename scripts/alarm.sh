#!/bin/bash

alarmPath="$HOME/.schedule"
alarmRing="$HOME/Music/嗨一点/Something Just Like This (Megamix) - AndyWuMusicland.mp3"

# 遍历clock文件
function range_schedule() {
    test -f $alarmPath || touch $alarmPath

    local minDuration=$((60 * 60 * 24))
    local recentClockMessage=""

    # 逐行读取
    while read line; do
        # 获得元组内容: (toggle, time, rule, message)
        clockItem=(${line//\|/})

        # 如果clock是关闭状态则continue
        if [[ ${clockItem[0]} == "disable" ]]; then
            continue
        fi

        # 计算设定时间和当前时间的差值
        local duration=$(($(date -d "${clockItem[1]}" +%s) - $(date +%s)))
        # 如果差值小于0,代表clock设定时间已过, continue
        if [[ $duration -lt 0 ]]; then
            continue
        fi

        # 如果差值比历史最小差值小，则改差值设为历史最小差值
        # clock message 设为最近的 clock message
        if [[ $duration -lt $minDuration ]]; then
            minDuration=$duration
            recentClockMessage=${clockItem[3]}
        fi
    done <$alarmPath

    # 如果历史最小差值没有被覆盖，则退出函数
    if [[ $minDuration -eq $((60 * 60 * 24)) ]]; then
        return
    fi

    # 休眠最小差值后，发送最近的 clock message 的通知
    sleep $minDuration
    send_notify $recentClockMessage
}

# 发送通知
function send_notify() {
    notify-send -u critical "[Clock]  "$(date +'%H:%M') $1
    if [[ -f $alarmRing ]]; then
        mpv $alarmRing &
    else
        notify-send -u normal "[Warning]" "Alarm Ring  be not founded"
    fi
    # 重新调用 range_schedule 进入寻找下一个符合条件的 clock
    range_schedule
}

range_schedule
