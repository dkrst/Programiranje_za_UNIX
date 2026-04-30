#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void chld_handler(int signum) {
  int status;
  pid_t pid;

  pid = wait(&status);
  printf("[parent] dijete %d pokupljeno (status %d)\n",
         pid, WEXITSTATUS(status));
}

int main() {
  struct sigaction sa;
  int trajanje[] = {3, 1, 2};
  int i, sek;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = chld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);

  for (i = 0; i < 3; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      printf("[child %d] PID %d, spavam %d s\n", i+1, getpid(), trajanje[i]);
      sleep(trajanje[i]);
      return i+1;
    }
  }

  for (sek = 1; sek <= 5; sek++) {
    sleep(1);
    printf("[parent] sekunda %d\n", sek);
  }

  return 0;
}
