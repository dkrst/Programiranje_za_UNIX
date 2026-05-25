/* TCP/IP server koji opsluzuje vise klijenata istovremeno
 * koristenjem fork-a, uz echo komunikaciju, podrsku za "KRAJ"
 * i clean exit na Ctrl+C (SIGINT).
 *
 * Razlika u odnosu na tcp_server.c:
 *   - nakon accept-a, glavni proces pozove fork; dijete preuzme
 *     komunikaciju s klijentom, a roditelj odmah ide na sljedeci
 *     accept (paralelna obrada vise klijenata);
 *   - svako dijete vrti read/write echo petlju dok klijent ne
 *     zatvori vezu ili ne posalje "KRAJ"; na "KRAJ" odgovara
 *     porukom "U REDU -- IZLAZIM!" i salje signal SIGTERM
 *     roditelju koji time zaustavlja accept petlju;
 *   - roditelj hvata i SIGINT (Ctrl+C) -- umjesto da odmah
 *     prekine proces, postavlja zastavicu za uredno gasenje. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT    9000
#define BACKLOG 5

static volatile sig_atomic_t zaustavi = 0;

static void rukovatelj_sigchld(int sig) {
  (void)sig;
  /* pokupi sve gotove child procese (neblokirajuci) */
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

/* Zajednicki rukovatelj za SIGINT (Ctrl+C) i SIGTERM
 * (kojeg nam posalje dijete kada primi "KRAJ"). */
static void rukovatelj_zaustavi(int sig) {
  (void)sig;
  zaustavi = 1;
}

static void posluzi_klijenta(int fd_klijent) {
  char        buffer[256];
  ssize_t     n;
  const char *poruka_kraja = "U REDU -- IZLAZIM!";

  /* Citamo dok klijent ne prekine vezu ili ne posalje "KRAJ".
   * Sve sto primimo vracamo natrag (echo). */
  while ((n = read(fd_klijent, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[n] = '\0';
    printf("[PID %d] Primljeno: %s\n", (int)getpid(), buffer);

    if (strncmp(buffer, "KRAJ", 4) == 0) {
      write(fd_klijent, poruka_kraja, strlen(poruka_kraja));
      kill(getppid(), SIGTERM);
      break;
    } else {
      write(fd_klijent, buffer, n);   /* echo natrag */
    }
  }
}

int main(void) {
  int                fd_server, fd_klijent;
  struct sockaddr_in adresa;
  struct sigaction   sa;

  setbuf(stdout, NULL);

  /* Rukovatelj SIGCHLD za pokupljenje zombi djece.
   * SA_RESTART -- prekinut accept se sam restarta. */
  sa.sa_handler = rukovatelj_sigchld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);

  /* Rukovatelji za uredno zaustavljanje: SIGTERM (dijete javi
   * "KRAJ") i SIGINT (korisnik Ctrl+C). Bez SA_RESTART -- zelimo
   * da accept vrati EINTR pa da izadjemo iz petlje. */
  sa.sa_handler = rukovatelj_zaustavi;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT,  &sa, NULL);

  fd_server = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_server < 0) { perror("socket"); exit(EXIT_FAILURE); }

  int opt = 1;
  setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&adresa, 0, sizeof(adresa));
  adresa.sin_family      = AF_INET;
  adresa.sin_addr.s_addr = htonl(INADDR_ANY);
  adresa.sin_port        = htons(PORT);

  if (bind(fd_server, (struct sockaddr *)&adresa, sizeof(adresa)) < 0) {
    perror("bind"); exit(EXIT_FAILURE);
  }
  if (listen(fd_server, BACKLOG) < 0) {
    perror("listen"); exit(EXIT_FAILURE);
  }

  printf("Server slusa na portu %d (PID %d)\n", PORT, (int)getpid());

  while (!zaustavi) {
    fd_klijent = accept(fd_server, NULL, NULL);
    if (fd_klijent < 0) {
      if (errno == EINTR) continue;   /* prekinut signalom */
      perror("accept");
      continue;
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      close(fd_klijent);
      continue;
    }

    if (pid == 0) {
      /* dijete: ne treba mu fd_server */
      close(fd_server);
      posluzi_klijenta(fd_klijent);
      close(fd_klijent);
      exit(EXIT_SUCCESS);
    }

    /* roditelj: ne treba mu fd_klijent */
    close(fd_klijent);
  }

  printf("Server izlazi.\n");
  close(fd_server);
  return 0;
}
