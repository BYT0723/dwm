#!/bin/sh
# Regression: _NET_FRAME_EXTENTS is published on the client at manage time
# (left,right,top,bottom = bw,bw,bw+titleh,bw) and refreshed on
# _NET_REQUEST_FRAME_EXTENTS.
set -eu
DISPLAY_NUM=${1:-:96}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/frame-ext.XXXXXX)
SW=1280
SH=800
cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XT:-}" ] && kill "$XT" 2>/dev/null || true
  [ -n "${PREPID:-}" ] && kill "$PREPID" 2>/dev/null || true
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
bw=$(sed -n 's/^static const unsigned int borderpx *= *\([0-9][0-9]*\).*/\1/p' "$ROOT/config.h")
bw=${bw:-0}
xterm -class mpv -geometry 80x24+100+100 -T frame-ext-test >/dev/null 2>&1 & XT=$!
wid=""; i=0
while [ "$i" -lt 20 ]; do
  wid=$(xdotool search --name "^frame-ext-test$" 2>/dev/null | head -n 1 || true)
  if [ -n "$wid" ]; then break; fi
  sleep 0.5; i=$((i+1))
done
[ -n "$wid" ] || { echo "FAIL: client never mapped"; exit 1; }
sleep 0.5
extents() { xprop -id "$1" _NET_FRAME_EXTENTS 2>/dev/null | sed 's/.* = //'; }
before=$(extents "$wid")
echo "at-manage: $before"
# expect "bw, bw, top, bw" with top > bw (titled floating client)
l=$(printf '%s' "$before" | awk -F', ' '{print $1}'); r=$(printf '%s' "$before" | awk -F', ' '{print $2}')
t=$(printf '%s' "$before" | awk -F', ' '{print $3}'); b=$(printf '%s' "$before" | awk -F', ' '{print $4}')
if [ "$l" -eq "$bw" ] && [ "$r" -eq "$bw" ] && [ "$b" -eq "$bw" ] && [ "$t" -gt "$bw" ]; then
  echo "PASS: manage-time extents ($before, border $bw)"
else
  echo "FAIL: extents $before, want $bw, $bw, >$bw, $bw"; exit 1
fi
# delete + request -> must come back identical
xprop -id "$wid" -remove _NET_FRAME_EXTENTS >/dev/null 2>&1 || true
if xprop -id "$wid" _NET_FRAME_EXTENTS 2>/dev/null | grep -q "="; then echo "FAIL: remove did not take"; exit 1; fi
python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_REQUEST_FRAME_EXTENTS')
ev = event.ClientMessage(window=int("$wid", 0), client_type=atom, data=(32, [0, 0, 0, 0, 0]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
sleep 0.8
after=$(extents "$wid")
if [ "$after" = "$before" ]; then
  echo "PASS: request refreshed extents ($after)"
else
  echo "FAIL: after request got '$after', want '$before'"; exit 1
fi
kill -0 "$DWMPID" 2>/dev/null && echo "PASS: dwm still running" || { echo "FAIL: dwm died"; exit 1; }

# fullscreen (via _NET_WM_STATE_FULLSCREEN) zeroes the border: extents must
# follow, and restore on exit. action: 1=add, 0=remove.
sendfs() {
python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
d = display.Display()
root = d.screen().root
state = d.intern_atom('_NET_WM_STATE')
fs = d.intern_atom('_NET_WM_STATE_FULLSCREEN')
ev = event.ClientMessage(window=int("$wid", 0), client_type=state,
                         data=(32, [int("$1"), fs, 0, 0, 0]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
}
sendfs 1; sleep 0.8
fs=$(extents "$wid")
if [ "$fs" = "0, 0, 0, 0" ]; then
  echo "PASS: fullscreen extents are zero ($fs)"
else
  echo "FAIL: fullscreen extents '$fs', want '0, 0, 0, 0'"; exit 1
fi
sendfs 0; sleep 0.8
back=$(extents "$wid")
if [ "$back" = "$before" ]; then
  echo "PASS: extents restored after leaving fullscreen ($back)"
else
  echo "FAIL: extents after fullscreen '$back', want '$before'"; exit 1
fi

# runtime border-width change (ConfigureRequest) must refresh extents too
python3 - <<PY
from Xlib import X, display
d = display.Display()
win = d.create_resource_object('window', int("$wid", 0))
win.configure(border_width=5)
d.sync()
PY
sleep 0.6
nb=$(extents "$wid")
want_top=$((5 + t - bw)) # 5 + titleh, titleh = top - bw from the manage-time read
if [ "$nb" = "5, 5, $want_top, 5" ]; then
  echo "PASS: border change refreshed extents ($nb)"
else
  echo "FAIL: border change gave '$nb', want '5, 5, $want_top, 5'"; exit 1
fi

# pre-map estimate: an unmapped window asking _NET_REQUEST_FRAME_EXTENTS
# must get the default answer (rules aren't resolved yet)
python3 - >"$WORK/premap-wid" 2>"$WORK/premap-err" <<PY &
from Xlib import X, display
import time
d = display.Display()
s = d.screen()
w = s.root.create_window(0, 0, 100, 100, 0, s.root_depth,
                         X.InputOutput, X.CopyFromParent)
w.set_wm_name("premap-est")
w.set_wm_class("premap-est", "premap-est")
d.sync()
print(w.id, flush=True)
# never map: stay alive so the window is not destroyed
while True:
    time.sleep(60)
PY
PREPID=$!
sleep 1
pw=$(cat "$WORK/premap-wid")
python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_REQUEST_FRAME_EXTENTS')
ev = event.ClientMessage(window=int("$pw", 0), client_type=atom, data=(32, [0, 0, 0, 0, 0]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
sleep 0.6
pe=$(extents "$pw")
pl=$(printf '%s' "$pe" | awk -F', ' '{print $1}'); pt=$(printf '%s' "$pe" | awk -F', ' '{print $3}')
if [ "$pl" -eq "$bw" ] && [ "$pt" -gt "$bw" ]; then
  echo "PASS: pre-map estimate answered ($pe)"
else
  echo "FAIL: pre-map estimate '$pe', want left=$bw top>$bw"; exit 1
fi
kill "$PREPID" 2>/dev/null || true
kill -0 "$DWMPID" 2>/dev/null && echo "PASS: dwm survived the fix suite" || { echo "FAIL: dwm died"; exit 1; }
