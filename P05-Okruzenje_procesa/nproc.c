#include <stdio.h>
#include <unistd.h>

int main() {
  int k;
  for (k=0;k<3;k++)
    fork();
  printf("PID: %d\t PPID: %d\n", getpid(), getppid());

  return 0;
}
