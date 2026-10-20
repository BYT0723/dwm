#!/bin/sh
# Static checks for Show Desktop hold-peek and the multi-event key dispatch.
# Run: ./tests/show-desktop-check.sh (also runs as part of `make test`)
set -eu
cd "$(dirname "$0")/.."
fail=0
err() { printf 'show-desktop-check: FAIL: %s\n' "$*"; fail=1; }
info() { printf 'show-desktop-check: %s\n' "$*"; }

# 1. root event mask must include KeyReleaseMask
if grep -q "KeyReleaseMask" dwm.c; then
  info "KeyReleaseMask selected"
else
  err "dwm.c missing KeyReleaseMask in root event mask"
fi

# 2. handler must dispatch KeyRelease
if grep -q "\[KeyRelease\] *= *keyrelease" dwm.c; then
  info "handler[KeyRelease] dispatches"
else
  err "dwm.c missing handler[KeyRelease] = keyrelease"
fi

# 3. one shared dispatch; the event itself is the phase parameter
if grep -q "void keyevent(XEvent \*e, unsigned int event)" dwm.c; then
  info "keyevent(e, event) is the single dispatch"
else
  err "dwm.c missing keyevent(XEvent *e, unsigned int event)"
fi
if grep -q "keyevent(e, KeyPressMask)" dwm.c &&
   grep -q "keyevent(e, KeyReleaseMask)" dwm.c; then
  info "keypress/keyrelease pass the event mask through"
else
  err "keypress/keyrelease must call keyevent with KeyPressMask/KeyReleaseMask"
fi

# 4. autorepeat debounce must live in the shared dispatch
if grep -q "XPeekEvent" dwm.c; then
  info "release path keeps the autorepeat debounce"
else
  err "dwm.c missing XPeekEvent debounce"
fi

# 5. bound actions take the event mask, so new events need no new type
if grep -q "void (\*func)(const Arg \*arg, unsigned int event)" dwm.c; then
  info "Key/Button func carry the event mask"
else
  err "binding func must be void (*)(const Arg *, unsigned int)"
fi
if grep -q "keys\[i\].func(&(keys\[i\].arg), event)" dwm.c; then
  info "keyevent hands the event mask to the action"
else
  err "keyevent must pass the event mask to keys[i].func"
fi

# 6. one Key table, and ev is the full event set: no implicit default
if grep -q "^static Key keys\[\]" config.h; then
  info "config.h keeps a single keys[] table"
else
  err "config.h must keep a single keys[] table"
fi
if grep -q "keysRelease" config.h dwm.c; then
  err "keysRelease still referenced; the event mask belongs in keys[]"
else
  info "no keysRelease[] left"
fi
if grep -q "unsigned int ev;" dwm.c; then
  info "Key rows carry an event mask"
else
  err "Key struct missing the ev mask"
fi
if grep -q "!(keys\[i\].ev & event)" dwm.c; then
  info "dispatch gates on the row's event mask"
else
  err "keyevent must gate dispatch on keys[i].ev"
fi
rows=$(awk '/^static Key keys\[\] = \{/,/^\};/' config.h | grep -cE '^[[:space:]]*\{')
masks=$(awk '/^static Key keys\[\] = \{/,/^\};/' config.h | grep -cE 'KeyPressMask')
tagrows=$(grep -c 'KeyPressMask},  *\\$' config.h || true)
if [ "$rows" -eq "$masks" ] && [ "$tagrows" -eq 3 ]; then
  info "every keys[] row declares its event mask ($rows rows)"
else
  err "keys[] rows without an explicit event mask ($masks/$rows rows, tagkeys $tagrows/3)"
fi

# 7. no per-phase wrappers: one action, one explicit two-event row
if grep -q "showdesktop_press\|showdesktop_release" config.h dwm.c; then
  err "per-phase wrappers are back; bind showdesktop once"
else
  info "no per-phase wrappers"
fi
if grep -Eq "showdesktop, *\{0\}, *KeyPressMask \| KeyReleaseMask" config.h; then
  info "showdesktop declares both events"
else
  err "config.h must bind showdesktop with KeyPressMask | KeyReleaseMask"
fi
if grep -q "void showdesktop(const Arg \*arg, unsigned int event)" dwm.c &&
   grep -q "if (event == KeyPressMask)" dwm.c; then
  info "showdesktop branches on the event mask"
else
  err "showdesktop must branch on the event mask"
fi

# 8. hold must mark only the clients it hid, release must restore only those
if grep -q "desktop_hidden = 1" dwm.c && grep -q "if (!c->desktop_hidden)" dwm.c; then
  info "hold/release use the desktop_hidden marker"
else
  err "hold/release must gate on desktop_hidden, not on HIDDEN alone"
fi

# 9. hold path must still hide via hidewin
if grep -q "hidewin(c)" dwm.c; then
  info "hold path hides via hidewin"
else
  err "hold path must use hidewin(c)"
fi

# 10. release order must not matter: a hold ends when any key of its chord
# comes up, and the release is dispatched to the row recorded at press time
# instead of being re-derived from kev->state. Generic to every binding that
# listens for KeyRelease, so no action needs release-order handling of its own.
if grep -q "holdrow" dwm.c && grep -q "IsModifierKey" dwm.c; then
  info "an active hold releases by its recorded row on any chord-key release"
else
  err "keyevent must record the pressed row and release by it"
fi
if grep -q "desktoprelease\|pressmods" dwm.c; then
  err "dispatch must not special-case an action or keep a press-mask table"
else
  info "no action-specific or keycode-table release handling"
fi

if [ "$fail" -ne 0 ]; then
  info "see docs/specs/SPEC-show-desktop-hold.md"
  exit 1
fi
info "ok"
