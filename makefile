CC      = gcc
CFLAGS  = -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic \
          -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
		
LFLAGS = -pthread

OBJ     = mdu.o worker.o system.o queue.o

.PHONY: all clean
all: mdu

mdu: $(OBJ)
	$(CC) $(LFLAGS) -o mdu $(OBJ)

worker.o: worker.c worker.h system.h queue.h
	$(CC) $(CFLAGS) -c worker.c

system.o: system.c system.h queue.h worker.h
	$(CC) $(CFLAGS) -c system.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f mdu $(OBJ)
