#!/bin/sh
# Drift check: every key binding in config.h keys[] must be documented in
# dwm.1. Renders the man page to plain text first, so roff escapes (\-)
# never get in the way of matching.
#
# Run: ./tests/check-man-keys.sh  (also runs as part of `make test`)
set -eu
cd "$(dirname "$0")/.."
fail=0
err() { printf 'check-man-keys: FAIL: %s\n' "$*"; fail=1; }
info() { printf 'check-man-keys: %s\n' "$*"; }

rendered="$(mktemp)"
rows="$(mktemp)"
trap 'rm -f "$rendered" "$rows"' EXIT INT TERM
if ! man -l ./dwm.1 2>/dev/null | col -b >"$rendered"; then
  groff -man -Tascii ./dwm.1 | col -b >"$rendered"
fi

# TAGKEYS macro: the four tag rows are documented as ranges, not per key.
for pat in 'Mod4-[1..n]' 'Mod4-Shift-[1..n]' 'Mod4-Control-[1..n]' \
           'Mod4-Control-Shift-[1..n]'; do
  if grep -qF "$pat" "$rendered"; then
    info "tag range $pat documented"
  else
    err "dwm.1 missing tag range $pat (TAGKEYS macro)"
  fi
done

# Gap manager rows share grouped man entries (one line covers u/i/o/6/7/8/9);
# check the groups exist and every gap key shows up in them.
gaplines="$(grep 'Mod4-Mod1-' "$rendered" || true)"
for key in u i o 6 7 8 9 0; do
  if printf '%s' "$gaplines" | grep -q "$key"; then
    info "gap key $key documented"
  else
    err "dwm.1 gap group missing key $key"
  fi
done
if printf '%s' "$gaplines" | grep -q 'Mod4-Mod1-Shift-'; then
  info "gap decrease group documented"
else
  err "dwm.1 missing Mod4-Mod1-Shift- gap group"
fi

# Explicit rows: MODKEY|ShiftMask + XK_d  ->  Mod4-Shift-d.
# Fields are ';'-separated (modifiers never contain ';', unlike '|').
awk '/^static Key keys\[\] = \{/,/^\};/ {
  line = $0
  sub(/^[ \t]+/, "", line)
  if (line ~ /^\/\//) next          # commented-out example rows
  if (line ~ /^TAGKEYS/) next       # covered by the range check above
  if (line !~ /XK_/) next
  if (line ~ /gaps/) next           # covered by the group check above
  split(line, parts, ",")
  mods = parts[1]; sub(/^[ \t]*\{/, "", mods)
  if (!match(line, /XK_[A-Za-z0-9_]+/)) next
  ksym = substr(line, RSTART + 3, RLENGTH - 3)
  print mods ";" ksym
}' config.h >"$rows"

# Only plain keyboard keys are documented; anything else (XF86 hardware
# keys, …) is intentionally skipped.
isdock() {
  case "$1" in
    [A-Za-z0-9]|Tab|Return|space|comma|period) return 0 ;;
    *) return 1 ;;
  esac
}

while IFS=';' read -r mods ksym; do
  [ -n "${mods:-}" ] || continue
  if ! isdock "$ksym"; then
    info "skipping non-keyboard key XK_$ksym"
    continue
  fi
  entry=""
  if [ "$mods" = "0" ]; then
    # no plain keyboard key is bound bare today; a bare letter would match
    # any rendered text, so fail loudly instead of false-passing
    err "bare key XK_$ksym needs explicit handling in $0"
    continue
  fi
  mods_ok=1
  oldifs="$IFS"; IFS='|'
  # shellcheck disable=SC2162
  for m in $mods; do
    case "$m" in
      *MODKEY*)      entry="${entry}Mod4-" ;;
      *SUPKEY*)      entry="${entry}Mod1-" ;;
      *ShiftMask*)   entry="${entry}Shift-" ;;
      *ControlMask*) entry="${entry}Control-" ;;
      *) err "unmapped modifier '$m' in row: $mods|XK_$ksym"; mods_ok=0; break ;;
    esac
  done
  IFS="$oldifs"
  [ "$mods_ok" = 1 ] || continue
  case "$ksym" in
    Tab|Return) key="$ksym" ;;
    space) key="space" ;;
    comma) key="," ;;
    period) key="." ;;
    *) key="$ksym" ;;  # single letters and digits map 1:1
  esac
  entry="${entry}${key}"
  if grep -qF "$entry" "$rendered"; then
    info "$entry documented"
  else
    err "dwm.1 missing entry for $entry (config.h row: $mods|XK_$ksym)"
  fi
done <"$rows"

if [ "$fail" -ne 0 ]; then
  info "add the missing binding(s) to the Keyboard commands section of dwm.1"
  exit 1
fi
info "ok"
