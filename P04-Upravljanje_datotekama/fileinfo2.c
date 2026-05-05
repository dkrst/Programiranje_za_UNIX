#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
  struct stat st_stat, st_lstat;

  if (argc != 2) {
    printf("koristenje: %s <ime_datoteke>\n", argv[0]);
    return 1;
  }

  if (stat(argv[1], &st_stat) < 0) {
    perror("stat");
    return 1;
  }
  if (lstat(argv[1], &st_lstat) < 0) {
    perror("lstat");
    return 1;
  }

  printf("stat  -> i-node: %ld, velicina: %ld B, tip: ",
         (long)st_stat.st_ino, (long)st_stat.st_size);
  if (S_ISREG(st_stat.st_mode)) printf("regularna\n");
  else if (S_ISDIR(st_stat.st_mode)) printf("direktorij\n");
  else if (S_ISLNK(st_stat.st_mode)) printf("simb. link\n");
  else printf("ostalo\n");

  printf("lstat -> i-node: %ld, velicina: %ld B, tip: ",
         (long)st_lstat.st_ino, (long)st_lstat.st_size);
  if (S_ISREG(st_lstat.st_mode)) printf("regularna\n");
  else if (S_ISDIR(st_lstat.st_mode)) printf("direktorij\n");
  else if (S_ISLNK(st_lstat.st_mode)) printf("simb. link\n");
  else printf("ostalo\n");

  return 0;
}
