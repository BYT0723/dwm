#!/bin/sh
# Headless, reproducible perf harness for dwm: a private Xvfb display, a
# throwaway HOME (so runautostart finds nothing) and synthetic workloads that
# drive the bar and window-management hot paths.
#
# CPU is read from /proc/<pid>/stat (utime + stime, 10 ms ticks) for dwm *and*
# Xvfb: dwm only marshals requests, the rendering happens in the X server, so
# measuring dwm alone hides most of the cost.
#
# usage: tests/perf-harness.sh <scenario> [iterations]
#   idle      sleep <iterations> seconds, no events: must stay at 0 ticks
#   status    XStoreName on the root window: updatestatus -> drawbar
#   tags      Mod+1..5 cycling: view -> arrange -> drawbar
#   focus     Mod+j cycling: focus -> drawbar + drawtitles
#   switcher  open/close the overlay: switchercollect -> switcherdraw
#   mixed     status + tag + focus events in one loop
#
# env:
#   DWM=<binary>            dwm to run (default: ./dwm)
#   RUNS=<n>                repeat the scenario n times (default 1)
#   CLIENTS=<n>             xterms to start (default 3, minimum 3)
#   DISP_NUM=<n>            Xvfb display number (default 98)
#   GRACE=1                 quit dwm through its keybinding at the end, so a
#                           -pg build writes gmon.out (set GMON_OUT_PREFIX to
#                           a path prefix to keep several runs apart)
#
# example:
#   tests/perf-harness.sh status 800
#   make CFLAGS='... -pg' LDFLAGS='... -pg' && \
#     GRACE=1 GMON_OUT_PREFIX=/tmp/dwm-gmon tests/perf-harness.sh status 1500
set -eu

SCEN=${1:-status}
ITER=${2:-100}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=${DWM:-$ROOT/dwm}
RUNS=${RUNS:-1}
CLIENTS=${CLIENTS:-3}
export DISPLAY=:${DISP_NUM:-98}

WORK=$(mktemp -d /tmp/opencode/dwm-perf.XXXXXX)

cleanup() {
  for p in ${DWMPID:-} ${XPID:-} ${XT1:-} ${XT2:-} ${XT3:-} ${EXTRAPIDS:-}; do
    kill "$p" 2>/dev/null || true
  done
  wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

ticks() { # utime + stime of a pid, in clock ticks
  awk '{print $14 + $15}' "/proc/$1/stat" 2>/dev/null || echo 0
}

# throwaway HOME + XDG dirs: no ~/.dwm, no ~/.local/share/dwm => no autostart
export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
unset XDG_RUNTIME_DIR || true

# the tab icon fallback the config points at
[ -f "$ROOT/icons/tab-fallback.png" ] &&
  cp "$ROOT/icons/tab-fallback.png" "$HOME/.dwm/"
[ -f "$ROOT/icons/client-fallback.png" ] &&
  cp "$ROOT/icons/client-fallback.png" "$HOME/.dwm/"

Xvfb "$DISPLAY" -screen 0 1600x900x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1

# status fixture: block ids, one pill per pane, plus a row break
FIXTURE=$(printf '\x01 date-1234 \x02 bat-80%% \x03 vol-40%% \x07 mem-1.2G \x06 disk-40G \x09 weather \x0b docker \x7f\x01 late')
xsetroot -name "$FIXTURE" 2>/dev/null || true

"$BIN" >"$WORK/dwm.log" 2>&1 &
DWMPID=$!
sleep 1.5

xterm -geometry 80x24+0+0 >/dev/null 2>&1 &
XT1=$!
xterm -geometry 80x24+200+100 >/dev/null 2>&1 &
XT2=$!
xterm -geometry 80x24+400+200 >/dev/null 2>&1 &
XT3=$!
sleep 2
n=3
while [ "$n" -lt "$CLIENTS" ]; do
  xterm -geometry 60x16+$((50 * n))+$((20 * n)) >/dev/null 2>&1 &
  EXTRAPIDS="${EXTRAPIDS:-} $!"
  n=$((n + 1))
done
[ "$CLIENTS" -gt 3 ] && sleep 3

run_work() {
  i=0
  case "$SCEN" in
  idle) sleep "$ITER" ;;
  status)
    while [ "$i" -lt "$ITER" ]; do
      xsetroot -name "$FIXTURE $i"
      sleep 0.01
      i=$((i + 1))
    done
    ;;
  tags)
    while [ "$i" -lt "$ITER" ]; do
      xdotool key --clearmodifiers "super+$((i % 5 + 1))"
      sleep 0.03
      i=$((i + 1))
    done
    ;;
  focus)
    while [ "$i" -lt "$ITER" ]; do
      xdotool key --clearmodifiers super+j
      sleep 0.02
      i=$((i + 1))
    done
    ;;
  switcher)
    while [ "$i" -lt "$ITER" ]; do
      xdotool key --clearmodifiers super+grave
      sleep 0.05
      xdotool key --clearmodifiers Escape
      sleep 0.05
      i=$((i + 1))
    done
    ;;
  mixed)
    while [ "$i" -lt "$ITER" ]; do
      xsetroot -name "$FIXTURE $i"
      xdotool key --clearmodifiers "super+$((i % 5 + 1))"
      xdotool key --clearmodifiers super+j
      sleep 0.02
      i=$((i + 1))
    done
    ;;
  *)
    echo "unknown scenario: $SCEN" >&2
    exit 2
    ;;
  esac
}

run_once() {
  a_dwm=$(ticks "$DWMPID")
  a_x=$(ticks "$XPID")
  start=$(date +%s%N)
  run_work
  end=$(date +%s%N)
  b_dwm=$(ticks "$DWMPID")
  b_x=$(ticks "$XPID")
  echo "$SCEN iters=$ITER dwm_ticks=$((b_dwm - a_dwm)) xvfb_ticks=$((b_x - a_x)) ms=$(((end - start) / 1000000))"
}

n=0
while [ "$n" -lt "$RUNS" ]; do
  run_once
  n=$((n + 1))
done

# let a -pg build exit through main() so gmon.out is written
if [ -n "${GRACE:-}" ]; then
  xdotool key --clearmodifiers super+ctrl+shift+q 2>/dev/null || true
  n=0
  while kill -0 "$DWMPID" 2>/dev/null && [ "$n" -lt 100 ]; do
    sleep 0.1
    n=$((n + 1))
  done
fi
