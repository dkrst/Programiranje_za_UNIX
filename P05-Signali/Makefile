CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS = 
TARGETS = potvrdi potvrdi2 alarm_clock stoperica stoperica2 razgovor maska maska2 sigchld

default: potvrdi

all: $(TARGETS)

potvrdi: potvrdi.o
	$(CC) $(LDFLAGS) potvrdi.o -o potvrdi

potvrdi2: potvrdi2.o
	$(CC) $(LDFLAGS) potvrdi2.o -o potvrdi2

alarm_clock: alarm_clock.o
	$(CC) $(LDFLAGS) alarm_clock.o -o alarm_clock

stoperica: stoperica.o
	$(CC) $(LDFLAGS) stoperica.o -o stoperica

stoperica2: stoperica2.o
	$(CC) $(LDFLAGS) stoperica2.o -o stoperica2

razgovor: razgovor.o
	$(CC) $(LDFLAGS) razgovor.o -o razgovor

maska: maska.o
	$(CC) $(LDFLAGS) maska.o -o maska

maska2: maska2.o
	$(CC) $(LDFLAGS) maska2.o -o maska2

sigchld: sigchld.o
	$(CC) $(LDFLAGS) sigchld.o -o sigchld

clean:
	rm -f $(TARGETS) *.o *~ a.out

.c.o:
	$(CC) $(CFLAGS) -c $<
