#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int brojac = 0;
int broji  = 1;

void rukovatelj(int signum) {
  switch (signum) {
    case SIGALRM:
      brojac++;
      alarm(1);
      break;
    case SIGINT:
      broji = 0;
      break;
  }
}

int main() {
  signal(SIGALRM, rukovatelj);
  signal(SIGINT,  rukovatelj);

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
