/* System V verzija mq_posalji.c - salje poruku u red poruka identificiran
 * cjelobrojnim "kljucem" (engl. key). Kljuc se izvodi iz putanje pomocu
 * funkcije ftok. Programom mozemo poslati tekst zadan kao argument
 * naredbenog retka. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define KEY_PATH "/tmp"      /* putanja koja postoji - bilo koja staticna */
#define KEY_ID   'K'         /* proizvoljni char za izvodjenje kljuca */
#define MAX_TEXT 128

struct moja_poruka {
  long mtype;
  char mtext[MAX_TEXT];
};

int main(int argc, char *argv[]) {
  key_t kljuc;
  int   msqid;
  struct moja_poruka p;
  const char *poruka;

  if (argc < 2)
    poruka = "Pozdrav iz System V reda!";
  else
    poruka = argv[1];

  /* generiraj kljuc iz putanje i char ID-a -- isti par (putanja, id) u
   * razlicitim procesima daje isti kljuc, pa procesi nadju isti red */
  kljuc = ftok(KEY_PATH, KEY_ID);
  if (kljuc < 0) { perror("ftok"); return 1; }

  /* otvori ili stvori red poruka */
  msqid = msgget(kljuc, IPC_CREAT | 0666);
  if (msqid < 0) { perror("msgget"); return 1; }

  /* posalji poruku tipa 1 */
  p.mtype = 1;
  strncpy(p.mtext, poruka, MAX_TEXT - 1);
  p.mtext[MAX_TEXT - 1] = '\0';

  if (msgsnd(msqid, &p, strlen(p.mtext) + 1, 0) < 0) {
    perror("msgsnd");
    return 1;
  }

  printf("Poslano: %s\n", p.mtext);
  return 0;
}
