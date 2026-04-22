#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
  char buf1[]="Prvi redak teksta";
  char buf2[]="Drugi redak teksta";
  int fd;
  
  fd=creat("file.hole", (mode_t)0644);
  if (fd==-1) {
    perror("creat");
    return 1;
  }
  
  if (write(fd, buf1, strlen(buf1)+1)!=strlen(buf1)+1) {
    perror("write buf1");
    return 1;
  }
  
  if (lseek(fd, 15, SEEK_CUR)==-1) {
    perror("lseek");
  }
  
  if (write(fd, buf2, strlen(buf2)+1)!=strlen(buf2)+1) 
    perror("write buf2");
  
  close(fd);
  exit(0);
}
