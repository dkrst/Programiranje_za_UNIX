#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("koristenje: %s <postojeca_datoteka> <novi_link>\n", argv[0]);
    return 1;
  }

  if (symlink(argv[1], argv[2]) < 0) {
    perror("symlink");
    return 1;
  }

  printf("Stvoren simbolicki link '%s' -> '%s'.\n", argv[2], argv[1]);
  return 0;
}
