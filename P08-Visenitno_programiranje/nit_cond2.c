/* Vise niti proizvodjaca i vise niti potrosaca dijele isti
 * cirkularni medjuspremnik. Struktura je gotovo identicna onoj
 * iz nit_cond.c, s dvije bitne razlike:
 *
 * 1. Stvaramo N_PROIZ proizvodjaca i N_POTR potrosaca.
 *    Svaki proizvodjac proizvede STAVKI_PO_NITI stavki, svaki
 *    potrosac konzumira STAVKI_PO_NITI stavki. Buduci da su
 *    brojevi jednaki, ukupan broj proizvedenih = ukupan broj
 *    potrosenih, pa nikoga necemo ostaviti da zauvijek ceka.
 *
 * 2. Koristimo pthread_cond_broadcast umjesto pthread_cond_signal
 *    da probudimo SVE niti koje cekaju na uvjetu. Razlog je
 *    suptilan ali vazan -- pojasnjeno u README-u.
 *
 * Petlja while oko provjere uvjeta sad je nuzna i s razloga koji
 * nismo isticali u prethodnom primjeru: kada jedna nit signalom
 * probudi sve potrosace, samo jedan ce stici uzeti podatak prije
 * nego buffer opet postane prazan, a ostali ce, nakon provjere
 * uvjeta, samo nastaviti spavati. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define VEL_BUFFERA      4
#define N_PROIZ          3       /* broj niti proizvodjaca */
#define N_POTR           3       /* broj niti potrosaca */
#define STAVKI_PO_NITI  10      /* svaka nit proizvede/potrosi ovoliko */

static int             buffer[VEL_BUFFERA];
static int             upis_idx   = 0;
static int             cit_idx    = 0;
static int             buff_items = 0;

static pthread_mutex_t mutex      = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  ima_mjesta = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  ima_robe   = PTHREAD_COND_INITIALIZER;

static void slucajna_pauza(void) {
  usleep((rand() % 191 + 10) * 1000);
}

void *proizvodjac(void *arg) {
  int id = *(int *)arg;
  for (int i = 0; i < STAVKI_PO_NITI; i++) {
    int stavka = id * 100 + i;   /* Razlikujemo tko je sto proizveo */

    pthread_mutex_lock(&mutex);
    while (buff_items == VEL_BUFFERA)
      pthread_cond_wait(&ima_mjesta, &mutex);

    buffer[upis_idx] = stavka;
    upis_idx = (upis_idx + 1) % VEL_BUFFERA;
    buff_items++;
    printf("Proizvodjac %d: stavio %d (buff_items %d)\n",
	   id, stavka, buff_items);

    /* Probudimo sve potrosace */
    pthread_cond_broadcast(&ima_robe);   
    pthread_mutex_unlock(&mutex);

    slucajna_pauza();
  }
  return NULL;
}

void *potrosac(void *arg) {
  int id = *(int *)arg;
  for (int i = 0; i < STAVKI_PO_NITI; i++) {
    pthread_mutex_lock(&mutex);

    /* dok je buffer prazan, cekaj signal "ima robe" */
    while (buff_items == 0)
      pthread_cond_wait(&ima_robe, &mutex);

    int v = buffer[cit_idx];
    cit_idx = (cit_idx + 1) % VEL_BUFFERA;
    buff_items--;
    printf("\t\t Potrosac %d: uzeo %d (buff_items %d)\n",
           id, v, buff_items);

    /* Probudimo sve proizvodjace */
    pthread_cond_broadcast(&ima_mjesta);  
    pthread_mutex_unlock(&mutex);

    slucajna_pauza();
  }
  return NULL;
}

int main(void) {
  pthread_t niti_p[N_PROIZ], niti_c[N_POTR];
  int       id_p[N_PROIZ],    id_c[N_POTR];

  srand((unsigned)time(NULL));

  for (int i = 0; i < N_PROIZ; i++) {
    id_p[i] = i;
    pthread_create(&niti_p[i], NULL, proizvodjac, &id_p[i]);
  }
  for (int i = 0; i < N_POTR; i++) {
    id_c[i] = i;
    pthread_create(&niti_c[i], NULL, potrosac, &id_c[i]);
  }

  for (int i = 0; i < N_PROIZ; i++) pthread_join(niti_p[i], NULL);
  for (int i = 0; i < N_POTR;  i++) pthread_join(niti_c[i], NULL);

  printf("Gotovo.\n");
  return 0;
}
