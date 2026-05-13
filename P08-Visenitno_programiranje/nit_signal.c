/* Signali u visenitnom procesu.
 *
 * Najvaznije pravilo: signal moze biti dostavljen BILO KOJOJ niti
 * koja nije blokirala taj signal. Ako svaka nit ima vlastiti
 * rukovatelj signala, ponasanje je nepredvidljivo - ne mozemo
 * unaprijed znati kojoj ce niti signal stici.
 *
 * Standardni obrazac: jedna nit je zaduzena za sve signale,
 * dok ostale niti signale blokiraju kroz pthread_sigmask.
 *
 * Ovaj primjer pokazuje upravo to: stvaramo nekoliko niti koje
 * blokiraju SIGINT, a glavna nit ostaje "signal handler" za njega
 * preko sigwait. */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#define BROJ_NITI 3

void *radnik(void *arg) {
  int id = *(int *)arg;
  sigset_t maska;

  /* blokiramo SIGINT u ovoj niti -- naslijedjena maska iz roditelja
   * obuhvaca SIGINT (vidi main) pa je ovo zapravo redundantno, ali
   * eksplicitno radimo radi jasnoce */
  sigemptyset(&maska);
  sigaddset(&maska, SIGINT);
  pthread_sigmask(SIG_BLOCK, &maska, NULL);

  while (1) {
    printf("Radnik %d radi...\n", id);
    sleep(1);
  }
  return NULL;
}

int main(void) {
  pthread_t niti[BROJ_NITI];
  int       podaci[BROJ_NITI];
  sigset_t  maska;
  int       primljeni_signal;

  /* prije stvaranja niti, blokiramo SIGINT u glavnoj niti --
   * sve naknadno stvorene niti naslijedit ce ovu masku */
  sigemptyset(&maska);
  sigaddset(&maska, SIGINT);
  pthread_sigmask(SIG_BLOCK, &maska, NULL);

  for (int i = 0; i < BROJ_NITI; i++) {
    podaci[i] = i;
    pthread_create(&niti[i], NULL, radnik, &podaci[i]);
  }

  printf("Pritisni Ctrl+C za izlaz...\n");

  /* sigwait sinkrono ceka jedan od signala iz maske;
   * kad signal stigne, sigwait vraca njegov broj */
  sigwait(&maska, &primljeni_signal);
  printf("\nGlavna nit primila SIGINT (%d), gasim radnike.\n",
         primljeni_signal);

  /* posaljemo otkaznicu svakoj niti */
  for (int i = 0; i < BROJ_NITI; i++)
    pthread_cancel(niti[i]);
  for (int i = 0; i < BROJ_NITI; i++)
    pthread_join(niti[i], NULL);

  printf("Sve niti su zavrsile.\n");
  return 0;
}
