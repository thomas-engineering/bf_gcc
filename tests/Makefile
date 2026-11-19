# Host unit tests (native gcc). The firmware itself is built with avr-gcc
# from main/ and bf_boot/.

CC     = gcc
CFLAGS = -std=gnu99 -funsigned-char -Wall -Wextra -Wshadow -I../main

all: test

build/test_main: test_main.c ../main/BCD.c ../main/BCD.h ../main/calendar.h
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ test_main.c ../main/BCD.c

test: build/test_main
	./build/test_main

clean:
	rm -rf build

.PHONY: all test clean
