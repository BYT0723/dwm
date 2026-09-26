#!/bin/sh
# Regression checks for pill-geometry radius edge cases.
#
# 1-2. pill_radius = 0 used to drop the battery icon background: the battery
# block (see print_battery in dwm-status-print.sh) emits only ^r/^c/^f
# graphics with no visible text, so its pane background comes solely from
# the pill body fill in drawstatusseg (drawpillcap paints [capx, pillw]
# before the first run). bar_pills always wraps pills in the internal
# PILL_OPEN .. PILL_CLOSE markers, but drawstatusseg used to ignore those
# markers when pill_radius == 0, so the body was never painted and the ^f
# spacing showed the bare bar background. The markers must be handled
# independent of the radius; only the rounded corner painting itself is
# gated on pillr.
#
# 3-5. a tiny radius must not eat lpad: first-run insets stay lpad
# (pillr <= lpad by construction) via drawleftcap, and the TabModeIcons
# shared pill keeps lpad edge pads plus its border at pillr 0.
#
# Run: ./tests/check-pill-radius.sh  (also runs as part of `make test`)
set -eu
cd "$(dirname "$0")/.."
fail=0
err() { printf 'check-pill-radius: FAIL: %s\n' "$*"; fail=1; }
info() { printf 'check-pill-radius: %s\n' "$*"; }

# 1. bar_pills must keep injecting the pill markers unconditionally:
#    they carry the body fill, not just the rounded shape.
if grep -q "pill_radius\|pillr" barparse.c; then
  err "barparse.c references the radius; pill markers must stay unconditional"
else
  info "pill markers stay unconditional in barparse.c"
fi

# 2. drawstatusseg must handle the PILL_OPEN / PILL_CLOSE markers
#    independent of the radius (gating them drops the body fill for
#    graphics-only pills at radius 0), and both sides must keep sharing
#    one spelling from barparse.h.
if grep -Eq "text\[i\] == PILL_(OPEN|CLOSE).*pill_radius" dwm.c; then
  err "dwm.c gates pill markers on pill_radius; graphics-only pills lose their body fill at radius 0"
else
  info "pill markers handled independent of pill_radius"
fi
if grep -q "text\[i\] == PILL_OPEN" dwm.c && grep -q "text\[i\] == PILL_CLOSE" dwm.c; then
  info "marker handling uses PILL_OPEN/PILL_CLOSE from barparse.h"
else
  err "marker handling must use PILL_OPEN/PILL_CLOSE, not '(' / ')' literals"
fi
if grep -q "PILL_OPEN\|PILL_CLOSE" barparse.c; then
  info "bar_pills emits PILL_OPEN/PILL_CLOSE from barparse.h"
else
  err "bar_pills must emit PILL_OPEN/PILL_CLOSE, not '(' / ')' literals"
fi

# 3. the square-pill fast path must still exist (no rounded caps at pillr 0),
#    and the first-run inset must stay tied to it.
if grep -q "square pill: no right cap" dwm.c; then
  info "square pills skip only the cap painting"
else
  err "square-pill cap guard missing in drawstatusseg"
fi
if grep -q "skip_pad = (pillr > 0)" dwm.c; then
  info "first-run inset tied to pillr"
else
  err "skip_pad is not tied to pillr in drawstatusseg"
fi

# 4. tags/layout left caps must not eat lpad at a tiny radius: the inset is
#    always lpad (pillr <= lpad by construction), painted by drawleftcap.
if grep -Eq "\? pillr : lpad|MAX\(pillr, lpad\)" dwm.c; then
  err "first-run inset collapses to pillr; inset is always lpad via drawleftcap"
else
  info "first-run insets stay lpad"
fi
if [ "$(grep -c "drawleftcap(x)" dwm.c)" -ge 2 ]; then
  info "drawleftcap paints the left cap in drawtags/drawlayout"
else
  err "drawtags/drawlayout must share drawleftcap instead of duplicating it"
fi

# 5. the TabModeIcons shared pill must keep its edge pads and its border
#    at pillr 0 (only the rounded caps may go away): pads are lpad like the
#    text pills, in both measurement (tablayout) and painting (tabdraw).
if grep -Eq "\(int\)pillr" dwm.c; then
  err "icons row still pads with raw pillr; use lpad so radius 0 keeps its pads"
else
  info "no raw-pillr pads left in the icons row"
fi

if [ "$fail" -ne 0 ]; then
  info "see drawstatusseg/drawpillcap in dwm.c"
  exit 1
fi
info "ok"
