/* Najjednostavniji UNIX Domain Socket server.
 *
 * Klijenti se spajaju, posalju jednu poruku, server ju ispise
 * i prekine vezu. Server se vrti u petlji i opsluzuje jednog
 * po jednog klijenta. Kada primi poruku "KRAJ", izlazi.
 *
 * UNIX domain socketi koriste putanju u datotecnom sustavu
 * kao "adresu" -- kod nas /tmp/uds_primjer. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PUTANJA "/tmp/uds_primjer"
#define BACKLOG 5

int main(void) {
  int                fd_server, fd_klijent;
  struct sockaddr_un adresa;
  char               buffer[256];
  ssize_t            n;
  int                kraj = 0;

  setbuf(stdout, NULL);   /* iskljuci buffering kad nismo na terminalu */

  /* Stvori socket */
  fd_server = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd_server < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* Brisemo datoteku (socket) ako vec postoji */
  unlink(PUTANJA);

  /* Pripremi adresu i vezi socket */
  memset(&adresa, 0, sizeof(adresa));
  adresa.sun_family = AF_UNIX;
  strncpy(adresa.sun_path, PUTANJA, sizeof(adresa.sun_path) - 1);

  if (bind(fd_server, (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
    perror("bind");
    close(fd_server);
    exit(EXIT_FAILURE);
  }

  /* Pocni slusati */
  if (listen(fd_server, BACKLOG) < 0) {
    perror("listen");
    close(fd_server);
    exit(EXIT_FAILURE);
  }

  printf("Server slusa na %s\n", PUTANJA);

  /* Petlja: prihvati klijenta, procitaj poruku, zatvori.
   * Petlja se prekida kad stigne poruka "KRAJ". */
  while (!kraj) {
    fd_klijent = accept(fd_server, NULL, NULL);
    if (fd_klijent < 0) {
      perror("accept");
      continue;
    }

    n = read(fd_klijent, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      buffer[n] = '\0';
      printf("Primljeno: %s\n", buffer);
      if (strncmp(buffer, "KRAJ", 4) == 0)
        kraj = 1;
    }

    close(fd_klijent);
  }

  printf("Server izlazi.\n");
  close(fd_server);
  unlink(PUTANJA);
  return 0;
}
