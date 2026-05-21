# Socketi

U svim primjerima do sada, procesi i niti međusobno su komunicirali unutar jednog računala — putem datoteka, cjevovoda, zajedničke memorije, redova poruka, signala ili semafora.

Velika većina današnjih programa, međutim, mora komunicirati i preko mreže — web preglednik s web serverom, baza podataka s aplikacijom, mikroservisi međusobno. Sučelje kojim se to radi u UNIX-u zove se **socket**, a definirano je 1983. godine u sklopu sustava 4.2BSD UNIX [2], kao dio integracije TCP/IP protokola u jezgru. Tim Billa Joya na Sveučilištu California, Berkeley osmislio ga je kao prirodno proširenje UNIX filozofije *"sve je datoteka"* — socket je u biti deskriptor datoteke nad kojim radimo `read` i `write`, samo što ga stvaramo posebnim sistemskim pozivom, a s druge strane veze se, u ovom slučaju, nalazi drugi proces na udaljenom računalu. Sučelje je gotovo bez izmjena preuzeto u POSIX standard, a danas je dio svakog modernog operacijskog sustava: Linux, *BSD, macOS, Solaris, čak i Windows (gdje je `Winsock` API gotovo identičan kopiji BSD socketa). Drugim riječima, sve što ovdje učimo o UNIX socketima vrijedi praktički univerzalno. Po tome socketi nisu posebnost — UNIX, započet u Bell Labs-u 1969., izvor je iznenađujuće velikog broja koncepata koje danas smatramo univerzalnima u računarstvu (npr. hijerarhijski datotečni sustav), od kojih su neki stari više od pola stoljeća!

U ovom poglavlju ćemo obraditi dvije domene socketa:

- **UNIX domain sockets** (`AF_UNIX`) — komunikacija između procesa na istom računalu, gdje "adresu" čini putanja u datotečnom sustavu.
- **Network sockets** (`AF_INET`) — TCP/IP komunikacija preko mreže (ili lokalno preko `127.0.0.1`), gdje adresu čine IP adresa i port.

Cilj nije iscrpno pokriti mrežno programiranje. Mrežno programiranje tema je dovoljno široka i opsežna za cijelu knjigu i daleko premašuje opseg ove skripte. U ovom poglavlju dati ćemo okvir za razumijevanje koncepta socketa i poticaj čitatelju da nastavi učiti i istraživati mehanizme mrežne komunikacije, koji su postali temelj modernog društva koje praktički počiva na ideji potpune povezanosti.

## Socket kao deskriptor

Prije nego dublje zaronimo u svijet adresa, portova i paketa, podsjetimo se još jednom da socket nije ništa drugo nego deskriptor datoteke — što je zapravo i logično ako znamo da je na UNIX-u sve datoteka. Funkcija `socket()` vraća `int` — isti tip podatka koji dobijemo kada s `open()` otvorimo datoteku, ili s `pipe()` stvorimo cjevovod. Nad njim možemo zvati `read()`, `write()` i `close()` baš kao na običnoj datoteci. Sve što smo do sada naučili o deskriptorima vrijedi i ovdje.

Razlika je samo u tome *kako* deskriptor stvaramo i kako ga povezujemo s drugom stranom komunikacije. Umjesto `open(putanja, ...)`, koristimo niz funkcija:

```c
int socket(int domena, int tip, int protokol);
int bind(int fd, const struct sockaddr *addr, socklen_t len);
int listen(int fd, int backlog);
int accept(int fd, struct sockaddr *addr, socklen_t *len);
int connect(int fd, const struct sockaddr *addr, socklen_t len);
int close(int fd);
```

Odgodimo na kratko detaljan opis argumenata i povratnih vrijednosti funkcija i promotrimo tipičan tijek razgovora između dvije strane:

```
SERVER                          KLIJENT
------                          -------
socket()
bind()
listen()
accept()  --- blokira ---       socket()
          <-- connect() ----    connect()
read()  <-- write() ------      write()
write() --- read() ------>      read()
close()                         close()
```

Server "otvara dućan" pozivima `socket`/`bind`/`listen`/`accept`, dok klijent samo poziva `socket`/`connect` i spaja se na otvorenu konekciju na kojoj server čeka klijente. Nakon što se veza jednom uspostavi, obje strane imaju deskriptor datoteke putem kojeg mogu "razgovarati" `read` i `write` funkcijama koje već dobro poznajemo.

Pogledajmo sad detaljnije svaku od ovih funkcija.

**`socket`** — stvara novi socket. Argumenti su:

- **`domena`** — `AF_UNIX` za lokalne sockete ili `AF_INET` za TCP/IP nad IPv4 (postoji i `AF_INET6` za IPv6).
- **`tip`** — najčešće `SOCK_STREAM` (pouzdana, tokom-orijentirana komunikacija, kao TCP) ili `SOCK_DGRAM` (paketi, kao UDP). U ovoj skripti koristit ćemo isključivo `SOCK_STREAM`.
- **`protokol`** — gotovo uvijek `0`, što znači "izaberi zadani protokol za ovu kombinaciju domene i tipa".

