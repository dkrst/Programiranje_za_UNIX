# Komunikacija između procesa

Procesi u UNIX sustavu žive u potpuno odvojenim adresnim prostorima — varijable jednog procesa su nevidljive drugom, čak i ako je jedan proces nastao od drugog pozivom `fork()`, ili ako imaju istog zajedničkog pretka — isti proces-roditelj stvorio ih je s dva poziva `fork()`. Ova izolacija je temeljna značajka modernog operacijskog sustava: ona štiti procese međusobno (jedan ne može srušiti ili neispravno izmijeniti podatke drugog), olakšava paralelno izvršavanje, i čini sustav robusnijim. Međutim, u praksi često želimo da procesi razmjenjuju podatke — bilo da prosljeđuju rezultate, koordiniraju rad, ili dijele zajedničke resurse. Skup mehanizama koji to omogućuju zajedničkim imenom zovemo **međuprocesna komunikacija** (engl. *Inter-Process Communication*, IPC).

UNIX nudi cijelu lepezu IPC mehanizama, od najjednostavnijeg cjevovoda do složenih koncepata dijeljene memorije, ili mrežnih socketa koji omogućavaju ne samo komunikaciju između procesa na istom računalu, već i među procesima koji se izvršavaju na različitim krajevima svijeta. IPC je opsežno područje — sveobuhvatna skripta koja bi pokrila sve mehanizme i ilustrirala ih dovoljnim brojem primjera vjerojatno bi zahtijevala volumen barem jednak ovoj skripti u cjelini. Stoga ćemo se ovdje ograničiti na osnovne mehanizme, dovoljno detaljno obrađene da čitatelj stekne uvid u područje i razumijevanje osnovnih koncepata međuprocesne komunikacije, a zainteresiranog čitatelja potaknemo da sam nastavi s istraživanjem onoga što ostaje izvan dometa ovog poglavlja. Prije nego krenemo, vrijedi se kratko podsjetiti na još jedan oblik komunikacije koji smo već obradili.

## Pregled mehanizama IPC-a

UNIX ima više IPC mehanizama, svaki s vlastitim svojstvima i tipičnom primjenom:

| Mehanizam | Opseg | Tipična primjena |
|---|---|---|
| **Signali** | unutar sustava | obavještavanje procesa o događajima (asinkroni "prekidi") |
| **Anonimni cjevovodi** (engl. *pipe*) | povezani procesi | jednosmjeran tok bajtova roditelj↔dijete |
| **Imenovani cjevovodi** (FIFO) | unutar sustava | jednosmjeran tok bajtova između bilo koja dva procesa |
| **POSIX redovi poruka** (engl. *message queues*) | unutar sustava | strukturirana komunikacija s prioritetima |
| **POSIX dijeljena memorija** | unutar sustava | dijeljenje memorijskog prostora između procesa |
| **POSIX semafori** | unutar sustava | sinkronizacija pristupa zajedničkim resursima |
| **System V IPC** (poruke, semafori, dijeljena memorija) | unutar sustava | starija, ali raširena alternativa POSIX-u |
| **Memory mapping** (`mmap`) | unutar sustava | mapiranje datoteka i anonimne memorijske regije |
| **Socketi** | unutar i između sustava | komunikacija lokalna ili preko mreže |

### Signali kao oblik IPC-a

Signali, koje smo detaljno obradili u prethodnom poglavlju, ujedno su i najjednostavniji oblik međuprocesne komunikacije. Jedan proces može poslati signal drugome (sistemskim pozivom `kill()`), a primatelj na njega reagira ovisno o tome je li definirao vlastiti rukovatelj signalom. Signali su u osnovi vrlo jednostavan komunikacijski kanal — pošiljatelj ne može uz signal poslati nikakve podatke (novije inačice signala kroz `sigqueue` mogu nositi i jednu cijelobrojnu vrijednost). Komunikacija koju signalima možemo ostvariti zapravo se svodi na jednostavne obavijesti: *"dogodilo se nešto"*. Usporedimo ovo s lampicom *"Check engine"* koju često nalazimo u modernim automobilima (a nažalost, često i svijetli): automobil nam signalizira da je nastupilo neko stanje na koje trebamo obratiti pažnju, ali nam ne daje nikakve dodatne informacije i na nama je da s tim postupimo kako najbolje znamo. Za pravu razmjenu podataka trebamo bogatije mehanizme koje obrađujemo u ovom poglavlju.

## Anonimni cjevovodi

Najjednostavniji i najstariji UNIX IPC mehanizam je **anonimni cjevovod** (engl. *pipe*). Cjevovod je jednosmjerni komunikacijski kanal — niz bajtova koji jedan proces piše s jednog kraja, a drugi čita s drugog. Implementiran je kao međuspremnik (engl. *buffer*) u jezgri.

S programerske strane, cjevovod se predstavlja kao **par file deskriptora**: jedan za čitanje, drugi za pisanje. Time se cjevovod uklapa u UNIX-ovu paradigmu *"sve je datoteka"* — čitamo i pišemo iste sistemske pozive (`read`, `write`) koje smo upoznali u poglavlju o ulazno/izlaznim operacijama.

Karakteristike anonimnih cjevovoda:

- **Jednosmjerni** — podaci teku od jednog procesa prema drugom, od pisača prema čitaču (engl. *writer to reader*); za dvosmjernu komunikaciju treba dva cjevovoda.
- **Anonimni** — nemaju ime u datotečnom sustavu; postoje samo dok ih neki proces drži otvorenima preko deskriptora.
- **Samo između povezanih procesa** — pošto su anonimni, jedini način da drugi proces dobije pristup deskriptoru je da ga **naslijedi** od roditelja kroz `fork()`. Cjevovod stoga obično povezuje roditelja i dijete (ili dvoje djece istog roditelja).
- **Ograničen kapacitet** — međuspremnik je konačan (na Linuxu obično 64 KB). Pisanje u puni cjevovod blokira pisača sve dok čitač nešto ne pročita; čitanje iz praznog cjevovoda blokira čitača sve dok pisač nešto ne napiše.

