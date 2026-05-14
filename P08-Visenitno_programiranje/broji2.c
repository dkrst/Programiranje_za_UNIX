#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#define NTHREADS 8

pthread_t thr_counter[NTHREADS];
unsigned long count = 0;
/* staticka inicijalizacija mutexa -- za globalne mutexe ne moramo
 * pozivati pthread_mutex_init ni pthread_mutex_destroy */
pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

void *counter(void *arg) {
  int *c = (int *)arg;
  double d;
  printf("c: %d\n", *c);
  for (int k=0; k<*c; k++) {
    /* petlja koja simulira "neki posao" -- NE radi se nad
     * dijeljenom varijablom, pa ju drzimo IZVAN kriticne sekcije */
    for (int j=0; j<50; j++)
      d = sqrt((double)j);

    /* zakljucavamo mutex samo oko pristupa dijeljenoj varijabli */
    pthread_mutex_lock(&count_lock);
    count++;
    pthread_mutex_unlock(&count_lock);
  }
  (void)d;
  pthread_exit(NULL);
}


int main() {
  int cnt = 100000, k;

  for (k=0; k<NTHREADS; k++) {
    pthread_create(&thr_counter[k], NULL, counter, (void *)&cnt);
  }

  for (k=0; k<NTHREADS; k++) {
    pthread_join(thr_counter[k], NULL);
  }

  printf("Ukupno (%d * %d) = %lu\n", NTHREADS, cnt, count);
  return 0;
}