Povratna vrijednost je deskriptor datoteke (`int >= 0`) za novostvoreni socket, ili `-1` uz postavljen `errno` u slučaju greške.

**`bind`** — vezuje socket za konkretnu adresu. Koristi ga server da kaže "Ja sam taj koji sluša na ovoj adresi". Argumenti su:

- **`fd`** — deskriptor socketa kojeg vežemo (dobiven prethodnim pozivom `socket`-a).
- **`addr`** — pokazivač na strukturu adrese. Konkretni tip strukture ovisi o domeni (`struct sockaddr_un` za `AF_UNIX`, `struct sockaddr_in` za `AF_INET`), a prilikom samog poziva tip se uvijek postavlja (cast operator) na `struct sockaddr *`.
- **`len`** — veličina strukture adrese u bajtovima, najčešće `sizeof(adresa)`.

Povratna vrijednost je `0` u slučaju uspjeha, `-1` u slučaju greške.

**`listen`** — označava socket kao "slušajući", odnosno spreman primati dolazne veze. Argumenti su:

- **`fd`** — deskriptor socketa (server-side).
- **`backlog`** — koliko klijenata sustav smije držati u redu čekanja prije nego što ih server prihvati pozivom `accept`. Tipično 5 do 128.

Povratna vrijednost je `0` u slučaju uspjeha, `-1` u slučaju greške.

**`accept`** — prihvaća sljedećeg klijenta iz reda čekanja. **Blokira** pozivajuću nit (ili proces) dok klijent ne stigne. Argumenti su:

- **`fd`** — deskriptor slušajućeg socketa.
- **`addr`** — izlazni argument, pokazivač na strukturu u koju funkcija upiše adresu klijenta koji se spojio (ili `NULL` ako nas to ne zanima).
- **`len`** — ulazno-izlazni argument: na ulazu mu predajemo veličinu strukture `addr`, na izlazu funkcija u njega upiše stvarnu veličinu zapisane adrese.

**Važno**: povratna vrijednost funkcije `accept` je novi deskriptor datoteke — onaj koji će server koristiti za "razgovor" s klijentom. Deskriptor `fd`, koji je argument funkcije, ostaje i dalje otvoren i možemo ga ponovo koristiti u sljedećem pozivu `accept` za prihvaćanje novog klijenta. Ovaj detalj je ključan za razumijevanje logike rada servera koji "osluškuje" dolazne pozive na socketu, osobito ukoliko server opslužuje više klijenata.

**`connect`** — koristi ga klijent da se spoji na server. Argumenti su isti kao kod `bind`-a: `fd` socketa, `addr` adresa servera, `len` njena veličina. Povratna vrijednost: `0` u slučaju uspjeha (veza uspostavljena), `-1` u slučaju greške (npr. ukoliko s druge strane ne postoji server koji na toj adresi očekuje dolazne veze).

**`close`** — zatvara socket. Ako je u tijeku razgovor, druga strana će dobiti EOF (`read` vraća `0`) i znat će da je veza prekinuta.

Ilustrirajmo korištenje opisanih funkcija na konkretnim primjerima.

## UNIX domain socketi

UNIX domain socketi (engl. *UNIX domain sockets*, kraće UDS) služe za komunikaciju između procesa **na istom računalu**. Adresa socketa je putanja u datotečnom sustavu — kad server pozove `bind()`, na disku se stvara posebna datoteka, preko koje klijenti mogu pronaći server. Tip datoteke je *socket*, jedan od sedam tipova datoteka o kojima smo već pisali u ranijim poglavljima. Tipičan izlist datoteke socket s `ls -l` izgleda ovako:

```
$ ls -l /tmp/uds_primjer
srwxr-xr-x 1 user user 0 May 20 08:54 /tmp/uds_primjer
```

Prvi znak ispisa, slovo `s`, govori nam da se radi o datoteci tipa socket. Veličina je `0` jer se u socketu podaci zapravo nikada stvarno ne pohranjuju — datoteka služi samo kao "imenovani" završetak komunikacije, a sav promet ide kroz interne strukture jezgre. Naredba `file` također će prepoznati i ispisati tip datoteke:

```
$ file /tmp/uds_primjer
/tmp/uds_primjer: socket
```

UDS su brži i jednostavniji od TCP/IP socketa za lokalnu komunikaciju jer ne idu kroz mrežni stog jezgre — sva razmjena ide kroz interne strukture jezgre. Mnogi sistemski servisi (X11, Docker, PostgreSQL) ih koriste za lokalnu komunikaciju klijenata s lokalnim serverom.

Adresa UDS-a definirana je strukturom `struct sockaddr_un` iz `<sys/un.h>`:

