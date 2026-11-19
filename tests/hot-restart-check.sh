#!/bin/sh
# Regression: a hot restart must survive restart signals that arrive while the
# new image is still starting up.
#
# execvp() resets SIGHUP/SIGTERM to SIG_DFL, so a second theme switch (or any
# other writer that sends SIGHUP) landing between exec and setup() used to kill
# dwm: the bar disappeared and no WM was left to serve key grabs. The fix keeps
# both signals blocked across exec and unblocks them only after setup() has
# installed the handlers.
#
# The test fires SIGHUP faster than a restart completes, which deliberately
# lands signals in that startup window. Without the fix dwm dies within a few
# iterations; with it, every signal is either handled or queued.
set -eu

DISPLAY_NUM=${1:-:98}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/dwm-hot-restart.XXXXXX)

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# throwaway HOME + XDG dirs: no ~/.dwm, no autostart
export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME" "$XDG_DATA_HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true

Xvfb "$DISPLAY_NUM" -screen 0 1280x720x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1

"$BIN" >"$WORK/dwm.log" 2>&1 &
DWMPID=$!
sleep 1

if ! kill -0 "$DWMPID" 2>/dev/null; then
  echo "FAIL: dwm did not start"
  cat "$WORK/dwm.log"
  exit 1
fi

# 0.02 s is shorter than a restart takes, so most signals land in the window
# between execvp() and setup(). 200 rounds cover the race with a wide margin.
i=0
while [ "$i" -lt 200 ]; do
  kill -HUP "$DWMPID" 2>/dev/null || true
  sleep 0.02
  if ! kill -0 "$DWMPID" 2>/dev/null; then
    echo "FAIL: dwm died after hot restart $i"
    cat "$WORK/dwm.log"
    exit 1
  fi
  i=$((i + 1))
done

echo "PASS: dwm survived 200 hot restarts with restart signals in flight"
