#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

int main(int argc, char **argv) {
  int fd;
  
  if (argc < 3) {
    printf("koristenje: %s <datoteka> <naredba> [argument1] [argument2] ...\n", argv[0]);
    return 0;
  }

  fd = creat(argv[1], (mode_t)0644);
  if (fd < 0) {
    perror("creat");
    return 1;
  }

  if (dup2(fd, STDOUT_FILENO) < 0) {
    perror("dup2");
    return 1;
  }
  if (fd != STDOUT_FILENO)
    close(fd);
  
  execvp(argv[2], &argv[2]);
  perror("execvp");
  
  return 1;  
}
