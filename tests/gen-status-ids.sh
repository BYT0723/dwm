#!/bin/sh
# Generate the C enum header and the shell assignments from status-ids.def.
# status-ids.def is the single source of truth; never hand-edit the outputs.
# Line format (after `#` comments are stripped): `<Name> <number>`.
# Outputs (override with OUT_H / OUT_SH, used by tests/check-status-ids.sh):
#   status-ids.h        `  St<Name> = <num>,` lines, included by barparse.h
#   contrib/status-ids.sh  `ST_<UPPER>=<num>` (click keys, match dwm $INDEX)
#                          `ST_<UPPER>_BYTE=$'\xNN'` (writer block prefixes)
set -eu
cd "$(dirname "$0")/.."
OUT_H=${OUT_H:-status-ids.h}
OUT_SH=${OUT_SH:-contrib/status-ids.sh}
DEF=${DEF:-status-ids.def}
clean=$(mktemp); trap 'rm -f "$clean"' EXIT
sed 's/#.*//' "$DEF" >"$clean"
{
  printf '%s\n' '/* GENERATED from status-ids.def -- do not hand-edit. */'
  while read -r name num rest; do
    case "$name" in '') continue ;; esac
    case "$name" in [0-9]*|*[!A-Za-z0-9_]*) echo "gen-status-ids: bad name: $name" >&2; exit 1 ;; esac
    case "$num" in ''|*[!0-9]*) echo "gen-status-ids: bad number: $name $num" >&2; exit 1 ;; esac
    [ -n "${rest:-}" ] && { echo "gen-status-ids: trailing text: $name $num $rest" >&2; exit 1; }
    printf '  St%s = %s,\n' "$name" "$num"
  done <"$clean"
} >"$OUT_H"
{
  printf '%s\n' '# GENERATED from status-ids.def -- do not hand-edit.'
  printf '%s\n' '# Sourced by ~/.dwm/dwm-status.sh and dwm-statuscmd.sh. Requires bash for $'\''\xNN'\''.'
  while read -r name num rest; do
    case "$name" in ''|Reserved*) continue ;; esac
    upper=$(printf '%s' "$name" | tr 'a-z' 'A-Z')
    hex=$(printf '%02x' "$num")
    printf 'ST_%s=%s\n' "$upper" "$num"
    printf '%s\n' "ST_${upper}_BYTE=\$'\\x${hex}'"
  done <"$clean"
} >"$OUT_SH"
