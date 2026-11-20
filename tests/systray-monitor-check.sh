#!/bin/sh
# Regression: the separate systray window must follow its own monitor's bar
# row. It is a root child, so its y has to come from that monitor's m->by + vp
# (like updatesystray), not from the root origin: root-relative y strands the
# tray off-screen whenever the tray monitor does not start at y=0 and moves it
# when another monitor's bar is toggled.
#
# Xvfb can only place its heads at the origin, so the test preloads a tiny
# Xinerama shim that reports two heads: the tray_pinning target at y=500 and a
# second head at y=0. Toggling the second monitor's bar (fake fullscreen,
# Mod+Shift+f) must leave the tray alone. The expose() fallback cannot mask
# this, because wintomon(other barwin) != systraytomon() and the tray is not
# repositioned from that monitor's exposes.
#
# Runs the built dwm on a private headless X display with a throwaway HOME.
set -eu

DISPLAY_NUM=${1:-:93}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=${BIN:-$ROOT/dwm}
WORK=$(mktemp -d /tmp/opencode/systray-monitor.XXXXXX)
SW=1280
SH=800
TRAY_Y=500

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# knobs from the config the binary was built with
pinning=$(sed -n 's/^static const *unsigned int *tray_pinning *= *\([0-9][0-9]*\).*/\1/p' "$ROOT/config.h" | head -n 1)
pinning=${pinning:-1}
failfirst=$(sed -n 's/^static const *int *tray_pinning_fail_first *= *\([0-9][0-9]*\).*/\1/p' "$ROOT/config.h" | head -n 1)
failfirst=${failfirst:-1}
if [ "$pinning" -eq 0 ]; then
  echo "SKIP: sloppy systray (tray_pinning = 0) follows the selected monitor"
  exit 0
fi
if [ "$pinning" -le 2 ]; then
  tray_idx=$((pinning - 1))
elif [ "$failfirst" -ne 0 ]; then
  tray_idx=0
else
  tray_idx=1
fi
other_idx=$((1 - tray_idx))

cat >"$WORK/xinerama.c" <<EOF
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

int XineramaIsActive(Display *dpy) { (void)dpy; return 1; }

XineramaScreenInfo *XineramaQueryScreens(Display *dpy, int *number) {
  XineramaScreenInfo *s = calloc(2, sizeof *s);

  (void)dpy;
  /* the tray head does not start at y=0; the other head does */
  s[$tray_idx].screen_number = $tray_idx;
  s[$tray_idx].x_org = 0;
  s[$tray_idx].y_org = $TRAY_Y;
  s[$tray_idx].width = $SW;
  s[$tray_idx].height = 300;
  s[$other_idx].screen_number = $other_idx;
  s[$other_idx].x_org = 0;
  s[$other_idx].y_org = 0;
  s[$other_idx].width = $SW;
  s[$other_idx].height = 500;
  *number = 2;
  return s;
}
EOF
${CC:-cc} -shared -fPIC -o "$WORK/xinerama.so" "$WORK/xinerama.c"

export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true
[ -f "$ROOT/icons/tab-fallback.png" ] &&
  cp "$ROOT/icons/tab-fallback.png" "$HOME/.dwm/tab-fallback.png"

Xvfb "$DISPLAY_NUM" -screen 0 ${SW}x${SH}x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
i=0
while [ "$i" -lt 40 ]; do
  if xdpyinfo >/dev/null 2>&1; then break; fi
  sleep 0.25
  i=$((i + 1))
done

LD_PRELOAD="$WORK/xinerama.so" "$BIN" >"$WORK/dwm.log" 2>&1 &
DWMPID=$!
sleep 1.5
if ! kill -0 "$DWMPID" 2>/dev/null; then
  echo "FAIL: dwm exited"
  cat "$WORK/dwm.log"
  exit 1
fi

fail=0
check() { # $1 description, $2 got, $3 want
  if [ "$2" = "$3" ]; then
    echo "PASS: $1 ($2)"
  else
    echo "FAIL: $1: got '$2', want '$3'"
    fail=1
  fi
}

# y of the systray window (the XEMBED selection owner) in root coordinates
trayy() {
  python3 - <<'PY'
from Xlib import display
d = display.Display()
owner = d.get_selection_owner(
    d.intern_atom('_NET_SYSTEM_TRAY_S%d' % d.get_default_screen()))
print('missing' if not owner else owner.get_geometry().y)
PY
}