### Sistemski poziv `pipe()`

```c
#include <unistd.h>

int pipe(int fd[2]);
```

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`fd`** — polje od dva cijela broja u koje funkcija upisuje dva nova file deskriptora: `fd[0]` (kraj za **čitanje**) i `fd[1]` (kraj za **pisanje**). Zgodno mnemoničko pomagalo: `0` se često asocira sa standardnim ulazom (čitanjem), `1` sa standardnim izlazom (pisanjem).

Nakon stvaranja cjevovoda, proces koji je pozvao `pipe()` drži oba kraja — i čitajući i pisajući deskriptor. U principu, ovaj proces sad može pisati u `fd[1]` i sam čitati to što je napisao iz `fd[0]`, dakle, razgovarati sam sa sobom:

![Cjevovod odmah nakon poziva pipe()](slike/cjevovod_pipe.png)

Ljude koji govore sami sa sobom obično "čudno" gledamo, a ni kod procesa ovo nema previše smisla — postoje znatno jednostavniji načini da proces sam sebi nešto dojavi (obične varijable u memoriji). Cjevovod postaje koristan tek kad ga **dijelimo s drugim procesom**, što se postiže pozivom `fork()` neposredno nakon `pipe()`-a.

Tipičan obrazac: pozovemo `pipe()` u roditelju, zatim `fork()`. Oba procesa sad imaju sve četiri "krajeve" cjevovoda (dva u roditelju, dva u djetetu — naslijeđeni). Da bi smjer bio jasan, **svaki proces zatvara onaj kraj koji ne koristi**: ako roditelj piše, zatvara `fd[0]`; ako dijete čita, zatvara `fd[1]`.

![Cjevovod nakon fork() — roditelj piše, dijete čita](slike/cjevovod_fork.png)

Time se uspostavlja jednoznačan kanal: roditeljev pisuće deskriptor je jedini koji ostaje otvoren za pisanje, a djetetov čitajući je jedini koji ostaje otvoren za čitanje. Suprotni smjer — od djeteta natrag prema roditelju — fizički postoji u jezgri, ali pošto su pripadajući deskriptori zatvoreni s obje strane, tim smjerom se ne može komunicirati. Ako trebamo komunikaciju u oba smjera, uobičajeno je rješenje stvoriti **dva** cjevovoda, svaki za svoj smjer.

### Cjevovod između roditelja i djeteta

- [**`cjev.c`**](cjev.c) — minimalan primjer cjevovoda: roditelj šalje poruku djetetu kroz cjevovod.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <sys/wait.h>

  int main(void) {
      int fd[2];
      pid_t pid;
      char buf[128];

      /* fd[0] - kraj za citanje
       * fd[1] - kraj za pisanje */
      if (pipe(fd) < 0) {
          perror("pipe");
          return 1;
      }

      pid = fork();
      if (pid < 0) {
          perror("fork");
          return 1;
      }

      if (pid == 0) {
          /* dijete - cita iz cjevovoda */
          close(fd[1]);                       /* ne treba nam pisanje */
          ssize_t n = read(fd[0], buf, sizeof(buf) - 1);
          if (n > 0) {
              buf[n] = '\0';
              printf("Dijete primilo: %s\n", buf);
          }
          close(fd[0]);
          return 0;
      }

      /* roditelj - pise u cjevovod */
      close(fd[0]);                         /* ne treba nam citanje */
      const char *poruka = "Pozdrav iz roditelja!";
      write(fd[1], poruka, strlen(poruka));
      close(fd[1]);

      wait(NULL);                           /* cekaj da dijete zavrsi */
      return 0;
  }
  ```

  Bitno je primijetiti redoslijed pozivа: prvo se otvara cjevovod (u roditelju), a tek **onda** se radi `fork()`. Time se osigurava da oba procesa naslijede iste deskriptore. Da je redoslijed obrnut, dijete ne bi imalo pristup cjevovodu.

  Zatvaranje "krivog" kraja u svakom procesu nije samo čistunska sitnica — bez toga čitač može u nekim slučajevima zauvijek čekati podatke koje samog sebe smatra "još uvijek mogućim pošiljateljem". `read` na cjevovod vraća `0` (kraj toka) tek kad **svi** deskriptori za pisanje budu zatvoreni; ako roditelj pošalje poruku ali dijete zaboravi zatvoriti svoj kopiju `fd[1]`, dijete će zauvijek blokirati u `read`-u jer cjevovod sa svoje strane "još uvijek može doći podatak".

  Pokretanje:

  ```
  $ ./cjev
  Dijete primilo: Pozdrav iz roditelja!
  ```

### Implementacija ljuskinog "pipe" operatora

Naredba ljuske `ls | wc -l` povezuje izlaz `ls`-a s ulazom `wc -l`-a kroz cjevovod. Ovaj klasičan UNIX idiom možemo implementirati i sami — uz `pipe()`, `fork()`, **`dup2()`** i `exec`. `dup2()` preusmjerava jedan deskriptor u drugi (već smo ga upoznali u poglavlju o ulazno/izlaznim operacijama). Trik je u tome da prije `exec`-a preusmjerimo standardne deskriptore na krajeve cjevovoda.

- [**`mojcjev.c`**](mojcjev.c) — implementacija naredbe `ls | wc -l` korištenjem `pipe`, `fork`, `dup2` i `exec`.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/wait.h>

  int main(void) {
      int fd[2];
      pid_t pid_ls, pid_wc;

      if (pipe(fd) < 0) {
          perror("pipe");
          return 1;
      }

      /* prvi proces: ls */
      pid_ls = fork();
      if (pid_ls < 0) { perror("fork"); return 1; }
      if (pid_ls == 0) {
          /* preusmjeri standardni izlaz na pisuci kraj cjevovoda */
          dup2(fd[1], STDOUT_FILENO);
          close(fd[0]);
          close(fd[1]);
          execlp("ls", "ls", (char *)NULL);
          perror("execlp ls");
          return 1;
      }

      /* drugi proces: wc -l */
      pid_wc = fork();
      if (pid_wc < 0) { perror("fork"); return 1; }
      if (pid_wc == 0) {
          /* preusmjeri standardni ulaz na citajuci kraj cjevovoda */
          dup2(fd[0], STDIN_FILENO);
          close(fd[0]);
          close(fd[1]);
          execlp("wc", "wc", "-l", (char *)NULL);
          perror("execlp wc");
          return 1;
      }

      /* roditelj: zatvori oba kraja i cekaj djecu */
      close(fd[0]);
      close(fd[1]);
      waitpid(pid_ls, NULL, 0);
      waitpid(pid_wc, NULL, 0);
      return 0;
  }
  ```

  Bitan detalj: roditelj **mora zatvoriti oba kraja** cjevovoda. Inače `wc` nikad ne dobije EOF (krajni proces drži pisuće deskriptore otvorenim) i program zaglavi.

  Pokretanje:

  ```
  $ ./mojcjev
  9
  ```

  (Rezultat će biti broj zapisa u trenutnom direktoriju — kao `ls | wc -l`.)

  Ovaj sažeti primjer pokazuje **kako ljuska zapravo radi pipe**: ona za svaku komponentu cjevovoda pokrene zaseban proces, a između njih postavi cjevovode kroz `dup2`. Naš program implementira točno ono što ljuska radi kad joj utipkamo `ls | wc -l`.

