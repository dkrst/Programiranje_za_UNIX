#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SHM_NAME "/moja_memorija"
#define VELICINA 256

int main(int argc, char *argv[]) {
  int fd;
  char *podaci;
  const char *poruka;

  if (argc < 2)
    poruka = "Pozdrav iz zajedničke memorije!";
  else
    poruka = argv[1];

  /* stvori (ili otvori postojeci) objekt dijeljene memorije */
  fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd < 0) { perror("shm_open"); return 1; }

  /* postavi velicinu (rezerviraj prostor) */
  if (ftruncate(fd, VELICINA) < 0) { perror("ftruncate"); return 1; }

  /* mapiraj objekt u adresni prostor */
  podaci = mmap(NULL, VELICINA, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
  if (podaci == MAP_FAILED) { perror("mmap"); return 1; }

  /* upisi poruku; kopiramo najvise VELICINA-1 bajtova kako bi
   * preostao prostor za zavrsni nul-znak (ako je poruka duza,
   * visak se odbacuje, a nikad se ne pise izvan rezerviranog bloka) */
  strncpy(podaci, poruka, VELICINA - 1);
  podaci[VELICINA - 1] = '\0';

  printf("Upisano u %s: %s\n", SHM_NAME, podaci);

  munmap(podaci, VELICINA);
  close(fd);
  return 0;
}
