#!/bin/sh
# Static checks for the window switcher overlay (Super+Tab).
# Run: ./tests/switcher-check.sh (also runs as part of `make test`)
set -eu
cd "$(dirname "$0")/.."
fail=0
err() { printf 'switcher-check: FAIL: %s\n' "$*"; fail=1; }
info() { printf 'switcher-check: %s\n' "$*"; }

# 1. switcher is bound once, press only: the modal grab loop owns the release.
# (Keys are intentionally NOT named here; check 9 pins the row to the knobs.)
n="$(grep -cE '\{[^}]*switcher, *\{0\}, *KeyPressMask\}' config.h || true)"
if [ "$n" -eq 1 ]; then
  info "switcher bound once (press only)"
else
  err "config.h must bind switcher exactly once with {0}, KeyPressMask (found $n)"
fi

# 2. the action carries the event mask like every other binding
if grep -q "void switcher(const Arg \*arg, unsigned int event)" dwm.c; then
  info "switcher carries the event mask"
else
  err "dwm.c missing void switcher(const Arg *arg, unsigned int event)"
fi

# 3. commit path must jump tags/monitors, revive hidden clients and focus
switcherblock="$(sed -n '/Window switcher {{{/,/Window switcher }}}/p' dwm.c)"
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

# 5. Super+grave keeps its old view-toggle behaviour
if grep -qE '\{MODKEY, *XK_grave, *view, *\{0\}, *KeyPressMask\}' config.h; then
  info "Super+grave still toggles to the previous tags"
else
  err "config.h must keep {MODKEY, XK_grave, view, {0}, KeyPressMask}"
fi

# 6. the new binding is documented (Mod4-Tab entry mentions Tab)
if grep -A3 'Mod4\\-Tab' dwm.1 | grep -q 'press Tab to cycle'; then
  info "dwm.1 documents Mod4-Tab"
else
  err "dwm.1 must document Mod4-Tab"
fi

# 7. strip keeps the thumbnail aspect ratio (letterbox, never stretch)
if grep -q "keep the aspect ratio" dwm.c; then
  info "strip letterboxes thumbnails"
else
  err "switcherdrawstrip must letterbox thumbnails to keep the aspect ratio"
fi
if grep -q "hoverpreview(switchdrw" dwm.c; then
  info "thumbnail rows composite into the switcher overlay"
else
  err "switcherdraw must call hoverpreview(switchdrw, ...) for thumbnails"
fi
stripblock="$(sed -n '/^static void switcherdrawstrip(/,/^}/p' dwm.c)"
if printf '%s' "$stripblock" | grep -q 'tabicon_get'; then
  info "strip title row carries the window icon"
else
  err "switcherdrawstrip must draw the window icon in the title row (tabicon_get)"
fi
if printf '%s' "$stripblock" | grep -q 'ICONSPACING'; then
  info "strip title row spaces the icon from the title"
else
  err "switcherdrawstrip must space the title-row icon with ICONSPACING"
fi
if printf '%s' "$stripblock" | awk '/hoverellipsize\(c->name/{seen=1} /drw_pic/{if (seen) ok=1} END{exit !ok}'; then
  info "strip icon position follows the measured title width"
else
  err "switcherdrawstrip must centre the icon+title group (drw_pic after measuring the title)"
fi
if printf '%s' "$stripblock" | grep -q -- '->class'; then
  err "switcherdrawstrip must not show a class header (title-row icon only)"
else
  info "strip cards show no class header"
fi

# 8. both list and strip layouts exist, one config line switches
if grep -q "switcher_layout" config.h && grep -q "SwitcherStrip" config.h; then
  info "switcher_layout selects list vs strip"
else
  err "config.h must define switcher_layout with SwitcherList/SwitcherStrip"
fi
if grep -q "switcherdrawstrip" dwm.c; then
  info "strip layout draws thumbnail cards"
else
  err "dwm.c must implement switcherdrawstrip"
fi

# 9. single source of truth: the modal loop reads its key/mods from the
# keys[] row, so there is nothing to drift — fail if any side knobs exist
if grep -q '\.func == switcher' dwm.c; then
  info "switcher looks up its keys[] row"
else
  err "dwm.c must derive the switcher key/mods from keys[] (no hardcoded keys)"
fi
if grep -q 'switcher_hold\|switcher_cycle' config.h dwm.c; then
  err "stale hold/cycle knobs must go: keys[] is the single source of truth"
else
  info "no hold/cycle knobs outside keys[]"
fi

# 10. scaled previews must sample a staged 1:1 pixmap, never the window
# picture: with an XRender transform set, the server reads a window picture
# from an offset region and cuts the window's top off, which showed up as
# cropped, stretched thumbnails (Firefox with the system titlebar)
if grep -q 'static Picture stageclient(Client \*c, int \*sw, int \*sh)' dwm.c; then
  info "previews stage the client into a 1:1 pixmap"
else
  err "dwm.c must implement stageclient() for scaled previews"
fi
n="$(grep -c 'XRenderCreatePicture(dpy, c->win' dwm.c || true)"
if [ "$n" -eq 1 ]; then
  info "window picture is created only in stageclient"
else
  err "window picture must only be created in stageclient (found $n sites)"
fi
for sym in hoverpreview takesnapshot; do
  block="$(sed -n "/^static void $sym(/,/^}/p" dwm.c)"
  if printf '%s' "$block" | grep -q 'stageclient(c,'; then
    info "$sym scales the staged pixmap"
  else
    err "$sym must call stageclient(c, ...) before scaling"
  fi
  if printf '%s' "$block" | grep -q 'XRenderCreatePicture(dpy, c->win'; then
    err "$sym must not sample the window picture directly"
  fi
done

if [ "$fail" -ne 0 ]; then
  exit 1
fi
info "ok"
