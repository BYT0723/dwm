#!/bin/bash

/usr/bin/amixer sset Master toggle &
/usr/bin/amixer sset Speaker toggle &
/usr/bin/amixer sset Headphone toggle &

bash ./dwm-status-refresh.sh
