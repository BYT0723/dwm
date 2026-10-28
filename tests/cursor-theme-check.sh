#!/bin/sh
# Contract: every cursor name dwm loads via drw_cur_create_named() must
# resolve in the installed themes (Bibata/Adwaita). Without the themed
# loader, grabs fall back to unthemed core glyphs.
set -eu
DISPLAY_NUM=${1:-:92}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d /tmp/opencode/cursor-theme.XXXXXX)
cleanup() {
  [ -n "${XPID:-}" ] && kill "$XPID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM
export HOME="$WORK/home"
mkdir -p "$HOME"
export DISPLAY="$DISPLAY_NUM"
unset XDG_RUNTIME_DIR || true
Xvfb "$DISPLAY_NUM" -screen 0 1280x800x24 >"$WORK/xvfb.log" 2>&1 &
XPID=$!
sleep 1
cat >"$WORK/probe.c" <<'EOF'
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>
#include <stdio.h>
int main(int argc, char *argv[]) {
  Display *dpy;
  int i, fail = 0;
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    printf("FAIL: cannot open display\n");
    return 2;
  }
  for (i = 1; i < argc; i++) {
    Cursor c = XcursorLibraryLoadCursor(dpy, argv[i]);
    printf("%s %s\n", c ? "OK" : "FAIL", argv[i]);
    if (!c)
      fail = 1;
  }
  XCloseDisplay(dpy);
  return fail;
}
EOF
cc -o "$WORK/probe" "$WORK/probe.c" -lX11 -lXcursor
# names must mirror the lookup chains in setup() (dwm.c)
NAMES="left_ptr move fleur nw-resize top_left_corner n-resize top_side ne-resize top_right_corner e-resize right_side se-resize bottom_right_corner s-resize bottom_side sw-resize bottom_left_corner w-resize left_side"
checked=0
for theme in Bibata-Modern-Ice Adwaita; do
  [ -d "/usr/share/icons/$theme/cursors" ] || [ -d "$HOME/.icons/$theme/cursors" ] || continue
  echo "theme: $theme"
  # shellcheck disable=SC2086
  if XCURSOR_THEME="$theme" "$WORK/probe" $NAMES; then
    echo "PASS: all dwm cursor names resolve under $theme"
  else
    echo "FAIL: missing cursor name under $theme (see above)"; exit 1
  fi
  checked=$((checked + 1))
done
if [ "$checked" = 0 ]; then
  echo "SKIP: no known cursor theme installed"
  exit 0
fi
echo "PASS: cursor theme contract holds"
