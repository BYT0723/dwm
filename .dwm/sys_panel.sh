#!/bin/bash

result=$(ps ax|grep -v grep|grep trayer)

if [ "$result" == "" ];then
    eval "trayer --edge top --align center --iconspacing 6 --widthtype request"
else
    echo $result
    eval "killall trayer"
fi
