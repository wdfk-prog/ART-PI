#!/usr/bin/env sh
# Merge Bootloader + APP Intel HEX files without expanding them byte-by-byte.
# Records are validated for syntax, length and checksum; the first HEX
# start-address record is retained by default.
set -eu

BOOT_HEX=""
APP_HEX=""
OUTPUT_HEX=""
START_ADDR_POLICY="first"
ALLOW_OVERLAP=0
QUIET=0
TMP_OUTPUT=""
SCRIPT_NAME="$(basename "$0")"

usage() {
    printf '%s\n' 'Usage: sh merge_hex_fast.sh --boot <boot.hex> --app <app.hex> --output <merged.hex> [--start-addr <first|second|none>] [--allow-overlap] [--quiet]' >&2
    exit 1
}

log_info() {
    [ "$QUIET" -eq 1 ] || printf '[%s] %s\n' "$SCRIPT_NAME" "$*" >&2
}

cleanup() {
    if [ -n "$TMP_OUTPUT" ] && [ -f "$TMP_OUTPUT" ]; then
        rm -f "$TMP_OUTPUT"
    fi
}
trap cleanup 0 1 2 3 15

collect_hex_meta() {
    awk '
        function hex2dec(h, i, c, n) {
            n = 0
            h = toupper(h)
            for (i = 1; i <= length(h); ++i) {
                c = substr(h, i, 1)
                n *= 16
                if (c >= "0" && c <= "9") n += c + 0
                else n += index("ABCDEF", c) + 9
            }
            return n
        }
        BEGIN { upper=0; min_abs=-1; max_abs=-1; eof_count=0; eof_seen=0; start_count=0; data_count=0; first_start="" }
        {
            sub(/\r$/, "", $0)
            if ($0 == "") next
            if (eof_seen) exit 25
            ++line_count
            if ($0 !~ /^:[0-9A-Fa-f]+$/ || length($0) < 11) exit 21
            len=hex2dec(substr($0,2,2))
            if (length($0) != 11 + len * 2) exit 22

            # Intel HEX checksum covers every record byte after the colon.
            checksum=0
            for (byte_pos=2; byte_pos <= length($0); byte_pos += 2) {
                checksum += hex2dec(substr($0, byte_pos, 2))
            }
            if ((checksum % 256) != 0) exit 24

            addr=hex2dec(substr($0,4,4))
            type=toupper(substr($0,8,2))
            if (type == "00") {
                if (len > 0) {
                    abs_start=upper+addr; abs_end=abs_start+len-1
                    if (min_abs < 0 || abs_start < min_abs) min_abs=abs_start
                    if (max_abs < 0 || abs_end > max_abs) max_abs=abs_end
                    ++data_count
                }
            } else if (type == "01") {
                if (len != 0 || addr != 0) exit 26
                ++eof_count
                eof_seen=1
            } else if (type == "02") {
                if (len != 2 || addr != 0) exit 26
                upper=hex2dec(substr($0,10,4))*16
            } else if (type == "03") {
                if (len != 4 || addr != 0) exit 26
                ++start_count
                if (first_start == "") first_start=$0
            } else if (type == "04") {
                if (len != 2 || addr != 0) exit 26
                upper=hex2dec(substr($0,10,4))*65536
            } else if (type == "05") {
                if (len != 4 || addr != 0) exit 26
                ++start_count
                if (first_start == "") first_start=$0
            } else {
                exit 26
            }
        }
        END {
            if (line_count == 0 || eof_count != 1 || data_count == 0) exit 23
            printf("MIN_ABS=%d\nMAX_ABS=%d\nSTART_COUNT=%d\nFIRST_START=%s\nDATA_COUNT=%d\nLINE_COUNT=%d\n", min_abs, max_abs, start_count, first_start, data_count, line_count)
        }
    ' "$1"
}

meta_value() {
    printf '%s\n' "$1" | awk -F= -v key="$2" '$1 == key { sub(/^[^=]*=/, "", $0); print; exit }'
}

