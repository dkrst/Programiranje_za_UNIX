/* UDP klijent.
 *
 * Salje jedan paket (datagram) UDP serveru, primi odgovor i
 * ispise ga. Bez connect-a -- adresa servera se navodi u
 * sendto, adresa odgovora dobiva se iz recvfrom.
 *
 * Pokretanje:
 *   ./udp_klijent "Pozdrav serveru!"
 *   ./udp_klijent "KRAJ"                  (zaustavlja server)
 *   ./udp_klijent "Poruka" 192.168.1.10   (drugi server)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9001

int main(int argc, char *argv[]) {
  int                fd;
  struct sockaddr_in adresa;
  socklen_t          len;
  const char        *poruka;
  const char        *ip = "127.0.0.1";
  char               buffer[256];
  ssize_t            n;

  if (argc < 2 || argc > 3) {
    fprintf(stderr,
            "Koristenje: %s \"poruka\" [ip]  (KRAJ za prekid izvrsavanja servera)\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }
  poruka = argv[1];
  if (argc == 3) ip = argv[2];

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&adresa, 0, sizeof(adresa));
  adresa.sin_family = AF_INET;
  adresa.sin_port   = htons(PORT);
  if (inet_pton(AF_INET, ip, &adresa.sin_addr) <= 0) {
    fprintf(stderr, "Neispravna IP adresa: %s\n", ip);
    close(fd);
    exit(EXIT_FAILURE);
  }

  /* Posalji paket: nema connect-a, adresa servera ide u sendto */
  if (sendto(fd, poruka, strlen(poruka), 0,
             (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
    perror("sendto");
    close(fd);
    exit(EXIT_FAILURE);
  }

  /* Primi odgovor */
  len = sizeof(adresa);
  n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
               (struct sockaddr *)&adresa, &len);
  if (n > 0) {
    buffer[n] = '\0';
    printf("Odgovor: %s\n", buffer);
  }

  close(fd);
  return 0;
}
