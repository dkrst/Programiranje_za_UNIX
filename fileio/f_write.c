#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

int main(int argc, char *argv[]) {
  int fd, n;
  char s;
  
  if (argc != 2) {
    printf("koristenje: %s <ime_datoteke>\n", argv[0]);
    return 0;
  }

  fd = creat(argv[1], FMODE);
  if (fd == -1) {
    perror("open");
    return 1;
  }
 
  while((n=read(STDIN_FILENO, &s, 1)) > 0)
    write(fd, &s, 1);
  
  close(fd);
  return 0;
}
