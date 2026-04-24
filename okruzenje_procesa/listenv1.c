#include <stdio.h>

int main(int argc, char *argv[], char *envp[]) {
  int k=0;

  while(envp[k] != NULL)
    printf("%s\n", envp[k++]);
  
  return 0;
}
