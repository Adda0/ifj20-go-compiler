# Makefile
# IFJ20 compiler
# Author: František Nečas (xnecas27), FIT BUT

.PHONY: clean test

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Wextra

mutable_string.o: mutable_string.c mutable_string.h

test:
	mkdir -p ../cmake-build
	cd ../cmake-build && cmake ../
	cd ../cmake-build && make all && ctest

clean:
	rm -f *.o
	rm -rf cmake-build/
