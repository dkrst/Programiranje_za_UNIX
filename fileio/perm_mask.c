#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define PRAVA S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
int main() {
  umask(0);
  if (creat("datoteka1", PRAVA) < 0)
    perror("creat datoteka1");

  umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (creat("datoteka2", PRAVA) < 0)
    perror("creat datoteka1");
  
  return 0;
}
