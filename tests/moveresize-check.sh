#!/bin/sh
# Regression: CSD-style clients (own titlebar, no dwm frame title, e.g. xunlei)
# drag their header via _NET_WM_MOVERESIZE; dwm must start a move/resize.
# Flow mirrors real clients: no pointer grab held when the message is sent,
# then motion + ButtonRelease end the operation.
set -eu
DISPLAY_NUM=${1:-:96}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/moveresize.XXXXXX)
SW=1280
SH=800
cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XT:-}" ] && kill "$XT" 2>/dev/null || true
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
xterm -class mpv -geometry 80x24+100+100 -T moveresize-test >/dev/null 2>&1 & XT=$!
wid=""; i=0
while [ "$i" -lt 20 ]; do
  wid=$(xdotool search --name "^moveresize-test$" 2>/dev/null | head -n 1 || true)
  if [ -n "$wid" ]; then break; fi
  sleep 0.5; i=$((i+1))
done
[ -n "$wid" ] || { echo "FAIL: client never mapped"; exit 1; }
sleep 0.5
geom() { xwininfo -id "$1" 2>/dev/null | awk '/Absolute upper-left X:/{ax=$4} /Absolute upper-left Y:/{ay=$4} END{printf "%d %d", ax, ay}'; }
before=$(geom "$wid")
echo "before: $before (wid $wid)"
xdotool mousemove 200 200
sleep 0.2
python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
wid = int("$wid", 0)
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_WM_MOVERESIZE')
ev = event.ClientMessage(window=wid, client_type=atom, data=(32, [0, 0, 8, 1, 1]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
sleep 0.3
xdotool mousemove 400 350
sleep 0.5
xdotool mouseup 1
sleep 0.8
after=$(geom "$wid")
echo "after: $after"
set -- $before; bx=$1; by=$2
set -- $after; ax=$1; ay=$2
dx=$((ax - bx)); dy=$((ay - by))
echo "delta: $dx,$dy"
if [ "$dx" -gt 50 ] && [ "$dy" -gt 50 ]; then
  echo "PASS: _NET_WM_MOVERESIZE drag moved floating client by $dx,$dy"
else
  echo "FAIL: client did not move (delta $dx,$dy)"; exit 1
fi
