#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define MAX_TEXT 128

/* struktura poruke - prvi clan mora biti tipa long (tip poruke) */
struct moja_poruka {
  long mtype;
  char mtext[MAX_TEXT];
};

int main(void) {
  int msqid;
  pid_t pid;

  /* IPC_PRIVATE = stvori novi privatni red poruka koji ce nasljediti
   * djeca preko fork-a; nije dostupan drugim nepovezanim procesima */
  msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
  if (msqid < 0) { perror("msgget"); return 1; }

  pid = fork();
  if (pid < 0) { perror("fork"); return 1; }

  if (pid == 0) {
    /* dijete - prima poruku */
    struct moja_poruka p;
    if (msgrcv(msqid, &p, MAX_TEXT, 0, 0) < 0) {
      perror("msgrcv");
      return 1;
    }
    printf("Dijete primilo (tip=%ld): %s\n", p.mtype, p.mtext);
    return 0;
  }

  /* roditelj - salje poruku */
  struct moja_poruka p;
  p.mtype = 1;
  strcpy(p.mtext, "Pozdrav od roditelja preko System V!");
  if (msgsnd(msqid, &p, strlen(p.mtext) + 1, 0) < 0) {
    perror("msgsnd");
    return 1;
  }

  wait(NULL);

  /* obrisi red - inace ostaje u sustavu! */
  msgctl(msqid, IPC_RMID, NULL);
  return 0;
}
