#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char buf[256];
  ssize_t n;

  if (argc != 2) {
    printf("koristenje: %s <simbolicki_link>\n", argv[0]);
    return 1;
  }

  /* readlink ne dodaje null terminator, pa ga moramo sami dodati */
  n = readlink(argv[1], buf, sizeof(buf) - 1);
  if (n < 0) {
    perror("readlink");
    return 1;
  }
  buf[n] = '\0';

  printf("Sadrzaj linka '%s': \"%s\"\n", argv[1], buf);
  return 0;
}
