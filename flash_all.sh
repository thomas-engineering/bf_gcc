#!/usr/bin/env bash
# Build the application and the bootloader, join both into one Intel-hex file
# (tmp/butterfly_full.hex) and flash it to the AVR Butterfly with the STK500.
#
#   ./flash_all.sh              build, join, flash, set lock bits, verify
#   ./flash_all.sh --no-flash   build and join only (no hardware needed)
#
# Environment: PORT (default /dev/ttyUSB0), VTARG (default 3.3)
#
# The flash write erases the whole chip, bootloader included, so the joined file
# carries both. The lock bits are set to 0xEF (boot loader protection mode 2),
# as documented in SESSION_START.MD. Fuses are never touched.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${PORT:-/dev/ttyUSB0}"
VTARG="${VTARG:-3.3}"
LOCK_BITS=0xEF
APP_END=$((0x3800))         # the bootloader section starts here
OUT="$ROOT/tmp/butterfly_full.hex"

FLASH=1
case "${1:-}" in
    "")         ;;
    --no-flash) FLASH=0 ;;
    *)          echo "usage: $0 [--no-flash]" >&2; exit 2 ;;
esac

# toolchain
# shellcheck source=env_avr.sh
source "$ROOT/env_avr.sh" > /dev/null

# 1. build both projects
echo "== build application"
make -C "$ROOT/main" clean all > /dev/null
echo "== build bootloader"
make -C "$ROOT/bf_boot" clean all > /dev/null
APP_HEX="$ROOT/main/build/main.hex"
BOOT_HEX="$ROOT/bf_boot/build/main.hex"

# 2. join: application data records, then bootloader data records, one EOF.
#    Start-address (03) and EOF (01) records of the single files are dropped.
#    Intel hex: ':' LL AAAA TT data CC  (record type at characters 8-9)
echo "== join -> $OUT"
mkdir -p "$ROOT/tmp"
{
    for f in "$APP_HEX" "$BOOT_HEX"; do
        tr -d '\r' < "$f" | awk '{ t = substr($0, 8, 2); if (t == "00" || t == "02" || t == "04") print }'
    done
    echo ":00000001FF"
} > "$OUT"

# the application must end below the bootloader section
APP_TOP=$(avr-size "$ROOT/main/build/main.elf" | awk 'NR == 2 { print $1 + $2 }')   # text + data
if [ "$APP_TOP" -gt "$APP_END" ]; then
    echo "error: application ends at $(printf '0x%X' "$APP_TOP"), beyond the bootloader start $(printf '0x%X' "$APP_END")" >&2
    exit 1
fi
echo "   application: $APP_TOP bytes (limit $APP_END), bootloader: $(avr-size -A "$ROOT/bf_boot/build/main.elf" | awk '/^\.text/ {print $2}') bytes"

if [ "$FLASH" -eq 0 ]; then
    echo "== done (not flashed)"
    exit 0
fi

# 3. flash with the STK500 (ISP)
AVRDUDE_DIR="$(ls -d "$HOME"/.arduino15/packages/arduino/tools/avrdude/* 2>/dev/null | sort -V | tail -n1)"
if [ -z "$AVRDUDE_DIR" ] || [ ! -x "$AVRDUDE_DIR/bin/avrdude" ]; then
    echo "avrdude not found under ~/.arduino15/packages/arduino/tools/avrdude" >&2
    exit 1
fi
DUDE=("$AVRDUDE_DIR/bin/avrdude" -C "$AVRDUDE_DIR/etc/avrdude.conf" -c stk500v2 -P "$PORT" -p m169 -B 125kHz)

echo "== STK500: set VTarget $VTARG V"
"${DUDE[@]}" -x "vtarg=$VTARG" 2>&1 | grep -iE "voltage|error|timeout" || true

echo "== write flash (chip erase + application + bootloader)"
"${DUDE[@]}" -U "flash:w:$OUT:i"

echo "== set lock bits $LOCK_BITS"
"${DUDE[@]}" -U "lock:w:$LOCK_BITS:m"

echo "== verify flash and lock bits"
"${DUDE[@]}" -U "flash:v:$OUT:i" -U "lock:v:$LOCK_BITS:m"

echo "== done: application and bootloader flashed and verified"
