#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int a, b, zbroj;

  if (argc < 3) {
    printf("koristenje: %s <1.broj> <2.broj>\n", argv[0]);
    exit(0);
  }

  a = atoi(argv[1]); // Konverzija iz polja karaktera u int
  b = atoi(argv[2]); // Konverzija iz polja karaktera u int
  zbroj = a + b;

  printf("%d + %d = %d\n", a, b, zbroj);
 
  exit(0);
}
