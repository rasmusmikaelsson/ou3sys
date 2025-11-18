cFlags = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
cc = gcc

.PHONY: all
all: mdu

mdu: mdu.o file.o queue.o
	$(cc) $(cFlags) -o mdu mdu.o file.o queue.o

mdu.o: mdu.c file.h queue.h
	$(cc) $(cFlags) -c mdu.c

file.o: file.c
	$(cc) $(cFlags) -c file.c

queue.o: queue.c
	$(cc) $(cflags) -c queue.c

.PHONY: clean
clean:
	@rm -f mdu

