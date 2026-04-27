CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS = 
TARGETS = argumenti zbroji readenv listenv1 listenv2 myls pokreni pokreni2 preusmjeri novi nproc noparent potrosac limitraj ret_stat ptr_err

default: ret_stat

all: $(TARGETS)

argumenti: argumenti.o
	$(CC) $(LDFLAGS) argumenti.o -o argumenti

zbroji: zbroji.o
	$(CC) $(LDFLAGS) zbroji.o -o zbroji

readenv: readenv.o
	$(CC) $(LDFLAGS) readenv.o -o readenv

listenv1: listenv1.o
	$(CC) $(LDFLAGS) listenv1.o -o listenv1

listenv2: listenv2.o
	$(CC) $(LDFLAGS) listenv2.o -o listenv2

myls: myls.o
	$(CC) $(LDFLAGS) myls.o -o myls

pokreni: pokreni.o
	$(CC) $(LDFLAGS) pokreni.o -o pokreni

pokreni2: pokreni2.o
	$(CC) $(LDFLAGS) pokreni2.o -o pokreni2

ptr_err: ptr_err.o
	$(CC) $(LDFLAGS) ptr_err.o -o ptr_err

preusmjeri: preusmjeri.o
	$(CC) $(LDFLAGS) preusmjeri.o -o preusmjeri

novi: novi.o
	$(CC) $(LDFLAGS) novi.o -o novi

ret_stat: ret_stat.o
	$(CC) $(LDFLAGS) ret_stat.o -o ret_stat

nproc: nproc.o
	$(CC) $(LDFLAGS) nproc.o -o nproc

noparent: noparent.o
	$(CC) $(LDFLAGS) noparent.o -o noparent

potrosac: potrosac.o
	$(CC) $(LDFLAGS) potrosac.o -o potrosac

limitraj: limitraj.o
	$(CC) $(LDFLAGS) limitraj.o -o limitraj

clean:
	rm -f $(TARGETS) *.o *~ a.out

.c.o:
	$(CC) $(CFLAGS) -c $<