## Imenovani cjevovodi (FIFO)

Anonimni cjevovodi imaju jedno ozbiljno ograničenje: budući da nemaju ime u datotečnom sustavu, mogu ih koristiti samo srodni procesi (preko `fork`-a). Što ako želimo da dva potpuno nezavisna procesa — pokrenuta od strane različitih korisnika, u različitim trenutcima — komuniciraju cjevovodom?

Odgovor su **imenovani cjevovodi** (engl. *named pipes*) ili **FIFO**. Riječ je o cjevovodima koji imaju ime u datotečnom sustavu — vidljivi su naredbom `ls`, mogu se otvoriti običnim `open()`-om, i obrisati naredbom `rm` ili `unlink()`-om. S programerske strane, koriste se gotovo identično anonimnim cjevovodima: `read` i `write` rade isto, semantika blokiranja je ista, ograničenje smjera (jedan smjer po cjevovodu) je isto.

Ključna razlika u korištenju: FIFO se ne stvara `pipe()`-om, nego **`mkfifo()`-om**, i otvara običnim `open()`-om kao da je obična datoteka. Tip "FIFO" upisuje se u `st_mode` polje i-noda (sjetite se `S_ISFIFO` makroa iz poglavlja o upravljanju datotekama).

```c
#include <sys/types.h>
#include <sys/stat.h>

int mkfifo(const char *pathname, mode_t mode);
```

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`pathname`** — putanja na kojoj će biti stvoren FIFO.
- **`mode`** — prava pristupa (kao za `open` ili `mkdir`); konačna prava se dobiju kombinacijom s `umask` procesa.

Bitne karakteristike koje treba imati na umu kod FIFO-a:

- **Otvaranje blokira po defaultu**: `open()` na FIFO za čitanje blokira sve dok neki drugi proces ne otvori isti FIFO za pisanje (i obratno). Time se procesi automatski "rendez-vous"-iraju.
- **Postoje na disku** sve dok ih netko ne obriše (`unlink` ili `rm`). Ako program zaboravi to napraviti, FIFO ostaje kao "smeće" u datotečnom sustavu.
- **Više čitača/pisača**: na isti FIFO može se istovremeno spojiti više procesa, ali tada poredak primanja nije zajamčen ako više pisača istovremeno piše.

### Primjer: razmjena poruke kroz FIFO

Stvorit ćemo dva mala programa — jedan koji šalje poruku, drugi koji je prima — koji komuniciraju kroz FIFO. Kad ih pokrenemo u dvije zasebne ljuske (ili u pozadini), ovo demonstrira IPC između potpuno **nepovezanih** procesa.

- [**`fifoposalji.c`**](fifoposalji.c) — stvara FIFO ako ne postoji, otvara ga za pisanje, šalje poruku.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <errno.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/stat.h>

  #define FIFO_PATH "/tmp/moj_fifo"

  int main(int argc, char *argv[]) {
      int fd;
      const char *poruka;

      if (argc < 2)
          poruka = "Pozdrav kroz FIFO!";
      else
          poruka = argv[1];

      /* stvori FIFO ako ne postoji */
      if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
          perror("mkfifo");
          return 1;
      }

      printf("Otvaram FIFO za pisanje (cekam citatelja)...\n");
      fd = open(FIFO_PATH, O_WRONLY);
      if (fd < 0) {
          perror("open");
          return 1;
      }

      write(fd, poruka, strlen(poruka));
      printf("Poruka poslana: %s\n", poruka);

      close(fd);
      return 0;
  }
  ```

- [**`fifoprimi.c`**](fifoprimi.c) — otvara isti FIFO za čitanje i ispisuje primljenu poruku.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <errno.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/stat.h>

  #define FIFO_PATH "/tmp/moj_fifo"

  int main(void) {
      int fd;
      char buf[256];
      ssize_t n;

      /* stvori FIFO ako jos ne postoji */
      if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
          perror("mkfifo");
          return 1;
      }

      printf("Otvaram FIFO za citanje (cekam pisaca)...\n");
      fd = open(FIFO_PATH, O_RDONLY);
      if (fd < 0) {
          perror("open");
          return 1;
      }

      n = read(fd, buf, sizeof(buf) - 1);
      if (n > 0) {
          buf[n] = '\0';
          printf("Primljeno: %s\n", buf);
      }

      close(fd);
      return 0;
  }
  ```

  Pokretanje (u dvije zasebne ljuske, ili u pozadini):

  ```
  ## ljuska 1:
  $ ./fifoprimi
  Otvaram FIFO za citanje (cekam pisaca)...

  ## ljuska 2 (paralelno):
  $ ./fifoposalji "Bok iz druge ljuske!"
  Otvaram FIFO za pisanje (cekam citatelja)...
  Poruka poslana: Bok iz druge ljuske!

  ## natrag u ljusku 1:
  Primljeno: Bok iz druge ljuske!
  ```

  Provjerimo i datotečni sustav — FIFO je stvarno tu:

  ```
  $ ls -l /tmp/moj_fifo
  prw-rw-rw- 1 dkrst users 0 May  7 18:00 /tmp/moj_fifo
  ```

  Početno slovo `p` označava FIFO ("**p**ipe"). Veličina je 0 jer FIFO sam ne čuva podatke na disku — samo ime i tip. Stvarni podaci prolaze kroz međuspremnik u jezgri, kao i kod anonimnih cjevovoda.

  Kad više ne trebamo FIFO, brišemo ga kao i bilo koju drugu datoteku:

  ```
  $ rm /tmp/moj_fifo
  ```

