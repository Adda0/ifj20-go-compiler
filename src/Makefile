# Makefile
# IFJ20 compiler
# Author: František Nečas  (xnecas27), FIT BUT
# Author: David Chocholatý (xchoch08), FIT BUT

.PHONY: clean test

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Wextra

CTESTFLAGS ?= ""

all: compiler


scanner.o: scanner.c scanner.h scanner_static.h stderr_message.h
mutable_string.o: mutable_string.c mutable_string.h
stderr_message.o: stderr_message.c stderr_message.h
compiler.o: compiler.c compiler.h


test:
	mkdir -p ../cmake-build
	cd ../cmake-build && cmake ../
	cd ../cmake-build && make all && ctest $(CTESTFLAGS)

clean:
	rm -f *.o
	rm -rf cmake-build/
