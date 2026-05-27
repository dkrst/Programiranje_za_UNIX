CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS =
LIBDIR = .
TARGETS = pozdrav pozdrav_fn niz1 niz2 niz3

default: pozdrav_fn

all: $(TARGETS)

pozdrav: pozdrav.o
	$(CC) $(LDFLAGS) pozdrav.o -o pozdrav

pozdrav_fn: pozdrav_fn.o funkcije.o
	$(CC) $(LDFLAGS) pozdrav_fn.o funkcije.o -o pozdrav_fn

# Tri nacina povezivanja istog programa (niz):
# niz1 -- direktno s objektnom datotekom nizfn.o
# niz2 -- eksplicitno s arhivom libniz.a
# niz3 -- preko -L -l opcija (gcc sam pronadje libniz.a)

niz1: niz.o nizfn.o
	$(CC) $(LDFLAGS) niz.o nizfn.o -o niz1

niz2: niz.o libniz.a
	$(CC) $(LDFLAGS) niz.o libniz.a -o niz2

niz3: niz.o libniz.a
	$(CC) $(LDFLAGS) -L$(LIBDIR) niz.o -lniz -o niz3

libniz.a: nizfn.o
	ar -r $(LIBDIR)/libniz.a nizfn.o

clean:
	rm -f $(TARGETS) *.o *.a *~ a.out

.c.o:
	$(CC) $(CFLAGS) -I$(LIBDIR) -c $<
