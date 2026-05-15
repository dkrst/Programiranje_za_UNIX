CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS =
TARGETS = pozdrav1 pozdrav2 pozdrav3 kvadrat kvadrat2 argumenti argumenti2 broji broji2 nit_cond nit_cond2 nit_signal nit_detached

# pthreads zahtijevaju -lpthread (na nekim sustavima i -pthread za CFLAGS)
LIBS_PTHREAD = -lpthread

default: pozdrav1

all: $(TARGETS)

.c.o:
	$(CC) $(CFLAGS) -c $<

pozdrav1: pozdrav1.o
	$(CC) $(LDFLAGS) pozdrav1.o -o pozdrav1 $(LIBS_PTHREAD)

pozdrav2: pozdrav2.o
	$(CC) $(LDFLAGS) pozdrav2.o -o pozdrav2 $(LIBS_PTHREAD)

pozdrav3: pozdrav3.o
	$(CC) $(LDFLAGS) pozdrav3.o -o pozdrav3 $(LIBS_PTHREAD)

kvadrat: kvadrat.o
	$(CC) $(LDFLAGS) kvadrat.o -o kvadrat $(LIBS_PTHREAD)

kvadrat2: kvadrat2.o
	$(CC) $(LDFLAGS) kvadrat2.o -o kvadrat2 $(LIBS_PTHREAD)

argumenti: argumenti.o
	$(CC) $(LDFLAGS) argumenti.o -o argumenti $(LIBS_PTHREAD)

argumenti2: argumenti2.o
	$(CC) $(LDFLAGS) argumenti2.o -o argumenti2 $(LIBS_PTHREAD)

broji: broji.o
	$(CC) $(LDFLAGS) broji.o -o broji $(LIBS_PTHREAD) -lm

broji2: broji2.o
	$(CC) $(LDFLAGS) broji2.o -o broji2 $(LIBS_PTHREAD) -lm

nit_cond: nit_cond.o
	$(CC) $(LDFLAGS) nit_cond.o -o nit_cond $(LIBS_PTHREAD)

nit_cond2: nit_cond2.o
	$(CC) $(LDFLAGS) nit_cond2.o -o nit_cond2 $(LIBS_PTHREAD)

nit_signal: nit_signal.o
	$(CC) $(LDFLAGS) nit_signal.o -o nit_signal $(LIBS_PTHREAD)

nit_detached: nit_detached.o
	$(CC) $(LDFLAGS) nit_detached.o -o nit_detached $(LIBS_PTHREAD)

clean:
	rm -f *.o $(TARGETS) *~ a.out