```c
struct sockaddr_un {
    sa_family_t sun_family;     /* uvijek AF_UNIX */
    char        sun_path[108];  /* putanja, npr. "/tmp/moj_socket" */
};
```

> **Zašto baš 108 znakova?** Veličina od 108 znakova povijesno je naslijeđe iz 4.2BSD-a (1983.) i zadržana je radi binarne kompatibilnosti. POSIX standard ne propisuje točnu veličinu — samo da mora biti barem 92 znaka — pa različite implementacije variraju (Linux/macOS/FreeBSD koriste 108, neki UNIX sustavi minimalnih 92). U praksi je tipična putanja dovoljno kratka da ovo nije ograničenje, ali u nekim modernim okolnostima (npr. duboke putanje u Dockerima ili Kubernetesima) ovo zna biti izvor problema.

### Primjer: `uds_server` i `uds_klijent`

Jednostavan klijent/server primjer korištenja UNIX domain socketa.

- [**`uds_server.c`**](uds_server.c) — slušatelj na `/tmp/uds_primjer`. Prima jednu poruku od svakog klijenta, ispiše ju i vrati klijentu (echo), pa prekine vezu. Iznimka: kad primi `"KRAJ"`, umjesto echoa odgovori `"U REDU -- IZLAZIM!"` i izlazi.

  ```c
  #define PUTANJA "/tmp/uds_primjer"
  #define BACKLOG 5

  int main(void) {
      int                fd_server, fd_klijent;
      struct sockaddr_un adresa;
      char               buffer[256];
      ssize_t            n;
      int                kraj = 0;
      const char        *poruka_kraja = "U REDU -- IZLAZIM!";

      setbuf(stdout, NULL);

      fd_server = socket(AF_UNIX, SOCK_STREAM, 0);

      /* Brisemo datoteku (socket) ako vec postoji */
      unlink(PUTANJA);

      memset(&adresa, 0, sizeof(adresa));
      adresa.sun_family = AF_UNIX;
      strncpy(adresa.sun_path, PUTANJA, sizeof(adresa.sun_path) - 1);

      bind(fd_server, (struct sockaddr *)&adresa, sizeof(adresa));
      listen(fd_server, BACKLOG);

      printf("Server slusa na %s\n", PUTANJA);

      while (!kraj) {
          fd_klijent = accept(fd_server, NULL, NULL);
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
      unlink(PUTANJA);
      return 0;
  }
  ```

  Promotrimo redoslijed poziva. `socket()` stvara socket, ali on još nije ni za što vezan — to je samo "neaktivan" deskriptor. `bind()` mu daje adresu (`/tmp/uds_primjer`) i u datotečnom sustavu stvara datoteku tog tipa. `listen()` označava socket kao "slušajući" i postavlja red duljine `BACKLOG` za klijente koji čekaju da budu prihvaćeni. Tek `accept()` vraća *novi* deskriptor za konkretnu vezu s jednim klijentom — i blokira dok takav klijent ne stigne.

  Bitno je razumjeti razliku između `fd_server` i `fd_klijent`. `fd_server` je *slušajući* socket — koristimo ga samo za `accept`-anje novih veza, ne za razgovor. `fd_klijent` je *konektirani* socket s konkretnim klijentom, kroz njega ide read/write razgovor. Server može tako simultano držati otvorene mnoge `fd_klijent`-e ako tako organizira opslugu.

  Petlja se prekida kada `strncmp(buffer, "KRAJ", 4) == 0` — dakle kad nam stigne poruka koja počinje znakovima `KRAJ`. Nakon izlaska iz petlje, zatvaramo slušajući socket i `unlink`-om brišemo socket datoteku, ostavljajući datotečni sustav čistim.

- [**`uds_klijent.c`**](uds_klijent.c) — spaja se, šalje poruku zadanu kao argument, čita odgovor servera i ispisuje ga.

  ```c
  int main(int argc, char *argv[]) {
      int                fd;
      struct sockaddr_un adresa;
      char               buffer[256];
      ssize_t            n;

      if (argc != 2) {
          fprintf(stderr, "Koristenje: %s \"poruka\"  (KRAJ za prekid izvrsavanja servera)\n", argv[0]);
          exit(EXIT_FAILURE);
      }

      fd = socket(AF_UNIX, SOCK_STREAM, 0);

      memset(&adresa, 0, sizeof(adresa));
      adresa.sun_family = AF_UNIX;
      strncpy(adresa.sun_path, PUTANJA, sizeof(adresa.sun_path) - 1);

      connect(fd, (struct sockaddr *)&adresa, sizeof(adresa));
      write(fd, argv[1], strlen(argv[1]));

      /* Procitaj odgovor servera */
      n = read(fd, buffer, sizeof(buffer) - 1);
      if (n > 0) {
          buffer[n] = '\0';
          printf("Odgovor: %s\n", buffer);
      }

      close(fd);
      return 0;
  }
  ```

  Klijent je kraći jer ne treba `bind` ni `listen` — ne čeka da mu se netko spoji, nego se sam spaja. `connect()` traži postojeću adresu i, ako uspije, ostavlja socket spreman za razgovor.

  Pokretanje (u dva terminala):

  ```
  Terminal A:                          Terminal B:
  $ ./uds_server                       $ ./uds_klijent "Pozdrav!"
  Server slusa na /tmp/uds_primjer     Odgovor: Pozdrav!
  Primljeno: Pozdrav!                  $ ./uds_klijent "Kako si?"
  Primljeno: Kako si?                  Odgovor: Kako si?
  Primljeno: KRAJ                      $ ./uds_klijent "KRAJ"
  Server izlazi.                       Odgovor: U REDU -- IZLAZIM!
  ```

