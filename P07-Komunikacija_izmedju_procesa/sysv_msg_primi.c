/* System V verzija mq_primi.c - prima i ispisuje jednu poruku iz reda
 * identificiranog istim kljucem kao u sysv_msg_posalji.c. Kad se sve
 * poruke procitaju, korisnik moze rucno obrisati red sa ipcrm. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define KEY_PATH "/tmp"
#define KEY_ID   'K'
#define MAX_TEXT 128

struct moja_poruka {
  long mtype;
  char mtext[MAX_TEXT];
};

int main(void) {
  key_t kljuc;
  int   msqid;
  struct moja_poruka p;
  ssize_t n;

  kljuc = ftok(KEY_PATH, KEY_ID);
  if (kljuc < 0) { perror("ftok"); return 1; }

  /* otvori postojeci red (bez IPC_CREAT) */
  msqid = msgget(kljuc, 0666);
  if (msqid < 0) { perror("msgget"); return 1; }

  /* primi poruku bilo kojeg tipa (msgtyp=0) */
  n = msgrcv(msqid, &p, MAX_TEXT, 0, 0);
  if (n < 0) { perror("msgrcv"); return 1; }

  printf("Primljeno (tip=%ld): %s\n", p.mtype, p.mtext);
  return 0;
}
