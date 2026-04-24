#include <stdio.h>

extern char **environ;

int main(int argc, char *argv[]) {
  int k=0;

  while(environ[k] != NULL)
    printf("%s\n", environ[k++]);
  
  return 0;
}