Na primjeru se otvara nekoliko zanimljivih pitanja oko životnog vijeka socket datoteke. Razmotrimo ih sad detaljnije.

**Može li socket datoteka u datotečnom sustavu postojati prije nego što naš server pozove `bind()`?** Odgovor je da može — sistemski poziv `mknod(2)` može stvoriti praznu datoteku tipa socket s odgovarajućim modom (`S_IFSOCK`). Takva datoteka, međutim, **nije povezana ni s jednim procesom**, što ima dvije važne posljedice. Prvo, nitko ne sluša na njoj, pa pokušaj klijenta da se na nju spoji neće uspjeti. Drugo — što je važnije za naš primjer — naš server **ne može jednostavno "preuzeti" tu postojeću datoteku za slušanje**. Ako server pokuša pozvati `bind()` na adresu na kojoj već postoji bilo kakva datoteka (uključujući i tu praznu socket datoteku), `bind()` vraća grešku `EADDRINUSE` ("Address already in use"). Jezgra ne razlikuje "živu" socket datoteku stvorenu prethodnim `bind`-om od one stvorene `mknod`-om — za nju je svaka postojeća datoteka prepreka.

Upravo iz ovog razloga dobra je programerska praksa da na kraju korištenja, nakon što nam socket više nije potreban, server obriše socket koji je koristio. Ovo radimo i u našem primjeru: u posljednjim redcima server poziva `close(fd_server)` te nakon toga `unlink(PUTANJA)`. Međutim, postoji mogućnost i da server prekine izvršavanje i bez "urednog" izlaska, npr. u slučaju prekida signalom, ili ukoliko teta čistačica zatreba slobodnu utičnicu za usisač pa naš server (računalo) jednostavno isključi iz struje. U svim tim slučajevima `unlink` se nikad ne pozove i socket datoteka ostaje u datotečnom sustavu, kao "siroče". Sljedeće pokretanje servera tada ne bi moglo proći `bind()`. Upravo zato u našem kodu na **početku** servera, prije `bind`-a, pozivamo `unlink(PUTANJA)` — kao osiguranje od bilo kakvog zaostatka iz prethodnih pokretanja, neovisno o tome jesu li završila uredno ili ne.

**Možemo li, umjesto našeg `uds_klijent`-a, koristiti neki drugi alat da se spojimo na socket i pošaljemo serveru poruku?** Pitanje nije samo akademsko: u stvarnim sustavima često je potrebno projektirati i implementirati serversku aplikaciju koja će opsluživati različite klijente, koje mogu razviti drugi programeri prema specifikaciji serverskog API-a (engl. *Application Programming Interface*) — sučelja putem kojeg definiramo kako komunicirati s našim serverom. S našim serverom možemo komunicirati putem bilo kojeg alata koji zna otvoriti socket i poslati u njega poruku. U sljedećem primjeru koristimo `nc` (engl. *netcat*) sa zastavicom `-U` koja `nc`-u govori da ciljna adresa nije TCP host/port nego UDS putanja:

```
$ echo "Pozdrav iz netcata!" | nc -U /tmp/uds_primjer
$ echo "KRAJ" | nc -U /tmp/uds_primjer
```

Server će obraditi obje poruke kao da su stigle iz našeg `uds_klijent`-a, i nakon druge će izaći. Slično tome se može koristiti i moćniji alat `socat`.

Pokušajte pokrenuti server te mu nakon toga iz različitih terminala pokušajte slati poruke iz našeg klijenta, iz `nc`-a, eventualno i drugih alata — i sami isprobajte kako se ponaša, koje su mu granice, što se događa ako ga dva klijenta gađaju u isto vrijeme. Ovo je odličan način da se konkretno uvjerite u sve što smo dosad opisali i da napravite vlastite zaključke o ponašanju UDS-a.

## Network socketi (TCP/IP)

Mrežna domena (`AF_INET`) koristi se za komunikaciju preko TCP/IP-a — kako između računala, tako i unutar istog računala kroz tzv. *loopback* adresu `127.0.0.1`. Adresa se sada sastoji od dvije komponente: **IP adrese** (identificira računalo u mreži) i **porta** (broj između 0 i 65535 koji identificira virtualnu pristupnu točku, tj. konkretnu aplikaciju koja tu točku koristi).

