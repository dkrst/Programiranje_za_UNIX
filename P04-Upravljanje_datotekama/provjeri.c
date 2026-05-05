#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("koristenje: %s <ime_datoteke>\n", argv[0]);
    return 1;
  }

  if (access(argv[1], F_OK) < 0) {
    printf("Datoteka '%s' ne postoji.\n", argv[1]);
    return 1;
  }

  printf("Prava korisnika nad datotekom '%s':\n", argv[1]);
  printf("  citanje:     %s\n", access(argv[1], R_OK) == 0 ? "DA" : "NE");
  printf("  pisanje:     %s\n", access(argv[1], W_OK) == 0 ? "DA" : "NE");
  printf("  izvrsavanje: %s\n", access(argv[1], X_OK) == 0 ? "DA" : "NE");

  return 0;
}
