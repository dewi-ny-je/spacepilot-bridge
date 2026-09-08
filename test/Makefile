# Host-side unit tests for the translation core (src/bridge.c).
#   make -C test          build and run
#   make -C test clean

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O1 -g
CFLAGS  += -I../src -DBRIDGE_DEBUG=0

test: test_bridge
	./test_bridge

test_bridge: test_bridge.c ../src/bridge.c ../src/bridge.h ../src/config.h ../src/buttons.h ../src/button_maps.h
	$(CC) $(CFLAGS) -o $@ test_bridge.c ../src/bridge.c

clean:
	rm -f test_bridge

.PHONY: test clean
