/* Rjesenje race conditiona iz nit_race.c kroz mutex.
 *
 * Mutex je sinkronizacijski mehanizam koji osigurava da samo
 * jedna nit u danom trenutku moze izvrsavati zasticeni dio koda
 * (kriticnu sekciju). Konceptualno je identican binarnom
 * semaforu (vidi P07), ali je optimiziran za sinkronizaciju
 * niti unutar istog procesa.
 *
 * Mutex se inicijalizira sa PTHREAD_MUTEX_INITIALIZER (za
 * staticki alocirane mutexe) ili pthread_mutex_init(). Zakljucava
 * se sa pthread_mutex_lock(), otkljucava sa pthread_mutex_unlock().
 *
 * Cijena: znatno sporije od nezasticenog koda (ali tocno!).
 *
 * Kod je strukturno identican onomu iz nit_race.c -- ista
 * razlozena inkrementacija s sched_yield -- samo je sad sve
 * unutar kriticne sekcije zasticene mutexom. */
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

#define BROJ_NITI 4
#define ITERACIJA 100000

static long              brojac = 0;
static pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;

void *radnik(void *arg) {
  (void)arg;
  for (int i = 0; i < ITERACIJA; i++) {
    pthread_mutex_lock(&mutex);        /* udji u kriticnu sekciju */
    long temp = brojac;
    sched_yield();                     /* dok smo unutra, nitko drugi nece uci */
    temp = temp + 1;
    brojac = temp;
    pthread_mutex_unlock(&mutex);     /* izadji iz kriticne sekcije */
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
  /* mutex inicijaliziran s _INITIALIZER ne treba destroy */
  return 0;
}
