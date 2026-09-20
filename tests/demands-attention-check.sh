#!/bin/sh
# Regression: _NET_WM_STATE_DEMANDS_ATTENTION mirrors into isurgent, so the
# client's tag lights up; removing the flag (or focusing) clears it.
set -eu
DISPLAY_NUM=${1:-:96}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/attention.XXXXXX)
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
snap() {
  xwd -root -silent >"$WORK/.snap.xwd" 2>/dev/null || true
  convert "$WORK/.snap.xwd" -crop "$2" +repage "$1" 2>/dev/null || true
}
ae() {
  d=$(compare -metric AE "$1" "$2" null: 2>&1 || true)
  case "$d" in
  [0-9]*) printf '%s' "$(printf '%s\n' "$d" | awk '{print int($1 + 0.5)}')" ;;
  *) printf '' ;;
  esac
}
# client lives on tag 1; we watch from tag 2 (the chat-ping scenario)
xterm -geometry 80x24+100+100 -T urgent-one >/dev/null 2>&1 & XT=$!
wid=""; i=0
while [ "$i" -lt 20 ]; do
  wid=$(xdotool search --name "^urgent-one$" 2>/dev/null | head -n 1 || true)
  if [ -n "$wid" ]; then break; fi
  sleep 0.5; i=$((i+1))
done
[ -n "$wid" ] || { echo "FAIL: client never mapped"; exit 1; }
sleep 0.5
xdotool mousemove 640 400 2>/dev/null || true
xdotool key --clearmodifiers super+2 >/dev/null 2>&1 || true
sleep 0.8
snap "$WORK/base.png" 1280x40+0+0
setattn() { # 1 = set, 0 = clear
python3 - <<PY
from Xlib import X, display
d = display.Display()
win = d.create_resource_object('window', int("$wid", 0))
state = d.intern_atom('_NET_WM_STATE')
attn = d.intern_atom('_NET_WM_STATE_DEMANDS_ATTENTION')
atomtype = d.intern_atom('ATOM')
if int("$1"):
    win.change_property(state, atomtype, 32, [attn])
else:
    win.delete_property(state)
d.sync()
PY
}
setattn 1
sleep 0.8
snap "$WORK/lit.png" 1280x40+0+0
n=$(ae "$WORK/base.png" "$WORK/lit.png")
if [ -n "$n" ] && [ "$n" != 0 ]; then
  echo "PASS: attention lit the tag ($n px differ)"
else
  echo "FAIL: bar unchanged after DEMANDS_ATTENTION (ae='$n')"; exit 1
fi
setattn 0
sleep 0.8
snap "$WORK/clear.png" 1280x40+0+0
n=$(ae "$WORK/base.png" "$WORK/clear.png")
if [ "$n" = 0 ]; then
  echo "PASS: removing attention restored the bar"
else
  echo "FAIL: bar still differs after removal (ae='$n')"; exit 1
fi

# focusing the client must make the WM drop the attention atom (EWMH 5.7),
# keeping the other state atoms intact
python3 - <<PY
from Xlib import X, display
d = display.Display()
win = d.create_resource_object('window', int("$wid", 0))
state = d.intern_atom('_NET_WM_STATE')
attn = d.intern_atom('_NET_WM_STATE_DEMANDS_ATTENTION')
maxv = d.intern_atom('_NET_WM_STATE_MAXIMIZED_VERT')
win.change_property(state, d.intern_atom('ATOM'), 32, [attn, maxv])
d.sync()
PY
sleep 0.8
xdotool windowactivate "$wid" 2>/dev/null || true
sleep 0.8
after=$(xprop -id "$wid" _NET_WM_STATE 2>/dev/null)
echo "state after focus: $after"
case "$after" in
*DEMANDS_ATTENTION*)
  echo "FAIL: attention atom survived focus ($after)"; exit 1 ;;
*MAXIMIZED_VERT*)
  echo "PASS: focus cleared attention and kept other state atoms" ;;
*)
  echo "FAIL: other state atoms lost ($after)"; exit 1 ;;
esac
kill -0 "$DWMPID" 2>/dev/null && echo "PASS: dwm still running" || { echo "FAIL: dwm died"; exit 1; }
