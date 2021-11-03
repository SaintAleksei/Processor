flags  :=-g -O0 -Wall -Wextra -Werror
sources:=cmdtable.c assembler.c processor.c
objects:=$(patsubst %.c, %.o, $(sources) )

all: assm proc

assm: $(objects) mainassm.o
	gcc $^ -o $@

proc: $(objects) mainproc.o
	gcc $^ -o $@

%.o: %.c
	gcc -c -MMD $(flags) $<

clean:
	rm *.o *.d

include $(wildcard *.d)
