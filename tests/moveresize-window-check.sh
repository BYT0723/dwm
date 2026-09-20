#!/bin/sh
# Regression: _NET_MOVERESIZE_WINDOW moves/resizes floating clients with
# ConfigureRequest semantics (NorthWest); tiled clients keep layout geometry.
set -eu
DISPLAY_NUM=${1:-:96}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/mrw.XXXXXX)
SW=1280
SH=800
cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XT1:-}" ] && kill "$XT1" 2>/dev/null || true
  [ -n "${XT2:-}" ] && kill "$XT2" 2>/dev/null || true
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
if xprop -root _NET_SUPPORTED 2>/dev/null | grep -q _NET_MOVERESIZE_WINDOW; then
  echo "PASS: _NET_SUPPORTED advertises _NET_MOVERESIZE_WINDOW"
else
  echo "FAIL: _NET_MOVERESIZE_WINDOW not in _NET_SUPPORTED"; exit 1
fi
sendmrw() { # wid flags x y w h
python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_MOVERESIZE_WINDOW')
ev = event.ClientMessage(window=int("$1", 0), client_type=atom,
                         data=(32, [$2, $3, $4, $5, $6]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
}
waitwid() {
  i=0
  while [ "$i" -lt 20 ]; do
    w=$(xdotool search --name "^$1$" 2>/dev/null | head -n 1 || true)
    if [ -n "$w" ]; then printf '%s' "$w"; return 0; fi
    sleep 0.5; i=$((i+1))
  done
  return 1
}
cgeom() { xwininfo -id "$1" 2>/dev/null | awk '/Absolute upper-left X:/{ax=$4} /Absolute upper-left Y:/{ay=$4} /^  Width:/{w=$2} /^  Height:/{h=$2} END{printf "%d %d %d %d", ax, ay, w, h}'; }
# case 1: all-flags move of a floating client (NW=1, all fields=0xF00)
xterm -class mpv -geometry 80x24+100+100 -T mrw-float >/dev/null 2>&1 & XT1=$!
wid1=$(waitwid mrw-float) || { echo "FAIL: float client never mapped"; exit 1; }
sleep 0.5
sendmrw "$wid1" 3841 400 300 640 400
sleep 0.8
set -- $(cgeom "$wid1")
# client abs = frame + border(2): (402, 302)
if [ "$1" -eq 402 ] && [ "$2" -eq 302 ]; then
  echo "PASS: floating client moved to requested position ($1,$2)"
else
  echo "FAIL: floating client at $1,$2 want 402,302"; exit 1
fi
# case 2: partial flags (x/y only = 0x300|NW) keep size
set -- $(cgeom "$wid1"); w0=$3; h0=$4
sendmrw "$wid1" 769 500 350 0 0
sleep 0.8
set -- $(cgeom "$wid1")
if [ "$1" -eq 502 ] && [ "$2" -eq 352 ] && [ "$3" -eq "$w0" ] && [ "$4" -eq "$h0" ]; then
  echo "PASS: partial flags moved only ($1,$2 size ${3}x${4} unchanged)"
else
  echo "FAIL: partial move gave $1,$2 ${3}x${4}"; exit 1
fi
# case 3: full flags with UNCHANGED size (the wmctrl -e move case):
# position must apply immediately, not linger until the next manual move
sendmrw "$wid1" 3841 600 400 "$w0" "$h0"
sleep 0.8
set -- $(cgeom "$wid1")
if [ "$1" -eq 602 ] && [ "$2" -eq 402 ] && [ "$3" -eq "$w0" ] && [ "$4" -eq "$h0" ]; then
  echo "PASS: same-size move applied immediately ($1,$2)"
else
  echo "FAIL: same-size move gave $1,$2 ${3}x${4}"; exit 1
fi
# case 4: tiled client ignores geometry
xterm -geometry 80x24 -T mrw-tiled >/dev/null 2>&1 & XT2=$!
wid2=$(waitwid mrw-tiled) || { echo "FAIL: tiled client never mapped"; exit 1; }
sleep 0.8
before=$(cgeom "$wid2")
sendmrw "$wid2" 3841 900 700 600 500
sleep 0.8
after=$(cgeom "$wid2")
if [ "$before" = "$after" ]; then
  echo "PASS: tiled client kept layout geometry ($after)"
else
  echo "FAIL: tiled client moved ($before -> $after)"; exit 1
fi
kill -0 "$DWMPID" 2>/dev/null && echo "PASS: dwm still running" || { echo "FAIL: dwm died"; exit 1; }
