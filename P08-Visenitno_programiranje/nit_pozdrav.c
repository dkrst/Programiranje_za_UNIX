/* Najjednostavniji primjer - stvori jednu nit koja ispiše poruku.
 * Glavna nit ceka da pomocna nit zavrsi prije nego sama izadje. */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *pozdrav(void *arg) {
  (void)arg;                 /* eksplicitno oznacimo da arg ne koristimo */
  printf("Pozdrav iz niti!\n");
  return NULL;
}

int main(void) {
  pthread_t nit;

  /* stvori nit koja ce izvrsavati funkciju pozdrav() */
  if (pthread_create(&nit, NULL, pozdrav, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  /* ceka da pomocna nit zavrsi; bez ovoga bi glavna nit
   * mogla izaci prije nego pomocna stigne ispisati poruku */
  pthread_join(nit, NULL);

  printf("Glavna nit zavrsava.\n");
  return 0;
}