## Dijeljena memorija

Cjevovodi i FIFO-i prirodni su za **slijedne** tokove podataka — bajt po bajt, jedna strana piše, druga čita. Kad procesi trebaju zajedno raditi nad istim podacima — npr. pet procesa istovremeno ažurira jedan brojač, ili dva procesa dijele veliku tablicu — cjevovod nije idealan jer svaka razmjena uključuje **kopiranje** kroz jezgru. **Dijeljena memorija** je drugačiji pristup: dva ili više procesa dobiju "prozor" u istu fizičku memorijsku regiju, pa pristupaju podacima izravno bez kopiranja, jednako brzo kao da je riječ o lokalnoj varijabli.

Pristupi dijeljenoj memoriji u UNIX-u dolaze u dvije inačice:

- **POSIX dijeljena memorija** — moderna, čistija, preporučena za nove programe. Identifikatori su putanje (počinju s `/`), pa imaju i vlasništvo i prava pristupa kao i obične datoteke.
- **System V dijeljena memorija** — starija, ali još uvijek raširena. Identifikatori su cjelobrojni "ključevi". Obrađujemo je kratko na kraju poglavlja.

POSIX dijeljena memorija u biti se sastoji od dva koraka: stvori se "objekt dijeljene memorije" (zapravo specijalna datoteka u memorijskom datotečnom sustavu, obično `/dev/shm`), i potom se taj objekt **mapira** u adresni prostor procesa pomoću `mmap()`. Tako svaki proces dobiva pokazivač kojim može čitati i pisati u dijeljeni dio.

```c
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

int   shm_open(const char *name, int oflag, mode_t mode);
int   shm_unlink(const char *name);
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int   munmap(void *addr, size_t length);
int   ftruncate(int fd, off_t length);
```

#### Funkcija `shm_open()`

Stvara novi (ili otvara postojeći) objekt dijeljene memorije. Ponaša se kao `open()` za obične datoteke, samo što "datoteka" živi u memorijskom datotečnom sustavu.

**Povratna vrijednost:** file deskriptor u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`name`** — ime objekta (mora počinjati s `/`, npr. `"/moj_brojac"`).
- **`oflag`** — kombinacija zastavica kao kod `open()`: `O_CREAT`, `O_RDWR`, `O_EXCL`, ...
- **`mode`** — prava pristupa (kao za `open`).

#### Funkcija `ftruncate()`

Postavlja veličinu datoteke (ili shm objekta) na zadanu vrijednost. Novostvoren shm objekt ima veličinu 0, pa ga prije mapiranja moramo "razvući" na željenu veličinu pomoću `ftruncate`.

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

#### Funkcija `mmap()`

Mapira datoteku (ili shm objekt) u adresni prostor procesa. Vraćeni pokazivač pokazuje na memorijski blok koji je *zapravo* sadržaj te datoteke — pisanje u njega odmah se reflektira na "datoteku" (a kod shm to je dijeljena memorija).

**Povratna vrijednost:** pokazivač na mapirani blok, ili `MAP_FAILED` (vrijednost `(void *)-1`) u slučaju greške.

**Argumenti:**

- **`addr`** — preferirana adresa za mapiranje; gotovo uvijek se zadaje `NULL`, što znači "neka jezgra odluči gdje".
- **`length`** — broj bajtova koji se mapira.
- **`prot`** — kombinacija dozvoljenih operacija: `PROT_READ`, `PROT_WRITE`, `PROT_EXEC`, `PROT_NONE`.
- **`flags`** — najvažnija je `MAP_SHARED` (promjene su vidljive drugim procesima i, ako je riječ o regularnoj datoteci, zapisuju se na disk) ili `MAP_PRIVATE` (proces dobiva privatnu kopiju, drugi je ne vide).
- **`fd`** — deskriptor datoteke (ili shm objekta) koja se mapira; za "anonimne" mape postavlja se `-1` uz zastavicu `MAP_ANONYMOUS`.
- **`offset`** — pomak unutar datoteke od kojeg počinje mapiranje (mora biti višekratnik veličine stranice).

#### Funkcija `munmap()`

Razmapirava ranije mapirani blok. Pokazivač na blok više ne vrijedi.

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

#### Funkcija `shm_unlink()`

Briše imenovani shm objekt iz sustava (slično `unlink`-u za datoteke). Ako je objekt još uvijek mapiran u nekom procesu, on i dalje radi sve dok ga taj proces ne `munmap`-a; tek tada se stvarno oslobađa.

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

### Primjer: dijeljena memorija bez sinkronizacije

Pokazat ćemo dva procesa koji dijele jedan cjelobrojni brojač i svaki ga povećava milijun puta. Očekivani rezultat na kraju je 2 × 1 000 000 = 2 000 000. Pokazat će se da to **nije uvijek slučaj** — radi se o klasičnom problemu *race condition*-a.

