# Makefile
# IFJ20 compiler
# Author: František Nečas  (xnecas27), FIT BUT
# Author: David Chocholatý (xchoch08), FIT BUT

.PHONY: all clean test pack

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Wextra -O2

CTESTFLAGS ?= ""

all: compiler

compiler: scanner.o mutable_string.o stderr_message.o compiler.o \
		  parser.o precedence_parser.o stacks.o symtable.o ast.o control_flow.o code_generator.o \
		  optimiser.o variable_vector.o

scanner.o: scanner.c scanner.h mutable_string.h compiler.h \
		   scanner_static.h stderr_message.h
mutable_string.o: mutable_string.c mutable_string.h
stderr_message.o: stderr_message.c stderr_message.h  compiler.h
compiler.o: compiler.c compiler.h parser.h scanner.h mutable_string.h stacks.h symtable.h \
			precedence_parser.h ast.h optimiser.h control_flow.h code_generator.h
parser.o: parser.c parser.h compiler.h scanner.h mutable_string.h stderr_message.h \
		  precedence_parser.h control_flow.h ast.h stacks.h precedence_parser.h
precedence_parser.o: precedence_parser.c precedence_parser.h scanner.h \
					 mutable_string.h compiler.h parser.h stderr_message.h stacks.h \
					 control_flow.h ast.h
stacks.o: stacks.c stacks.h scanner.h mutable_string.h compiler.h precedence_parser.h \
		  symtable.h stderr_message.h ast.h
symtable.o: symtable.c symtable.h stderr_message.h compiler.h
ast.o: ast.c ast.h symtable.h stderr_message.h compiler.h
control_flow.o: control_flow.c control_flow.h ast.h symtable.h
code_generator.o: code_generator.c code_generator.h control_flow.h ast.h symtable.h \
				  ast.h stderr_message.h compiler.h mutable_string.h
optimiser.o: optimiser.c optimiser.h control_flow.h symtable.h ast.h \
			 code_generator.h stderr_message.h compiler.h variable_vector.h
variable_vector.o: variable_vector.c variable_vector.h symtable.h ast.h stderr_message.h


test:
	mkdir -p ../cmake-build
	cd ../cmake-build && cmake ../
	cd ../cmake-build && make all && ctest $(CTESTFLAGS)

clean:
	rm -f *.o
	rm -rf ../cmake-build/
	rm compiler

pack:
	mkdir pack
	cp *.c *.h Makefile pack
	cp ../doc/dokumentace.pdf pack
	cp ../rozdeleni ../rozsireni pack
	cd pack && zip ../xchoch08.zip *
	rm -rf pack
