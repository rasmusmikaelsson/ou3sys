cFlags = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
cc = gcc

mdu: mdu.o
	$(cc) $(cFlags) -o mdu mdu.o

mdu.o: mdu.c
	$(cc) $(cFlags) -c mdu.c