- [**`shm_demo.c`**](shm_demo.c) — dijeljeni brojač **bez** sinkronizacije.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/mman.h>
  #include <sys/wait.h>
  #include <sys/stat.h>

  #define SHM_NAME "/moj_brojac"
  #define ITERACIJA 1000000

  int main(void) {
      int fd;
      int *brojac;
      pid_t pid;

      /* stvori (ili otvori postojeci) objekt dijeljene memorije */
      fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
      if (fd < 0) { perror("shm_open"); return 1; }

      /* postavi velicinu na sizeof(int) */
      if (ftruncate(fd, sizeof(int)) < 0) { perror("ftruncate"); return 1; }

      /* mapiraj objekt u adresni prostor */
      brojac = mmap(NULL, sizeof(int),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);
      if (brojac == MAP_FAILED) { perror("mmap"); return 1; }

      *brojac = 0;

      pid = fork();
      if (pid < 0) { perror("fork"); return 1; }

      /* oba procesa povecavaju brojac istovremeno - bez sinkronizacije! */
      for (int i = 0; i < ITERACIJA; i++)
          (*brojac)++;

      if (pid == 0) {
          /* dijete */
          munmap(brojac, sizeof(int));
          close(fd);
          return 0;
      }

      /* roditelj */
      wait(NULL);
      printf("Konacna vrijednost brojaca: %d (ocekivano: %d)\n",
             *brojac, 2 * ITERACIJA);

      munmap(brojac, sizeof(int));
      close(fd);
      shm_unlink(SHM_NAME);
      return 0;
  }
  ```

  Bitno je primijetiti redoslijed: **najprije** stvorimo i mapiramo dijeljenu memoriju, **onda** pozivamo `fork()`. Tako oba procesa nakon `fork`-a nastavljaju s istim mapiranjem koje pokazuje na isti dijeljeni objekt.

  Pokretanje primjera nekoliko puta zaredom:

  ```
  $ ./shm_demo
  Konacna vrijednost brojaca: 2000000 (ocekivano: 2000000)
  $ ./shm_demo
  Konacna vrijednost brojaca: 1000000 (ocekivano: 2000000)
  $ ./shm_demo
  Konacna vrijednost brojaca: 1837421 (ocekivano: 2000000)
  $ ./shm_demo
  Konacna vrijednost brojaca: 2000000 (ocekivano: 2000000)
  ```

  Različiti pokušaji daju **različite rezultate** — ponekad točan, češće manje od očekivanog. Razlog je što operacija `(*brojac)++` na razini procesora nije atomarna; ona se dijeli na tri koraka: pročitaj trenutnu vrijednost iz memorije, povećaj je za 1, upiši natrag. Ako se dva procesa **prekriva** u tim koracima — proces A pročita vrijednost 100, prije nego što je upiše 101, proces B pročita istu vrijednost 100, povećaj na 101, upiše 101 u memoriju, pa onda i A upiše 101 — gubi se jedna inkrementacija.

  Da bismo ovaj problem riješili, trebamo nekakav mehanizam koji garantira da samo jedan proces u datom trenutku može mijenjati brojač. To je posao **sinkronizacijskih primitiva**.

## Sinkronizacija pomoću semafora

**Semafor** je sinkronizacijska primitiva koja čuva cjelobrojnu vrijednost i podržava dvije atomarne operacije:

- **`wait`** (povijesno ime: `P`) — smanji vrijednost za 1; ako bi rezultat bio negativan, blokiraj dok netko drugi ne pozove `post`.
- **`post`** (povijesno ime: `V`) — povećaj vrijednost za 1; ako su procesi blokirani u `wait`-u, jedan od njih se odblokira.

Kad se semafor inicijalizira na 1 i koristi se kao zaštita kritične sekcije, on funkcionira kao **mutex** (engl. *mutual exclusion lock*). Drugačije inicijalne vrijednosti omogućuju složenije obrasce sinkronizacije (npr. semafor inicijaliziran na `N` dopušta `N` procesa istodobno u kritičnoj sekciji).

POSIX standard nudi dvije inačice semafora:

- **Imenovane** semafore — identificirani putanjom kao i shm objekti, dostupni nepovezanim procesima.
- **Anonimne** semafore — žive u procesovoj memoriji ili u dijeljenoj memoriji, dostupni samo procesima koji ih dijele.

Mi ćemo koristiti **imenovane** semafore jer su jednostavniji za upotrebu i pristupa im se gotovo identično kao shm objektima.

```c
#include <semaphore.h>

sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
int    sem_wait(sem_t *sem);
int    sem_post(sem_t *sem);
int    sem_close(sem_t *sem);
int    sem_unlink(const char *name);
```

#### Funkcija `sem_open()`

Stvara ili otvara imenovani semafor.

**Povratna vrijednost:** pokazivač na `sem_t` u slučaju uspjeha, `SEM_FAILED` u slučaju greške.

**Argumenti:**

- **`name`** — ime semafora (kao kod shm: počinje s `/`).
- **`oflag`** — `O_CREAT`, eventualno `O_EXCL`.
- **`mode`** — prava pristupa (samo pri stvaranju).
- **`value`** — početna vrijednost (samo pri stvaranju). Za "mutex" obrazac koristi se `1`.

#### Funkcije `sem_wait()` i `sem_post()`

`sem_wait` smanjuje vrijednost semafora za 1, blokira ako je već 0. `sem_post` povećava za 1, oslobađa eventualno blokirane procese.

**Povratna vrijednost (obje):** `0` u slučaju uspjeha, `-1` u slučaju greške.

#### Funkcije `sem_close()` i `sem_unlink()`

`sem_close` zatvara *deskriptor* semafora u trenutnom procesu (semafor i dalje postoji u sustavu). `sem_unlink` briše imenovani semafor iz sustava (kao `shm_unlink` za shm objekte).

### Primjer: dijeljeni brojač sa semaforom

Sad kad imamo i `shm` i semafor, ispravljamo prethodni primjer. Brojač i dalje živi u dijeljenoj memoriji, ali je svaka inkrementacija zaštićena semaforom inicijaliziranim na 1.

- [**`shm_sem_demo.c`**](shm_sem_demo.c) — dijeljeni brojač **sa** sinkronizacijom.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/mman.h>
  #include <sys/wait.h>
  #include <sys/stat.h>
  #include <semaphore.h>

  #define SHM_NAME "/moj_brojac"
  #define SEM_NAME "/moj_sem"
  #define ITERACIJA 1000000

  int main(void) {
      int fd;
      int *brojac;
      sem_t *sem;
      pid_t pid;

      /* dijeljena memorija (kao prije) */
      fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
      if (fd < 0) { perror("shm_open"); return 1; }
      if (ftruncate(fd, sizeof(int)) < 0) { perror("ftruncate"); return 1; }
      brojac = mmap(NULL, sizeof(int),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0);
      if (brojac == MAP_FAILED) { perror("mmap"); return 1; }
      *brojac = 0;

      /* semafor - inicijalna vrijednost 1 (binarni semafor) */
      sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
      if (sem == SEM_FAILED) { perror("sem_open"); return 1; }

      pid = fork();
      if (pid < 0) { perror("fork"); return 1; }

      /* oba procesa povecavaju brojac, ali sad pod zastitom semafora */
      for (int i = 0; i < ITERACIJA; i++) {
          sem_wait(sem);                       /* udji u kriticnu sekciju */
          (*brojac)++;
          sem_post(sem);                       /* napusti kriticnu sekciju */
      }

      if (pid == 0) {
          /* dijete */
          sem_close(sem);
          munmap(brojac, sizeof(int));
          close(fd);
          return 0;
      }

      /* roditelj */
      wait(NULL);
      printf("Konacna vrijednost brojaca: %d (ocekivano: %d)\n",
             *brojac, 2 * ITERACIJA);

      sem_close(sem);
      sem_unlink(SEM_NAME);
      munmap(brojac, sizeof(int));
      close(fd);
      shm_unlink(SHM_NAME);
      return 0;
  }
  ```

  Pokretanje:

  ```
  $ ./shm_sem_demo
  Konacna vrijednost brojaca: 2000000 (ocekivano: 2000000)
  $ ./shm_sem_demo
  Konacna vrijednost brojaca: 2000000 (ocekivano: 2000000)
  $ ./shm_sem_demo
  Konacna vrijednost brojaca: 2000000 (ocekivano: 2000000)
  ```

  Sad je rezultat **uvijek točan**, neovisno o tome kako se procesi međusobno prepleću — semafor garantira da je svaka inkrementacija atomarna iz perspektive drugih procesa. Cijena je manja brzina (svaki ulazak/izlazak iz kritične sekcije ima trošak), ali kod gdje su podaci točni je gotovo uvijek bolji od bržeg koji daje pogrešne rezultate.

## POSIX redovi poruka

Cjevovodi i FIFO-i prenose **niz neformatiranih bajtova** — ako primatelj ne zna unaprijed gdje jedna poruka završava i druga počinje, mora si sam izgraditi protokol (npr. zaglavlje s duljinom poruke). **Redovi poruka** rješavaju taj problem: prenose **diskretne, atomarne poruke** s definiranim granicama. Dodatno nude i **prioritete** — poruka višeg prioriteta čita se prije poruke nižeg, neovisno o redoslijedu slanja.

POSIX redovi poruka identificiraju se imenom kao i shm/sem objekti.

```c
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

mqd_t   mq_open(const char *name, int oflag, ...);
int     mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);
ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);
int     mq_close(mqd_t mqdes);
int     mq_unlink(const char *name);
```

#### Funkcija `mq_open()`

Otvara (ili stvara) red poruka. Pri stvaranju treba dodatne argumente: `mode` (prava) i `attr` (atributi reda — najvažniji su `mq_maxmsg` koliko poruka može stati u red, i `mq_msgsize` najveća veličina jedne poruke u bajtovima).

**Povratna vrijednost:** *deskriptor* reda poruka (`mqd_t`) u slučaju uspjeha, `(mqd_t)-1` u slučaju greške.

#### Funkcija `mq_send()`

Stavlja poruku u red. Ako je red pun, blokira (osim ako je red otvoren u neblokirajući mod).

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`mqdes`** — deskriptor reda.
- **`msg_ptr`** — pokazivač na podatke poruke.
- **`msg_len`** — broj bajtova u poruci (mora biti `≤ mq_msgsize`).
- **`msg_prio`** — prioritet (0 do `MQ_PRIO_MAX`-1; veći broj znači viši prioritet).

#### Funkcija `mq_receive()`

Uzima sljedeću poruku iz reda. Ako je red prazan, blokira (osim ako je red u neblokirajućem modu). **Uvijek se uzima poruka najvišeg prioriteta**; ako više poruka ima isti prioritet, uzima se najstarija.

**Povratna vrijednost:** broj pročitanih bajtova, ili `-1` u slučaju greške.

**Argumenti:**

- **`mqdes`** — deskriptor reda.
- **`msg_ptr`** — međuspremnik za poruku.
- **`msg_len`** — veličina međuspremnika (mora biti `≥ mq_msgsize` koja je upisana u atribute reda — inače se javlja greška).
- **`msg_prio`** — pokazivač u koji se upisuje prioritet primljene poruke (može biti `NULL` ako nas prioritet ne zanima).

### Primjer: razmjena poruka kroz red

