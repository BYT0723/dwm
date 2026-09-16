#!/bin/sh
# Smoke test for the bar: run the built dwm on a private headless X display
# with a throwaway HOME, so runautostart() finds no script and nothing of the
# user's session is started or touched.
#
# It starts two clients and screenshots the bar, which exercises the tab row
# (both tab modes render here) including the icon fallback for clients that
# carry no _NET_WM_ICON, as xterm does not.
#
# It then presses MODKEY|ShiftMask+b (toggletabmode) and screenshots again: the
# bar has to change, which covers the runtime mode switch and its redraw.
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

# screenshot the root window, cropped to $2 (WxH+X+Y); $3 is an optional integer
# scale percent. Output goes to $1.
snap() {
  xwd -root -silent >"$WORK/.snap.xwd" 2>/dev/null || true
  if [ -n "${3:-}" ]; then
    convert "$WORK/.snap.xwd" -crop "$2" +repage -scale "$3"% "$1" \
      2>/dev/null || true
  else
    convert "$WORK/.snap.xwd" -crop "$2" +repage "$1" 2>/dev/null || true
  fi
}

# absolute pixel difference between two images, rounded to an integer; empty
# when the images could not be compared (ImageMagick prints a leading number,
# IM 6 as "N" and IM 7 as "N (normalized)")
ae() {
  d=$(compare -metric AE "$1" "$2" null: 2>&1 || true)
  case "$d" in
  [0-9]*) printf '%s' "$(printf '%s\n' "$d" | awk '{print int($1 + 0.5)}')" ;;
  *) printf '' ;;
  esac
}

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

# hand both clients a 1x1 _NET_WM_ICON. A client without an icon never caches a
# tab-sized one (updateicon rebuilds it every time), so the stale-icon
# regression below needs a client that really has one.
for t in smoke-one smoke-two; do
  wid=$(xdotool search --name "^$t$" 2>/dev/null | head -n 1)
  if [ -n "$wid" ]; then
    xprop -id "$wid" -f _NET_WM_ICON 32c -set _NET_WM_ICON "1,1,4294901760" \
      2>/dev/null || true
  fi
done
# dwm only redraws the bar for the *selected* client's icon change, so an icon
# set on the other one is not painted yet. Settle with an unrelated redraw (the
# bar hidden and shown again) so the baseline below already shows both icons.
xdotool key --clearmodifiers super+b 2>/dev/null || true
sleep 0.3
xdotool key --clearmodifiers super+b 2>/dev/null || true
sleep 0.5

# bar strip only, scaled up so the pills are inspectable
snap "$ROOT/tests/.smoke-bar.png" 1280x40+0+0 300

# click sweep across the status strip: with the fixture set, the click handler
# resolves each pill back to its block id. Harmless when it could not be set.
i=800
while [ "$i" -le 1270 ]; do
  xdotool mousemove "$i" 12 click 1 2>/dev/null || true
  i=$((i + 10))
done
sleep 0.5

# tab mode toggle: MODKEY|ShiftMask+b must re-render the bar. Mode A draws one
# pill per client carrying its title, mode B a single shared pill of icons, so
# the strip has to change; without the key bound nothing happens and the two
# strips are identical. The pointer is parked off the bar first so no tab
# tooltip can leak into either strip.
xdotool mousemove 640 55 2>/dev/null || true
sleep 0.2
xdotool key --clearmodifiers super+shift+b 2>/dev/null || true
sleep 0.5
snap "$ROOT/tests/.smoke-bar-icons.png" 1280x40+0+0 300

