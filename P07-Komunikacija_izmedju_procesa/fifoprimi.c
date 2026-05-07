#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/moj_fifo"

int main(void) {
  int fd;
  char buf[256];
  ssize_t n;

  /* stvori FIFO ako jos ne postoji */
  if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
    perror("mkfifo");
    return 1;
  }

  printf("Otvaram FIFO za citanje (cekam pisaca)...\n");
  fd = open(FIFO_PATH, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  n = read(fd, buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = '\0';
    printf("Primljeno: %s\n", buf);
  }

  close(fd);
  return 0;
}
