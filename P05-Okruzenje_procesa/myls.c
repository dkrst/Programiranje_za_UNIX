#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2)
    execlp("ls", "ls", "-al", NULL); 
  else
    execlp("ls", "ls", "-al", argv[1], NULL);

  perror("execlp");
  return 1;  
}
