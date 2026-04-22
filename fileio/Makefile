CC = /usr/bin/gcc
CFLAGS = -Wall
TARGETS = read_file io_copy f_strip f_hole perm_mask f_write f_cat 

default: perm_mask

all: $(TARGETS)

read_file: read_file.o
	$(CC) $(CFLAGS) read_file.o -o read_file

io_copy: io_copy.o
	$(CC) $(CFLAGS) io_copy.o -o io_copy

f_strip: f_strip.o
	$(CC) $(CFLAGS) f_strip.o -o f_strip

f_hole: f_hole.o
	$(CC) $(CFLAGS) f_hole.o -o f_hole

perm_mask: perm_mask.o
	$(CC) $(CFLAGS) perm_mask.o -o perm_mask

f_write: f_write.o
	$(CC) $(CFLAGS) f_write.o -o f_write

f_cat: f_cat.o
	$(CC) $(CFLAGS) f_cat.o -o f_cat

clean:
	rm -f $(TARGETS) *.o *~ a.out file.strip file.hole datoteka1 datoteka2

.c.o:
	$(CC) $(CFLAGS) -c $<
