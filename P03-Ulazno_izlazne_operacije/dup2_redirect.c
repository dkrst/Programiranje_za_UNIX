#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
  int fd;

  fd = open("izlaz.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  if (dup2(fd, STDOUT_FILENO) != fd)
    close(fd);

  printf("Ovaj tekst zavrsava u datoteci izlaz.txt!\n");

  return 0;
}
