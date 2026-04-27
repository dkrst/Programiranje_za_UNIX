#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int pid, rv;
  
  if (argc < 2) {
    printf("koristenje: %s <return_value>\n", argv[0]);
    return 0;
  }
  
  pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  } else if (pid == 0) {
    return atoi(argv[1]);
  } else {
    wait(&rv);  
    printf("CHILD EXIT STATUS: %d\n", rv);
    // printf("CHILD EXIT STATUS: %d\n", WEXITSTATUS(rv));
  }
  
  return 0;
}
