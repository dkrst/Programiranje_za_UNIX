CC = /usr/bin/gcc
FLAGS = -Wall
TARGETS = pozdrav pozdrav_fn

default: pozdrav_fn

all: $(TARGETS)

pozdrav: pozdrav.o
	$(CC) $(CFLAGS) pozdrav.o -o pozdrav

pozdrav_fn: pozdrav_fn.o funkcije.o
	$(CC) $(FLAGS) pozdrav_fn.o funkcije.o -o pozdrav_fn

clean:
	rm -f $(TARGETS) *.o *~ a.out

.c.o:
	$(CC) $(FLAGS) -c $<
