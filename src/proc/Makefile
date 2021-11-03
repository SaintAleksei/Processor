flags  :=-g -O0 -Wall -Wextra -Werror
dirs   := . ..
prog   := proc

VPATH  := $(dirs)

$(prog): $(notdir $(patsubst %.c,%.o,$(wildcard $(addsuffix /*.c,$(dirs) ) ) ) )
	gcc $^ -o $@

%.o: %.c
	gcc -c -MMD $(addprefix -I,$(dirs) ) $(flags) $<

clean:
	rm *.o *.d

include $(wildcard *.d)
