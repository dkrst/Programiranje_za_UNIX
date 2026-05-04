#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
  int fd, newfd;

  fd = open("izlaz.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  close(STDOUT_FILENO);    // zatvori standardni izlaz (deskriptor 1)
  newfd = dup(fd);         // dup vraća najnižu slobodnu vrijednost = 1

  if (newfd != fd)
    close(fd);

  printf("Ovaj tekst nece zavrsiti u terminalu!\n");

  return 0;
}
