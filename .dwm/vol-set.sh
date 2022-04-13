#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Error : At least one parameter is required "
    exit 1
fi

/usr/bin/amixer -qM set Master $1% umute
