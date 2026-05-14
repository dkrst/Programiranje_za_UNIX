#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void *pozdrav(void *arg) {
  sleep(1);
  printf("Pozdrav iz druge niti!\n");
  printf("Druga nit izlazi!\n");
  return NULL;
}

int main(void) {
  pthread_t nit;

  /* stvori nit koja ce izvrsavati funkciju pozdrav() */
  if (pthread_create(&nit, NULL, pozdrav, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  printf("Pozdrav iz prve niti.\n");
  pthread_join(nit, NULL);
  printf("Prva nit nit izlazi!\n");
  return 0;
}
