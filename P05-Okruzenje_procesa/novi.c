#include <stdio.h>
#include <unistd.h>

int main() {
  int pid;

  pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  } else if (pid == 0) {
    printf("CHILD  (%d)\t PID: %d\t PPID: %d\n", pid, getpid(), getppid());
  } else {              
    printf("PARENT (%d)\t PID: %d\t PPID: %d\n", pid, getpid(), getppid());
  }
  
  return 0;
}