Adresa je definirana strukturom `struct sockaddr_in` iz `<netinet/in.h>`:

```c
struct sockaddr_in {
    sa_family_t    sin_family;   /* AF_INET */
    in_port_t      sin_port;     /* port (network byte order!) */
    struct in_addr sin_addr;     /* IP adresa (network byte order!) */
};
```

### Network byte order

Različita računala mogu interno predstavljati višebajtne cijele brojeve na različite načine — neki kao *little-endian* (najmanje značajan bajt prvi), drugi kao *big-endian* (najznačajniji bajt prvi). Danas u svjetskom računarstvu dominira little-endian (svi Intel i AMD procesori, većina ARM-a u uobičajenoj konfiguraciji), dok je big-endian zadržan u manjini sustava i u nekim mrežnim protokolima. Da bi komunikacija među računalima različite arhitekture radila, TCP/IP propisuje da se brojevi u mrežnim zaglavljima uvijek šalju u *network byte order*-u (što je big-endian) — pod tim nazivom misli se upravo na poredak koji se koristi *na mreži*, dogovoren standardom, neovisno o arhitekturi pošiljatelja i primatelja.

Za pretvaranje između lokalnog i mrežnog poretka koriste se funkcije:

- `htons(x)` (engl. *host to network short*) — pretvara 16-bitni broj iz lokalnog u mrežni poredak.
- `htonl(x)` (engl. *host to network long*) — pretvara 32-bitni broj.
- `ntohs(x)` i `ntohl(x)` — obrnuti smjer.

Za port (16 bita) koristimo `htons`, za IP adresu (32 bita kod IPv4) `htonl`. Ako ovo zaboravimo, na little-endian računalu (kakvi su praktički svi danas) socket će raditi "obrnuto" — bind na port 9000 zapravo će obuhvatiti potpuno drugačiji broj.

Ove funkcije uputno je koristiti **uvijek**, čak i kad znamo da radimo kod na sustavu na kojem je lokalni i mrežni poredak značajnih znamenki isti, a `htons`/`htonl` pretvaranje ne radi ništa (na big-endian sustavu, gdje su lokalni i mrežni poredak isti, `htons`/`htonl` su zapravo no-op i broj prolazi neizmijenjen). Pozivom ovih funkcija u svakom slučaju jamčimo da se adresa uvijek pretvori na ispravan način, neovisno o tome koji byte order koristi naš sustav, čime osiguravamo prenosivost koda na proizvoljnu arhitekturu.

Prije nego napišemo primjer, uvedimo još dvije pomoćne funkcije koje će nam trebati. Za pretvaranje IP adrese iz teksta u binarni oblik koristi se `inet_pton`, a za obrnuti smjer `inet_ntop`:

```c
int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
```

- `af` je adresna familija — kod nas uvijek `AF_INET` za IPv4.
- `src` je izvor (string kod `inet_pton`, binarna struktura `in_addr` kod `inet_ntop`).
- `dst` je odredište (obrnuto od `src`).
- `size` (samo kod `inet_ntop`) je veličina odredišnog buffera. Za IPv4 dovoljan je `INET_ADDRSTRLEN` (16 bajtova).

Treća pomoćna funkcija je `setsockopt`, kojom postavljamo razne opcije nad socketom:

```c
int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);
```

U primjeru ćemo ju koristiti s opcijom `SO_REUSEADDR` na razini `SOL_SOCKET`, koja serveru dopušta odmah ponovno vezanje na port nakon zatvaranja — bez nje bi sustav držao port zauzetim još otprilike minutu (zbog TCP-ovog stanja `TIME_WAIT`), pa bismo prilikom restarta dobivali "Address already in use" grešku.

### Primjer: `tcp_server` i `tcp_klijent`

Strukturno identičan UDS primjeru, samo koristi mrežnu domenu umjesto UNIX domain.

- [**`tcp_server.c`**](tcp_server.c) — slušatelj na portu 9000. Prima jednu poruku od svakog klijenta, ispiše ju i vrati klijentu (echo), pa prekine vezu. Iznimka: kad primi `"KRAJ"`, umjesto echoa odgovori `"U REDU -- IZLAZIM!"` i izlazi.

  ```c
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

      /* Dopusti ponovno koristenje porta odmah nakon zatvaranja */
      int opt = 1;
      setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

      /* Pripremi adresu: slusaj na svim mreznim sucjeljima, port 9000 */
      memset(&adresa, 0, sizeof(adresa));
      adresa.sin_family      = AF_INET;
      adresa.sin_addr.s_addr = htonl(INADDR_ANY);
      adresa.sin_port        = htons(PORT);

      bind(fd_server, (struct sockaddr *)&adresa, sizeof(adresa));
      listen(fd_server, BACKLOG);

      printf("Server slusa na portu %d\n", PORT);

      while (!kraj) {
          fd_klijent = accept(fd_server, NULL, NULL);
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
  ```

  Logika je *točno* ista kao u UDS serveru. Razlikuju se samo dvije stvari:

  - **Tip strukture adrese**: `struct sockaddr_in` umjesto `struct sockaddr_un`. Polja su drugačija — par (IP, port) umjesto putanje.
  - **`INADDR_ANY`** u `sin_addr.s_addr` znači "slušaj na svim mrežnim sučeljima ovog računala". Ako bismo umjesto toga stavili konkretnu IP adresu (npr. `127.0.0.1`), server bi slušao samo na loopback-u, ne na vanjskim mrežnim adresama.

  I nema više `unlink`-a — kod TCP-a nema socket datoteke u datotečnom sustavu o kojoj treba brinuti, port se automatski oslobađa kad proces izađe.

