#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc, char *argv[]) {
  struct stat st;

  if (argc != 2) {
    printf("koristenje: %s <ime_datoteke>\n", argv[0]);
    return 1;
  }

  if (lstat(argv[1], &st) < 0) {
    perror("lstat");
    return 1;
  }

  printf("Datoteka: %s\n", argv[1]);
  printf("  tip:           ");
  if      (S_ISREG(st.st_mode))  printf("regularna datoteka\n");
  else if (S_ISDIR(st.st_mode))  printf("direktorij\n");
  else if (S_ISLNK(st.st_mode))  printf("simbolicki link\n");
  else if (S_ISCHR(st.st_mode))  printf("karakter specijalna\n");
  else if (S_ISBLK(st.st_mode))  printf("blok specijalna\n");
  else if (S_ISFIFO(st.st_mode)) printf("FIFO\n");
  else if (S_ISSOCK(st.st_mode)) printf("socket\n");

  printf("  i-node broj:   %ld\n", (long)st.st_ino);
  printf("  prava:         %o\n", st.st_mode & 0777);
  printf("  vlasnik UID:   %d\n", st.st_uid);
  printf("  grupa GID:     %d\n", st.st_gid);
  printf("  velicina:      %ld B\n", (long)st.st_size);
  printf("  broj linkova:  %ld\n", (long)st.st_nlink);
  printf("  zadnji pristup:  %s", ctime(&st.st_atime));
  printf("  zadnja izmjena:  %s", ctime(&st.st_mtime));
  printf("  zadnja promjena: %s", ctime(&st.st_ctime));

  return 0;
}
