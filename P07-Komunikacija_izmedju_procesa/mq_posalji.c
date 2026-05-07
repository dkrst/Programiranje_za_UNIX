#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>

#define MQ_NAME "/moj_mq"
#define MAX_MSG_SIZE 256

int main(int argc, char *argv[]) {
  mqd_t mq;
  const char *poruka;
  unsigned prioritet = 0;

  if (argc < 2) {
    poruka = "Pozdrav kroz red poruka!";
  } else {
    poruka = argv[1];
    if (argc >= 3) prioritet = (unsigned)atoi(argv[2]);
  }

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = MAX_MSG_SIZE;
  attr.mq_curmsgs = 0;

  mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, 0666, &attr);
  if (mq == (mqd_t)-1) {
    perror("mq_open");
    return 1;
  }

  if (mq_send(mq, poruka, strlen(poruka) + 1, prioritet) < 0) {
    perror("mq_send");
    mq_close(mq);
    return 1;
  }

  printf("Poslana poruka (prioritet=%u): %s\n", prioritet, poruka);
  mq_close(mq);
  return 0;
}
