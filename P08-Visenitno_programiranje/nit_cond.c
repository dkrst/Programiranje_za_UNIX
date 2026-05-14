/* Klasicni problem proizvodjac-potrosac (engl. producer-consumer).
 *
 * Jedan proizvodjac stavlja podatke u ograniceni cirkularni
 * medjuspremnik (kruzni red), jedan potrosac ih vadi i obradjuje.
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
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define VEL_BUFFERA 4        /* kruzni red duljine 4 */
#define BROJ_STAVKI 120      /* koliko stavki proizvodjac proizvodi */

static int             buffer[VEL_BUFFERA];
static int             upis_idx   = 0;  /* slijedeca pozicija za upis */
static int             cit_idx    = 0;  /* slijedeca pozicija za citanje */
static int             buff_items = 0;  /* broj stavki trenutno u buferu */

static pthread_mutex_t mutex      = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  ima_mjesta = PTHREAD_COND_INITIALIZER;   /* signal: ima slobodnog mjesta */
static pthread_cond_t  ima_robe   = PTHREAD_COND_INITIALIZER;   /* signal: ima cega za citati */

/* nasumicna pauza izmedju 10 i 200 milisekundi */
static void slucajna_pauza(void) {
  usleep((rand() % 191 + 10) * 1000);
}

void *proizvodjac(void *arg) {
  (void)arg;
  for (int i = 0; i < BROJ_STAVKI; i++) {
    pthread_mutex_lock(&mutex);

    /* dok je buffer pun, cekaj signal "ima mjesta" */
    while (buff_items == VEL_BUFFERA)
      pthread_cond_wait(&ima_mjesta, &mutex);

    buffer[upis_idx] = i;
    upis_idx = (upis_idx + 1) % VEL_BUFFERA;
    buff_items++;
    printf("Proizvodjac: stavio %d (buff_items %d)\n", i, buff_items);

    /* obavijesti potrosaca da ima novih podataka */
    pthread_cond_signal(&ima_robe);
    pthread_mutex_unlock(&mutex);

    slucajna_pauza();   /* nasumicno vrijeme proizvodnje */
  }
  return NULL;
}

void *potrosac(void *arg) {
  (void)arg;
  for (int i = 0; i < BROJ_STAVKI; i++) {
    pthread_mutex_lock(&mutex);

    /* dok je buffer prazan, cekaj signal "ima robe" */
    while (buff_items == 0)
      pthread_cond_wait(&ima_robe, &mutex);

    int vrijednost = buffer[cit_idx];
    cit_idx = (cit_idx + 1) % VEL_BUFFERA;
    buff_items--;
    printf("                                Potrosac: uzeo %d (buff_items %d)\n",
           vrijednost, buff_items);

    /* obavijesti proizvodjaca da je oslobodjeno mjesto */
    pthread_cond_signal(&ima_mjesta);
    pthread_mutex_unlock(&mutex);

    slucajna_pauza();   /* nasumicno vrijeme obrade */
  }
  return NULL;
}

int main(void) {
  pthread_t nit_p, nit_c;

  srand((unsigned)time(NULL));

  pthread_create(&nit_p, NULL, proizvodjac, NULL);
  pthread_create(&nit_c, NULL, potrosac, NULL);

  pthread_join(nit_p, NULL);
  pthread_join(nit_c, NULL);

  printf("Gotovo.\n");
  return 0;
}
