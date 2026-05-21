/* TCP/IP server s echo komunikacijom. Identican po ponasanju
 * kao uds_server, samo koristi mreznu domenu (AF_INET) umjesto
 * UNIX domain.
 *
 * Klijenti se spajaju i salju jednu poruku. Server vraca poruku
 * natrag klijentu (echo) i prekida vezu. Server se vrti u petlji
 * i opsluzuje jednog po jednog klijenta.
 *
 * Iznimka: kada klijent posalje "KRAJ", server umjesto echoa
 * odgovara s "U REDU -- IZLAZIM!" i prekida izvrsavanje. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT    9000
#define BACKLOG 5

int main(void) {
  int                fd_server, fd_klijent;
  struct sockaddr_in adresa;
  char               buffer[256];
  ssize_t            n;
  int                kraj = 0;
  const char        *poruka_kraja = "U REDU -- IZLAZIM!";

  setbuf(stdout, NULL);

  /* Stvori socket: AF_INET = IPv4, SOCK_STREAM = TCP */
  fd_server = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_server < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* Dopusti ponovno koristenje porta odmah nakon zatvaranja */
  int opt = 1;
  setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  /* Pripremi adresu: slusaj na svim mreznim sucjeljima, port 9000 */
  memset(&adresa, 0, sizeof(adresa));
  adresa.sin_family      = AF_INET;
  adresa.sin_addr.s_addr = htonl(INADDR_ANY);
  adresa.sin_port        = htons(PORT);

  if (bind(fd_server, (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
    perror("bind");
    close(fd_server);
    exit(EXIT_FAILURE);
  }

  if (listen(fd_server, BACKLOG) < 0) {
    perror("listen");
    close(fd_server);
    exit(EXIT_FAILURE);
  }

  printf("Server slusa na portu %d\n", PORT);

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

      if (strncmp(buffer, "KRAJ", 4) == 0) {
        write(fd_klijent, poruka_kraja, strlen(poruka_kraja));
        kraj = 1;
      } else {
        write(fd_klijent, buffer, n);   /* echo natrag */
      }
    }

    close(fd_klijent);
  }

  printf("Server izlazi.\n");
  close(fd_server);
  return 0;
}
