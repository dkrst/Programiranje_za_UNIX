#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("koristenje: %s <postojeca_datoteka> <novi_link>\n", argv[0]);
    return 1;
  }

  if (link(argv[1], argv[2]) < 0) {
    perror("link");
    return 1;
  }

  printf("Stvoren hard link '%s' -> '%s'.\n", argv[2], argv[1]);
  return 0;
}
