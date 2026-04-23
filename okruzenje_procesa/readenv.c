#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  char *vrijednost;
  
  if (argc < 2) {  // Nije zadano ime environment varijable
    printf("koristenje: %s <ime varijable>\n", argv[0]);
    return 0;
  }

  vrijednost = getenv(argv[1]);
  if (!vrijednost)
    printf("%s: environment varijabla ne postoji\n", argv[1]);
  else
    printf("%s = %s\n", argv[1], vrijednost);
  
  return 0;
}
