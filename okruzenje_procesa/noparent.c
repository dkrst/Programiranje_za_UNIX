#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
  int pid;
  
  pid = fork();
  if (pid == 0) {      // CHILD 1
    pid = fork();
    if (pid == 0) {    // CHILD 2
      printf("CHILD 2:\t PID: %d\t PPID: %d\n", getpid(), getppid());
      sleep(3);
      printf("CHILD 2:\t PID: %d\t PPID: %d\n", getpid(), getppid());
    } else {           // CHILD 1 - PARENT 2
      sleep(1);
      printf("CHILD 1:\t PID: %d\t PPID: %d\n", getpid(), getppid());
      exit(0);
    }
  } else {             // PARENT 1
    pid = wait(NULL);
    printf("PARENT 1:\t PID: %d\t izlazi: %d\n", getpid(), pid);
    printf("PARENT 1:\t PID: %d\t PPID: %d\n", getpid(), getppid());
    sleep(5);
  }
  
  return 0;
}
