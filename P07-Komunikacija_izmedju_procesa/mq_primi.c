#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>

#define MQ_NAME "/moj_mq"
#define MAX_MSG_SIZE 256

int main(void) {
  mqd_t mq;
  char buf[MAX_MSG_SIZE];
  unsigned prioritet;
  ssize_t n;

  mq = mq_open(MQ_NAME, O_RDONLY);
  if (mq == (mqd_t)-1) {
    perror("mq_open");
    return 1;
  }

  printf("Cekam poruku...\n");
  n = mq_receive(mq, buf, MAX_MSG_SIZE, &prioritet);
  if (n < 0) {
    perror("mq_receive");
    mq_close(mq);
    return 1;
  }

  printf("Primljeno (prioritet=%u): %s\n", prioritet, buf);

  mq_close(mq);
  mq_unlink(MQ_NAME);                    /* ukloni red kad smo gotovi */
  return 0;
}
