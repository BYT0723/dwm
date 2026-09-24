#!/bin/sh
# Regression: the per-client titlebar is hidden only in fake fullscreen
# (Mod+Shift+f: monocle layout with the bar hidden). Plain tile layout and
# maximize (Mod+f) keep their titlebars.
#
# Probe: xwininfo "Relative upper-left Y" of a tiled client is its offset
# inside the frame, i.e. titleh() — no titlebar metrics are hardcoded.
# Expectations adapt to the config the binary was built with (title_show,
# title_hide_fullscreen), the same way floating-center.sh reads borderpx.
#
# Runs the built dwm on a private headless X display with a throwaway HOME.
set -eu

DISPLAY_NUM=${1:-:95}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/fullscreen-titlebar.XXXXXX)
SW=1280
SH=800

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XT1:-}" ] && kill "$XT1" 2>/dev/null || true
  [ -n "${XT2:-}" ] && kill "$XT2" 2>/dev/null || true
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

# knobs from the config the binary was built with
title_show=$(sed -n 's/^static const *int *title_show *= *\([0-9][0-9]*\).*/\1/p' "$ROOT/config.h" | head -n 1)
title_show=${title_show:-1}
title_hide_fs=$(sed -n 's/^static const *int *title_hide_fullscreen *= *\([0-9][0-9]*\).*/\1/p' "$ROOT/config.h" | head -n 1)
title_hide_fs=${title_hide_fs:-1}

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

# client geometry: "absx absy w h rely" (rely = offset inside the frame)
cgeom() {
  timeout 5 xwininfo -id "$1" 2>/dev/null | awk '
    /Absolute upper-left X:/ {ax=$4}
    /Absolute upper-left Y:/ {ay=$4}
    /Relative upper-left Y:/ {ry=$4}
    /^  Width:/ {w=$2}
    /^  Height:/ {h=$2}
    END {printf "%d %d %d %d %d", ax, ay, w, h, ry}'
}

# geometry may still be settling right after a layout switch; sample until
# two consecutive reads agree
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
check_eq() { # $1=label $2=got $3=want
  if [ "$2" = "$3" ]; then
    echo "PASS: $1 ($2)"
  else
    echo "FAIL: $1 (got $2, want $3)"
    fail=1
  fi
}

# two tiled clients, so maximize visibly changes the focused one's width
# (with a single client tile-master and monocle-with-gaps coincide)
xterm -class xterm -T fstitle-a >/dev/null 2>&1 & XT1=$!
waitwid fstitle-a >/dev/null || { echo "FAIL: first client never mapped"; exit 1; }
xterm -class xterm -T fstitle-b >/dev/null 2>&1 & XT2=$!
wid=$(waitwid fstitle-b) || { echo "FAIL: second client never mapped"; exit 1; }

# 1. tile: titlebar visible iff title_show
set -- $(settlegeom "$wid")
tile_ry=$5; tile_w=$3
if [ "$title_show" = 1 ]; then
  if [ "$tile_ry" -gt 0 ]; then
    echo "PASS: tile shows the titlebar (offset $tile_ry)"
  else
    echo "FAIL: tile hides the titlebar (offset $tile_ry)"; fail=1
  fi
else
  check_eq "tile with title_show=0" "$tile_ry" 0
fi

# 2. maximize (Mod+f) keeps the titlebar; the width jump proves the mode engaged
xdotool key Super+f
sleep 0.5
set -- $(settlegeom "$wid")
if [ "$3" -gt $((tile_w + 100)) ]; then
  echo "PASS: maximize engaged (width $tile_w -> $3)"
else
  echo "FAIL: maximize did not engage (width $3, tile was $tile_w)"; fail=1
fi
check_eq "maximize keeps the titlebar" "$5" "$tile_ry"

# 3. back to tile
xdotool key Super+f
sleep 0.5
set -- $(settlegeom "$wid")
check_eq "leaving maximize restores the titlebar" "$5" "$tile_ry"

# 4. fake fullscreen (Mod+Shift+f) hides the titlebar iff configured
xdotool key Super+Shift+f
sleep 0.5
set -- $(settlegeom "$wid")
if [ "$title_show" = 1 ] && [ "$title_hide_fs" = 1 ]; then
  check_eq "fullscreen hides the titlebar" "$5" 0
  if [ "$1" = 0 ] && [ "$2" = 0 ]; then
    echo "PASS: fullscreen client at the monitor origin ($1,$2)"
  else
    echo "FAIL: fullscreen client not at the origin ($1,$2)"; fail=1
  fi
else
  check_eq "fullscreen keeps the titlebar (not configured to hide)" "$5" "$tile_ry"
fi

# 5. leaving fullscreen restores the titlebar
xdotool key Super+Shift+f
sleep 0.5
set -- $(settlegeom "$wid")
check_eq "leaving fullscreen restores the titlebar" "$5" "$tile_ry"

exit "$fail"
