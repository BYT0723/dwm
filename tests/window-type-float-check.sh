#!/bin/sh
# Regression: SPLASH/UTILITY/NOTIFICATION/MENU windows float instead of
# stealing the tiled master area.
set -eu
DISPLAY_NUM=${1:-:96}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/wintype.XXXXXX)
SW=1280
SH=800
cleanup() {
  [ -n "${SPLASHPID:-}" ] && kill "$SPLASHPID" 2>/dev/null || true
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XTA:-}" ] && kill "$XTA" 2>/dev/null || true
  [ -n "${XTB:-}" ] && kill "$XTB" 2>/dev/null || true
}
trap cleanup EXIT INT TERM
export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true
[ -f "$ROOT/icons/tab-fallback.png" ] && cp "$ROOT/icons/tab-fallback.png" "$HOME/.dwm/tab-fallback.png"
Xvfb "$DISPLAY_NUM" -screen 0 ${SW}x${SH}x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1
"$BIN" >"$WORK/dwm.log" 2>&1 &
DWMPID=$!
sleep 1.5
if ! kill -0 "$DWMPID" 2>/dev/null; then echo "FAIL: dwm exited"; cat "$WORK/dwm.log"; exit 1; fi
# create an unmapped window, tag it SPLASH, then map: manage() must float it.
# NB: the python helper must stay alive (background) or the X server
# destroys its windows when the connection closes.
python3 - >"$WORK/splash-wid" 2>"$WORK/splash-err.log" <<PY &
from Xlib import X, display
import time
d = display.Display()
s = d.screen()
win = s.root.create_window(100, 100, 200, 100, 0, s.root_depth,
                           X.InputOutput, X.CopyFromParent)
win.set_wm_name("splash-one")
win.set_wm_class("splash-one", "splash-one")
win.change_property(d.intern_atom('_NET_WM_WINDOW_TYPE'),
                    d.intern_atom('ATOM'), 32,
                    [d.intern_atom('_NET_WM_WINDOW_TYPE_SPLASH')])
win.map()
d.sync()
print(win.id, flush=True)
while True:
    time.sleep(60)
PY
SPLASHPID=$!
sleep 1
wid=$(cat "$WORK/splash-wid")
i=0
while [ "$i" -lt 20 ]; do
  if xdotool search --name "^splash-one$" >/dev/null 2>&1; then break; fi
  sleep 0.5; i=$((i+1))
done
sleep 0.8
set -- $(xwininfo -id "$wid" 2>/dev/null | awk '/^  Width:/{w=$2} /^  Height:/{h=$2} END{printf "%d %d", w, h}')
echo "splash client size: ${1}x${2} (asked 200x100)"
# tiled would stretch it to the master area (~1200px wide); floating keeps it
if [ "$1" -eq 200 ] && [ "$2" -eq 100 ]; then
  echo "PASS: splash window floats at requested size"
else
  echo "FAIL: splash window resized to ${1}x${2} (tiled?)"; exit 1
fi
kill -0 "$DWMPID" 2>/dev/null && echo "PASS: dwm still running" || { echo "FAIL: dwm died"; exit 1; }

# runtime type change: flipping a mapped, tiled client to UTILITY must
# re-arrange, so the remaining tiled client takes over the master area.
xterm -geometry 80x24 -T rt-a >/dev/null 2>&1 & XTA=$!
xterm -geometry 80x24 -T rt-b >/dev/null 2>&1 & XTB=$!
waitname() {
  i=0
  while [ "$i" -lt 20 ]; do
    w=$(xdotool search --name "^$1$" 2>/dev/null | head -n 1 || true)
    if [ -n "$w" ]; then printf '%s' "$w"; return 0; fi
    sleep 0.5; i=$((i+1))
  done
  return 1
}
wa=$(waitname rt-a) || { echo "FAIL: rt-a never mapped"; exit 1; }
wb=$(waitname rt-b) || { echo "FAIL: rt-b never mapped"; exit 1; }
sleep 0.8
width() { xwininfo -id "$1" 2>/dev/null | awk '/^  Width:/{print $2}'; }
wa0=$(width "$wa"); wb0=$(width "$wb")
xprop -id "$wa" -f _NET_WM_WINDOW_TYPE 32a \
  -set _NET_WM_WINDOW_TYPE _NET_WM_WINDOW_TYPE_UTILITY >/dev/null 2>&1 || true
sleep 0.8
wa1=$(width "$wa"); wb1=$(width "$wb")
echo "rt-a width $wa0 -> $wa1, rt-b width $wb0 -> $wb1"
if [ "$wb1" -gt $((wb0 + 100)) ] && [ "$wa1" -ge $((wa0 - 4)) ] && [ "$wa1" -le $((wa0 + 4)) ]; then
  echo "PASS: runtime UTILITY change re-arranged (tiled sibling now $wb1 wide)"
else
  echo "FAIL: runtime UTILITY change did not re-flow (a $wa0->$wa1, b $wb0->$wb1)"; exit 1
fi
kill -0 "$DWMPID" 2>/dev/null && echo "PASS: dwm survived the runtime type change" || { echo "FAIL: dwm died"; exit 1; }
