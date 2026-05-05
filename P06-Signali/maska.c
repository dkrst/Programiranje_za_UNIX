#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

void int_handler(int signum) {
  printf("SIGINT obraden\n");
}

int main() {
  struct sigaction sa;
  sigset_t blok, prethodna;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = int_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  sigemptyset(&blok);
  sigaddset(&blok, SIGINT);

  sigprocmask(SIG_BLOCK, &blok, &prethodna);
  sleep(5);
  sigprocmask(SIG_SETMASK, &prethodna, NULL);

  return 0;
}
