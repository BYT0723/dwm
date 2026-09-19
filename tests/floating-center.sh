#!/bin/sh
# Regression test: floating clients are placed by their client area, not the
# frame. The per-client titlebar grows the frame downward; manage() must not
# let that chrome displace the content:
#   - edge request (e.g. 0,0 = "no position") -> the client area is centred
#   - explicit request (e.g. +100+100, or mpv centring itself) -> the client
#     area stays where asked, the frame grows upward around it
#
# Runs the built dwm on a private headless X display with a throwaway HOME,
# opens two floating xterms (class "mpv" matches the floating rule) and checks
# their client geometry with xwininfo. No titlebar metrics are hardcoded: the
# title height is read back as the client's offset inside its frame.
set -eu

DISPLAY_NUM=${1:-:96}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/floating-center.XXXXXX)
SW=1280
SH=800

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XT1:-}" ] && kill "$XT1" 2>/dev/null || true
  [ -n "${XT2:-}" ] && kill "$XT2" 2>/dev/null || true
  [ -n "${XT3:-}" ] && kill "$XT3" 2>/dev/null || true
  [ -n "${XT4:-}" ] && kill "$XT4" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true
[ -f "$ROOT/icons/tab-fallback.png" ] &&
  cp "$ROOT/icons/tab-fallback.png" "$HOME/.dwm/tab-fallback.png"

Xvfb "$DISPLAY_NUM" -screen 0 ${SW}x${SH}x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1
"$BIN" >"$WORK/dwm.log" 2>&1 &
DWMPID=$!
sleep 1.5
if ! kill -0 "$DWMPID" 2>/dev/null; then
  echo "FAIL: dwm exited"
  cat "$WORK/dwm.log"
  exit 1
fi

# frame border width, from the config the binary was built with (the mpv
# rule leaves bw at the default)
bw=$(sed -n 's/^static const unsigned int borderpx *= *\([0-9][0-9]*\).*/\1/p' "$ROOT/config.h")
bw=${bw:-0}

# wait up to ~10s for a client window with the given title
waitwid() {
  i=0
  while [ "$i" -lt 20 ]; do
    wid=$(xdotool search --name "^$1$" 2>/dev/null | head -n 1)
    if [ -n "$wid" ]; then
      printf '%s' "$wid"
      return 0
    fi
    sleep 0.5
    i=$((i + 1))
  done
  return 1
}

# client geometry: "absx absy w h relx rely"
cgeom() {
  timeout 5 xwininfo -id "$1" 2>/dev/null | awk '
    /Absolute upper-left X:/ {ax=$4}
    /Absolute upper-left Y:/ {ay=$4}
    /Relative upper-left X:/ {rx=$4}
    /Relative upper-left Y:/ {ry=$4}
    /^  Width:/ {w=$2}
    /^  Height:/ {h=$2}
    END {printf "%d %d %d %d %d %d", ax, ay, w, h, rx, ry}'
}

# geometry may still be settling right after mapping (the client re-asserts
# its size via ConfigureRequest); sample until two consecutive reads agree
settlegeom() {
  prev=""
  i=0
  while [ "$i" -lt 10 ]; do
    cur=$(cgeom "$1")
    if [ -n "$cur" ] && [ "$cur" = "$prev" ]; then
      printf '%s' "$cur"
      return 0
    fi
    prev=$cur
    sleep 0.3
    i=$((i + 1))
  done
  printf '%s' "$cur"
}

fail=0

# case 1: no position requested -> client area centred on the monitor
xterm -class mpv -geometry 80x24 -T fc-edge >/dev/null 2>&1 & XT1=$!
wid1=$(waitwid fc-edge) || { echo "FAIL: edge client never mapped"; exit 1; }
set -- $(settlegeom "$wid1")
cx=$(($1 + $3 / 2))
cy=$(($2 + $4 / 2))
# +-2 tolerance: the client sits inside the frame border, whose width is not
# part of the centring arithmetic (pre-existing semantics)
if [ "$cx" -ge $((SW / 2 - 2)) ] && [ "$cx" -le $((SW / 2 + 2)) ] &&
  [ "$cy" -ge $((SH / 2 - 2)) ] && [ "$cy" -le $((SH / 2 + 2)) ]; then
  echo "PASS: edge request centres the client area ($cx,$cy near $((SW / 2)),$((SH / 2)))"
else
  echo "FAIL: edge request did not centre the client area ($cx,$cy, want near $((SW / 2)),$((SH / 2)))"
  fail=1
fi

# case 2: explicit position -> client area exactly where asked (+ frame border)
xterm -class mpv -geometry 80x24+100+100 -T fc-explicit >/dev/null 2>&1 & XT2=$!
wid2=$(waitwid fc-explicit) || { echo "FAIL: explicit client never mapped"; exit 1; }
set -- $(settlegeom "$wid2")
if [ "$1" -eq $((100 + bw)) ] && [ "$2" -eq $((100 + bw)) ]; then
  echo "PASS: explicit request keeps the client area at +100+100 (abs $1,$2, frame border $bw)"
else
  echo "FAIL: explicit request displaced the client area (abs $1,$2, want $((100 + bw)),$((100 + bw)))"
  fail=1
fi

# case 3: off-monitor request (e.g. mpv centring on the whole multi-monitor
# display while opening on one monitor) -> clamped into the monitor, then
# centred like an edge request instead of sticking to the corner
xterm -class mpv -geometry 80x24+2300+780 -T fc-clamped >/dev/null 2>&1 & XT3=$!
wid3=$(waitwid fc-clamped) || { echo "FAIL: clamped client never mapped"; exit 1; }
set -- $(settlegeom "$wid3")
cx=$(($1 + $3 / 2))
cy=$(($2 + $4 / 2))
if [ "$cx" -ge $((SW / 2 - 2)) ] && [ "$cx" -le $((SW / 2 + 2)) ] &&
  [ "$cy" -ge $((SH / 2 - 2)) ] && [ "$cy" -le $((SH / 2 + 2)) ]; then
  echo "PASS: clamped request is centred, not stuck to the corner ($cx,$cy near $((SW / 2)),$((SH / 2)))"
else
  echo "FAIL: clamped request stuck to the corner ($cx,$cy, want near $((SW / 2)),$((SH / 2)))"
  fail=1
fi

# case 4: oversized request -> frame scaled down proportionally to fit the
# monitor (outer geometry = client + title offset + frame border both sides)
xterm -class mpv -geometry 300x90+0+0 -T fc-huge >/dev/null 2>&1 & XT4=$!
wid4=$(waitwid fc-huge) || { echo "FAIL: huge client never mapped"; exit 1; }
set -- $(settlegeom "$wid4")
outerw=$(($3 + 2 * bw))
outerh=$(($4 + $6 + 2 * bw))
if [ "$outerw" -le "$SW" ] && [ "$outerh" -le "$SH" ]; then
  echo "PASS: oversized request fits the monitor (outer ${outerw}x${outerh} within ${SW}x${SH})"
else
  echo "FAIL: oversized request overflows the monitor (outer ${outerw}x${outerh}, want within ${SW}x${SH})"
  fail=1
fi

exit "$fail"
