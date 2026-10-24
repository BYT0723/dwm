#!/bin/sh
# Static checks for tags-pill left-segment showdesktop peek.
# Run: ./tests/check-tags-pill-peek.sh (also runs as part of `make test`)
set -eu
cd "$(dirname "$0")/.."
fail=0
err() { printf 'tags-pill-peek-check: FAIL: %s\n' "$*"; fail=1; }
info() { printf 'tags-pill-peek-check: %s\n' "$*"; }

# 1. a dedicated click type isolates the left segment from ClkTagBar dots
if grep -q "ClkTagLeft" dwm.c; then
  info "ClkTagLeft exists in dwm.c"
else
  err "dwm.c missing ClkTagLeft"
fi

# 2. the Compact left cell registers its own slot (single + combo branches)
if grep -q "addslot.*ClkTagLeft" dwm.c; then
  info "left segment registers a ClkTagLeft slot"
else
  err "drawtagscompact must addslot() the left segment with ClkTagLeft"
fi

# 3. self-contained peek handler (movemouse-style, no ButtonRelease dispatch)
if grep -q "void peekdesktop(const Arg \*arg, unsigned int event)" dwm.c; then
  info "peekdesktop handler exists"
else
  err "dwm.c missing peekdesktop(const Arg *arg, unsigned int event)"
fi

# 4. grab before press, loop to release, press/release paired (scoped to peekdesktop)
peekbody=$(awk '/void peekdesktop\(const Arg \*arg, unsigned int event\)/,/^}/' dwm.c)
if printf '%s' "$peekbody" | grep -q "XGrabPointer"; then
  info "peekdesktop grabs the pointer"
else
  err "peekdesktop must XGrabPointer before hiding"
fi
if printf '%s' "$peekbody" | grep -q "ButtonRelease"; then
  info "peekdesktop waits for ButtonRelease"
else
  err "peekdesktop must loop until ButtonRelease"
fi
if printf '%s' "$peekbody" | grep -q "showdesktop(NULL, KeyPressMask)" &&
   printf '%s' "$peekbody" | grep -q "showdesktop(NULL, KeyReleaseMask)"; then
  info "peekdesktop pairs showdesktop press/release"
else
  err "peekdesktop must call showdesktop press then release"
fi

# 5. exactly one new buttons[] row, left press only
if grep -Eq "ClkTagLeft, *0, *Button1, *peekdesktop" config.h; then
  info "buttons[] has the ClkTagLeft Button1 peekdesktop row"
else
  err "config.h must add {ClkTagLeft, 0, Button1, peekdesktop, {0}}"
fi
if grep -q "ClkTagLeft" config.h; then
  info "config.h mentions ClkTagLeft"
else
  err "config.h click comment must mention ClkTagLeft"
fi

if [ "$fail" -ne 0 ]; then
  info "see docs/specs/SPEC-tags-pill-showdesktop.md"
  exit 1
fi
info "ok"
