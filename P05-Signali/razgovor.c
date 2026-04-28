#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int radi = 1;
int prosao = 0;

void usr1_handler(int signum) {
  prosao++;
}

void term_handler(int signum) {
  radi = 0;
}

int main() {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    /* CHILD */
    signal(SIGUSR1, usr1_handler);
    signal(SIGTERM, term_handler);

    while (radi) {
      pause();
      if (radi)
        printf("[child]  primio sam SIGUSR1, prolaz %d\n", prosao);
    }

    printf("[child]  zavrsavam, ukupno prolaza: %d\n", prosao);
    return 0;
  }

  /* PARENT */
  printf("[parent] pokrenuo dijete s PID-om %d\n", pid);

  for (int k = 0; k < 5; k++) {
    sleep(1);
    printf("[parent] saljem SIGUSR1 djetetu\n");
    kill(pid, SIGUSR1);
  }

  sleep(1);
  printf("[parent] saljem SIGTERM djetetu\n");
  kill(pid, SIGTERM);

  wait(NULL);
  printf("[parent] dijete je zavrsilo, izlazim\n");
  return 0;
}
