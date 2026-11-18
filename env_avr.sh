#!/usr/bin/env bash
# Adds the Arduino IDE's avr-gcc to PATH. Use:  source ./env_avr.sh
AVR_GCC_BIN="$(ls -d "$HOME"/.arduino15/packages/arduino/tools/avr-gcc/*/bin 2>/dev/null | sort -V | tail -n1)"
if [ -z "$AVR_GCC_BIN" ] || [ ! -x "$AVR_GCC_BIN/avr-gcc" ]; then
    echo "avr-gcc not found under ~/.arduino15/packages/arduino/tools/avr-gcc" >&2
    return 1 2>/dev/null || exit 1
fi
case ":$PATH:" in
    *":$AVR_GCC_BIN:"*) ;;
    *) export PATH="$AVR_GCC_BIN:$PATH" ;;
esac
echo "avr-gcc on PATH: $(avr-gcc --version | head -n1)"