- [**`tcp_klijent.c`**](tcp_klijent.c) — spaja se, šalje poruku zadanu kao argument, čita odgovor servera i ispisuje ga. Drugi opcionalni argument je IP servera; ako je izostavljen, koristi se `127.0.0.1`.

  ```c
  int main(int argc, char *argv[]) {
      int                fd;
      struct sockaddr_in adresa;
      const char        *poruka = argv[1];
      const char        *ip     = (argc == 3) ? argv[2] : "127.0.0.1";
      char               buffer[256];
      ssize_t            n;

      if (argc < 2 || argc > 3) {
          fprintf(stderr,
                  "Koristenje: %s \"poruka\" [ip]  (KRAJ za prekid izvrsavanja servera)\n",
                  argv[0]);
          exit(EXIT_FAILURE);
      }

      fd = socket(AF_INET, SOCK_STREAM, 0);

      memset(&adresa, 0, sizeof(adresa));
      adresa.sin_family = AF_INET;
      adresa.sin_port   = htons(PORT);

      /* inet_pton: pretvori IP iz teksta u binarni oblik */
      inet_pton(AF_INET, ip, &adresa.sin_addr);

      connect(fd, (struct sockaddr *)&adresa, sizeof(adresa));
      write(fd, poruka, strlen(poruka));

      /* Procitaj odgovor servera */
      n = read(fd, buffer, sizeof(buffer) - 1);
      if (n > 0) {
          buffer[n] = '\0';
          printf("Odgovor: %s\n", buffer);
      }

      close(fd);
      return 0;
  }
  ```

  Pokretanje (u dva terminala):

  ```
  Terminal A:                          Terminal B:
  $ ./tcp_server                       $ ./tcp_klijent "Pozdrav!"
  Server slusa na portu 9000           Odgovor: Pozdrav!
  Primljeno: Pozdrav!                  $ ./tcp_klijent "Kako si?"
  Primljeno: Kako si?                  Odgovor: Kako si?
  Primljeno: KRAJ                      $ ./tcp_klijent "KRAJ"
  Server izlazi.                       Odgovor: U REDU -- IZLAZIM!
  ```

  Ako pokrenemo klijent na drugom računalu u istoj mreži, predamo mu IP adresu servera kao drugi argument: `./tcp_klijent "Pozdrav!" 192.168.1.42`.

## Više klijenata istovremeno

Naš `tcp_server` ima jednu očitu manu: dok poslužuje jednog klijenta, svi ostali čekaju. Ako klijentova operacija dugo traje (npr. server treba računati nešto složeno), ostali klijenti su blokirani. U produkciji to gotovo uvijek nije prihvatljivo — moramo opslužiti više klijenata paralelno.

Postoji nekoliko obrazaca kojima se to postiže.

### Pristup s `fork`-om

Najjednostavniji pristup, koji se prirodno nadovezuje na sve što znamo iz P05: čim `accept` vrati novi deskriptor, glavni proces pozove `fork`. Dijete preuzme razgovor s klijentom, roditelj odmah pozove sljedeći `accept`.

Ovo je dosta napredan primjer — kombinira fork, signale, runtime stanje koje signali mijenjaju, i kontroliran exit umjesto naglog prekida. Sve te tehnike pokriveni su pojedinačno u prethodnim poglavljima skripte, pa pretpostavljamo da je čitatelj do sada s njima upoznat. Konkretno:

- Razgovor s klijentom u dijetetu je **echo petlja** — sve što primi vraća natrag, dok klijent ne zatvori vezu ili pošalje `"KRAJ"`. Na `"KRAJ"` dijete odgovara s `"U REDU -- IZLAZIM!"` i pošalje signal `SIGTERM` roditelju.
- Roditelj hvata **`SIGTERM`** (od djeteta) i **`SIGINT`** (Ctrl+C korisnika) istim handlerom, koji postavlja zastavu `zaustavi`. Kada glavna `accept` petlja vidi da je zastava postavljena, uredno izlazi iz petlje i ispiše `Server izlazi.`. Time umjesto naglog prekida (kakav bismo dobili bez handlera za SIGINT) imamo **clean exit** — server uredno zatvori slušajući socket i izađe, a budući da je `SO_REUSEADDR` postavljen, port je odmah ponovo dostupan za sljedeće pokretanje.
- **`SIGCHLD`** se hvata kao i prije, kako se ne bi gomilali zombi procesi nakon završetka djece (P06).

