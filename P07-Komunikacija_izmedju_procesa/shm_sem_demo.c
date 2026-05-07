#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SHM_NAME "/moj_brojac"
#define SEM_NAME "/moj_sem"
#define ITERACIJA 1000000

int main(void) {
  int fd;
  int *brojac;
  sem_t *sem;
  pid_t pid;

  /* dijeljena memorija */
  fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (fd < 0) { perror("shm_open"); return 1; }
  if (ftruncate(fd, sizeof(int)) < 0) { perror("ftruncate"); return 1; }
  brojac = mmap(NULL, sizeof(int),
                PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
  if (brojac == MAP_FAILED) { perror("mmap"); return 1; }
  *brojac = 0;

  /* semafor - inicijalna vrijednost 1 (binarni semafor) */
  sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
  if (sem == SEM_FAILED) { perror("sem_open"); return 1; }

  pid = fork();
  if (pid < 0) { perror("fork"); return 1; }

  /* oba procesa povecavaju brojac, ali sad pod zastitom semafora */
  for (int i = 0; i < ITERACIJA; i++) {
    sem_wait(sem);                       /* udji u kriticnu sekciju */
    (*brojac)++;
    sem_post(sem);                       /* napusti kriticnu sekciju */
  }

  if (pid == 0) {
    /* dijete */
    sem_close(sem);
    munmap(brojac, sizeof(int));
    close(fd);
    return 0;
  }

  /* roditelj */
  wait(NULL);
  printf("Konacna vrijednost brojaca: %d (ocekivano: %d)\n",
         *brojac, 2 * ITERACIJA);

  sem_close(sem);
  sem_unlink(SEM_NAME);
  munmap(brojac, sizeof(int));
  close(fd);
  shm_unlink(SHM_NAME);
  return 0;
}
