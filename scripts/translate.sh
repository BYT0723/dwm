#!/bin/bash

if [[ -x ydict ]]; then
    if [[ -x go ]]; then
        echo "install go environment please !"
        exit 1
    else
        go install github.com/TimothyYe/ydict
    fi
fi

while read -p " >>==>> " words; do
    if [[ $words == "" ]]; then
        continue
    elif [[ $words =~ " " ]]; then
        ydict -v 1 $words
    elif [[ $(echo $words | grep -e "^[a-zA-Z]\+\$") == "" ]]; then
        ydict $words
    else
        ydict -v 1 -c $words
    fi
done
