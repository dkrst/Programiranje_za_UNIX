CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS = 
TARGETS = fileinfo fileinfo2 provjeri prava makelink makesymlink readsymlink dotakni mojls

default: fileinfo

all: $(TARGETS)

fileinfo: fileinfo.o
	$(CC) $(LDFLAGS) fileinfo.o -o fileinfo

fileinfo2: fileinfo2.o
	$(CC) $(LDFLAGS) fileinfo2.o -o fileinfo2

provjeri: provjeri.o
	$(CC) $(LDFLAGS) provjeri.o -o provjeri

prava: prava.o
	$(CC) $(LDFLAGS) prava.o -o prava

makelink: makelink.o
	$(CC) $(LDFLAGS) makelink.o -o makelink

makesymlink: makesymlink.o
	$(CC) $(LDFLAGS) makesymlink.o -o makesymlink

readsymlink: readsymlink.o
	$(CC) $(LDFLAGS) readsymlink.o -o readsymlink

dotakni: dotakni.o
	$(CC) $(LDFLAGS) dotakni.o -o dotakni

mojls: mojls.o
	$(CC) $(LDFLAGS) mojls.o -o mojls

clean:
	rm -f $(TARGETS) *.o *~ a.out

.c.o:
	$(CC) $(CFLAGS) -c $<
