#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  int n, fd;
  char s;

  fd = open("moja_datoteka.txt", O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 1;  
  }
  
  while((n=read(fd, &s, 1)) > 0) {
    if (write(STDOUT_FILENO, &s, 1) != 1) {
      perror("write");
      return 1;
    }
  }
  
  close(fd);
  return 0;
}
