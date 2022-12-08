#!/usr/bin/env bash

if ! [[ -n $(command -v ydict) ]]; then
    if ! [[ -n $(command -v go) ]]; then
        echo "install go environment please !"
        exit
    else
        read -p "install ydict ? (y/n): " flag
        if [[ $flag == "y" ]]; then
            go install github.com/TimothyYe/ydict@latest
        else
            exit
        fi
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
