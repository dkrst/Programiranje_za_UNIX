#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_t glavna;

void *pozdrav(void *arg) {
  sleep(1);
  printf("Pozdrav iz druge niti!\n");
  pthread_join(glavna, NULL);
  printf("Druga nit izlazi!\n");
  return NULL;
}

int main(void) {
  pthread_t nit;

  glavna = pthread_self();
  /* stvori nit koja ce izvrsavati funkciju pozdrav() */
  if (pthread_create(&nit, NULL, pozdrav, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  printf("Pozdrav iz prve niti.\n");
  printf("Prva nit nit izlazi!\n");
  pthread_exit(NULL);
}
