/* Najjednostavniji UDP echo server.
 *
 * Za razliku od TCP-a (SOCK_STREAM), UDP koristi SOCK_DGRAM:
 * komunikacija je orijentirana na pakete (datagrame), nema
 * uspostave veze, nema listen-a ni accept-a. Svaki paket je
 * samostalna jedinica.
 *
 * Server u petlji recvfrom-om cita dolazne pakete, ispise ih
 * i s sendto vraca pakete natrag posiljatelju (echo). Na
 * poruku "KRAJ" odgovara s "U REDU -- IZLAZIM!" i prekida
 * izvrsavanje.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9001

int main(void) {
  int                fd;
  struct sockaddr_in adresa, adresa_klijenta;
  socklen_t          len;
  char               buffer[256];
  ssize_t            n;
  int                kraj = 0;
  const char        *poruka_kraja = "U REDU -- IZLAZIM!";

  setbuf(stdout, NULL);

  /* SOCK_DGRAM = UDP */
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&adresa, 0, sizeof(adresa));
  adresa.sin_family      = AF_INET;
  adresa.sin_addr.s_addr = htonl(INADDR_ANY);
  adresa.sin_port        = htons(PORT);

  if (bind(fd, (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
    perror("bind");
    close(fd);
    exit(EXIT_FAILURE);
  }

  /* Bez listen-a i accept-a -- UDP nema vezu */
  printf("UDP server slusa na portu %d\n", PORT);

  while (!kraj) {
    len = sizeof(adresa_klijenta);
    n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
                 (struct sockaddr *)&adresa_klijenta, &len);
    if (n > 0) {
      buffer[n] = '\0';
      printf("Primljeno: %s\n", buffer);

      if (strncmp(buffer, "KRAJ", 4) == 0) {
        sendto(fd, poruka_kraja, strlen(poruka_kraja), 0,
               (struct sockaddr *)&adresa_klijenta, len);
        kraj = 1;
      } else {
        sendto(fd, buffer, n, 0,
               (struct sockaddr *)&adresa_klijenta, len);   /* echo */
      }
    }
  }

  printf("Server izlazi.\n");
  close(fd);
  return 0;
}
