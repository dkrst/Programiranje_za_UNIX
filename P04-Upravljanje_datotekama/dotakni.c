#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
  int fd;

  if (argc != 2) {
    printf("koristenje: %s <ime_datoteke>\n", argv[0]);
    return 1;
  }

  /* Ako datoteka ne postoji, stvori je praznu */
  fd = open(argv[1], O_WRONLY | O_CREAT, 0644);
  if (fd < 0) {
    perror("open");
    return 1;
  }
  close(fd);

  /* Postavi atime i mtime na trenutno vrijeme.
   * NULL kao drugi argument znaci "koristi trenutno vrijeme". */
  if (utime(argv[1], NULL) < 0) {
    perror("utime");
    return 1;
  }

  printf("Datoteka '%s' azurirana.\n", argv[1]);
  return 0;
}