- [**`mq_posalji.c`**](mq_posalji.c) — šalje poruku u red, s opcionalno zadanim prioritetom.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <fcntl.h>
  #include <mqueue.h>
  #include <sys/stat.h>

  #define MQ_NAME "/moj_mq"
  #define MAX_MSG_SIZE 256

  int main(int argc, char *argv[]) {
      mqd_t mq;
      const char *poruka;
      unsigned prioritet = 0;

      if (argc < 2) {
          poruka = "Pozdrav kroz red poruka!";
      } else {
          poruka = argv[1];
          if (argc >= 3) prioritet = (unsigned)atoi(argv[2]);
      }

      struct mq_attr attr;
      attr.mq_flags = 0;
      attr.mq_maxmsg = 10;
      attr.mq_msgsize = MAX_MSG_SIZE;
      attr.mq_curmsgs = 0;

      mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, 0666, &attr);
      if (mq == (mqd_t)-1) {
          perror("mq_open");
          return 1;
      }

      if (mq_send(mq, poruka, strlen(poruka) + 1, prioritet) < 0) {
          perror("mq_send");
          mq_close(mq);
          return 1;
      }

      printf("Poslana poruka (prioritet=%u): %s\n", prioritet, poruka);
      mq_close(mq);
      return 0;
  }
  ```

- [**`mq_primi.c`**](mq_primi.c) — čita sljedeću poruku iz reda i ispisuje je s prioritetom.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <fcntl.h>
  #include <mqueue.h>

  #define MQ_NAME "/moj_mq"
  #define MAX_MSG_SIZE 256

  int main(void) {
      mqd_t mq;
      char buf[MAX_MSG_SIZE];
      unsigned prioritet;
      ssize_t n;

      mq = mq_open(MQ_NAME, O_RDONLY);
      if (mq == (mqd_t)-1) {
          perror("mq_open");
          return 1;
      }

      printf("Cekam poruku...\n");
      n = mq_receive(mq, buf, MAX_MSG_SIZE, &prioritet);
      if (n < 0) {
          perror("mq_receive");
          mq_close(mq);
          return 1;
      }

      printf("Primljeno (prioritet=%u): %s\n", prioritet, buf);

      mq_close(mq);
      mq_unlink(MQ_NAME);                    /* ukloni red kad smo gotovi */
      return 0;
  }
  ```

  Demonstracija prioriteta (više poruka, primatelj ih čita po prioritetu):

  ```
  ## posalji vise poruka razlicitih prioriteta:
  $ ./mq_posalji "Niska vaznost" 1
  Poslana poruka (prioritet=1): Niska vaznost
  $ ./mq_posalji "Visoka vaznost" 9
  Poslana poruka (prioritet=9): Visoka vaznost
  $ ./mq_posalji "Srednja vaznost" 5
  Poslana poruka (prioritet=5): Srednja vaznost

  ## primi ih jednu po jednu (najvazniji prvi):
  $ ./mq_primi
  Cekam poruku...
  Primljeno (prioritet=9): Visoka vaznost

  $ ./mq_primi
  Cekam poruku...
  Primljeno (prioritet=5): Srednja vaznost

  $ ./mq_primi
  Cekam poruku...
  Primljeno (prioritet=1): Niska vaznost
  ```

  Iako su poruke poslane redom 1-9-5, primljene su 9-5-1 — **u opadajućem redoslijedu prioriteta**, kao što se i očekuje.

## Mapiranje datoteka u memoriju

Funkciju `mmap()` upoznali smo gore u kontekstu dijeljene memorije. Ona ima i drugu, jednako važnu primjenu: **mapiranje regularnih datoteka** u adresni prostor procesa. Umjesto da datoteku čitamo `read`-om u međuspremnik, dobivamo pokazivač na memorijski blok koji *je* datoteka — pristupamo bajtovima datoteke kao da su elementi polja.

Razmotrimo razliku:

```c
/* klasicni pristup: read u medjuspremnik */
char buf[1024];
int fd = open("podaci.bin", O_RDONLY);
read(fd, buf, sizeof(buf));
char prvi = buf[0];

/* mmap pristup: datoteka mapirana u memoriju */
int fd = open("podaci.bin", O_RDONLY);
struct stat st;
fstat(fd, &st);
char *podaci = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
char prvi = podaci[0];
```

Prednosti mapiranja:

- **Bez kopiranja** — jezgra "pretvara" stranice datoteke u stranice u memoriji procesa. Pri prvom pristupu nekoj stranici, jezgra je tek tada učita s diska (engl. *demand paging*) — pa ne plaćamo cijenu učitavanja onoga što ne pročitamo.
- **Slučajan pristup** — možemo skočiti u sredinu datoteke (`podaci[ogromni_offset]`) bez `lseek`-a.
- **Više procesa može mapirati istu datoteku** — što omogućuje da se velike datoteke (npr. baze podataka) dijele između više procesa bez dupliciranja u memoriji.

Nedostaci:

- **Veličina mora biti unaprijed poznata** — `mmap` zahtijeva da znamo koliko bajtova mapirati. Za dinamičke datoteke koje rastu (npr. log datoteke) ovo je nezgodno.
- **Pokazivač sjedi na konačnoj veličini** — ako kroz mapu pišemo izvan granica, dobijemo `SIGSEGV`.
- **Greške se manifestiraju kao `SIGBUS`/`SIGSEGV`** — što je teže za debugirati od greške koju vrati `read()`.

Za male datoteke ili kad treba upravljati streaming-om, klasični `read`/`write` ostaju primjereniji. Za velike datoteke kojima se pristupa nasumično, ili koje treba dijeliti između procesa, `mmap` je često znatno brži i elegantniji.

## System V IPC — kratko upoznavanje

Prije nego što je POSIX standardizirao IPC funkcije koje smo upravo upoznali, AT&T-jev System V UNIX je imao vlastiti skup mehanizama: System V poruke, semafore i dijeljenu memoriju. Iako su POSIX inačice danas preporučene za nove programe, System V API i dalje postoji na svim modernim UNIX sustavima i čest je u starijim kodovima — pa je dobro znati prepoznati ga.

System V mehanizmi imaju jedinstven obrazac:

1. **Identifikator** je cjelobrojni "ključ" (`key_t`), ne putanja. Dobiva se ili pozivom `ftok()` (koji od putanje + brojke generira ključ) ili specijalnom vrijednošću `IPC_PRIVATE` (znači: stvori novi red kojeg će dijeliti samo srodni procesi).
2. **Stvaranje/otvaranje** se radi `*get` funkcijom: `msgget`, `semget`, `shmget`. Vraćaju cjelobrojni "id".
3. **Operacije** koriste taj id: `msgsnd`/`msgrcv`, `semop`, `shmat`/`shmdt`.
4. **Brisanje** se radi `*ctl` funkcijom s naredbom `IPC_RMID`: `msgctl(id, IPC_RMID, NULL)`.

