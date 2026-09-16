#!/bin/sh
# Smoke test for the bar: run the built dwm on a private headless X display
# with a throwaway HOME, so runautostart() finds no script and nothing of the
# user's session is started or touched.
#
# The status fixture is pushed through the root window name, which is how dwm
# reads its status. Some sandboxes block root-window writes (xsetroot and
# XChangeProperty both silently do nothing there); on such a host this script
# still verifies that dwm starts and draws the bar, but the status strip stays
# empty. Run it on a normal X server to exercise the pills too.
set -eu

DISPLAY_NUM=${1:-:97}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/dwm-smoke.XXXXXX)

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# throwaway HOME + XDG dirs: no ~/.dwm, no ~/.local/share/dwm => no autostart
export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME" "$XDG_DATA_HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true

Xvfb "$DISPLAY_NUM" -screen 0 1280x64x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1

# status fixture: block ids 1..9, one pill per pane, plus a portrait cut
FIXTURE=$(printf '\x01 date-1234 \x02 bat-80%% \x07 mem-1.2G \x06 disk-40G \x09 weather \x7f\x01 late')
xsetroot -name "$FIXTURE" 2>/dev/null || true

"$BIN" >"$WORK/dwm.log" 2>&1 &
DWMPID=$!
sleep 1.5

if ! kill -0 "$DWMPID" 2>/dev/null; then
  echo "FAIL: dwm exited"
  cat "$WORK/dwm.log"
  exit 1
fi
echo "PASS: dwm running on $DISPLAY_NUM"

xwd -root -silent >"$WORK/root.xwd" 2>/dev/null || true
# bar strip only, scaled up so the pills are inspectable
convert "$WORK/root.xwd" -crop 1280x40+0+0 +repage -scale 300% \
  "$ROOT/tests/.smoke-bar.png" 2>/dev/null || true

# click sweep across the status strip: with the fixture set, the click handler
# resolves each pill back to its block id (9 weather, 7 mem, 6 disk, 2
# battery, 1 date). Harmless when the fixture could not be set.
i=800
while [ "$i" -le 1270 ]; do
  xdotool mousemove "$i" 12 click 1 2>/dev/null || true
  i=$((i + 10))
done
sleep 0.5

echo "--- dwm.log ---"
cat "$WORK/dwm.log" 2>/dev/null || true
echo "--- workspace ---"
echo "$WORK"
