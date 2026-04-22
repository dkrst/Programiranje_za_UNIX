#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int rw(int fdin, int fdout) {
  int n;
  char s;
  while((n=read(fdin, &s, 1)) > 0)
    write(fdout, &s, 1);

  return n;
}

int main(int argc, char **argv) {
  int k, fd;
  if (argc < 2) {
    rw(STDIN_FILENO, STDOUT_FILENO);
  } else {
    for (k=1; k<argc; k++) {
      fd = open(argv[k], O_RDONLY);
      if (fd < 0)    // Ne mogu otvoriti
	perror("open");
      else {
	rw(fd, STDOUT_FILENO);
	close(fd);
      }
    }
  }

  return 0;
}
