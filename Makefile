B=build
CC=gcc
CC_FLAGS=-Wall -Werror -std=gnu23 -g3 -I./

C_FILES=$(shell find . -name "*.c")
C_O_FILES=${addprefix $B/,${subst .c,.o,${C_FILES}}}

LINK=${firstword ${${patsubst %.c,${CC},${C_FILES}}}}
LINK_FLAGS=-lglfw -lGL -lc

.PHONY: all
all : $B/gb

.PHONY: asan
asan: CC_FLAGS := $(CC_FLAGS) -fsanitize=address,undefined,leak
asan: LINK_FLAGS := $(LINK_FLAGS) -fsanitize=address,undefined,leak
asan: $B/gb

$B/gb: ${C_O_FILES}
	@mkdir -p build
	${CC} ${LINK} -o $@ ${LINK_FLAGS} ${C_O_FILES}

${C_O_FILES} : $B/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	${CC} -c -o $@ ${CC_FLAGS} $<

.PHONY: clean
clean:
	rm -rf $B
