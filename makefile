cFlags = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
cc = gcc

.PHONY: all
all: mdu

mdu: mdu.o file.o
	$(cc) $(cFlags) -o mdu mdu.o file.o

mdu.o: mdu.c file.h
	$(cc) $(cFlags) -c mdu.c

file.o: file.c
	$(cc) $(cFlags) -c file.c

.PHONY: clean
clean:
	@rm -f mdu

