#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

int main() {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  sleep(5);

  return 0;
}
