#!/usr/bin/env sh
# Join optional firmware naming fields in a stable order:
#   <prefix>_<middle>_<version>_<suffix>
set -eu

PREFIX=""
MIDDLE=""
VERSION=""
SUFFIX=""

usage() {
    printf '%s\n' 'Usage: sh name_joiner.sh [--prefix <value>] [--middle <value>] [--version <value>] [--suffix <value>]' >&2
    exit 1
}

append_part() {
    current="$1"
    part="$2"
    if [ -z "$part" ]; then
        printf '%s\n' "$current"
    elif [ -z "$current" ]; then
        printf '%s\n' "$part"
    else
        printf '%s_%s\n' "$current" "$part"
    fi
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix) [ "$#" -ge 2 ] || usage; PREFIX="$2"; shift 2 ;;
        --middle) [ "$#" -ge 2 ] || usage; MIDDLE="$2"; shift 2 ;;
        --version) [ "$#" -ge 2 ] || usage; VERSION="$2"; shift 2 ;;
        --suffix) [ "$#" -ge 2 ] || usage; SUFFIX="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) printf 'ERROR: unknown argument: %s\n' "$1" >&2; usage ;;
    esac
done

RESULT=""
RESULT="$(append_part "$RESULT" "$PREFIX")"
RESULT="$(append_part "$RESULT" "$MIDDLE")"
RESULT="$(append_part "$RESULT" "$VERSION")"
RESULT="$(append_part "$RESULT" "$SUFFIX")"

[ -n "$RESULT" ] || {
    printf '%s\n' 'ERROR: at least one naming part must be provided' >&2
    exit 1
}

printf '%s\n' "$RESULT"
