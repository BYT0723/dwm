#!/bin/sh
# Regression: shrinking past the client's minimum size must keep the
# opposite edge fixed (no reverse displacement). Covers the directions
# with a west or north component: 0 TL, 1 T, 2 TR, 6 BL, 7 L.
set -eu
DISPLAY_NUM=${1:-:94}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/moveresize-min.XXXXXX)
SW=1280
SH=800
cleanup() {
  [ -n "${XT:-}" ] && kill "$XT" 2>/dev/null || true
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
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
geom() {
  xwininfo -id "$1" 2>/dev/null | awk '
    /Absolute upper-left X:/{ax=$4}
    /Absolute upper-left Y:/{ay=$4}
    /^  Width:/{w=$2}
    /^  Height:/{h=$2}
    END{printf "%d %d %d %d", ax, ay, w, h}'
}
sendmsg() { # wid dir
  python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
wid = int("$1", 0)
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_WM_MOVERESIZE')
ev = event.ClientMessage(window=wid, client_type=atom, data=(32, [0, 0, $2, 1, 1]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
}
sendtiny() { # wid -- shrink to the hint minimum via _NET_MOVERESIZE_WINDOW
  python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
wid = int("$1", 0)
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_MOVERESIZE_WINDOW')
ev = event.ClientMessage(window=wid, client_type=atom, data=(32, [3841, 400, 300, 10, 10]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
}
xterm -class mpv -geometry 80x24+100+100 -T moveresize-min >/dev/null 2>&1 & XT=$!
wid=""; i=0
while [ "$i" -lt 20 ]; do
  wid=$(xdotool search --name "^moveresize-min$" 2>/dev/null | head -n 1 || true)
  if [ -n "$wid" ]; then break; fi
  sleep 0.5; i=$((i+1))
done
[ -n "$wid" ] || { echo "FAIL: client never mapped"; exit 1; }
sleep 0.5
fail=0
for dir in 7 1 0 2 6; do
  sendtiny "$wid"
  sleep 0.8
  before=$(geom "$wid")
  set -- $before; bx=$1; by=$2; bw=$3; bh=$4
  e0=$((bx + bw)); s0=$((by + bh))
  echo "dir $dir: at min ${bw}x${bh} east=$e0 south=$s0"
  xdotool mousemove 200 200
  sleep 0.2
  sendmsg "$wid" "$dir"
  sleep 0.3
  xdotool mousedown 1
  sleep 0.2
  xdotool mousemove_relative -- 35 27
  sleep 0.3
  xdotool mousemove_relative -- 35 27
  sleep 0.3
  xdotool mousemove_relative -- 35 27
  sleep 0.5
  ptr_before=$(xdotool getmouselocation 2>/dev/null | awk '{print $1, $2}')
  xdotool mouseup 1
  sleep 0.8
  ptr_after=$(xdotool getmouselocation 2>/dev/null | awk '{print $1, $2}')
  after=$(geom "$wid")
  set -- $after; ax=$1; ay=$2; aw=$3; ah=$4
  e1=$((ax + aw)); s1=$((ay + ah))
  de=$((e1 - e0)); ds=$((s1 - s0))
  echo "dir $dir: after ${aw}x${ah} east-delta=$de south-delta=$ds"
  ok=1
  case $dir in
    7) [ "$de" -ge -5 ] && [ "$de" -le 5 ] || ok=0 ;;
    1) [ "$ds" -ge -5 ] && [ "$ds" -le 5 ] || ok=0 ;;
    0) [ "$de" -ge -5 ] && [ "$de" -le 5 ] || ok=0
       [ "$ds" -ge -5 ] && [ "$ds" -le 5 ] || ok=0 ;;
    2) [ "$ds" -ge -5 ] && [ "$ds" -le 5 ] || ok=0 ;;
    6) [ "$de" -ge -5 ] && [ "$de" -le 5 ] || ok=0 ;;
  esac
  if [ "$ok" = 1 ]; then
    echo "PASS: dir $dir opposite edge fixed"
  else
    echo "FAIL: dir $dir opposite edge moved (east $de, south $ds)"; fail=1
  fi
  if [ "$ptr_before" = "$ptr_after" ]; then
    echo "PASS: dir $dir pointer stayed at release point ($ptr_after)"
  else
    echo "FAIL: dir $dir pointer jumped on release ($ptr_before -> $ptr_after)"; fail=1
  fi
done
if ! kill -0 "$DWMPID" 2>/dev/null; then echo "FAIL: dwm died"; fail=1; fi
if [ "$fail" = 0 ]; then
  echo "PASS: min-size clamp keeps the opposite edge fixed"
else
  echo "FAIL: reverse displacement at min size (see above)"; exit 1
fi
