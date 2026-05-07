#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
  int fd[2];
  pid_t pid_ls, pid_wc;

  if (pipe(fd) < 0) {
    perror("pipe");
    return 1;
  }

  /* prvi proces: ls */
  pid_ls = fork();
  if (pid_ls < 0) {
    perror("fork");
    return 1;
  }
  if (pid_ls == 0) {
    /* preusmjeri standardni izlaz na pisuci kraj cjevovoda */
    dup2(fd[1], STDOUT_FILENO);
    close(fd[0]);
    close(fd[1]);
    execlp("ls", "ls", (char *)NULL);
    perror("execlp ls");
    return 1;
  }

  /* drugi proces: wc -l */
  pid_wc = fork();
  if (pid_wc < 0) {
    perror("fork");
    return 1;
  }
  if (pid_wc == 0) {
    /* preusmjeri standardni ulaz na citajuci kraj cjevovoda */
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);
    close(fd[1]);
    execlp("wc", "wc", "-l", (char *)NULL);
    perror("execlp wc");
    return 1;
  }

  /* roditelj: zatvori oba kraja i cekaj djecu */
  close(fd[0]);
  close(fd[1]);
  waitpid(pid_ls, NULL, 0);
  waitpid(pid_wc, NULL, 0);
  return 0;
}
