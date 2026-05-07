#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define SHM_NAME "/moj_brojac"
#define ITERACIJA 1000000

int main(void) {
  int fd;
  int *brojac;
  pid_t pid;

  fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd < 0) { perror("shm_open"); return 1; }
  if (ftruncate(fd, sizeof(int)) < 0) { perror("ftruncate"); return 1; }
  brojac = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
  if (brojac == MAP_FAILED) { perror("mmap"); return 1; }

  *brojac = 0;

  pid = fork();
  if (pid < 0) { perror("fork"); return 1; }

  /* oba procesa povecavaju brojac istovremeno - bez sinkronizacije */
  for (int i = 0; i < ITERACIJA; i++)
    (*brojac)++;

  if (pid == 0) {
    /* dijete je gotovo - izlazi (inace bi i ono ispisalo rezultat) */
    munmap(brojac, sizeof(int));
    close(fd);
    return 0;
  }

  /* roditelj ceka dijete kako ne bi ostao zombie proces, pa
   * tek onda procita i ispise konacnu vrijednost brojaca */
  wait(NULL);
  printf("Konacna vrijednost: %d (ocekivano: %d)\n",
         *brojac, 2 * ITERACIJA);

  munmap(brojac, sizeof(int));
  close(fd);
  shm_unlink(SHM_NAME);
  return 0;
}
