#!/usr/bin/env sh
# Extract a simple #define value from a specific header file.
set -eu

HEADER_FILE=""
MACRO_NAME="VERSION"

usage() {
    printf '%s\n' 'Usage: sh get_version_from_header.sh --header-file <path> [--macro <name>]' >&2
    exit 1
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --header-file) [ "$#" -ge 2 ] || usage; HEADER_FILE="$2"; shift 2 ;;
        --macro) [ "$#" -ge 2 ] || usage; MACRO_NAME="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) printf 'ERROR: unknown argument: %s\n' "$1" >&2; usage ;;
    esac
done

[ -n "$HEADER_FILE" ] || usage
[ -f "$HEADER_FILE" ] || {
    printf 'ERROR: header file not found: %s\n' "$HEADER_FILE" >&2
    exit 2
}

VALUE="$(awk -v key="$MACRO_NAME" '
    $0 ~ "^[[:space:]]*#[[:space:]]*define[[:space:]]+" key "([[:space:]]|$)" {
        line=$0
        sub("^[[:space:]]*#[[:space:]]*define[[:space:]]+" key "[[:space:]]+", "", line)
        sub("[[:space:]]*(//.*)?$", "", line)
        gsub("^[[:space:]]+|[[:space:]]+$", "", line)
        if (line ~ /^\".*\"$/) {
            sub(/^\"/, "", line)
            sub(/\"$/, "", line)
        }
        print line
        exit
    }
' "$HEADER_FILE")"

[ -n "$VALUE" ] || {
    printf 'ERROR: macro %s not found or empty in %s\n' "$MACRO_NAME" "$HEADER_FILE" >&2
    exit 3
}

printf '%s\n' "$VALUE"
