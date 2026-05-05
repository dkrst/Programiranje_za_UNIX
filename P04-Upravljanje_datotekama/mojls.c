#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char *argv[]) {
  DIR *dp;
  struct dirent *entry;
  const char *path;

  /* bez argumenta, listamo trenutni direktorij */
  path = (argc < 2) ? "." : argv[1];

  dp = opendir(path);
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

    printf("%s\n", entry->d_name);
  }

  closedir(dp);
  return 0;
}
