#!/bin/sh
# Smoke test for the bar: run the built dwm on a private headless X display
# with a throwaway HOME, so runautostart() finds no script and nothing of the
# user's session is started or touched.
#
# It starts two clients and screenshots the bar, which exercises the tab row
# (both tab modes render here) including the icon fallback for clients that
# carry no _NET_WM_ICON, as xterm does not.
#
# The status strip is fed through the root window name, which is how dwm reads
# its status. Some sandboxes block root-window writes (xsetroot and
# XChangeProperty both silently do nothing there); on such a host the status
# strip stays empty while the tags and tabs still render. Run it on a normal X
# server to exercise the status pills too.
set -eu

DISPLAY_NUM=${1:-:97}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/dwm-smoke.XXXXXX)

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XT1:-}" ] && kill "$XT1" 2>/dev/null || true
  [ -n "${XT2:-}" ] && kill "$XT2" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# throwaway HOME + XDG dirs: no ~/.dwm, no ~/.local/share/dwm => no autostart
export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true

# the tab icon fallback the config points at
[ -f "$ROOT/icons/tab-fallback.png" ] &&
  cp "$ROOT/icons/tab-fallback.png" "$HOME/.dwm/tab-fallback.png"

Xvfb "$DISPLAY_NUM" -screen 0 1280x64x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1

# status fixture: block ids, one pill per pane, plus a portrait cut
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

# two clients => the tab row has something to lay out (xterm has no icon, so
# this also covers the fallback path)
xterm -geometry 40x10+0+120 -T smoke-one >/dev/null 2>&1 & XT1=$!
xterm -geometry 40x10+400+120 -T smoke-two >/dev/null 2>&1 & XT2=$!
sleep 1.5

xwd -root -silent >"$WORK/root.xwd" 2>/dev/null || true
# bar strip only, scaled up so the pills are inspectable
convert "$WORK/root.xwd" -crop 1280x40+0+0 +repage -scale 300% \
  "$ROOT/tests/.smoke-bar.png" 2>/dev/null || true

# click sweep across the status strip: with the fixture set, the click handler
# resolves each pill back to its block id. Harmless when it could not be set.
i=800
while [ "$i" -le 1270 ]; do
  xdotool mousemove "$i" 12 click 1 2>/dev/null || true
  i=$((i + 10))
done
sleep 0.5

echo "--- dwm.log ---"
cat "$WORK/dwm.log" 2>/dev/null || true
echo "--- bar strip: $ROOT/tests/.smoke-bar.png ---"
echo "--- workspace: $WORK ---"
