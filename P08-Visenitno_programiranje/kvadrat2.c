#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void *kvadrat(void *arg) {
  int broj = *(int*)arg;
  int *r = (int*)malloc(sizeof(int));
  
  *r = broj*broj;
  pthread_exit((void*)r);
}

int main(int argc, char **argv) {
  pthread_t nit;
  int broj;
  int *retval;
  
  if (argc < 2) {
    printf("koristenje: %s <broj>\n", argv[0]);
    return 0;
  }

  broj = atoi(argv[1]);
  /* stvori nit koja ce izvrsavati funkciju pozdrav() */
  if (pthread_create(&nit, NULL, kvadrat, &broj) != 0) {
    perror("pthread_create");
    return 1;
  }

  pthread_join(nit, (void**)&retval);
  printf("%d^2 = %d\n", broj, *retval);
  return 0;
}