Bitna posebnost koja zna iznenaditi: System V objekti **opstaju i nakon završetka procesa koji ih je stvorio** — dok ih netko eksplicitno ne obriše ili dok se sustav ne ponovno pokrene. Ako program zaboravi pozvati `IPC_RMID`, objekt ostaje u sustavu kao "smeće". Sustav se može pregledati naredbom **`ipcs`**, a ručno čistiti naredbom **`ipcrm`**:

```
$ ipcs                   # prikazi sve aktivne System V IPC objekte
$ ipcrm -q <msqid>       # obrisi red poruka
$ ipcrm -m <shmid>       # obrisi shared memory segment
$ ipcrm -s <semid>       # obrisi semafor
```

### Primjer: System V red poruka

Da bismo ilustrirali drugačiji API, dat ćemo kratki primjer ekvivalentan onomu što smo radili s POSIX redovima — razmjena poruke između roditelja i djeteta. Koristimo `IPC_PRIVATE` kao ključ jer su procesi srodni (povezani `fork`-om).

- [**`msg_demo.c`**](msg_demo.c) — System V red poruka, slanje poruke od roditelja djetetu.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/ipc.h>
  #include <sys/msg.h>
  #include <sys/wait.h>

  #define MAX_TEXT 128

  /* struktura poruke - prvi clan mora biti tipa long (tip poruke) */
  struct moja_poruka {
      long mtype;
      char mtext[MAX_TEXT];
  };

  int main(void) {
      int msqid;
      pid_t pid;

      /* IPC_PRIVATE = stvori novi privatni red poruka koji ce nasljediti
       * djeca preko fork-a; nije dostupan drugim nepovezanim procesima */
      msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
      if (msqid < 0) { perror("msgget"); return 1; }

      pid = fork();
      if (pid < 0) { perror("fork"); return 1; }

      if (pid == 0) {
          /* dijete - prima poruku */
          struct moja_poruka p;
          if (msgrcv(msqid, &p, MAX_TEXT, 0, 0) < 0) {
              perror("msgrcv");
              return 1;
          }
          printf("Dijete primilo (tip=%ld): %s\n", p.mtype, p.mtext);
          return 0;
      }

      /* roditelj - salje poruku */
      struct moja_poruka p;
      p.mtype = 1;
      strcpy(p.mtext, "Pozdrav od roditelja preko System V!");
      if (msgsnd(msqid, &p, strlen(p.mtext) + 1, 0) < 0) {
          perror("msgsnd");
          return 1;
      }

      wait(NULL);

      /* obrisi red - inace ostaje u sustavu! */
      msgctl(msqid, IPC_RMID, NULL);
      return 0;
  }
  ```

  Bitna razlika prema POSIX `mq_*` API-ju: System V poruka nije puki niz bajtova nego **strukturira sa `long mtype` poljem na početku**. To polje koristi se i za "kanale" — u istom redu mogu biti poruke različitih `mtype` vrijednosti, a `msgrcv` može filtrirati po njima (npr. "uzmi prvu poruku tipa 5").

  Pokretanje:

  ```
  $ ./msg_demo
  Dijete primilo (tip=1): Pozdrav od roditelja preko System V!
  ```

  U ovom primjeru roditelj briše red (`IPC_RMID`) na kraju — što je dobra praksa. Ako bismo zaboravili to napraviti, red bi ostao u sustavu i bio bi vidljiv u `ipcs` ispisu.

System V semafori i dijeljena memorija imaju analogan API (`semget`/`semop`/`semctl` za semafore, `shmget`/`shmat`/`shmdt`/`shmctl` za shared memory). Konceptualno rade istu stvar kao POSIX inačice — samo s drugačijim načinom imenovanja i upravljanja.

## Što smo zapravo radili

Vrijedi se na kraju ovog poglavlja na trenutak osvrnuti na sliku koja je nastala. Procesi u UNIX sustavu, ako žele surađivati, imaju na raspolaganju lepezu mehanizama — od najjednostavnijih (signali, cjevovodi) preko sofisticiranih (redovi poruka, dijeljena memorija sa semaforima) do mrežno-orijentiranih (socketi, koje obrađujemo u sljedećem poglavlju). Svaki mehanizam ima svoje mjesto:

- **Signali** za jednostavno obavještavanje — kad nije važan sadržaj, samo da se nešto dogodilo.
- **Cjevovodi i FIFO** za jednosmjeran tok bajtova — klasika za "filterski" stil programa kakav vidimo svaki dan u UNIX ljusci (`ls | grep | wc`).
- **Redovi poruka** kad nam treba strukturirana, atomarna razmjena diskretnih poruka — eventualno s prioritetima.
- **Dijeljena memorija + semafori** za izrazito brzu razmjenu velikih količina podataka — uz oprez da svaki pristup zajedničkim podacima mora biti sinkroniziran.

Kao što su gotovo svi UNIX alati zapravo tanki sloj iznad sistemskih poziva, tako je i lepeza IPC mehanizama u biti zbirka pažljivo dizajniranih primitiva u jezgri. Razumijevanjem ovih primitiva razumijemo i kako rade veći sustavi koje koristimo svakodnevno: bazu podataka, web poslužitelje, procesne arhitekture orkestracijskih alata. Iza svega stoji par desetaka sistemskih poziva, isti već desetljećima.

## Prevođenje

Direktorij dolazi s priloženim `Makefile`-om koji prati iste konvencije kao i u ostalim poglavljima. Posebnost je u tome što **POSIX shm, semafori i redovi poruka traže linkanje s posebnim bibliotekama**:

- POSIX shared memory (`shm_open` itd.) — `-lrt`
- POSIX semafori (`sem_open` itd.) — `-lpthread`
- POSIX redovi poruka (`mq_*`) — `-lrt`

Ove dodatne biblioteke su u Makefile-u već navedene gdje su potrebne. System V IPC nema dodatnih ovisnosti.

```sh
make all          # gradi sve primjere
make cjev         # gradi pojedinačni primjer
make clean        # čisti generirane datoteke
```
