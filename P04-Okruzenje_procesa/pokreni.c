#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("koristenje: %s <naredba> [argument1] [argument2] ...\n", argv[0]);
    return 0;
  }

  execvp(argv[1], &argv[1]);
  perror("execvp");
  
  return 1;  
}
