/* TCP klijent za tcp_server.
 *
 * Spaja se na server (default: 127.0.0.1, port 9000), procita
 * jedan red sa stdin-a, posalje serveru, procita odgovor i
 * ispise ga.
 *
 * Pokretanje:
 *   ./tcp_klijent                  # spaja se na 127.0.0.1:9000
 *   ./tcp_klijent 192.168.1.10     # spaja se na drugi IP, port 9000
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
  const char        *ip = (argc >= 2) ? argv[1] : "127.0.0.1";
  char               buffer[1024];
  ssize_t            n;

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

  printf("Spojen na %s:%d. Upisi poruku: ", ip, PORT);
  fflush(stdout);

  if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
    close(fd);
    return 0;
  }

  /* Posalji */
  if (write(fd, buffer, strlen(buffer)) < 0) {
    perror("write");
    close(fd);
    exit(EXIT_FAILURE);
  }

  /* Procitaj odgovor */
  n = read(fd, buffer, sizeof(buffer) - 1);
  if (n > 0) {
    buffer[n] = '\0';
    printf("Odgovor: %s", buffer);
  }

  close(fd);
  return 0;
}