emit_hex_body() {
    awk -v keep_start="$2" '
        { sub(/\r$/, "", $0); if ($0 == "") next; type=substr($0,8,2); if (type == "01") next; if ((type == "03" || type == "05") && keep_start != "1") next; print }
    ' "$1"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --boot) [ "$#" -ge 2 ] || usage; BOOT_HEX="$2"; shift 2 ;;
        --app) [ "$#" -ge 2 ] || usage; APP_HEX="$2"; shift 2 ;;
        --output) [ "$#" -ge 2 ] || usage; OUTPUT_HEX="$2"; shift 2 ;;
        --start-addr) [ "$#" -ge 2 ] || usage; START_ADDR_POLICY="$2"; shift 2 ;;
        --allow-overlap) ALLOW_OVERLAP=1; shift ;;
        --quiet) QUIET=1; shift ;;
        -h|--help) usage ;;
        *) printf 'ERROR: unknown argument: %s\n' "$1" >&2; usage ;;
    esac
done

[ -f "$BOOT_HEX" ] || { printf 'ERROR: boot HEX not found: %s\n' "$BOOT_HEX" >&2; exit 2; }
[ -f "$APP_HEX" ] || { printf 'ERROR: app HEX not found: %s\n' "$APP_HEX" >&2; exit 2; }
[ -n "$OUTPUT_HEX" ] || usage
case "$START_ADDR_POLICY" in first|second|none) ;; *) usage ;; esac

if ! BOOT_META="$(collect_hex_meta "$BOOT_HEX")"; then
    printf 'ERROR: invalid boot Intel HEX: %s\n' "$BOOT_HEX" >&2
    exit 2
fi
if ! APP_META="$(collect_hex_meta "$APP_HEX")"; then
    printf 'ERROR: invalid app Intel HEX: %s\n' "$APP_HEX" >&2
    exit 2
fi

BOOT_MIN_ABS="$(meta_value "$BOOT_META" MIN_ABS)"
BOOT_MAX_ABS="$(meta_value "$BOOT_META" MAX_ABS)"
BOOT_START="$(meta_value "$BOOT_META" FIRST_START)"
APP_MIN_ABS="$(meta_value "$APP_META" MIN_ABS)"
APP_MAX_ABS="$(meta_value "$APP_META" MAX_ABS)"
APP_START="$(meta_value "$APP_META" FIRST_START)"

if [ "$ALLOW_OVERLAP" -ne 1 ] && [ "$BOOT_MAX_ABS" -ge "$APP_MIN_ABS" ] && [ "$APP_MAX_ABS" -ge "$BOOT_MIN_ABS" ]; then
    printf 'ERROR: data range overlap: boot=[0x%08X,0x%08X], app=[0x%08X,0x%08X]\n' \
        "$BOOT_MIN_ABS" "$BOOT_MAX_ABS" "$APP_MIN_ABS" "$APP_MAX_ABS" >&2
    exit 3
fi

case "$START_ADDR_POLICY" in
    first) SELECTED_START="$BOOT_START" ;;
    second) SELECTED_START="$APP_START" ;;
    none) SELECTED_START="" ;;
esac

OUTPUT_DIR="$(dirname "$OUTPUT_HEX")"
[ -d "$OUTPUT_DIR" ] || mkdir -p "$OUTPUT_DIR"
TMP_OUTPUT="${OUTPUT_HEX}.tmp.$$"
{
    emit_hex_body "$BOOT_HEX" 0
    # Each Intel HEX input starts with an implicit address base of zero. Reset
    # the base before the second image so an APP without an initial type-02/04
    # record cannot inherit the bootloader's final extended address.
    printf ':020000040000FA\n'
    emit_hex_body "$APP_HEX" 0
    [ -z "$SELECTED_START" ] || printf '%s\n' "$SELECTED_START"
    printf ':00000001FF\n'
} > "$TMP_OUTPUT" || exit 4
mv -f "$TMP_OUTPUT" "$OUTPUT_HEX" || exit 4
TMP_OUTPUT=""

log_info "boot range: 0x$(printf '%08X' "$BOOT_MIN_ABS") - 0x$(printf '%08X' "$BOOT_MAX_ABS")"
log_info "app  range: 0x$(printf '%08X' "$APP_MIN_ABS") - 0x$(printf '%08X' "$APP_MAX_ABS")"
log_info "merged hex: $OUTPUT_HEX"
