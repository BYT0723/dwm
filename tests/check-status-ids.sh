#!/bin/sh
# Fail loudly when status block ids drift.
#
# Single source of truth: status-ids.def. make derives both consumers from
# it in one step (tests/gen-status-ids.sh):
#   C:      status-ids.h (included by barparse.h; config.h pills use St*).
#   shell:  contrib/status-ids.sh (ST_* numbers for dwm-statuscmd.sh keys,
#           ST_*_BYTE control bytes for dwm-status.sh block args).
# This script checks, against the def:
#   1. both generated files are fresh (else: run make).
#   2. config.h pill arrays use St* names only, all known.
#   3. when $DWM_STATUS_DIR (default ~/.dwm) exists:
#      - its status-ids.sh is this repo's generated one (symlink it);
#      - dwm-status.sh has no hardcoded \xNN block ids and every ${ST_*}
#        resolves in the def (writer must use ST_*_BYTE);
#      - every [$ST_*, key in dwm-statuscmd.sh resolves in the def and is
#        actually emitted by the writer (no dead click actions).
#
# Run: ./tests/check-status-ids.sh  (also runs as part of `make test`)
# Override the writer location: DWM_STATUS_DIR=/path/to/scripts ./tests/check-status-ids.sh
set -eu
cd "$(dirname "$0")/.."
fail=0
err() { printf 'check-status-ids: FAIL: %s\n' "$*"; fail=1; }
info() { printf 'check-status-ids: %s\n' "$*"; }

DEF=status-ids.def
clean_def=$(mktemp); trap 'rm -f "$clean_def" ${tmp_h:-} ${tmp_sh:-}' EXIT
sed 's/#.*//' $DEF >"$clean_def"
def_names=$(awk 'NF==2 {print $1}' "$clean_def" | tr '\n' ' ')
def_real=$(awk 'NF==2 && $1 !~ /^Reserved/ {print toupper($1)"="$2}' "$clean_def" | tr '\n' ' ')
def_num() { # def_num UPPERNAME -> number or empty (case-insensitive on def side)
  awk -v n="$1" 'NF==2 && toupper($1)==n {print $2}' "$clean_def"
}
contains() { # contains LIST VALUE
  case " $1 " in *" $2 "*) return 0 ;; *) return 1 ;; esac
}

# 1. generated files must be fresh
tmp_h=$(mktemp); tmp_sh=$(mktemp)
OUT_H=$tmp_h OUT_SH=$tmp_sh ./tests/gen-status-ids.sh
if cmp -s status-ids.h "$tmp_h"; then
  info "C header matches $DEF"
else
  err "status-ids.h is stale; run: make status-ids.h"
fi
if cmp -s contrib/status-ids.sh "$tmp_sh"; then
  info "shell assignments match $DEF"
else
  err "contrib/status-ids.sh is stale; run: make contrib/status-ids.sh"
fi
grep -q '#include "status-ids.h"' barparse.h ||
  err "barparse.h does not include status-ids.h"

# 2. pill arrays: names only, and every name must be in the def
for arr in st_pills lt_pills st_pills_portrait lt_pills_portrait; do
  init=$(awk "/static const int $arr\\[\\]/ {f=1} f {printf \"%s\", \$0; if (/\\}/) exit}" config.h |
    sed 's/.*{//; s/}.*//')
  [ -n "$init" ] || { err "array $arr not found in config.h"; continue; }
  if printf '%s' "$init" | grep -Eq '(^|[^A-Za-z0-9_])-?[0-9]+'; then
    err "$arr has a raw number, use St* names: {$init }"
  fi
  # shellcheck disable=SC2086
  for tok in $(printf '%s' "$init" | tr ',' ' '); do
    tok=$(printf '%s' "$tok" | tr -d '[:space:]')
    [ -n "$tok" ] || continue
    short=${tok#St}
    case "$short" in PillBreak | PillEnd) continue ;; esac # list mechanics, not block ids
    if ! contains "$def_names" "$short"; then
      err "$arr references unknown id '$tok' (not in $DEF)"
    fi
  done
done

# 3. writer side, when present
wdir="${DWM_STATUS_DIR:-$HOME/.dwm}"
if [ ! -f "$wdir/dwm-status.sh" ]; then
  info "skip writer check (no $wdir/dwm-status.sh)"
else
  if [ ! -f "$wdir/status-ids.sh" ]; then
    err "$wdir/status-ids.sh missing; run: make install-ids"
  elif ! cmp -s contrib/status-ids.sh "$wdir/status-ids.sh"; then
    # install-ids copies only on change, so any diff here means ids changed
    err "$wdir/status-ids.sh differs from contrib/status-ids.sh; run: make install-ids"
  else
    info "writer sources the generated assignments"
  fi
  # no hardcoded block-id bytes: they must come from the generated file
  if grep -Eq '\\x(0|1)[0-9a-fA-F]' "$wdir/dwm-status.sh"; then
    err "dwm-status.sh hardcodes a \\xNN block id; use \${ST_*_BYTE} from status-ids.sh"
  fi
  emitted=""
  for v in $(grep -o 'ST_[A-Z0-9_]*' "$wdir/dwm-status.sh" | sort -u); do
    [ "$v" = "ST_" ] && continue # ${ST_*_BYTE} in a comment, not a variable
    case "$v" in
      ST_*_BYTE) base=${v%_BYTE}; base=${base#ST_} ;;
      *) err "dwm-status.sh uses \${$v}; writer blocks must use \${ST_*_BYTE}"; continue ;;
    esac
    n=$(def_num "$base")
    if [ -z "$n" ]; then
      err "dwm-status.sh uses \${$v}, unknown in $DEF"
    else
      case "$base" in
        RESERVED*) err "dwm-status.sh emits reserved id \${$v}" ;;
        *) emitted="$emitted $n" ;;
      esac
    fi
  done
  info "writer emits ids:$emitted"
  if [ -f "$wdir/dwm-statuscmd.sh" ]; then
    for key in $(grep -o '\[[^,]*,' "$wdir/dwm-statuscmd.sh" | tr -d '[,' | sort -u); do
      case "$key" in
        '$ST_'*) base=$(printf '%s' "$key" | sed 's/^\$ST_//') ;;
        [0-9]*) err "dwm-statuscmd.sh key [$key] is a raw number; use \$ST_*"; continue ;;
        *) continue ;; # dispatch code (e.g. $cmdIndex), not the table
      esac
      n=$(def_num "$base")
      if [ -z "$n" ]; then
        err "dwm-statuscmd.sh key [$key] unknown in $DEF"
      elif ! contains "$def_real" "$base=$n"; then
        err "dwm-statuscmd.sh key [$key] is not an emittable block"
      elif ! contains "$emitted" "$n"; then
        err "dwm-statuscmd.sh action [$key] (id $n) has no writer block in dwm-status.sh"
      fi
    done
  else
    info "skip click table (no $wdir/dwm-statuscmd.sh)"
  fi
fi

if [ "$fail" -ne 0 ]; then
  info "edit $DEF, run make, restart the status daemon, then re-run make test"
  exit 1
fi
info "ok"
