#!/bin/sh
# Regression: the bar's click table must cover every drawn element.
#
# BarSlot slots[BAR_SLOTS] is filled left zone -> center tabs -> right zone.
# With a 32-slot table a busy tab row exhausted the budget before the right
# zone, so the right-hand status blocks (weather/date/proxy/mpd/battery) were
# drawn but no longer clickable. The test opens enough visible clients to
# overflow 32 and then requires the last right-zone block to answer a click.
#
# Run: tests/bar-slot-budget-check.sh [display]
# Needs Xvfb, xterm and xdotool; skips loudly when they are missing.
set -eu

DISP=${1:-:74}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=${BIN:-$ROOT/dwm}

command -v Xvfb >/dev/null || { echo "SKIP: Xvfb not installed"; exit 0; }
command -v xterm >/dev/null || { echo "SKIP: xterm not installed"; exit 0; }
command -v xdotool >/dev/null || { echo "SKIP: xdotool not installed"; exit 0; }
[ -x "$BIN" ] || { echo "FAIL: $BIN missing; build dwm first"; exit 1; }

WORK=$(mktemp -d /tmp/opencode/dwm-slot-budget.XXXXXX)
export HOME="$WORK/home"
export XDG_DATA_HOME="$WORK/xdg"
mkdir -p "$HOME/.dwm" "$XDG_DATA_HOME"
export DISPLAY="$DISP"
unset XDG_RUNTIME_DIR || true

XPIDS=
cleanup() {
  [ -n "${DWMPID:-}" ] && kill "$DWMPID" 2>/dev/null || true
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
  for p in $XPIDS; do
    kill "$p" 2>/dev/null || true
  done
}
trap cleanup EXIT INT TERM

printf '#!/bin/sh\necho "$1" >>%s/click.log\n' "$WORK" >"$HOME/.dwm/dwm-statuscmd.sh"
chmod +x "$HOME/.dwm/dwm-statuscmd.sh"

Xvfb "$DISP" -screen 0 1280x1024x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1
"$BIN" >"$WORK/dwm.log" 2>&1 &
DWMPID=$!
sleep 1.2
kill -0 "$DWMPID" 2>/dev/null || {
  echo "FAIL: dwm did not start"; cat "$WORK/dwm.log"; exit 1
}

# one visible block per id the zones draw; text only, the geometry is what
# matters here (the last id in bar_right is the battery)
printf '\x08cpu \x07mem \x06disk \x0bnet \x12vps \x09weather \x01date \x0eproxy \x0ampd \x02battery' >"$WORK/status"
cat >"$WORK/setname.c" <<'EOF'
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
int main(int argc, char **argv) {
  FILE *f = fopen(argv[2], "rb");
  static unsigned char buf[4096];
  size_t n;
  Display *d = XOpenDisplay(argv[1]);
  if (!f || !d) return 1;
  n = fread(buf, 1, sizeof buf, f);
  XChangeProperty(d, DefaultRootWindow(d), XA_WM_NAME, XA_STRING, 8,
                  PropModeReplace, buf, (int)n);
  XSync(d, False);
  XCloseDisplay(d);
  return 0;
}
EOF
cc -O1 -o "$WORK/setname" "$WORK/setname.c" -lX11 || {
  echo "SKIP: cc cannot build the status setter"; exit 0
}
"$WORK/setname" "$DISP" "$WORK/status"
sleep 0.4

# enough visible clients to exhaust a 32-slot table (15 fixed + N tabs + 7
# right status cells); xterm sends a real MapRequest, like any app
i=0
while [ "$i" -lt 24 ]; do
  xterm -e sleep 641 >/dev/null 2>&1 &
  XPIDS="$XPIDS $!"
  i=$((i + 1))
done
sleep 2

: >"$WORK/click.log"
x=560
while [ "$x" -lt 1280 ]; do
  xdotool mousemove "$x" 5 click 1 >/dev/null 2>&1 || true
  x=$((x + 4))
done
sleep 0.5

got=$(sort -n -u "$WORK/click.log" 2>/dev/null | tr '\n' ' ')
echo "reachable status ids: ${got:-none}"
if echo " $got " | grep -q ' 2 '; then
  echo "PASS: right-zone battery block stays clickable with 24 tabs"
else
  echo "FAIL: right-zone clicks were dropped (BAR_SLOTS too small)"
  exit 1
fi
