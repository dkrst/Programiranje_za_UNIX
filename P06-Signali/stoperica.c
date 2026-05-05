#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int brojac = 0;
int broji = 1;

void alrm_handler(int signum) {
  brojac++;
  alarm(1);
}

void int_handler(int signum) {
  broji = 0;
}

int main() {
  signal(SIGALRM, alrm_handler);
  signal(SIGINT,  int_handler);

  printf("Stoperica pokrenuta -- pritisnite Ctrl+C za zaustavljanje.\n");
  alarm(1);

  while (broji) {
    pause();
    if (broji)
      printf("tik %d\n", brojac);
  }

  printf("Proteklo: %d sekundi\n", brojac);
  return 0;
}
