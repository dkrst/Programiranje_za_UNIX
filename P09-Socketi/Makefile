CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS =
TARGETS = uds_server uds_klijent tcp_server tcp_klijent tcp_server_fork udp_server udp_klijent

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

udp_server: udp_server.o
	$(CC) $(LDFLAGS) udp_server.o -o udp_server

udp_klijent: udp_klijent.o
	$(CC) $(LDFLAGS) udp_klijent.o -o udp_klijent

clean:
	rm -f *.o $(TARGETS)
