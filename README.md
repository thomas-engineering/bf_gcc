# bf_gcc – AVR Butterfly firmware for avr-gcc

The demo application and the serial bootloader of the **Atmel AVR Butterfly**
(ATmega169 evaluation board with LCD, joystick, speaker, temperature / light /
voltage sensors and a dataflash), ported to **avr-gcc / avr-libc**.

| Folder      | Content                                                                 |
|-------------|-------------------------------------------------------------------------|
| `main/`     | Butterfly application ("AVR Butterfly GCC"), Makefile, `readme.txt` (development diary) |
| `bf_boot/`  | Bootloader, Makefile, `readme.txt` (installation instructions)          |
| `tests/`    | Host unit tests (native `gcc`) for the BCD and calendar helpers         |
| `env_avr.sh`   | Puts the Arduino IDE's avr-gcc on the `PATH` (`source ./env_avr.sh`)  |
| `flash_all.sh` | Builds both projects, joins the hex files, flashes them with an STK500 |

## History of the code

- **Original:** the Butterfly firmware was written by Atmel (Atmel Norway) for
  the IAR EWAVR compiler. The port is based on its REV_06, with fixes from REV_07.
- **2003–2009, first gcc port:** Martin Thomas ported the application and the
  bootloader to avr-gcc/avr-libc. The first versions date from October 2003. The
  work covered the program-memory handling (`PROGMEM`), the interrupt handlers, the
  EEPROM and sleep functions, and the Makefiles. His changes are marked `// mt`.
  The releases went up to version 0.7.1 (January 2007), with later updates
  for WinAVR 6/2008 (March 2009). The full diary is in `main/readme.txt` and
  `bf_boot/readme.txt`.
- **2026, revived on GitHub:** the code was brought up to date with avr-gcc 7.3.0
  and avr-libc 2.0.0 (Arduino package) and tested on real hardware (AVR Butterfly
  and STK500). Claude Code was used as an assistant. The changes include
  `ATOMIC_BLOCK` instead of `cli()`/`sei()`, key click and ADC work moved out of
  the interrupts, buffer-overrun fixes, `bool` and `stdint` types, include guards,
  braces on all `if`/`else` and single-statement loops, host unit tests and a
  GitHub Actions build.

## The two branches

Both branches build the same firmware, with the same table and string data in
flash. They differ only in **how data stored in flash is read**.

| | `main` | `flash-address-space` |
|---|---|---|
| Flash tables | `const T x[] PROGMEM` | `const __flash T x[]` |
| Reading a table | `pgm_read_byte()`, `pgm_read_word()`, `memcpy_P()` | plain `x[i]`, the compiler emits `lpm` |
| Text strings | `PSTR("...")`, `PROGMEM` strings | named `static const __flash char TXT_...[]` constants (no `PSTR`) |
| `LCD_puts_f()` parameter | `const char *` | `const __flash char *` |
| Compilers | any avr-gcc / avr-libc version | avr-gcc 4.7 or newer only (named address spaces) |
| App size (avr-gcc 7.3) | 12894 bytes | 12806 bytes |

- **`main`** is the portable variant. It uses only standard avr-libc calls
  (`pgm_read_byte`, `pgm_read_word`, `memcpy_P`) and so works with older
  compilers and avr-libc versions. This is the default branch.
- **`flash-address-space`** is the shorter, type-checked variant. The compiler
  knows which pointers point into flash, so no `pgm_read_*` call is needed. The
  `__flash` extension was added in avr-gcc 4.7. The branch starts at commit
  `76a31ac` of `main`, before the switch to `pgm_read_*`.

**Keeping them apart.** Do not merge `main` into `flash-address-space`, because
that would bring the `pgm_read_*` code back. Cherry-pick individual commits from
`main` instead.

## Download

Repository: <https://github.com/thomas-engineering/bf_gcc>

**With git, one branch only:**

```sh
# portable variant (default branch)
git clone --branch main --single-branch https://github.com/thomas-engineering/bf_gcc.git

# __flash variant
git clone --branch flash-address-space --single-branch https://github.com/thomas-engineering/bf_gcc.git
```

**With git, all branches, then switch:**

```sh
git clone https://github.com/thomas-engineering/bf_gcc.git
cd bf_gcc
git switch main                    # or:
git switch flash-address-space
```

**As a zip file, without git:**

- <https://github.com/thomas-engineering/bf_gcc/archive/refs/heads/main.zip>
- <https://github.com/thomas-engineering/bf_gcc/archive/refs/heads/flash-address-space.zip>

On the GitHub page you can also pick the branch in the drop-down at the top left
and use *Code → Download ZIP*.

## Build

```sh
source ./env_avr.sh        # avr-gcc from the Arduino IDE; skip it if avr-gcc is already on the PATH
make -C main               # application   -> main/build/main.hex
make -C bf_boot            # bootloader    -> bf_boot/build/main.hex
make -C tests              # host unit tests
./flash_all.sh --no-flash  # build both and join them into tmp/butterfly_full.hex
```

Tested with avr-gcc 7.3.0 and avr-libc 2.0.0 (Arduino package) on Linux.
The `flash-address-space` branch needs avr-gcc 4.7 or newer.

Uploading through the bootloader with avrdude needs `-e` together with `-D`:

```sh
avrdude -c butterfly -p atmega169 -P /dev/ttyUSB0 -b 19200 -e -D -U flash:w:main/build/main.hex:i
```

## License

The original firmware is copyright Atmel Corporation (see the source headers).
This repository does not contain a separate license file; check the headers and
the original Atmel terms before redistributing.
