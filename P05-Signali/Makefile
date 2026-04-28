CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS = 
TARGETS = potvrdi alarm_clock stoperica stoperica2 razgovor

default: potvrdi

all: $(TARGETS)

potvrdi: potvrdi.o
	$(CC) $(LDFLAGS) potvrdi.o -o potvrdi

alarm_clock: alarm_clock.o
	$(CC) $(LDFLAGS) alarm_clock.o -o alarm_clock

stoperica: stoperica.o
	$(CC) $(LDFLAGS) stoperica.o -o stoperica

stoperica2: stoperica2.o
	$(CC) $(LDFLAGS) stoperica2.o -o stoperica2

razgovor: razgovor.o
	$(CC) $(LDFLAGS) razgovor.o -o razgovor

clean:
	rm -f $(TARGETS) *.o *~ a.out

.c.o:
	$(CC) $(CFLAGS) -c $<
