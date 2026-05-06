#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

int main(int argc, char *argv[]) {
  DIR *dp;
  struct dirent *entry;
  struct stat st;
  char path[1024];
  const char *dir;

  /* bez argumenta, listamo trenutni direktorij */
  dir = (argc < 2) ? "." : argv[1];

  dp = opendir(dir);
  if (dp == NULL) {
    perror("opendir");
    return 1;
  }

  /* readdir vraca po jedan zapis u svakom pozivu;
   * NULL znaci kraj direktorija (ili greska) */
  while ((entry = readdir(dp)) != NULL) {
    /* preskoci skrivene datoteke (ukljucujuci . i ..) */
    if (entry->d_name[0] == '.')
      continue;

    /* sastavi punu putanju "dir/ime" za poziv lstat-u */
    snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

    if (lstat(path, &st) < 0) {
      perror(path);
      continue;
    }

    printf("%10ld  %s\n", (long)st.st_size, entry->d_name);
  }

  closedir(dp);
  return 0;
}
