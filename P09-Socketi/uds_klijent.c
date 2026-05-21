/* Klijent za uds_server.
 *
 * Spaja se na UNIX domain socket /tmp/uds_primjer, posalje
 * jednu poruku zadanu kao argument naredbenog retka, procita
 * odgovor servera i ispise ga.
 *
 * Pokretanje:
 *   ./uds_klijent "Pozdrav serveru!"
 *   ./uds_klijent "KRAJ"     (zaustavlja server)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PUTANJA "/tmp/uds_primjer"

int main(int argc, char *argv[]) {
  int                fd;
  struct sockaddr_un adresa;
  const char        *poruka;
  char               buffer[256];
  ssize_t            n;

  if (argc != 2) {
    fprintf(stderr, "Koristenje: %s \"poruka\"  (KRAJ za prekid izvrsavanja servera)\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  poruka = argv[1];

  /* Stvori socket */
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* Pripremi adresu servera */
  memset(&adresa, 0, sizeof(adresa));
  adresa.sun_family = AF_UNIX;
  strncpy(adresa.sun_path, PUTANJA, sizeof(adresa.sun_path) - 1);

  /* Spoji se */
  if (connect(fd, (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
    perror("connect");
    close(fd);
    exit(EXIT_FAILURE);
  }

  /* Posalji poruku */
  if (write(fd, poruka, strlen(poruka)) < 0) {
    perror("write");
    close(fd);
    exit(EXIT_FAILURE);
  }

  /* Procitaj odgovor servera */
  n = read(fd, buffer, sizeof(buffer) - 1);
  if (n > 0) {
    buffer[n] = '\0';
    printf("Odgovor: %s\n", buffer);
  }

  close(fd);
  return 0;
}