- [**`tcp_server_fork.c`**](tcp_server_fork.c) — varijanta TCP servera s `fork`-om za svakog novog klijenta, s echo komunikacijom, podrškom za `"KRAJ"` i clean exit na Ctrl+C.

  Cijela razlika u odnosu na `tcp_server.c` je u glavnoj petlji i u funkciji `posluzi_klijenta`:

  ```c
  while (!zaustavi) {
      fd_klijent = accept(fd_server, NULL, NULL);
      if (fd_klijent < 0) {
          if (errno == EINTR) continue;    /* prekinut signalom */
          perror("accept");
          continue;
      }

      pid_t pid = fork();
      if (pid == 0) {
          /* dijete: ne treba mu slusajuci socket */
          close(fd_server);
          posluzi_klijenta(fd_klijent);
          close(fd_klijent);
          exit(EXIT_SUCCESS);
      }
      /* roditelj: ne treba mu klijentov socket */
      close(fd_klijent);
  }
  ```

  Funkcija `posluzi_klijenta` čita od klijenta i sve mu vraća natrag (echo), dok klijent ne zatvori vezu ili pošalje `"KRAJ"`:

  ```c
  static void posluzi_klijenta(int fd_klijent) {
      char        buffer[256];
      ssize_t     n;
      const char *poruka_kraja = "U REDU -- IZLAZIM!";

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
  ```

  Ovaj obrazac koristi dvije važne osobine UNIX-a koje smo već upoznali: nakon `fork`-a, **dijete naslijedi sve otvorene deskriptore** roditelja (P05), pa tako i `fd_klijent`. I roditelj i dijete inicijalno imaju otvoren `fd_klijent`, zbog čega oboje moraju pozvati `close` — dijete kad završi razgovor, roditelj odmah jer mu nije potreban. Ako roditelj zaboravi zatvoriti svoj primjerak, deskriptor će ostati otvoren u procesu i klijent neće prepoznati da je veza zatvorena.

  Postavljanje handlera za signale podsjeća na ono iz P06. Za `SIGCHLD` koristimo `SA_RESTART`, jer ne želimo da `SIGCHLD` (koji stiže svaki put kad neko dijete završi) prekida našu `accept` petlju:

  ```c
  static void rukovatelj_sigchld(int sig) {
      (void)sig;
      while (waitpid(-1, NULL, WNOHANG) > 0)
          ;
  }

  sa.sa_handler = rukovatelj_sigchld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);
  ```

  Za `SIGTERM` (koji nam dijete šalje pri primitku "KRAJ") i `SIGINT` (Ctrl+C) koristimo **isti** handler — oba znače "zaustavi se" — i **bez** `SA_RESTART`-a, jer baš želimo da `accept` vrati grešku `EINTR`:

  ```c
  static volatile sig_atomic_t zaustavi = 0;

  static void rukovatelj_zaustavi(int sig) {
      (void)sig;
      zaustavi = 1;
  }

  sa.sa_handler = rukovatelj_zaustavi;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT,  &sa, NULL);
  ```

  Kad signal stigne, handler postavi `zaustavi = 1`. `accept` se vrati s greškom `EINTR`, u petlji to prepoznamo i nastavimo na sljedeću iteraciju, ali `while (!zaustavi)` provjera u sljedećem prolazu vidi da treba izaći. Tako uredno zatvorimo slušajući socket i izađemo iz `main`-a, što je standardna definicija *clean exit*-a.

  Test s nekoliko klijenata paralelno:

  ```
  Terminal A:                          Terminal B:
  $ ./tcp_server_fork                  $ ./tcp_klijent "Klijent A"
  Server slusa na portu 9000 ...       Odgovor: Klijent A
  [PID 1235] Primljeno: Klijent A      $ ./tcp_klijent "Klijent B"
  [PID 1236] Primljeno: Klijent B      Odgovor: Klijent B
  [PID 1237] Primljeno: KRAJ           $ ./tcp_klijent "KRAJ"
  Server izlazi.                       Odgovor: U REDU -- IZLAZIM!
  ```

  Probajte ovaj server pokrenuti, spojiti se s nekoliko klijenata, i pritisnuti **Ctrl+C** u terminalu servera — uvjerit ćete se da server uredno izlazi (ispisuje "Server izlazi.") umjesto da bude nasilno prekinut. Bez `SIGINT` handlera, `Ctrl+C` bi proces prekinuo izvana, bez ikakvog clean-up koda.

  Svaki klijent dobiva svoj proces, paralelno se opslužuju, ne čekaju jedan drugog.

### Alternativni pristupi

`fork` nije jedini način. Spomenimo ukratko alternativne obrasce, koje nećemo razrađivati kroz primjer:

