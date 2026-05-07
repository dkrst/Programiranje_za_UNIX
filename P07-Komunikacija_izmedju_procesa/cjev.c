#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
  int fd[2];
  pid_t pid;
  char buf[128];

  /* fd[0] - kraj za citanje
   * fd[1] - kraj za pisanje */
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
    /* dijete - cita iz cjevovoda */
    close(fd[1]);                       /* ne treba nam pisanje */
    ssize_t n = read(fd[0], buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = '\0';
      printf("Dijete primilo: %s\n", buf);
    }
    close(fd[0]);
    return 0;
  }

  /* roditelj - pise u cjevovod */
  close(fd[0]);                         /* ne treba nam citanje */
  const char *poruka = "Pozdrav iz roditelja!";
  write(fd[1], poruka, strlen(poruka));
  close(fd[1]);

  wait(NULL);                           /* cekaj da dijete zavrsi */
  return 0;
}
