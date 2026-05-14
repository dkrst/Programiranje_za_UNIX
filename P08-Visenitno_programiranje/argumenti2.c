/* Stvaramo vise niti odjednom, svakoj predajemo razlicit argument.
 * Cesti pocetnicki problem: ako predajemo &i u petlji, sve niti
 * vide istu varijablu i citaju ju "kasno" kad smo vec povecali
 * broj. Rjesenje: predajemo svakoj niti pokazivac na ZASEBNI
 * podatak (ovdje element polja podaci[]). */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define BROJ_NITI 5

void *radnik(void *arg) {
  int id = *(int *)arg;
  printf("Nit %d pocinje rad\n", id);
  sleep(1);                           /* simulira neki posao */
  printf("Nit %d zavrsila\n", id);
  return NULL;
}

int main(void) {
  pthread_t niti[BROJ_NITI];
  int       podaci[BROJ_NITI];        /* zasebna kopija ID-a za svaku nit */
  int       i;

  for (i = 0; i < BROJ_NITI; i++) {
    podaci[i] = i;
    if (pthread_create(&niti[i], NULL, radnik, &podaci[i]) != 0) {
      perror("pthread_create");
      return 1;
    }
  }

  /* ceka da sve niti zavrse */
  for (i = 0; i < BROJ_NITI; i++)
    pthread_join(niti[i], NULL);

  printf("Sve niti su zavrsile.\n");
  return 0;
}
