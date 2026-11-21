#!/bin/sh
# Regression: a broken tray client must not take dwm down.
#
# systraydock() used to trust the dock request: the tray window itself, the
# root, a window already managed as a client, a vanished window (uninitialised
# XWindowAttributes) and a 0x0 icon all reached XAddToSaveSet/XReparentWindow/
# ConfigureWindow with BadMatch/BadAlloc, and xerror() turns an unexpected X
# error into a fatal exit. A tray helper that races or misbehaves therefore
# killed the WM: the bar vanished and every key binding stopped working.
#
# Run: tests/systray-guards-check.sh [display]
# Needs Xvfb, cc, xdotool; skips loudly when they are missing.
set -eu

DISP=${1:-:79}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=${BIN:-$ROOT/dwm}

command -v Xvfb >/dev/null || { echo "SKIP: Xvfb not installed"; exit 0; }
command -v xdotool >/dev/null || { echo "SKIP: xdotool not installed"; exit 0; }
command -v cc >/dev/null || { echo "SKIP: cc not installed"; exit 0; }
[ -x "$BIN" ] || { echo "FAIL: $BIN missing; build dwm first"; exit 1; }

WORK=$(mktemp -d /tmp/opencode/dwm-systray-guards.XXXXXX)
export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISP"
unset XDG_RUNTIME_DIR || true

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${PROBEPID:-}" ] && kill "$PROBEPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

cc -O1 -o "$WORK/probe" "$ROOT/tests/systray-dock-probe.c" -lX11 ||
  { echo "FAIL: cannot build the dock probe"; exit 1; }

Xvfb "$DISP" -screen 0 1280x1024x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1

fail=0
start_dwm() {
  "$BIN" >"$WORK/dwm.log" 2>&1 &
  DWMPID=$!
  sleep 1.2
  kill -0 "$DWMPID" 2>/dev/null || {
    echo "FAIL: dwm did not start"; cat "$WORK/dwm.log"; exit 1
  }
}
stop_dwm() {
  kill "$DWMPID" 2>/dev/null || true
  wait "$DWMPID" 2>/dev/null || true
  DWMPID=
}

# dock a case and require dwm to survive it
case_survives() {
  name=$1 mode=$2
  start_dwm
  "$WORK/probe" "$DISP" "$mode" >/dev/null 2>&1 || true
  sleep 0.6
  if kill -0 "$DWMPID" 2>/dev/null; then
    echo "PASS: dwm survives $name"
  else
    echo "FAIL: dwm exited on $name"
    tail -5 "$WORK/dwm.log"
    fail=1
  fi
  stop_dwm
}

case_survives "a dock of the tray window itself" tray
case_survives "a dock of the root window" root
case_survives "a dock of a 0x0 icon" zero
case_survives "a normal icon dock" normal

# a window mapped before its dock request is briefly managed; it must end up in
# the tray instead of leaving a stray client or killing the WM
start_dwm
"$WORK/probe" "$DISP" managed >/dev/null 2>&1 &
PROBEPID=$!
sleep 2
if ! kill -0 "$DWMPID" 2>/dev/null; then
  echo "FAIL: dwm exited on a managed window dock"
  tail -5 "$WORK/dwm.log"
  fail=1
elif [ -z "$(xdotool search --classname Systray 2>/dev/null | head -1)" ]; then
  echo "FAIL: managed icon was not docked"
  fail=1
else
  echo "PASS: a managed window is released and docked"
fi
kill "$PROBEPID" 2>/dev/null || true
wait "$PROBEPID" 2>/dev/null || true
PROBEPID=
stop_dwm

[ "$fail" -eq 0 ] || exit 1
echo "systray-guards-check: ok"
