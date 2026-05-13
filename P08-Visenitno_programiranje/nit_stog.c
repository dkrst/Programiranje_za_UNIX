/* Demonstracija ceste pogreske: nit vraca pokazivac na svoju
 * lokalnu varijablu. Lokalne varijable nalaze se na stogu niti.
 * Kad nit zavrsi, njen stog se oslobadja i memorija na koju
 * pokazivac upucuje vise nije validna. Pristup kroz nju je
 * "undefined behavior" - moze raditi, moze rusiti program,
 * ili (najgore) tiho vracati pogresne podatke.
 *
 * Ovaj program ce na vecini sustava IZGLEDATI kao da radi,
 * sto je upravo opasnost ove vrste greske.
 *
 * Usporedi s nit_join.c koji ispravno koristi malloc. */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *lose(void *arg) {
  (void)arg;
  int lokalna = 42;        /* na stogu ove niti! */
  return &lokalna;          /* OPASNO: stog ce nestati */
}

int main(void) {
  pthread_t nit;
  void *rez;

  pthread_create(&nit, NULL, lose, NULL);
  pthread_join(nit, &rez);

  /* U trenutku ovog ispisa, stog niti je vec oslobodjen.
   * Pristup *(int *)rez je nedefinirano ponasanje. */
  printf("Procitano (mozda krivo): %d\n", *(int *)rez);
  printf("Pravilan pristup: vidi nit_join.c (malloc, ne lokalne)\n");

  return 0;
}
