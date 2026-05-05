#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  int pid, s;
  
  if (argc < 2) {
    printf("koristenje: %s <naredba> [argument1] [argument2] ...\n", argv[0]);
    return 0;
  }

  pid=fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  } else if (pid == 0) {
    execvp(argv[1], &argv[1]);
    perror("execvp");
    return 127;
  } else {
    wait(&s);
    if (WIFEXITED(s))
      printf("Noramaln izlaz, izlazni status: %d\n", WEXITSTATUS(s));
    else if (WIFSIGNALED(s))
      printf("Prekid signalom, signal: %d\n", WTERMSIG(s));
  }
    
  return 0;  
}
