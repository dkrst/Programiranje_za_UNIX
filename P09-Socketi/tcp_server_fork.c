/* TCP echo server koji opsluzuje vise klijenata istovremeno,
 * koristenjem fork-a za svakog klijenta.
 *
 * Razlika u odnosu na tcp_server.c: nakon accept-a, glavni
 * proces pozove fork. Dijete preuzme komunikaciju s klijentom
 * (kompletni razgovor odvija se u djetetu), a roditelj odmah
 * ide na novi accept.
 *
 * Ovaj obrazac koristi fundamentalna svojstva iz P05:
 *   - dijete naslijedi sve otvorene deskriptore (ukljucujuci
 *     fd_klijent)
 *   - roditelj i dijete neovisno rade na razlicitim socketima
 *
 * Kako ne bismo ostavljali zombi procese, hvatamo SIGCHLD i u
 * rukovatelju pozovemo waitpid (vidi P06).
 */
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

static void rukovatelj_sigchld(int sig) {
  (void)sig;
  /* pokupi sve gotove djecu (neblokirajuci) */
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

static void posluzi_klijenta(int fd_klijent) {
  char    buffer[1024];
  ssize_t n;

  /* Citamo dok klijent ne prekine vezu */
  while ((n = read(fd_klijent, buffer, sizeof(buffer))) > 0) {
    write(fd_klijent, buffer, n);   /* echo */
  }
  /* read vraca 0 kad druga strana zatvori vezu */
}

int main(void) {
  int                fd_server, fd_klijent;
  struct sockaddr_in adresa;
  struct sigaction   sa;

  setbuf(stdout, NULL);

  /* Rukovatelj SIGCHLD za pokupljenje zombi djece */
  sa.sa_handler = rukovatelj_sigchld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);

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

  printf("Multi-klijent echo server slusa na portu %d (PID %d)\n",
         PORT, (int)getpid());

  while (1) {
    fd_klijent = accept(fd_server, NULL, NULL);
    if (fd_klijent < 0) {
      if (errno == EINTR) continue;   /* prekinut signalom, idi opet */
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
      printf("[%d] novi klijent\n", (int)getpid());
      posluzi_klijenta(fd_klijent);
      close(fd_klijent);
      printf("[%d] klijent otisao\n", (int)getpid());
      exit(EXIT_SUCCESS);
    }

    /* roditelj: ne treba mu fd_klijent */
    close(fd_klijent);
  }

  close(fd_server);
  return 0;
}
