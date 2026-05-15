/* TCP echo server.
 *
 * Slusa na portu 9000. Kad se klijent spoji, server cita poruku
 * i odmah ju vraca natrag (echo). Opsluzuje jednog po jednog
 * klijenta -- dok jedan radi, ostali cekaju u redu accept-a.
 *
 * Kljucne razlike u odnosu na UNIX domain socket:
 *   - AF_INET umjesto AF_UNIX
 *   - struct sockaddr_in umjesto sockaddr_un
 *   - adresa = (IP, port) umjesto putanje
 *   - htons() pretvara port iz host u network byte order
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT    9000
#define BACKLOG 5

int main(void) {
  int                fd_server, fd_klijent;
  struct sockaddr_in adresa;
  char               buffer[1024];
  ssize_t            n;

  setbuf(stdout, NULL);

  /* Stvori socket: AF_INET = IPv4, SOCK_STREAM = TCP */
  fd_server = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_server < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  /* Dopusti ponovno koristenje porta odmah nakon zatvaranja
   * (inace bismo cekali ~minutu zbog TIME_WAIT stanja TCP-a) */
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

  printf("Echo server slusa na portu %d\n", PORT);

  while (1) {
    struct sockaddr_in adresa_klijenta;
    socklen_t          len = sizeof(adresa_klijenta);
    char               ip_str[INET_ADDRSTRLEN];

    fd_klijent = accept(fd_server, (struct sockaddr *)&adresa_klijenta, &len);
    if (fd_klijent < 0) {
      perror("accept");
      continue;
    }

    /* Pretvori IP iz binarnog u tekst i ispisi tko se spojio */
    inet_ntop(AF_INET, &adresa_klijenta.sin_addr, ip_str, sizeof(ip_str));
    printf("Klijent spojen: %s:%d\n", ip_str, ntohs(adresa_klijenta.sin_port));

    /* Procitaj poruku i vrati ju natrag */
    n = read(fd_klijent, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      buffer[n] = '\0';
      printf("Primljeno: %s", buffer);
      write(fd_klijent, buffer, n);   /* echo */
    }

    close(fd_klijent);
  }

  close(fd_server);
  return 0;
}
