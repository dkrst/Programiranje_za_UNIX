#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define BUFFSIZE 1024

int main() {
  int n;
  char buf[BUFFSIZE];

  while((n=read(STDIN_FILENO, buf, BUFFSIZE)) > 0) {
      if (write(STDOUT_FILENO, buf, n) != n) {
	  perror("write");
	  return 1;
      }
  }
    
  if (n<0)
    perror("read");

  return 0;
}
