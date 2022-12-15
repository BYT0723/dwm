#!/bin/bash

# Toggle TouchPad
synclient TouchpadOff=$(synclient -l | grep -c 'TouchpadOff.*=.*0')
