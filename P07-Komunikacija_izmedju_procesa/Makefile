CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS = 
TARGETS = cijev prebroji fifoposalji fifoprimi shm_pisi shm_citaj mq_posalji mq_primi msg_demo

# POSIX shm trebaju -lrt na nekim sustavima (Linux pre-glibc-2.34)
LIBS_RT = -lrt
# POSIX message queue trebaju -lrt
LIBS_MQ = -lrt

default: cijev

all: $(TARGETS)

cijev: cijev.o
	$(CC) $(LDFLAGS) cijev.o -o cijev

prebroji: prebroji.o
	$(CC) $(LDFLAGS) prebroji.o -o prebroji

fifoposalji: fifoposalji.o
	$(CC) $(LDFLAGS) fifoposalji.o -o fifoposalji

fifoprimi: fifoprimi.o
	$(CC) $(LDFLAGS) fifoprimi.o -o fifoprimi

shm_pisi: shm_pisi.o
	$(CC) $(LDFLAGS) shm_pisi.o -o shm_pisi $(LIBS_RT)

shm_citaj: shm_citaj.o
	$(CC) $(LDFLAGS) shm_citaj.o -o shm_citaj $(LIBS_RT)

mq_posalji: mq_posalji.o
	$(CC) $(LDFLAGS) mq_posalji.o -o mq_posalji $(LIBS_MQ)

mq_primi: mq_primi.o
	$(CC) $(LDFLAGS) mq_primi.o -o mq_primi $(LIBS_MQ)

msg_demo: msg_demo.o
	$(CC) $(LDFLAGS) msg_demo.o -o msg_demo

clean:
	rm -f $(TARGETS) *.o *~ a.out

.c.o:
	$(CC) $(CFLAGS) -c $<
