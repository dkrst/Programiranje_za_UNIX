/* TCP/IP klijent. Identican po ponasanju kao uds_klijent, samo
 * koristi mreznu domenu (AF_INET) umjesto UNIX domain.
 *
 * Spaja se na server (default: 127.0.0.1, port 9000), salje
 * jednu poruku zadanu kao argument naredbenog retka, procita
 * odgovor servera i ispise ga.
 *
 * Pokretanje:
 *   ./tcp_klijent "Pozdrav serveru!"
 *   ./tcp_klijent "KRAJ"                  (zaustavlja server)
 *   ./tcp_klijent "Poruka" 192.168.1.10   (drugi server)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9000

int main(int argc, char *argv[]) {
  int                fd;
  struct sockaddr_in adresa;
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

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&adresa, 0, sizeof(adresa));
  adresa.sin_family = AF_INET;
  adresa.sin_port   = htons(PORT);

  /* inet_pton: pretvori IP iz teksta u binarni oblik */
  if (inet_pton(AF_INET, ip, &adresa.sin_addr) <= 0) {
    fprintf(stderr, "Neispravna IP adresa: %s\n", ip);
    close(fd);
    exit(EXIT_FAILURE);
  }

  if (connect(fd, (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
    perror("connect");
    close(fd);
    exit(EXIT_FAILURE);
  }

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