- **Niti** (P08): umjesto `fork`-a, glavna nit za svakog klijenta stvori novu nit kroz `pthread_create`. Niti su lakše od procesa (manje memorije, brže stvaranje), ali zahtijevaju pažljivu sinkronizaciju ako dijele bilo kakvo stanje. Klasičan obrazac za servere srednje veličine.

- **Multipleksiranje I/O** (`select`, `poll`, `epoll`): jedan proces (bez fork-a, bez niti) pomoću sistemskih poziva `select`, `poll` ili `epoll` istovremeno prati više deskriptora i radi samo s onima na kojima ima podataka. Ovaj pristup omogućuje vrlo velik broj istovremenih veza (deseci tisuća) s minimalnim resursima, ali je programski složeniji jer cijeli server radi u jednoj petlji koja žonglira između svih veza. Koriste ga visokoperformantni serveri (`nginx`, `redis`, `node.js`).

- **Thread pool**: kombinacija — pri pokretanju servera stvori se fiksan broj niti koje iz reda preuzimaju novodošle klijente. Izbjegava se trošak stalnog stvaranja niti za svakog klijenta, ali se zadržava paralelnost.

Izbor pristupa ovisi o očekivanom opterećenju, vrsti rada koji server radi za klijenta (CPU vs I/O), i složenosti koju smo spremni unijeti u kod.

## Prevođenje

```
$ make all
```

Pokrenite svaki server u jednom terminalu, klijente u drugima.

Za UNIX domain primjer, datoteka `/tmp/uds_primjer` ostat će na disku ako server nasilno prekinemo (Ctrl+C). Sljedeće pokretanje servera ju automatski uklanja (`unlink`), ali možete je i ručno obrisati: `rm -f /tmp/uds_primjer`.

Za TCP primjere, koristimo port 9000. Ako vam port nije slobodan (zauzeo ga je drugi proces), promijenite konstantu `PORT` u izvornom kodu i prevedite ponovno.

## Što smo zapravo radili

- **Socket** je generalizacija deskriptora datoteke za komunikaciju između procesa, lokalno ili preko mreže. Sve što znamo o deskriptorima iz P03 vrijedi i ovdje.
- **Domena** određuje "namespace" adresa. `AF_UNIX` koristi putanje u datotečnom sustavu, `AF_INET` koristi par (IP, port).
- **Server** prolazi kroz `socket → bind → listen → accept`. Slušajući socket (`fd_server`) različit je od konektiranog socketa (`fd_klijent`); jedan prima nove veze, drugi vodi razgovor.
- **Klijent** prolazi kroz `socket → connect`, pa razgovor.
- Nakon uspostave veze, obje strane razgovaraju kroz `read`/`write` kao na obične datoteke.
- **Mrežni byte order** je big-endian; koristimo `htons`/`htonl` za pretvaranje, inače računala različite arhitekture ne mogu razgovarati.
- Za **više klijenata istovremeno** najjednostavniji obrazac je `fork` po klijentu — ali postoje i alternative (niti, `select`/`poll`/`epoll`, thread poolovi) koje treba znati kad performanse postanu kritične.

Mrežno programiranje je obimno područje. Spomenimo nekoliko važnih tema koje nismo razrađivali:

- **UDP** (`SOCK_DGRAM`) za komunikaciju paketima bez uspostave veze.
- **IPv6** (`AF_INET6`) kao zamjenu za IPv4.
- **Sigurna komunikacija** kroz TLS/SSL (najpoznatija implementacija je OpenSSL).
- **Pravilno čitanje cijele poruke** — `read` može vratiti manje bajtova nego što smo tražili (engl. *short read*); robusan kod radi u petlji dok ne pročita željeni iznos ili dok ne dobije EOF. Naši primjeri pretpostavljaju da jedan `read` vraća cijelu poruku, što je u praksi često, ali ne i garantirano.
- **Sastavljanje protokola** — kako definirati strukturu poruka iznad obične tokom-orijentirane veze (gdje su granice poruke, kako se kodiraju tipovi podataka, ...).

Sve ovo i mnogo više obrađeno je u referencama navedenim niže.

## Bibliografija

[1] W. R. Stevens, B. Fenner, and A. M. Rudoff, *UNIX Network Programming, Volume 1: The Sockets Networking API*, 3rd ed. Boston, MA, USA: Addison-Wesley Professional, 2003.

[2] S. J. Leffler, R. S. Fabry, and W. N. Joy, "A 4.2BSD Interprocess Communication Primer," Tech. Rep. UCB/CSD-83-145, EECS Department, University of California, Berkeley, July 1983.

[3] W. R. Stevens and S. A. Rago, *Advanced Programming in the UNIX Environment*, 3rd ed. Boston, MA, USA: Addison-Wesley Professional, 2013.

[4] B. Hall, *Beej's Guide to Network Programming*. Dostupno online: https://beej.us/guide/bgnet/
