/* Klasicni problem proizvodjac-potrosac (engl. producer-consumer).
 *
 * Jedan proizvodjac stavlja podatke u ograniceni medjuspremnik
 * (kruzni red), jedan potrosac ih vadi i obradjuje.
 *
 * Problem koji rjesavamo:
 *   - kako potrosac da "ceka" kad je medjuspremnik prazan?
 *   - kako proizvodjac da "ceka" kad je medjuspremnik pun?
 *
 * Aktivno cekanje u petlji (busy-wait) bilo bi neefikasno -
 * trosilo bi CPU. Rjesenje su KONDICIJSKE VARIJABLE.
 *
 * Kondicijska varijabla (pthread_cond_t) omogucuje niti da se
 * "uspava" cekajuci da neki uvjet postane istinit, dok druga nit
 * (koja je promijenila stanje) signalom budi spavajucu nit.
 *
 * Kljucni obrazac:
 *   pthread_mutex_lock(&m);
 *   while (!uvjet)                    // UVIJEK while, ne if!
 *     pthread_cond_wait(&cv, &m);     // atomski otpusti mutex i cekaj
 *   ... napravi posao ...
 *   pthread_mutex_unlock(&m);
 *
 * pthread_cond_wait atomski otpusta mutex i uspaljuje nit; kad
 * je nit probudjena, mutex se opet zakljucava prije povratka.
 * while petlja (umjesto if) potrebna je zbog tzv. "spurious
 * wakeups" - sustavi mogu probuditi nit i bez signala. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define VEL_BUFFERA 4        /* kruzni red duljine 4 */
#define BROJ_STAVKI 12       /* koliko stavki proizvodjac proizvodi */

static int             buffer[VEL_BUFFERA];
static int             upis_idx = 0;  /* slijedeca pozicija za upis */
static int             cit_idx  = 0;   /* slijedeca pozicija za citanje */
static int             punjenje = 0;   /* broj stavki trenutno u buferu */

static pthread_mutex_t mutex     = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  ima_mjesta = PTHREAD_COND_INITIALIZER;   /* signal: ima slobodnog mjesta */
static pthread_cond_t  ima_robe  = PTHREAD_COND_INITIALIZER;   /* signal: ima cega za citati */

void *proizvodjac(void *arg) {
  (void)arg;
  for (int i = 0; i < BROJ_STAVKI; i++) {
    pthread_mutex_lock(&mutex);

    /* dok je buffer pun, cekaj signal "ima mjesta" */
    while (punjenje == VEL_BUFFERA)
      pthread_cond_wait(&ima_mjesta, &mutex);

    buffer[upis_idx] = i;
    upis_idx = (upis_idx + 1) % VEL_BUFFERA;
    punjenje++;
    printf("Proizvodjac: stavio %d (punjenje %d)\n", i, punjenje);

    /* obavijesti potrosaca da ima novih podataka */
    pthread_cond_signal(&ima_robe);
    pthread_mutex_unlock(&mutex);

    usleep(50000);    /* simulira vrijeme proizvodnje */
  }
  return NULL;
}

void *potrosac(void *arg) {
  (void)arg;
  for (int i = 0; i < BROJ_STAVKI; i++) {
    pthread_mutex_lock(&mutex);

    /* dok je buffer prazan, cekaj signal "ima robe" */
    while (punjenje == 0)
      pthread_cond_wait(&ima_robe, &mutex);

    int vrijednost = buffer[cit_idx];
    cit_idx = (cit_idx + 1) % VEL_BUFFERA;
    punjenje--;
    printf("                                Potrosac: uzeo %d (punjenje %d)\n",
           vrijednost, punjenje);

    /* obavijesti proizvodjaca da je oslobodjeno mjesto */
    pthread_cond_signal(&ima_mjesta);
    pthread_mutex_unlock(&mutex);

    usleep(120000);   /* potrosac sporiji od proizvodjaca */
  }
  return NULL;
}

int main(void) {
  pthread_t nit_p, nit_c;

  pthread_create(&nit_p, NULL, proizvodjac, NULL);
  pthread_create(&nit_c, NULL, potrosac, NULL);

  pthread_join(nit_p, NULL);
  pthread_join(nit_c, NULL);

  printf("Gotovo.\n");
  return 0;
}