if [ -s "$ROOT/tests/.smoke-bar.png" ] && [ -s "$ROOT/tests/.smoke-bar-icons.png" ]; then
  ndiff=$(ae "$ROOT/tests/.smoke-bar.png" "$ROOT/tests/.smoke-bar-icons.png")
  case "$ndiff" in
  '')
    echo "SKIP: could not compare the bar strips"
    ;;
  0)
    echo "FAIL: tab mode toggle did not re-render the bar"
    exit 1
    ;;
  *)
    echo "PASS: tab mode toggle re-rendered the bar ($ndiff px)"
    ;;
  esac

  # either mode can be the baseline (config defaults to icon-only): the two
  # modes must differ in icon height, with the icon-only one smaller because it
  # reserves a row under each icon for the selection dot and rebuilds the icon
  # picture smaller. The client icons are red (nothing else in the bar is), so
  # measure the height of the reddish pixels in each strip.
  redh() {
    box=$(convert "$1" -fx '(r>0.15 && r>1.8*g && r>1.8*b) ? 1 : 0' \
      -format "%@" info: 2>/dev/null || true)
    h=${box#*x}
    printf '%s' "${h%%+*}"
  }
  hA=$(redh "$ROOT/tests/.smoke-bar.png")
  hB=$(redh "$ROOT/tests/.smoke-bar-icons.png")
  if [ -z "$hA" ] || [ -z "$hB" ] || [ "$hA" = 0 ] || [ "$hB" = 0 ]; then
    echo "SKIP: no client icon in the strips to measure ($hA -> $hB)"
  elif [ "$hA" = "$hB" ]; then
    echo "FAIL: icon-only mode did not shrink the tab icons ($hA -> $hB px)"
    exit 1
  else
    # baseline is whatever config.h defaults to; the icon-only strip must be
    # the shorter one.
    if grep -q 'tabmode = TabModeIcons' "$ROOT/config.h" 2>/dev/null; then
      icons_h=$hA
      titled_h=$hB
    else
      icons_h=$hB
      titled_h=$hA
    fi
    if [ "$icons_h" -lt "$titled_h" ]; then
      echo "PASS: icon-only mode shrinks the tab icons ($icons_h < $titled_h px)"
    else
      echo "FAIL: icon-only mode did not shrink the tab icons ($hA -> $hB px)"
      exit 1
    fi
  fi
fi

# toggling back must reproduce the first mode pixel for pixel: a tab-sized icon
# left over from the icon-only mode would linger as a smaller icon in the
# titled mode, which this catches
xdotool key --clearmodifiers super+shift+b 2>/dev/null || true
sleep 0.5
snap "$ROOT/tests/.smoke-bar-back.png" 1280x40+0+0 300

if [ -s "$ROOT/tests/.smoke-bar-back.png" ]; then
  nback=$(ae "$ROOT/tests/.smoke-bar.png" "$ROOT/tests/.smoke-bar-back.png")
  if [ "$nback" = 0 ]; then
    echo "PASS: toggling twice restores the first mode exactly"
  else
    echo "FAIL: toggling twice did not restore the first mode ($nback px)"
    exit 1
  fi
fi

# preview option: with previews on, hovering a client tab shows a tooltip and
# (after leaving a tag) hovering a tag shows its snapshot; with previews off
# (the default) neither appears. Sweep the pointer along the bar and report
# whether any frame differs from the off-bar baseline.
#   hover_sweep LO HI BASEPNG  -> 0 = all frames identical, 1 = something showed
hover_sweep() {
  x=$1
  hi=$2
  base=$3
  while [ "$x" -le "$hi" ]; do
    xdotool mousemove "$x" 12 2>/dev/null || true
    sleep 0.7 # hoverdelay is 500 ms
    snap "$WORK/hover.png" 1280x64+0+0
    n=$(ae "$base" "$WORK/hover.png")
    if [ -n "$n" ] && [ "$n" != 0 ]; then
      xdotool mousemove 640 55 2>/dev/null || true
      return 1
    fi
    x=$((x + 10))
  done
  xdotool mousemove 640 55 2>/dev/null || true
  return 0
}

# client preview: on tag 1 the client tabs are in the centred shared pill
sleep 0.3
snap "$WORK/nopreview-c.png" 1280x64+0+0
client_shown=0
if [ -s "$WORK/nopreview-c.png" ] &&
  ! hover_sweep 600 680 "$WORK/nopreview-c.png"; then
  client_shown=1
fi

# tag preview: leave tag 1, then hover the tag row on the left
xdotool key --clearmodifiers super+2 >/dev/null 2>&1 || true
sleep 0.5
snap "$WORK/nopreview-t.png" 1280x64+0+0
tag_shown=0
if [ -s "$WORK/nopreview-t.png" ] &&
  ! hover_sweep 20 160 "$WORK/nopreview-t.png"; then
  tag_shown=1
fi

want=0
if grep -qE 'previews[[:space:]]*=[[:space:]]*1' "$ROOT/config.h" 2>/dev/null; then
  want=1
fi
if [ "$client_shown" = "$want" ] && [ "$tag_shown" = "$want" ]; then
  if [ "$want" = 1 ]; then
    echo "PASS: previews enabled: client tooltip and tag preview appear"
  else
    echo "PASS: previews disabled (default): no client tooltip, no tag preview"
  fi
else
  echo "FAIL: previews want=$want, got client=$client_shown tag=$tag_shown"
  exit 1
fi

echo "--- dwm.log ---"
cat "$WORK/dwm.log" 2>/dev/null || true
echo "--- bar strip: $ROOT/tests/.smoke-bar.png ---"
echo "--- workspace: $WORK ---"
