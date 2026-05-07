#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/moj_fifo"

int main(int argc, char *argv[]) {
  int fd;
  const char *poruka;

  if (argc < 2)
    poruka = "Pozdrav kroz FIFO!";
  else
    poruka = argv[1];

  /* stvori FIFO ako ne postoji */
  if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
    perror("mkfifo");
    return 1;
  }

  printf("Otvaram FIFO za pisanje (cekam citatelja)...\n");
  fd = open(FIFO_PATH, O_WRONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  write(fd, poruka, strlen(poruka));
  printf("Poruka poslana: %s\n", poruka);

  close(fd);
  return 0;
}
