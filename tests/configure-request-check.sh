#!/bin/sh
# Regression: a floating client's own ConfigureRequest must actually resize the
# window.  dwm keeps the request in c->x/y/w/h before applying size hints, so
# the old code path through resize() saw "no change" and skipped resizeclient(),
# silently dropping the request (scrcpy shrank on every device rotation and only
# jumped back when dragging its titlebar).
#
# Runs the built dwm on a private headless X display; a python3-xlib client
# stands in for scrcpy: no size hints first, then min==max aspect-ratio hints
# updated to the new orientation's ratio right before each resize, which is what
# SDL/scrcpy does.
set -eu

DISPLAY_NUM=${1:-:98}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/dwm
WORK=$(mktemp -d /tmp/opencode/configure-request.XXXXXX)
SW=1280
SH=800

cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true

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

python3 - <<'PY'
import sys
import time

from Xlib import X, Xutil, display

d = display.Display()
screen = d.screen()
root = screen.root
fail = 0


def wait_geom(win, w, h, timeout=3.0):
    end = time.time() + timeout
    while time.time() < end:
        d.sync()
        geometry = win.get_geometry()
        if (geometry.width, geometry.height) == (w, h):
            return True
        time.sleep(0.05)
    return False


def check(win, w, h, label):
    global fail
    win.configure(width=w, height=h)
    d.sync()
    if wait_geom(win, w, h):
        print("PASS: %s -> %dx%d applied" % (label, w, h))
    else:
        geometry = win.get_geometry()
        print("FAIL: %s -> requested %dx%d, window is %dx%d"
              % (label, w, h, geometry.width, geometry.height))
        fail = 1


def set_aspect(win, cw, ch):
    win.set_wm_normal_hints(flags=Xutil.PAspect,
                            min_aspect=(cw, ch), max_aspect=(cw, ch))
    d.sync()


win = root.create_window(100, 100, 400, 300, 0, screen.root_depth,
                         X.InputOutput, X.CopyFromParent,
                         background_pixel=screen.white_pixel,
                         event_mask=X.StructureNotifyMask)
win.set_wm_class("mpv", "mpv")  # matches the floating rule in config.h
win.set_wm_name("configure-request-test")
win.map()
d.sync()
time.sleep(1.0)

# plain resizes on a floating client, no size hints at all
check(win, 520, 380, "grow")
check(win, 320, 240, "shrink")

# scrcpy-style aspect lock: the ratio hint changes to the new orientation's
# ratio first, then the client resizes itself to the matching size
set_aspect(win, 300, 600)
check(win, 300, 600, "portrait")
set_aspect(win, 600, 300)
time.sleep(0.3)
geometry = win.get_geometry()
if (geometry.width, geometry.height) == (300, 600):
    print("PASS: aspect-only hint change leaves the geometry alone")
else:
    # the client is about to resize itself anyway, so this is not fatal; it is
    # only checked to keep the immediate clamp from coming back
    print("FAIL: aspect-only hint change resized the window to %dx%d"
          % (geometry.width, geometry.height))
    fail = 1
check(win, 600, 300, "rotated landscape")
set_aspect(win, 300, 600)
check(win, 300, 600, "rotated portrait")

sys.exit(fail)
PY

echo "configure-request-check: done"
