#!/bin/sh
# Regression check: a graphics-only status block must keep its pixels.
#
# The battery block (print_battery in ~/.dwm/dwm-status-print.sh) emits only
# ^r/^c/^f graphics and no text. The pill body fill in drawstatusseg is
# deferred to the first text run (or PILL_CLOSE), so when the battery was the
# only block in its pill the close-time fill painted the body over the already
# drawn rects and the icon vanished. A text sibling hid the bug: its flush
# happened first, before the battery was painted.
#
# checked here end-to-end: run dwm on a private headless X display, feed it a
# battery-only status and a battery+text status, and require the battery's
# outline and fill pixels on the bar in both cases.
#
# Run: ./tests/check-status-graphics-pill.sh [display]  (needs Xvfb, xsetroot,
# xwd and ImageMagick; skips loudly when they are missing)
set -eu

DISP=${1:-:96}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm

command -v Xvfb >/dev/null || { echo "SKIP: Xvfb not installed"; exit 0; }
command -v xsetroot >/dev/null || { echo "SKIP: xsetroot not installed"; exit 0; }
command -v xwd >/dev/null || { echo "SKIP: xwd not installed"; exit 0; }
command -v convert >/dev/null || { echo "SKIP: ImageMagick not installed"; exit 0; }
[ -x "$BIN" ] || { echo "FAIL: $BIN missing; build dwm first"; exit 1; }

WORK=$(mktemp -d /tmp/opencode/dwm-graphics-pill.XXXXXX)
cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# throwaway HOME: no ~/.dwm/autostart.sh of the live session
export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISP"
unset XDG_RUNTIME_DIR || true

Xvfb "$DISP" -screen 0 1280x64x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1

"$BIN" >"$WORK/dwm.log" 2>&1 &
DWMPID=$!
sleep 1.2
if ! kill -0 "$DWMPID" 2>/dev/null; then
  echo "FAIL: dwm exited"
  cat "$WORK/dwm.log"
  exit 1
fi

# battery graphics as print_battery emits them: red outline, green charge fill
BAT='^c#ff0000^^r8,13,2,6^^r10,11,16,10^'
BAT="$BAT"'^c#000000^^r11,12,14,8^'
BAT="$BAT"'^c#00ff00^^r12,13,12,6^^d^^f32^'

# count pixels matching one -fx test; Xvfb screenshots are unblended, so the
# fixture colours stay exact
count_px() {
  convert "$1" -fx "$2 ? 1 : 0" -format '%[fx:mean*w*h]' info: 2>/dev/null |
    awk '{print int($1 + 0.5)}'
}

fail=0
case_battery() {
  name=$1
  fixture=$2

  xsetroot -name "$fixture" 2>/dev/null || {
    echo "SKIP: cannot write the root window name on $DISP"
    return
  }
  sleep 0.8
  xwd -root -silent >"$WORK/$name.xwd" 2>/dev/null || true
  convert "$WORK/$name.xwd" -crop 1280x40+0+0 +repage "$WORK/$name.png" \
    2>/dev/null || true
  if [ ! -s "$WORK/$name.png" ]; then
    echo "SKIP: could not screenshot $DISP"
    return
  fi
  outline=$(count_px "$WORK/$name.png" '(r>0.9 && g<0.1 && b<0.1)')
  fill=$(count_px "$WORK/$name.png" '(g>0.9 && r<0.1 && b<0.1)')
  if [ "${outline:-0}" -gt 0 ] && [ "${fill:-0}" -gt 0 ]; then
    echo "PASS: $name shows the graphics-only block (outline=$outline fill=$fill)"
  else
    echo "FAIL: $name lost the graphics-only block (outline=$outline fill=$fill)"
    echo "      drawstatusseg must flush the pill body before the first ^r"
    fail=1
  fi
}

# battery alone in its pill: the reported bug
case_battery battery-alone "$(printf '\002')$BAT"
# text sibling in the same pill: the case that always worked
case_battery battery-with-text "$(printf '\016')^c#00ffff^XX$(printf '\002')$BAT"

[ "$fail" -eq 0 ] || exit 1
echo "check-status-graphics-pill: ok"
