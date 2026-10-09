#!/bin/sh
# Regression: toggletitle takes effect immediately on tiled clients and keeps
# cell-hinted terminals (xterm as proxy) cell-aligned, i.e. no interior
# padding appears. Probe: xwininfo "Relative upper-left Y" of the client is
# its offset inside the frame (titleh), and (client_h - base_h) % inc_h from
# WM_NORMAL_HINTS must stay 0 across both toggles.
#
# The toggle key is parsed from the config's toggletitle binding so the test
# survives rebinding (MODKEY is assumed to be Mod4/Super, as in config.h).
#
# Runs the built dwm on a private headless X display with a throwaway HOME.
set -eu

DISPLAY_NUM=${1:-:95}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/title-toggle.XXXXXX)

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  [ -n "${XT:-}" ] && kill "$XT" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true

Xvfb "$DISPLAY_NUM" -screen 0 1280x800x24 >"$WORK/xvfb.log" 2>&1 &
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

# client geometry: "w h rely" (rely = offset inside the frame = titleh)
cgeom() {
  timeout 5 xwininfo -id "$1" 2>/dev/null | awk '
    /Relative upper-left Y:/ {ry=$4}
    /^  Width:/ {w=$2}
    /^  Height:/ {h=$2}
    END {printf "%d %d %d", w, h, ry}'
}

# size hints: "basew baseh incw inch"
hints() {
  xprop -id "$1" WM_NORMAL_HINTS 2>/dev/null | awk '
    /base size:/ {bw=$5; bh=$7}
    /resize increment:/ {iw=$5; ih=$7}
    END {printf "%d %d %d %d", bw, bh, iw, ih}'
}

# geometry may still be settling right after the toggle; sample until
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

# derive the toggle key from config.h, e.g.
#   {MODKEY|ShiftMask, XK_t, toggletitle, {0}} -> super+shift+t
keyline=$(grep "toggletitle" "$ROOT/config.h" | grep -v "^\s*//" | head -n 1)
if [ -z "$keyline" ]; then
  echo "SKIP: no toggletitle binding in config.h"
  exit 0
fi
mods=$(printf '%s' "$keyline" | sed -n 's/^[^{]*{\([^,]*\),.*/\1/p')
xk=$(printf '%s' "$keyline" | sed -n 's/^[^{]*{[^,]*,[[:space:]]*\(XK_[A-Za-z0-9_]*\).*/\1/p')
togglekey=""
case "$mods" in *MODKEY*) togglekey="super";; esac
case "$mods" in *ControlMask*) togglekey="${togglekey:+${togglekey}+}ctrl";; esac
case "$mods" in *ShiftMask*) togglekey="${togglekey:+${togglekey}+}shift";; esac
keyname=$(printf '%s' "$xk" | sed 's/^XK_//' | tr 'A-Z' 'a-z')
togglekey="${togglekey:+${togglekey}+}${keyname}"
if [ -z "$keyname" ]; then
  echo "SKIP: could not parse toggletitle key from config.h"
  exit 0
fi
echo "INFO: toggling with key $togglekey"

xterm -class xterm -T titletoggle -fn 10x20 >/dev/null 2>&1 & XT=$!
wid=$(waitwid titletoggle) || { echo "FAIL: client never mapped"; exit 1; }
sleep 1.5

set -- $(hints "$wid")
bh=$2; ih=$4
if [ "$ih" -le 0 ]; then
  echo "SKIP: terminal reports no height increment"
  exit 0
fi

# $1=w $2=h $3=rely -> prints "h rely mod"
sample() {
  set -- $(settlegeom "$wid")
  mod=$(( ($2 - bh) % ih ))
  printf '%s %s %s' "$2" "$3" "$mod"
}

set -- $(sample); h0=$1; rely0=$2; mod0=$3
check_eq "tiled terminal cell-aligned before toggle" "$mod0" 0

xdotool key --clearmodifiers "$togglekey" 2>/dev/null || true
sleep 1
set -- $(sample); h1=$1; rely1=$2; mod1=$3
# one toggle must flip the strip: shown -> 0, hidden -> visible (>0)
if [ "$rely0" -gt 0 ]; then
  check_eq "toggle takes effect without a new window" "$rely1" 0
elif [ "$rely1" -gt 0 ]; then
  echo "PASS: toggle takes effect without a new window ($rely1)"
else
  echo "FAIL: toggle takes effect without a new window (got $rely1, want >0)"
  fail=1
fi
check_eq "tiled terminal still cell-aligned after toggle" "$mod1" 0

xdotool key --clearmodifiers "$togglekey" 2>/dev/null || true
sleep 1
set -- $(sample); h2=$1; rely2=$2; mod2=$3
check_eq "toggle round-trips title offset" "$rely2" "$rely0"
check_eq "toggle round-trips client height" "$h2" "$h0"
check_eq "tiled terminal still cell-aligned after round-trip" "$mod2" 0

exit $fail
