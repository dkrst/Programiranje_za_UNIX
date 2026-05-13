CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS =
TARGETS = nit_pozdrav nit_join nit_args nit_stog nit_race nit_mutex nit_cond nit_signal nit_detached

# pthreads zahtijevaju -lpthread (na nekim sustavima i -pthread za CFLAGS)
LIBS_PTHREAD = -lpthread

default: nit_pozdrav

all: $(TARGETS)

.c.o:
	$(CC) $(CFLAGS) -c $<

nit_pozdrav: nit_pozdrav.o
	$(CC) $(LDFLAGS) nit_pozdrav.o -o nit_pozdrav $(LIBS_PTHREAD)

nit_join: nit_join.o
	$(CC) $(LDFLAGS) nit_join.o -o nit_join $(LIBS_PTHREAD)

nit_args: nit_args.o
	$(CC) $(LDFLAGS) nit_args.o -o nit_args $(LIBS_PTHREAD)

nit_stog: nit_stog.o
	$(CC) $(LDFLAGS) nit_stog.o -o nit_stog $(LIBS_PTHREAD)

nit_race: nit_race.o
	$(CC) $(LDFLAGS) nit_race.o -o nit_race $(LIBS_PTHREAD)

nit_mutex: nit_mutex.o
	$(CC) $(LDFLAGS) nit_mutex.o -o nit_mutex $(LIBS_PTHREAD)

nit_cond: nit_cond.o
	$(CC) $(LDFLAGS) nit_cond.o -o nit_cond $(LIBS_PTHREAD)

nit_signal: nit_signal.o
	$(CC) $(LDFLAGS) nit_signal.o -o nit_signal $(LIBS_PTHREAD)

nit_detached: nit_detached.o
	$(CC) $(LDFLAGS) nit_detached.o -o nit_detached $(LIBS_PTHREAD)

clean:
	rm -f *.o $(TARGETS) *~ a.out
