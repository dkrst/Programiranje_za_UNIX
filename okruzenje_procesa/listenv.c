#include <stdio.h>

int main(int argc, char *argv[], char *environ[]) {
  int k=0;

  while(environ[k] != NULL)
    printf("%s\n", environ[k++]);
  
  return 0;
}
