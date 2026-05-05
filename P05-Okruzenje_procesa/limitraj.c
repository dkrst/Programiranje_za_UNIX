#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>

int main(int argc, char **argv) {
  struct rlimit l;
  if (argc < 2) {
    printf("koristenje: %s <naredba> [argument1] [argument2] ...\n", argv[0]);
    return 0;
  }

  l.rlim_cur = 3;
  l.rlim_max = 3;
  setrlimit(RLIMIT_CPU, &l);
  
  execvp(argv[1], &argv[1]);
  perror("execvp");
  
  return 1;  
}
