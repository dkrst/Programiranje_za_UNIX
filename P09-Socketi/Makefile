CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS =
TARGETS = uds_server uds_klijent tcp_server tcp_klijent tcp_server_fork

default: tcp_server

all: $(TARGETS)

.c.o:
	$(CC) $(CFLAGS) -c $<

uds_server: uds_server.o
	$(CC) $(LDFLAGS) uds_server.o -o uds_server

uds_klijent: uds_klijent.o
	$(CC) $(LDFLAGS) uds_klijent.o -o uds_klijent

tcp_server: tcp_server.o
	$(CC) $(LDFLAGS) tcp_server.o -o tcp_server

tcp_klijent: tcp_klijent.o
	$(CC) $(LDFLAGS) tcp_klijent.o -o tcp_klijent

tcp_server_fork: tcp_server_fork.o
	$(CC) $(LDFLAGS) tcp_server_fork.o -o tcp_server_fork

clean:
	rm -f *.o $(TARGETS)
