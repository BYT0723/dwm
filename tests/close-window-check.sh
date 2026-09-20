#!/bin/sh
# Regression: _NET_CLOSE_WINDOW closes the target client via the same
# graceful path as the titlebar close button; other clients survive.
set -eu
DISPLAY_NUM=${1:-:96}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/close-window.XXXXXX)
SW=1280
SH=800
cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XT1:-}" ] && kill "$XT1" 2>/dev/null || true
  [ -n "${XT2:-}" ] && kill "$XT2" 2>/dev/null || true
  [ -n "${XT3:-}" ] && kill "$XT3" 2>/dev/null || true
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
if xprop -root _NET_SUPPORTED 2>/dev/null | grep -q _NET_CLOSE_WINDOW; then
  echo "PASS: _NET_SUPPORTED advertises _NET_CLOSE_WINDOW"
else
  echo "FAIL: _NET_CLOSE_WINDOW not in _NET_SUPPORTED"; exit 1
fi
xterm -class mpv -geometry 80x24+100+100 -T close-victim >/dev/null 2>&1 & XT1=$!
xterm -class mpv -geometry 80x24+600+100 -T close-survivor >/dev/null 2>&1 & XT2=$!
waitwid() {
  i=0
  while [ "$i" -lt 20 ]; do
    w=$(xdotool search --name "^$1$" 2>/dev/null | head -n 1 || true)
    if [ -n "$w" ]; then printf '%s' "$w"; return 0; fi
    sleep 0.5; i=$((i+1))
  done
  return 1
}
wid1=$(waitwid close-victim) || { echo "FAIL: victim never mapped"; exit 1; }
wid2=$(waitwid close-survivor) || { echo "FAIL: survivor never mapped"; exit 1; }
sleep 0.5
python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_CLOSE_WINDOW')
ev = event.ClientMessage(window=int("$wid1", 0), client_type=atom,
                         data=(32, [0, 0, 0, 0, 0]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
# victim should be gone, survivor must remain
i=0
while [ "$i" -lt 20 ]; do
  if ! xdotool search --name "^close-victim$" >/dev/null 2>&1; then break; fi
  sleep 0.5; i=$((i+1))
done
if xdotool search --name "^close-victim$" >/dev/null 2>&1; then
  echo "FAIL: victim still alive after _NET_CLOSE_WINDOW"; exit 1
else
  echo "PASS: _NET_CLOSE_WINDOW closed the target"
fi
if xdotool search --name "^close-survivor$" >/dev/null 2>&1; then
  echo "PASS: sibling client survived"
else
  echo "FAIL: sibling client died too"; exit 1
fi
if ! kill -0 "$DWMPID" 2>/dev/null; then echo "FAIL: dwm died"; cat "$WORK/dwm.log"; exit 1; fi
echo "PASS: dwm still running"

# case 2: closing a hidden client must work (no focus-steal, no bad sel)
xterm -class mpv -geometry 80x24+800+100 -T close-hidden >/dev/null 2>&1 & XT3=$!
wid3=$(waitwid close-hidden) || { echo "FAIL: hidden victim never mapped"; exit 1; }
sleep 0.5
xdotool windowactivate "$wid3" 2>/dev/null || true
sleep 0.5
xdotool key --clearmodifiers super+h 2>/dev/null || true   # Mod+h hides the selected client
sleep 0.5
if xwininfo -id "$wid3" 2>/dev/null | grep -q "Map State: IsViewable"; then
  echo "FAIL: super+h did not hide the client (still viewable)"
  exit 1
fi
echo "PASS: client hidden (Map State not IsViewable)"
python3 - <<PY
from Xlib import X, display
from Xlib.protocol import event
d = display.Display()
root = d.screen().root
atom = d.intern_atom('_NET_CLOSE_WINDOW')
ev = event.ClientMessage(window=int("$wid3", 0), client_type=atom,
                         data=(32, [0, 0, 0, 0, 0]))
root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
d.sync()
PY
i=0
while [ "$i" -lt 20 ]; do
  if ! xdotool search --name "^close-hidden$" >/dev/null 2>&1; then break; fi
  sleep 0.5; i=$((i+1))
done
if xdotool search --name "^close-hidden$" >/dev/null 2>&1; then
  echo "FAIL: hidden client survived _NET_CLOSE_WINDOW"; exit 1
else
  echo "PASS: _NET_CLOSE_WINDOW closed a hidden client"
fi
if xdotool search --name "^close-survivor$" >/dev/null 2>&1; then
  echo "PASS: sibling survived the hidden-client close"
else
  echo "FAIL: hidden-client close took the sibling down too"; exit 1
fi
kill -0 "$DWMPID" 2>/dev/null && echo "PASS: dwm survived hidden-client close" || { echo "FAIL: dwm died"; cat "$WORK/dwm.log"; exit 1; }
