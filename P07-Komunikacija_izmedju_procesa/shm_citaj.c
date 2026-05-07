#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHM_NAME "/moja_memorija"
#define VELICINA 256

int main(void) {
  int fd;
  char *podaci;

  fd = shm_open(SHM_NAME, O_RDONLY, 0);
  if (fd < 0) { perror("shm_open"); return 1; }

  podaci = mmap(NULL, VELICINA, PROT_READ, MAP_SHARED, fd, 0);
  if (podaci == MAP_FAILED) { perror("mmap"); return 1; }

  printf("Procitano iz %s: %s\n", SHM_NAME, podaci);

  munmap(podaci, VELICINA);
  close(fd);
  return 0;
}
