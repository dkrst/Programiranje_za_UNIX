#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int brojac = 0;

void uhvati(int signum) {
  brojac++;
}

int main() {
  signal(SIGINT, uhvati);

  while(brojac < 2) {
    pause();
    if (brojac == 1)
      printf("Pritisnite ponovo CTRL - C ukoliko zelite izaci\n");
  }
  
  return 0;
}
