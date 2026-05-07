#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/moj_fifo"

int main(void) {
  int fd;
  char s;
  ssize_t n;

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

  /* citaj sa standardnog ulaza i prosljedjuj u FIFO znak po znak,
   * sve dok read ne vrati 0 (korisnik je utipkao Ctrl+D u terminalu) */
  while ((n = read(STDIN_FILENO, &s, 1)) > 0)
    write(fd, &s, 1);

  close(fd);
  return 0;
}
