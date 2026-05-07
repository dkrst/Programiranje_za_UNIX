#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
  int fd[2];
  pid_t pid;

  if (pipe(fd) < 0) {
    perror("pipe");
    return 1;
  }

  pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    /* preusmjeri standardni izlaz na fd[1] */
    dup2(fd[1], STDOUT_FILENO);
    if (fd[1] != STDOUT_FILENO)
      close(fd[1]);
    close(fd[0]);
    execlp("ls", "ls", (char *)NULL);
    perror("execlp ls");
    return 1;
  } else {
    /* preusmjeri standardni ulaz na fd[0] */
    dup2(fd[0], STDIN_FILENO);
    if (fd[0] != STDIN_FILENO)
      close(fd[0]);
    close(fd[1]);
    execlp("wc", "wc", "-l", (char *)NULL);
    perror("execlp wc");
    return 1;
  }
}
