#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char buf[256];
  ssize_t n;

  if (argc != 3) {
    printf("koristenje: %s <postojeca_datoteka> <novi_link>\n", argv[0]);
    return 1;
  }

  if (symlink(argv[1], argv[2]) < 0) {
    perror("symlink");
    return 1;
  }

  printf("Stvoren simbolicki link '%s' -> '%s'.\n", argv[2], argv[1]);

  /* readlink ne dodaje null terminator, pa ga moramo sami dodati */
  n = readlink(argv[2], buf, sizeof(buf) - 1);
  if (n < 0) {
    perror("readlink");
    return 1;
  }
  buf[n] = '\0';

  printf("Sadrzaj linka (procitan readlink-om): \"%s\"\n", buf);
  return 0;
}
