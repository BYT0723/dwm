#!/bin/bash

result=$(ps ax|grep -v grep|grep trayer)

if [ "$result" == "" ];then
    eval "trayer --edge bottom --align center --iconspacing 6 --widthtype request --SetDockType false"
else
    echo $result
    eval "killall trayer"
fi
