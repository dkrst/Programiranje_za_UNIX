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

  /* stvori FIFO ako jos ne postoji */
  if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
    perror("mkfifo");
    return 1;
  }

  printf("Otvaram FIFO za citanje (cekam pisca)...\n");
  fd = open(FIFO_PATH, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  /* citaj iz FIFO-a i ispisuj na standardni izlaz znak po znak,
   * dok read ne vrati 0 (kad pisac zatvori svoj kraj cjevovoda) */
  while ((n = read(fd, &s, 1)) > 0)
    write(STDOUT_FILENO, &s, 1);

  close(fd);
  return 0;
}
