/* Race condition demonstracija u kontekstu niti.
 *
 * Vise niti inkrementira zajednicki brojac. Operacija brojac++
 * razlaze se na assembler razini u 3 koraka: ucitaj iz memorije
 * u registar, povecaj registar, vrati u memoriju. Ako raspoređivac
 * prekine nit nakon prvog koraka, druga nit moze ucitati istu
 * staru vrijednost - pa nakon obje inkrementacije imamo +1
 * umjesto +2.
 *
 * Razlika prema P07 (shm_brojac): tamo su procesi imali odvojene
 * adresne prostore pa smo trebali dijeljenu memoriju. Ovdje niti
 * dijele adresni prostor pa je obicna globalna varijabla
 * automatski vidljiva svima.
 *
 * Da bi race bio jasno vidljiv, eksplicitno razlazemo
 * inkrementaciju u tri koraka i ubacujemo male prekide;
 * "obican" brojac++ ce na modernim procesorima race pokazati
 * tek povremeno jer se sekvenca cesto izvrsi vrlo brzo. */
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

#define BROJ_NITI 4
#define ITERACIJA 100000

static long brojac = 0;     /* dijeljen IZMEDJU SVIH NITI */

void *radnik(void *arg) {
  (void)arg;
  for (int i = 0; i < ITERACIJA; i++) {
    long temp = brojac;       /* 1. ucitaj iz memorije */
    sched_yield();            /* pustimo drugu nit da nas pretekne */
    temp = temp + 1;          /* 2. povecaj */
    brojac = temp;            /* 3. vrati u memoriju */
  }
  return NULL;
}

int main(void) {
  pthread_t niti[BROJ_NITI];

  for (int i = 0; i < BROJ_NITI; i++)
    pthread_create(&niti[i], NULL, radnik, NULL);

  for (int i = 0; i < BROJ_NITI; i++)
    pthread_join(niti[i], NULL);

  printf("Brojac = %ld  (ocekivano: %d)\n",
         brojac, BROJ_NITI * ITERACIJA);
  return 0;
}
