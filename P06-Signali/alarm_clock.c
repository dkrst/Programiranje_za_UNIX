#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int brojac = 0;

void alrm_handler(int signum) {
  brojac++;
  alarm(1);
}

int main() {
  signal(SIGALRM, alrm_handler);
  alarm(1);

  while (brojac < 5) {
    pause();
    printf("tik %d\n", brojac);
  }

  printf("kraj!\n");
  return 0;
}
