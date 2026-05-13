/* Detached niti.
 *
 * Po defaultu niti su "joinable" - njihovi resursi ostaju u
 * sustavu dok ih netko ne pokupi pozivom pthread_join (slicno
 * kao zombi procesi u P05). Ako nit ne joinamo, resursi se NE
 * oslobadjaju i imamo curenje memorije.
 *
 * Detached niti su drugacije: nakon zavrsetka njihovi resursi
 * se automatski oslobadjaju. Nitku nije zainteresirano za njihov
 * povratak. Koristimo ih za "ispali i zaboravi" zadatke -- npr.
 * pozadinske operacije u serverima koje sluze jedan zahtjev pa
 * zavrsavaju.
 *
 * Nit mozemo napraviti detached na dva nacina:
 *   1) postaviti atribut prije stvaranja (pthread_attr_setdetachstate)
 *   2) pozvati pthread_detach() naknadno
 *
 * Posljedica: detached nit se NE SMIJE joinati - to je greska. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void *pozadinski_posao(void *arg) {
  int id = *(int *)arg;
  free(arg);                      /* oslobodimo argument jer ga vise ne trebamo */
  printf("Detached nit %d: radim svoj posao...\n", id);
  sleep(2);
  printf("Detached nit %d: gotovo, oslobadjam resurse automatski.\n", id);
  return NULL;
}

int main(void) {
  pthread_t      nit;
  pthread_attr_t atribut;

  /* postavi atribut "detached" prije stvaranja niti */
  pthread_attr_init(&atribut);
  pthread_attr_setdetachstate(&atribut, PTHREAD_CREATE_DETACHED);

  for (int i = 0; i < 3; i++) {
    int *id = malloc(sizeof(int));
    *id = i;
    pthread_create(&nit, &atribut, pozadinski_posao, id);
  }

  pthread_attr_destroy(&atribut);

  /* NE radimo pthread_join - niti su detached.
   * Ali moramo dati niti vremena da odrade posao prije nego
   * glavna nit izadje (cime se gasi cijeli proces). */
  printf("Glavna nit ceka 3 sekunde pa izlazi.\n");
  sleep(3);
  printf("Glavna nit izlazi.\n");
  return 0;
}
