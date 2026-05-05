#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

int brojac = 0;

void int_handler(int signum) {
  brojac++;
}

int main() {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = int_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGINT, &sa, NULL);

  while (brojac < 2) {
    pause();
    if (brojac == 1)
      printf("Pritisnite ponovo CTRL - C ukoliko zelite izaci\n");
  }

  printf("Korisnik je potvrdio izlazak - kraj programa!\n");
  return 0;
}
