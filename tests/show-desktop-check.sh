#!/bin/sh
# Static checks for Show Desktop's self-contained hold and the press-only
# key dispatch.
# Run: ./tests/show-desktop-check.sh (also runs as part of `make test`)
set -eu
cd "$(dirname "$0")/.."
fail=0
err() { printf 'show-desktop-check: FAIL: %s\n' "$*"; fail=1; }
info() { printf 'show-desktop-check: %s\n' "$*"; }

# 1. keys[] is press-only: no per-row event column anywhere
if grep -qE '^ *unsigned int ev;' dwm.c; then
  err "Key struct still carries an event mask column"
else
  info "keys[] rows carry no event mask"
fi
if grep -q "KeyPressMask" config.h; then
  err "config.h still declares per-row event masks"
else
  info "config.h binds keys without event masks"
fi
if sed -n '/wa.event_mask = SubstructureRedirectMask/,/;/p' dwm.c |
   grep -q "KeyReleaseMask"; then
  err "root event mask still selects KeyReleaseMask"
else
  info "root event mask is press-only"
fi

# 2. one press dispatch, no release handler in the framework
if grep -q "\[KeyPress\] *= *keyevent" dwm.c; then
  info "handler[KeyPress] dispatches keyevent"
else
  err "dwm.c missing handler[KeyPress] = keyevent"
fi
if grep -q "void keyevent(XEvent \*e)" dwm.c &&
   ! grep -q "void keyevent(XEvent \*e, unsigned int" dwm.c; then
  info "keyevent(e) is press-only"
else
  err "keyevent must take only the event"
fi
if grep -q "\[KeyRelease\] *=" dwm.c || grep -q "void keyrelease" dwm.c; then
  err "release dispatch is back; each hold owns its release"
else
  info "no release dispatch left"
fi
if grep -q "holdrow\|holdcode" dwm.c; then
  err "generic hold bookkeeping is back"
else
  info "no generic hold bookkeeping"
fi
if grep -q "keys\[i\].func(&(keys\[i\].arg), KeyPressMask)" dwm.c; then
  info "keyevent hands KeyPressMask to the action"
else
  err "keyevent must pass KeyPressMask to keys[i].func"
fi

# 3. showdesktop owns its hold: grab, hide, wait, restore
body=$(awk '/^void showdesktop\(const Arg \*arg, unsigned int event\)/,/^}/' dwm.c)
if [ -z "$body" ]; then
  err "dwm.c missing showdesktop(const Arg *arg, unsigned int event)"
fi
for sym in XGrabKeyboard XUngrabKeyboard XMaskEvent desktophide desktoprestore; do
  if printf '%s' "$body" | grep -q "$sym"; then
    info "showdesktop uses $sym"
  else
    err "showdesktop must use $sym"
  fi
done

# 4. the hold ends on the bound key or once the chord modifiers are gone
if printf '%s' "$body" | grep -q "ev.xkey.keycode == code" &&
   printf '%s' "$body" | grep -q "IsModifierKey" &&
   printf '%s' "$body" | grep -q "XQueryPointer"; then
  info "release order does not end the hold early"
else
  err "showdesktop must end on the bound key or its chord modifiers"
fi
if grep -q "keys\[i\].func == showdesktop" dwm.c; then
  info "the showdesktop row in keys[] is the source of truth"
else
  err "dwm.c must derive the hold chord from keys[]"
fi

# 5. auto-repeat must not end the hold
if printf '%s' "$body" | grep -q "keyrepeat(&ev.xkey)"; then
  info "autorepeat pairs are not a release"
else
  err "showdesktop must debounce autorepeat"
fi
guard=$(awk '/^static int keyrepeat/,/^}/' dwm.c)
if [ -n "$guard" ] && printf '%s' "$guard" | grep -q "XQueryKeymap" &&
   ! printf '%s' "$guard" | grep -q "XPeekEvent"; then
  info "autorepeat is read from the key state, not the event queue"
else
  err "keyrepeat must ask XQueryKeymap (XPeekEvent races the synthetic press)"
fi

# 6. binding: one row, press only
n=$(grep -cE '\{[^}]*showdesktop, *\{0\}, *Key(Press|Release)Mask' config.h || true)
if [ "$n" -eq 0 ] && grep -qE "showdesktop, *\{0\}\}" config.h; then
  info "showdesktop is bound once without an event mask"
else
  err "config.h must bind showdesktop once as {MODKEY|ShiftMask, XK_d, showdesktop, {0}}"
fi

# 7. hold must mark only the clients it hid, release must restore only those
if grep -q "desktop_hidden = 1" dwm.c && grep -q "if (!c->desktop_hidden)" dwm.c; then
  info "hold/release use the desktop_hidden marker"
else
  err "hold/release must gate on desktop_hidden, not on HIDDEN alone"
fi
if grep -q "hidewin(c)" dwm.c; then
  info "hold path hides via hidewin"
else
  err "hold path must use hidewin(c)"
fi

# 8. the mouse peek shares the same hide/restore halves
peek=$(awk '/^void peekdesktop/,/^}/' dwm.c)
if printf '%s' "$peek" | grep -q "desktophide()" &&
   printf '%s' "$peek" | grep -q "desktoprestore()"; then
  info "peekdesktop shares desktophide/desktoprestore"
else
  err "peekdesktop must reuse desktophide/desktoprestore"
fi

# 9. the events that cannot wait are handled by one shared forwarder
if grep -q "static void desktopevent" dwm.c &&
   printf '%s' "$body" | grep -q "desktopevent(&ev)" &&
   printf '%s' "$peek" | grep -q "desktopevent(&ev)"; then
  info "both holds share desktopevent"
else
  err "showdesktop/peekdesktop must share desktopevent"
fi

if [ "$fail" -ne 0 ]; then
  info "see docs/specs/SPEC-show-desktop-hold.md"
  exit 1
fi
info "ok"
