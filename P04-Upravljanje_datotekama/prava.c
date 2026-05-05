#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
  mode_t mode;

  if (argc != 3) {
    printf("koristenje: %s <oktalna_prava> <ime_datoteke>\n", argv[0]);
    printf("primjer: %s 644 dat.txt\n", argv[0]);
    return 1;
  }

  /* strtol s bazom 8 - korisnik zadaje oktalni zapis poput 644 */
  mode = (mode_t)strtol(argv[1], NULL, 8);

  if (chmod(argv[2], mode) < 0) {
    perror("chmod");
    return 1;
  }

  printf("Prava datoteke '%s' postavljena na %o.\n", argv[2], mode);
  return 0;
}
