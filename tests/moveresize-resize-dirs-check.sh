#!/bin/sh
# Regression: _NET_WM_MOVERESIZE directions 0-7 each resize the expected
# edge(s); the opposite edge stays fixed. Mirrors moveresize-check.sh.
# One floating client is reused; geometry is reset between directions with
# _NET_MOVERESIZE_WINDOW (absolute, already covered by
# moveresize-window-check.sh), so no spawn/kill races per direction.
set -eu
DISPLAY_NUM=${1:-:97}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/moveresize-dirs.XXXXXX)
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
if xprop -root _NET_SUPPORTED 2>/dev/null | grep -q _NET_WM_MOVERESIZE; then
  echo "PASS: _NET_SUPPORTED advertises _NET_WM_MOVERESIZE"
else
  echo "FAIL: _NET_WM_MOVERESIZE not in _NET_SUPPORTED"; exit 1
fi
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
sendreset() { # wid -- absolute reset via _NET_MOVERESIZE_WINDOW (flags all, NW)
  python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
wid = int("$1", 0)
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_MOVERESIZE_WINDOW')
ev = event.ClientMessage(window=wid, client_type=atom, data=(32, [3841, 400, 300, 640, 400]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
}
xterm -class mpv -geometry 80x24+100+100 -T moveresize-dirs >/dev/null 2>&1 & XT=$!
wid=""; i=0
while [ "$i" -lt 20 ]; do
  wid=$(xdotool search --name "^moveresize-dirs$" 2>/dev/null | head -n 1 || true)
  if [ -n "$wid" ]; then break; fi
  sleep 0.5; i=$((i+1))
done
[ -n "$wid" ] || { echo "FAIL: client never mapped"; exit 1; }
sleep 0.5
fail=0
for dir in 0 1 2 3 4 5 6 7; do
  sendreset "$wid"
  sleep 0.8
  before=$(geom "$wid")
  set -- $before; bx=$1; by=$2; bw=$3; bh=$4
  xdotool mousemove 200 200
  sleep 0.2
  sendmsg "$wid" "$dir"
  sleep 0.3
  # hold button 1 for the whole drag, like a real CSD client press-drag-release;
  # a lone mouseup without a press never reaches the grab
  xdotool mousedown 1
  sleep 0.2
  # step the pointer so at least one motion lands inside the grab even if
  # the first step races the warp; total (+105,+81)
  xdotool mousemove_relative -- 35 27
  sleep 0.3
  xdotool mousemove_relative -- 35 27
  sleep 0.3
  xdotool mousemove_relative -- 35 27
  sleep 0.5
  xdotool mouseup 1
  sleep 0.8
  after=$(geom "$wid")
  set -- $after; ax=$1; ay=$2; aw=$3; ah=$4
  dx=$((ax - bx)); dy=$((ay - by)); dw=$((aw - bw)); dh=$((ah - bh))
  echo "dir $dir: before($bx,$by,${bw}x${bh}) after($ax,$ay,${aw}x${ah}) d($dx,$dy,${dw}x${dh})"
  ok=1
  case $dir in
    0) [ "$dw" -lt -30 ] || ok=0; [ "$dh" -lt -30 ] || ok=0; [ "$dx" -gt 30 ] || ok=0; [ "$dy" -gt 30 ] || ok=0 ;;
    1) [ "$dh" -lt -30 ] || ok=0; [ "$dy" -gt 30 ] || ok=0; [ "$dw" -ge -20 ] && [ "$dw" -le 20 ] || ok=0 ;;
    2) [ "$dw" -gt 30 ] || ok=0; [ "$dh" -lt -30 ] || ok=0; [ "$dy" -gt 30 ] || ok=0 ;;
    3) [ "$dw" -gt 30 ] || ok=0; [ "$dh" -ge -20 ] && [ "$dh" -le 20 ] || ok=0 ;;
    4) [ "$dw" -gt 30 ] || ok=0; [ "$dh" -gt 30 ] || ok=0 ;;
    5) [ "$dh" -gt 30 ] || ok=0; [ "$dw" -ge -20 ] && [ "$dw" -le 20 ] || ok=0 ;;
    6) [ "$dw" -lt -30 ] || ok=0; [ "$dh" -gt 30 ] || ok=0; [ "$dx" -gt 30 ] || ok=0 ;;
    7) [ "$dw" -lt -30 ] || ok=0; [ "$dx" -gt 30 ] || ok=0; [ "$dh" -ge -20 ] && [ "$dh" -le 20 ] || ok=0 ;;
  esac
  if [ "$ok" = 1 ]; then
    echo "PASS: dir $dir resized as expected"
  else
    echo "FAIL: dir $dir geometry unexpected"; fail=1
  fi
done
if [ "$fail" = 0 ]; then
  echo "PASS: all 8 _NET_WM_MOVERESIZE directions resized as expected"
else
  echo "FAIL: one or more directions misbehaved (see above)"; exit 1
fi