# "id y" for every override-redirect dwm window (the barwins)
barwins() {
  python3 - <<'PY'
from Xlib import display
d = display.Display()
for w in d.screen().root.query_tree().children:
    try:
        if (w.get_wm_class() == ('dwm', 'dwm') and
                w.get_attributes().override_redirect):
            print(w.id, w.get_geometry().y)
    except Exception:
        pass
PY
}

pointer_y() {
  xdotool getmouselocation | sed -n 's/.*y:\([0-9-]*\).*/\1/p'
}

i=0
t0=$(trayy)
while [ "$t0" = missing ] && [ "$i" -lt 20 ]; do
  sleep 0.25
  t0=$(trayy)
  i=$((i + 1))
done
if [ "$t0" = missing ]; then
  echo "FAIL: dwm never owned the systray selection"
  cat "$WORK/dwm.log"
  exit 1
fi
if [ "$t0" -gt "$TRAY_Y" ]; then
  echo "PASS: tray starts on the tray monitor's bar row (y=$t0)"
else
  echo "FAIL: tray starts at y=$t0, want > $TRAY_Y"
  fail=1
fi

# select the other monitor: park the pointer on the tray head and cycle
# focusmon (+1) until setcurrentmon warps it out of that head
xdotool mousemove --sync 640 $((TRAY_Y + 100))
i=0
py=$(pointer_y)
while [ "$py" -ge "$TRAY_Y" ] && [ "$i" -lt 3 ]; do
  xdotool key super+period
  sleep 0.4
  py=$(pointer_y)
  i=$((i + 1))
done
if [ "$py" -lt "$TRAY_Y" ]; then
  echo "PASS: other monitor selected"
else
  echo "FAIL: could not focus the other monitor (pointer y=$py)"
  fail=1
fi
check "focusing another monitor leaves the tray" "$(trayy)" "$t0"

# the other monitor's bar must really toggle when it is fake-fullscreened
otherbar=$(barwins | awk -v y="$TRAY_Y" '$2 < y {print $1; exit}')
if [ -z "${otherbar:-}" ]; then
  echo "FAIL: found no bar window on the other monitor"
  fail=1
else
  by0=$(barwins | awk -v id="$otherbar" '$1 == id {print $2}')
  if [ "$by0" -gt 0 ]; then
    echo "PASS: other monitor's bar is visible (y=$by0)"
  else
    echo "FAIL: other monitor's bar starts hidden (y=$by0)"
    fail=1
  fi
  xdotool key super+shift+f
  sleep 0.7
  by1=$(barwins | awk -v id="$otherbar" '$1 == id {print $2}')
  if [ "$by1" -lt 0 ]; then
    echo "PASS: fake fullscreen hides the other monitor's bar (y=$by1)"
  else
    echo "FAIL: other monitor's bar not hidden (y=$by1)"
    fail=1
  fi
  check "hiding another monitor's bar leaves the tray" "$(trayy)" "$t0"
  xdotool key super+shift+f
  sleep 0.7
  by2=$(barwins | awk -v id="$otherbar" '$1 == id {print $2}')
  if [ "$by2" -gt 0 ]; then
    echo "PASS: leaving fake fullscreen restores the bar (y=$by2)"
  else
    echo "FAIL: other monitor's bar not restored (y=$by2)"
    fail=1
  fi
  check "restoring another monitor's bar leaves the tray" "$(trayy)" "$t0"
fi

# the reported case: fake fullscreen on the tray monitor itself
xdotool key super+comma
sleep 0.4
if [ "$(pointer_y)" -lt "$TRAY_Y" ]; then
  xdotool key super+period
  sleep 0.4
fi
if [ "$(pointer_y)" -lt "$TRAY_Y" ]; then
  echo "FAIL: could not focus the tray monitor"
  fail=1
else
  xdotool key super+shift+f
  sleep 0.7
  hidden=$(trayy)
  if [ "$hidden" != "$t0" ]; then
    echo "PASS: fake fullscreen hides the tray with its bar (y=$hidden)"
  else
    echo "FAIL: tray still at y=$hidden while the bar is hidden"
    fail=1
  fi
  xdotool key super+shift+f
  sleep 0.7
  check "leaving fake fullscreen returns the tray" "$(trayy)" "$t0"
fi

if [ "$fail" -ne 0 ]; then
  echo "FAIL: systray-monitor-check"
  exit 1
fi
echo "PASS: systray stays on its monitor's bar row"
