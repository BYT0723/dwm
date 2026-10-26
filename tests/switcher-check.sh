#!/bin/sh
# Static checks for the Alt+Tab overlay window switcher.
# Run: ./tests/switcher-check.sh (also runs as part of `make test`)
set -eu
cd "$(dirname "$0")/.."
fail=0
err() { printf 'switcher-check: FAIL: %s\n' "$*"; fail=1; }
info() { printf 'switcher-check: %s\n' "$*"; }

# 1. Alt+Tab is bound once, press only: the modal grab loop owns the release
if grep -qE '\{SUPKEY, *XK_Tab, *switcher, *\{0\}, *KeyPressMask\}' config.h; then
  info "Alt+Tab bound to switcher (press only)"
else
  err "config.h must bind {SUPKEY, XK_Tab, switcher, {0}, KeyPressMask}"
fi

# 2. the action carries the event mask like every other binding
if grep -q "void switcher(const Arg \*arg, unsigned int event)" dwm.c; then
  info "switcher carries the event mask"
else
  err "dwm.c missing void switcher(const Arg *arg, unsigned int event)"
fi

# 3. commit path must jump tags/monitors, revive hidden clients and focus
switcherblock="$(sed -n '/Alt+Tab switcher {{{/,/Alt+Tab switcher }}}/p' dwm.c)"
for sym in setcurrentmon showwin focus restack managedon; do
  if printf '%s' "$switcherblock" | grep -q "$sym"; then
    info "switcher references $sym"
  else
    err "switcher code must reference $sym (commit path)"
  fi
done

# 4. the overlay window follows the toolwin pattern, not the bar
if grep -q "switchwin" dwm.c && grep -q "switchdrw" dwm.c; then
  info "dedicated switchwin/switchdrw overlay"
else
  err "dwm.c must paint on a dedicated switchwin/switchdrw, not the bar"
fi

# 5. Super+Tab keeps its old view-toggle behaviour
if grep -qE '\{MODKEY, *XK_Tab, *view, *\{0\}, *KeyPressMask\}' config.h; then
  info "Super+Tab still toggles to the previous tags"
else
  err "config.h must keep {MODKEY, XK_Tab, view, {0}, KeyPressMask}"
fi

# 6. the new binding is documented, or check-man-keys goes red
if grep -q 'Mod1\\-Tab' dwm.1; then
  info "dwm.1 documents Mod1-Tab"
else
  err "dwm.1 must document Mod1-Tab"
fi

# 7. thumbnails are opt-in and default off
if grep -qE 'switcher_thumbnails *= *0' config.h; then
  info "switcher_thumbnails defaults to 0"
else
  err "config.h must define switcher_thumbnails defaulting to 0"
fi
if grep -q "hoverpreview(switchdrw" dwm.c; then
  info "thumbnail rows composite into the switcher overlay"
else
  err "switcherdraw must call hoverpreview(switchdrw, ...) for thumbnails"
fi

if [ "$fail" -ne 0 ]; then
  exit 1
fi
info "ok"
