#!/usr/bin/env sh
# Convert/copy firmware artifacts and give them one deterministic product name.
set -eu

PROJECT_ROOT=""
PREFIX=""
MIDDLE=""
VERSION=""
SUFFIX=""
ELF_PATH=""
HEX_PATH=""
BIN_PATH=""
OBJCOPY_CMD="arm-none-eabi-objcopy"
OBJCOPY_ENABLED=1
BOOT_HEX_PATH=""
MERGE_HEX_PATH=""
MERGED_START_ADDR="first"
MERGE_ALLOW_OVERLAP=0
NAME_JOINER_PATH=""
SCRIPT_DIR="$(CDPATH= cd "$(dirname "$0")" && pwd)"

usage() {
    printf '%s\n' 'Usage: sh firmware_namer.sh [--project-root <dir>] [--prefix <value>] [--middle <value>] [--version <value>] [--suffix <value>] [--elf <path>] [--hex <path>] [--bin <path>] [--objcopy <cmd>] [--no-objcopy] [--boot-hex <path>] [--merge-hex <path>] [--merged-start-addr <first|second|none>] [--allow-merge-overlap]' >&2
    exit 1
}

resolve_path() {
    path="$1"
    [ -n "$path" ] || { printf '\n'; return; }
    if [ -f "$path" ]; then printf '%s\n' "$path"
    elif [ -n "$PROJECT_ROOT" ] && [ -f "$PROJECT_ROOT/$path" ]; then printf '%s\n' "$PROJECT_ROOT/$path"
    else printf '%s\n' "$path"
    fi
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --project-root) [ "$#" -ge 2 ] || usage; PROJECT_ROOT="$2"; shift 2 ;;
        --prefix) [ "$#" -ge 2 ] || usage; PREFIX="$2"; shift 2 ;;
        --middle) [ "$#" -ge 2 ] || usage; MIDDLE="$2"; shift 2 ;;
        --version) [ "$#" -ge 2 ] || usage; VERSION="$2"; shift 2 ;;
        --suffix) [ "$#" -ge 2 ] || usage; SUFFIX="$2"; shift 2 ;;
        --elf) [ "$#" -ge 2 ] || usage; ELF_PATH="$2"; shift 2 ;;
        --hex) [ "$#" -ge 2 ] || usage; HEX_PATH="$2"; shift 2 ;;
        --bin) [ "$#" -ge 2 ] || usage; BIN_PATH="$2"; shift 2 ;;
        --objcopy) [ "$#" -ge 2 ] || usage; OBJCOPY_CMD="$2"; shift 2 ;;
        --no-objcopy) OBJCOPY_ENABLED=0; shift ;;
        --boot-hex) [ "$#" -ge 2 ] || usage; BOOT_HEX_PATH="$2"; shift 2 ;;
        --merge-hex) [ "$#" -ge 2 ] || usage; MERGE_HEX_PATH="$2"; shift 2 ;;
        --merged-start-addr) [ "$#" -ge 2 ] || usage; MERGED_START_ADDR="$2"; shift 2 ;;
        --allow-merge-overlap) MERGE_ALLOW_OVERLAP=1; shift ;;
        -h|--help) usage ;;
        *) printf 'ERROR: unknown argument: %s\n' "$1" >&2; usage ;;
    esac
done

[ -n "$PROJECT_ROOT" ] || PROJECT_ROOT="$SCRIPT_DIR/../.."
[ -d "$PROJECT_ROOT" ] || { printf 'ERROR: invalid project root: %s\n' "$PROJECT_ROOT" >&2; exit 1; }
NAME_JOINER_PATH="$SCRIPT_DIR/name_joiner.sh"
MERGE_HEX_PATH="${MERGE_HEX_PATH:-$SCRIPT_DIR/merge_hex_fast.sh}"

case "$MERGED_START_ADDR" in first|second|none) ;; *) usage ;; esac

ELF_PATH="$(resolve_path "$ELF_PATH")"
HEX_PATH="$(resolve_path "$HEX_PATH")"
BIN_PATH="$(resolve_path "$BIN_PATH")"
BOOT_HEX_PATH="$(resolve_path "$BOOT_HEX_PATH")"

if [ -n "$ELF_PATH" ] && [ -f "$ELF_PATH" ]; then
    HEX_PATH="${ELF_PATH%.*}.hex"
    BIN_PATH="${ELF_PATH%.*}.bin"
    if [ "$OBJCOPY_ENABLED" -eq 1 ]; then
        "$OBJCOPY_CMD" -O ihex "$ELF_PATH" "$HEX_PATH" || exit 3
        "$OBJCOPY_CMD" -O binary "$ELF_PATH" "$BIN_PATH" || exit 3
    fi
fi

[ -f "$HEX_PATH" ] || HEX_PATH=""
[ -f "$BIN_PATH" ] || BIN_PATH=""
[ -n "$HEX_PATH$BIN_PATH" ] || {
    printf '%s\n' 'ERROR: provide an ELF or an existing HEX/BIN artifact' >&2
    exit 1
}

BASE_NAME="$(sh "$NAME_JOINER_PATH" --prefix "$PREFIX" --middle "$MIDDLE" --version "$VERSION" --suffix "$SUFFIX")" || exit 2

if [ -n "$HEX_PATH" ]; then
    OUT_HEX="$(dirname "$HEX_PATH")/$BASE_NAME.hex"
    if [ -n "$BOOT_HEX_PATH" ]; then
        [ -f "$BOOT_HEX_PATH" ] || { printf 'ERROR: boot HEX not found: %s\n' "$BOOT_HEX_PATH" >&2; exit 1; }
        set -- --boot "$BOOT_HEX_PATH" --app "$HEX_PATH" --output "$OUT_HEX" --start-addr "$MERGED_START_ADDR" --quiet
        [ "$MERGE_ALLOW_OVERLAP" -eq 0 ] || set -- "$@" --allow-overlap
        sh "$MERGE_HEX_PATH" "$@" || exit 4
        printf '[firmware_namer] merged hex: %s\n' "$OUT_HEX"
    else
        cp -f "$HEX_PATH" "$OUT_HEX" || exit 4
        printf '[firmware_namer] hex: %s\n' "$OUT_HEX"
    fi
fi

if [ -n "$BIN_PATH" ]; then
    OUT_BIN="$(dirname "$BIN_PATH")/$BASE_NAME.bin"
    cp -f "$BIN_PATH" "$OUT_BIN" || exit 4
    printf '[firmware_namer] bin: %s\n' "$OUT_BIN"
fi
