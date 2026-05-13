/* Pokazujemo kako pomocna nit moze vratiti rezultat glavnoj.
 * Nit racuna kvadrat broja i vraca rezultat preko pokazivaca,
 * koji glavna nit dohvaca kroz drugi argument pthread_join. */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *kvadrat(void *arg) {
  int x = *(int *)arg;
  /* alociramo memoriju na hrpi (heap) jer ce glavna nit citati
   * rezultat nakon sto se ova nit zavrsi i njen stog nestane */
  int *rezultat = malloc(sizeof(int));
  if (rezultat == NULL) return NULL;
  *rezultat = x * x;
  return rezultat;
}

int main(void) {
  pthread_t nit;
  int broj = 7;
  void *povratna_vrijednost;

  if (pthread_create(&nit, NULL, kvadrat, &broj) != 0) {
    perror("pthread_create");
    return 1;
  }

  /* drugi argument je adresa pokazivaca u koji ce pthread_join
   * pohraniti povratnu vrijednost niti (ono sto je nit vratila
   * kroz return ili pthread_exit) */
  pthread_join(nit, &povratna_vrijednost);

  int *rezultat = (int *)povratna_vrijednost;
  printf("%d^2 = %d\n", broj, *rezultat);
  free(rezultat);

  return 0;
}
