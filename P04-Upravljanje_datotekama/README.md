# Upravljanje datotekama

U ovom poglavlju upoznat ćemo dublji UNIX pogled na datoteke: njihove **atribute**, kako jezgra organizira informacije o njima, te kako iz programa dohvatiti i mijenjati ta svojstva. Dok smo se u prethodnom poglavlju bavili **sadržajem** datoteka — čitanjem i pisanjem bajtova — ovdje stojimo "stepenicu više" i gledamo **metapodatke**: tip datoteke, vlasnika, prava pristupa, vremena pristupa, broj linkova, fizičku organizaciju na disku.

## Tipovi datoteka

UNIX-ova filozofija *"sve je datoteka"* znači da kroz datotečno sučelje pristupamo različitim vrstama resursa. Sustav podržava sljedeće tipove datoteka:

| Tip | Opis |
|---|---|
| **regularna datoteka** | "obična" datoteka — niz bajtova: tekstualne datoteke, binarni programi, slike, arhive |
| **direktorij** | datoteka koja sadrži popis imena drugih datoteka i pripadnih i-node brojeva |
| **simbolički link** | datoteka koja sadrži putanju do druge datoteke (UNIX-ov "prečac") |
| **blok specijalna datoteka** | uređaj kojemu se pristupa u blokovima fiksne veličine (npr. disk: `/dev/sda`) |
| **karakter specijalna datoteka** | uređaj kojemu se pristupa znak po znak (npr. terminal: `/dev/tty`) |
| **FIFO** (imenovani cjevovod) | jednosmjerni komunikacijski kanal između procesa, vidljiv u datotečnom sustavu |
| **socket** | krajnja točka za lokalnu ili mrežnu međuprocesnu komunikaciju |

Bez obzira na tip, svim ovim resursima upravlja se istim malim skupom sistemskih poziva (`open`, `read`, `write`, `close`, `lseek`) — što je ujedno temelj UNIX-ove jednostavnosti i moći.

## Svojstva datoteka

Svaka datoteka u UNIX sustavu, neovisno o tipu, ima skup **atributa** (engl. *file attributes*) koje jezgra održava neovisno o samom sadržaju datoteke: tko je vlasnik, kakva su prava pristupa, kolika je veličina, kada je posljednji put mijenjana, koliko ima linkova... Iz programa, ovi se atributi dohvaćaju jednim od tri sistemska poziva iz `stat` obitelji:

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *path, struct stat *buf);
```

**Povratna vrijednost (sve tri funkcije):** `0` u slučaju uspjeha, `-1` u slučaju greške.

Tri funkcije razlikuju se po načinu na koji identificiraju datoteku i po tome kako se ponašaju ako je riječ o simboličkom linku:

- **`stat()`** — datoteka se zadaje **putanjom**; ako je putanja simbolički link, funkcija ga **slijedi** i vraća atribute datoteke na koju link pokazuje.
- **`fstat()`** — datoteka se zadaje **file deskriptorom** (deskriptor mora biti otvoren prethodnim pozivom `open()`).
- **`lstat()`** — kao `stat()`, ali ako je putanja simbolički link, **ne slijedi ga** nego vraća atribute samog linka (njegovu veličinu, vrijeme stvaranja itd.).

U svim trima slučajevima, drugi argument je pokazivač na strukturu `struct stat` u koju jezgra upisuje atribute. Strukturu prethodno alocira pozivatelj (najčešće kao lokalnu varijablu na stogu).

### Struktura `struct stat`

```c
struct stat {
    mode_t    st_mode;     // tip datoteke i prava pristupa
    ino_t     st_ino;      // i-node broj
    dev_t     st_dev;      // identifikator uređaja na kojem datoteka leži
    dev_t     st_rdev;     // identifikator uređaja (samo za specijalne datoteke)
    nlink_t   st_nlink;    // broj hard linkova
    uid_t     st_uid;      // user ID vlasnika
    gid_t     st_gid;      // group ID vlasnika
    off_t     st_size;     // velicina datoteke u bajtovima
    time_t    st_atime;    // vrijeme zadnjeg pristupa sadrzaju
    time_t    st_mtime;    // vrijeme zadnje izmjene sadrzaja
    time_t    st_ctime;    // vrijeme zadnje promjene statusa (atributa)
    blksize_t st_blksize;  // optimalna velicina I/O bloka
    blkcnt_t  st_blocks;   // broj alociranih blokova na disku
};
```

Pojašnjenje pojedinih polja:

- **`st_mode`** — sadrži dvije logičke informacije u istom polju: tip datoteke (regularna, direktorij, link...) i prava pristupa (devet bitova `rwxrwxrwx` plus posebni bitovi). Tip se ne čita izravno iz brojčane vrijednosti, nego pomoću posebnih makro funkcija opisanih u nastavku.
- **`st_ino`** — broj i-node zapisa, jedinstvene strukture u kojoj jezgra čuva sve atribute datoteke. Dvije datoteke s istim i-node brojem na istom datotečnom sustavu su zapravo **ista datoteka** (vidi hard linkove).
- **`st_dev`** — identifikator uređaja (datotečnog sustava) na kojem datoteka leži. Par (`st_dev`, `st_ino`) jedinstveno identificira datoteku na cijelom sustavu.
- **`st_rdev`** — koristi se samo za blok i karakter specijalne datoteke (uređaje); kodira "major" i "minor" broj uređaja.
- **`st_nlink`** — broj hard linkova koji pokazuju na ovaj i-node. Kad padne na 0, datoteka se fizički briše s diska.
- **`st_uid`, `st_gid`** — brojčani identifikatori vlasnika i grupe vlasnika. Tekstualna imena (npr. `dkrst`, `users`) dohvaćaju se naknadno iz sistemskih datoteka `/etc/passwd` i `/etc/group`.
- **`st_size`** — veličina sadržaja u bajtovima. Za regularne datoteke je to broj bajtova, za simboličke linkove duljina puta na koji link pokazuje, a za direktorije veličina ovisi o implementaciji.
- **`st_atime`** — vrijeme zadnjeg **pristupa sadržaju** (čitanja). U modernim sustavima se često ne ažurira pri svakom čitanju radi performansi.
- **`st_mtime`** — vrijeme zadnje **izmjene sadržaja** (pisanja u datoteku).
- **`st_ctime`** — vrijeme zadnje promjene **statusa** datoteke: ne sadržaja, nego nekog atributa (prava pristupa, vlasništvo, broj linkova). Razlika prema `mtime` je suptilna ali važna: `chmod` mijenja `ctime` ali ne `mtime`.
- **`st_blksize`** — optimalna veličina jedinice I/O prijenosa za ovu datoteku; često 4096 B na suvremenim sustavima.
- **`st_blocks`** — broj 512-bajtnih blokova fizički alociranih datoteci. Kod *rijetkih* datoteka ("rupa") može biti znatno manji od `st_size / 512`.

### Otkrivanje tipa datoteke

Tip datoteke kodiran je u `st_mode` polju, ali ne kao broj — provjera se radi pomoću makro funkcija definiranih u `<sys/stat.h>`:

| Makro | Vraća true ako je datoteka... |
|---|---|
| `S_ISREG(m)` | regularna |
| `S_ISDIR(m)` | direktorij |
| `S_ISLNK(m)` | simbolički link |
| `S_ISBLK(m)` | blok specijalna |
| `S_ISCHR(m)` | karakter specijalna |
| `S_ISFIFO(m)` | FIFO (imenovani cjevovod) |
| `S_ISSOCK(m)` | socket |

Tipičan obrazac uporabe: `if (S_ISREG(st.st_mode)) { ... }`.

### Ispis atributa datoteke

- [**`fileinfo.c`**](fileinfo.c) — prima ime datoteke kao argument naredbenog retka i ispisuje njene osnovne atribute pozivom `stat()`.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <time.h>

  int main(int argc, char *argv[]) {
      struct stat st;

      if (argc != 2) {
          printf("koristenje: %s <ime_datoteke>\n", argv[0]);
          return 1;
      }

      if (stat(argv[1], &st) < 0) {
          perror("stat");
          return 1;
      }

      printf("Datoteka: %s\n", argv[1]);
      printf("  tip:           ");
      if      (S_ISREG(st.st_mode))  printf("regularna datoteka\n");
      else if (S_ISDIR(st.st_mode))  printf("direktorij\n");
      else if (S_ISLNK(st.st_mode))  printf("simbolicki link\n");
      else if (S_ISCHR(st.st_mode))  printf("karakter specijalna\n");
      else if (S_ISBLK(st.st_mode))  printf("blok specijalna\n");
      else if (S_ISFIFO(st.st_mode)) printf("FIFO\n");
      else if (S_ISSOCK(st.st_mode)) printf("socket\n");

      printf("  i-node broj:   %ld\n", (long)st.st_ino);
      printf("  prava:         %o\n", st.st_mode & 0777);
      printf("  vlasnik UID:   %d\n", st.st_uid);
      printf("  grupa GID:     %d\n", st.st_gid);
      printf("  velicina:      %ld B\n", (long)st.st_size);
      printf("  broj linkova:  %ld\n", (long)st.st_nlink);
      printf("  zadnji pristup:  %s", ctime(&st.st_atime));
      printf("  zadnja izmjena:  %s", ctime(&st.st_mtime));
      printf("  zadnja promjena: %s", ctime(&st.st_ctime));

      return 0;
  }
  ```

  Konverzija `%o` ispisuje prava pristupa u oktalnom obliku (zajednički prikaz prava na UNIX sustavima — npr. `644` za `rw-r--r--`, `755` za `rwxr-xr-x`). Funkcija `ctime()` iz `<time.h>` pretvara `time_t` (sekunde od epohe, 1.1.1970) u čitljivi datum/vrijeme. Eksplicitne pretvorbe (`(long)st.st_ino`) nužne su jer su `ino_t`, `off_t` i drugi tipovi različiti na različitim sustavima — bez pretvorbe `printf` može ispisati pogrešne brojeve.

  Pokretanje:

  ```
  $ ./fileinfo fileinfo.c
  Datoteka: fileinfo.c
    tip:           regularna datoteka
    i-node broj:   165
    prava:         644
    vlasnik UID:   1000
    grupa GID:     1000
    velicina:      1300 B
    broj linkova:  1
    zadnji pristup:  Tue May  5 14:45:04 2026
    zadnja izmjena:  Tue May  5 14:44:43 2026
    zadnja promjena: Tue May  5 14:44:43 2026
  ```

  Pokušajte i s drugim vrstama datoteka:

  ```
  $ ./fileinfo /etc            # direktorij
  $ ./fileinfo /dev/null       # karakter specijalna
  $ ./fileinfo /dev/sda        # blok specijalna (ako postoji)
  ```

### Razlika `stat` i `lstat`

- [**`fileinfo2.c`**](fileinfo2.c) — istovremeno poziva `stat()` i `lstat()` nad istom datotekom i ispisuje rezultate jedan iznad drugoga, kako bi razlika između dva sistemska poziva bila vidljiva odjednom.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <sys/stat.h>

  int main(int argc, char *argv[]) {
      struct stat st_stat, st_lstat;

      if (argc != 2) {
          printf("koristenje: %s <ime_datoteke>\n", argv[0]);
          return 1;
      }

      if (stat(argv[1], &st_stat) < 0) {
          perror("stat");
          return 1;
      }
      if (lstat(argv[1], &st_lstat) < 0) {
          perror("lstat");
          return 1;
      }

      printf("stat  -> i-node: %ld, velicina: %ld B, tip: ",
             (long)st_stat.st_ino, (long)st_stat.st_size);
      if      (S_ISREG(st_stat.st_mode))  printf("regularna\n");
      else if (S_ISDIR(st_stat.st_mode))  printf("direktorij\n");
      else if (S_ISLNK(st_stat.st_mode))  printf("simb. link\n");
      else printf("ostalo\n");

      printf("lstat -> i-node: %ld, velicina: %ld B, tip: ",
             (long)st_lstat.st_ino, (long)st_lstat.st_size);
      if      (S_ISREG(st_lstat.st_mode))  printf("regularna\n");
      else if (S_ISDIR(st_lstat.st_mode))  printf("direktorij\n");
      else if (S_ISLNK(st_lstat.st_mode))  printf("simb. link\n");
      else printf("ostalo\n");

      return 0;
  }
  ```

  Pokrenimo program najprije nad običnom datotekom — oba poziva trebala bi vratiti **identične** rezultate jer nema linka za slijediti:

  ```
  $ ./fileinfo2 fileinfo.c
  stat  -> i-node: 165, velicina: 1300 B, tip: regularna
  lstat -> i-node: 165, velicina: 1300 B, tip: regularna
  ```

  Sad stvorimo simbolički link na `fileinfo.c` i ponovimo:

  ```
  $ ln -s fileinfo.c link.c
  $ ./fileinfo2 link.c
  stat  -> i-node: 165, velicina: 1300 B, tip: regularna
  lstat -> i-node: 186, velicina: 10 B, tip: simb. link
  ```

  Razlika je sad jasna: `stat()` slijedi simbolički link i vraća atribute **ciljne** datoteke (`fileinfo.c` — i-node 165, 1300 B), dok `lstat()` ostaje na samom linku i vraća njegove atribute (i-node 186, 10 B — koliko je dugačak string `"fileinfo.c"`).

  Kad god radimo s programom koji pretražuje datotečni sustav (npr. `find` ili rekurzivno listanje direktorija), mora se koristiti `lstat()` — inače bi simbolički link na direktorij doveo do beskonačne petlje. Za većinu drugih namjena `stat()` je prirodniji izbor jer korisnika ne zanima radi li o linku ili "stvarnoj" datoteci.

## Korisnici, grupe i prava pristupa

UNIX je višekorisnički sustav, što znači da na istom računalu istovremeno može raditi više korisnika. Svakom korisniku pridružen je jedinstveni cjelobrojni **UID** (*User ID*), a svaki korisnik pripada jednoj ili više **grupa**, od kojih svaka ima svoj **GID** (*Group ID*). Mapiranje imena u brojeve nalazi se u sistemskim datotekama `/etc/passwd` (korisnici) i `/etc/group` (grupe).

Kao što smo upoznali u prvom poglavlju, prava pristupa svakoj datoteci dijele se u tri razine — vlasnik (*user*), grupa (*group*), ostali (*others*) — i tri tipa prava: čitanje (`r`), pisanje (`w`), izvršavanje (`x`). Sveukupno **devet bitova** koji se zapisuju u `st_mode` polje strukture `stat`.

### Procesi i njihovo vlasništvo

Vlasništvo nije samo svojstvo datoteka — nego i **procesa**. Svaki proces u sustavu ima pridruženu skupinu identifikatora koji određuju "u čije ime" radi. Najvažnija dva su:

- **stvarni** (*real*) **UID i GID** — tko je doslovno pokrenuo proces; preuzima se od ljuske u kojoj je naredba upisana.
- **efektivni** (*effective*) **UID i GID** — koje ovlasti proces zapravo ima u trenutku provjere. Jezgra prilikom svake operacije nad datotekom (otvaranje, brisanje, izmjena) provjerava efektivni UID/GID procesa protiv vlasništva datoteke.

U pravilu su stvarni i efektivni UID/GID jednaki — proces radi s ovlastima korisnika koji ga je pokrenuo. Postoje međutim situacije kad treba dati programu privremeno **veće** ovlasti od onih koje ima korisnik koji ga pokreće. Klasičan primjer je naredba `passwd` za promjenu lozinke: lozinke su pohranjene u datoteci `/etc/shadow` koju može mijenjati samo `root`, ali svaki korisnik mora moći promijeniti vlastitu lozinku. Rješenje je **set-UID bit** — poseban bit u pravima izvršne datoteke koji jezgri kaže: *"kad se ovaj program pokrene, postavi efektivni UID procesa na UID vlasnika izvršne datoteke, ne na UID korisnika koji ga je pokrenuo"*. Datoteka `/usr/bin/passwd` u vlasništvu je root-a i ima postavljen set-UID bit, pa se svaki put kad ju običan korisnik pokrene, proces izvršava s efektivnim UID-om root-a (i može mijenjati `/etc/shadow`), ali stvarni UID ostaje korisnikov (pa program zna tko je pokrenuo program i može mu mijenjati samo *njegovu* lozinku).

### Provjera prava pristupa — `access()`

Za testiranje da li trenutni proces ima određena prava nad datotekom (čitanje, pisanje, izvršavanje, ili samo provjera postojanja), POSIX nudi sistemski poziv `access`.

```c
#include <unistd.h>

int access(const char *pathname, int mode);
```

**Povratna vrijednost:** `0` ako su sva tražena prava ostvarena (ili datoteka postoji u slučaju `F_OK`); `-1` ako barem jedno pravo nije dopušteno ili je došlo do greške.

**Argumenti:**

- **`pathname`** — putanja do datoteke koja se provjerava.
- **`mode`** — kombinacija (bitovni `OR`) jedne ili više konstanti:
  - `R_OK` — provjeri pravo čitanja,
  - `W_OK` — provjeri pravo pisanja,
  - `X_OK` — provjeri pravo izvršavanja,
  - `F_OK` — provjeri samo postojanje datoteke (zanemaruje prava).

Bitno je razumjeti da `access` provjerava prava pomoću **stvarnog** UID-a i GID-a procesa, ne efektivnog. Razlika postaje važna kod programa s postavljenim set-UID bitom (npr. `passwd`): `access` će vratiti odgovor kakav bi imao stvarni korisnik, ne onaj kojeg je program privremeno preuzeo. Ovo je namjerno ponašanje — daje set-UID programima način da provjere "smije li to korisnik koji me je pokrenuo zaista raditi".

- [**`provjeri.c`**](provjeri.c) — prima ime datoteke kao argument i ispisuje koja prava (čitanje, pisanje, izvršavanje) trenutni korisnik nad njom ima.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>

  int main(int argc, char *argv[]) {
      if (argc != 2) {
          printf("koristenje: %s <ime_datoteke>\n", argv[0]);
          return 1;
      }

      if (access(argv[1], F_OK) < 0) {
          printf("Datoteka '%s' ne postoji.\n", argv[1]);
          return 1;
      }

      printf("Prava korisnika nad datotekom '%s':\n", argv[1]);
      printf("  citanje:     %s\n", access(argv[1], R_OK) == 0 ? "DA" : "NE");
      printf("  pisanje:     %s\n", access(argv[1], W_OK) == 0 ? "DA" : "NE");
      printf("  izvrsavanje: %s\n", access(argv[1], X_OK) == 0 ? "DA" : "NE");

      return 0;
  }
  ```

  Program prvo pozivom `access(argv[1], F_OK)` provjerava postoji li datoteka uopće — to je preporučena praksa jer `access` s drugim zastavicama ne razlikuje "datoteka ne postoji" od "datoteka postoji ali nemam prava". Ako datoteka postoji, slijede tri zasebne provjere za svako od tri prava.

  Pokretanje (kao običan korisnik):

  ```
  $ ./provjeri /etc/passwd
  Prava korisnika nad datotekom '/etc/passwd':
    citanje:     DA
    pisanje:     NE
    izvrsavanje: NE

  $ ./provjeri /etc/shadow
  Prava korisnika nad datotekom '/etc/shadow':
    citanje:     NE
    pisanje:     NE
    izvrsavanje: NE

  $ ./provjeri ./provjeri
  Prava korisnika nad datotekom './provjeri':
    citanje:     DA
    pisanje:     DA
    izvrsavanje: DA

  $ ./provjeri nepostoji.txt
  Datoteka 'nepostoji.txt' ne postoji.
  ```

  Iz primjera je vidljivo da običan korisnik može čitati `/etc/passwd` (koji sadrži korisnička imena i UID-ove), ali ne i `/etc/shadow` (koji sadrži kriptirane lozinke i kojem pristup ima samo root).

### Promjena prava pristupa — `chmod()` i `fchmod()`

```c
#include <sys/stat.h>

int chmod(const char *path, mode_t mode);
int fchmod(int fd, mode_t mode);
```

**Povratna vrijednost (obje funkcije):** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`path`** / **`fd`** — datoteka identificirana putanjom (`chmod`) ili otvorenim file deskriptorom (`fchmod`).
- **`mode`** — nova prava pristupa, zadana kao bitovni `OR` konstanti tipa `mode_t` (`S_IRUSR | S_IWUSR | ...`) ili kao oktalni broj (`0644`, `0755`).

Da bi proces smio mijenjati prava pristupa neke datoteke, mora biti zadovoljen jedan od dva uvjeta: efektivni UID procesa mora biti jednak UID-u vlasnika datoteke, **ili** proces mora biti pokrenut kao root.

- [**`prava.c`**](prava.c) — jednostavna inačica naredbe `chmod` iz ljuske: prima oktalni zapis prava pristupa i ime datoteke, te postavlja zadana prava.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <sys/stat.h>

  int main(int argc, char *argv[]) {
      mode_t mode;

      if (argc != 3) {
          printf("koristenje: %s <oktalna_prava> <ime_datoteke>\n", argv[0]);
          printf("primjer: %s 644 dat.txt\n", argv[0]);
          return 1;
      }

      /* strtol s bazom 8 - korisnik zadaje oktalni zapis poput 644 */
      mode = (mode_t)strtol(argv[1], NULL, 8);

      if (chmod(argv[2], mode) < 0) {
          perror("chmod");
          return 1;
      }

      printf("Prava datoteke '%s' postavljena na %o.\n", argv[2], mode);
      return 0;
  }
  ```

  Funkcija `strtol` iz standardne C biblioteke pretvara string u cijeli broj; treći argument (`8`) određuje brojčanu bazu — za oktalne brojeve to je 8. (Mogli bismo koristiti i `strtol(..., 0)` koji automatski prepoznaje bazu prema prefiksu: `0` za oktalni, `0x` za heksadekadski, ali smo se odlučili za eksplicitnu bazu radi jasnoće.)

  Ekvivalent u ljusci je naredba `chmod` koja prihvaća iste oktalne brojeve:

  ```
  $ ls -l dat.txt
  -rw-r--r-- 1 dkrst users 5 May  5 15:00 dat.txt

  $ ./prava 600 dat.txt
  Prava datoteke 'dat.txt' postavljena na 600.

  $ ls -l dat.txt
  -rw------- 1 dkrst users 5 May  5 15:00 dat.txt

  $ chmod 644 dat.txt          # ekvivalentno ./prava 644 dat.txt
  $ ls -l dat.txt
  -rw-r--r-- 1 dkrst users 5 May  5 15:00 dat.txt
  ```

### Promjena vlasništva — `chown()`, `fchown()`, `lchown()`

```c
#include <unistd.h>

int chown(const char *path, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int lchown(const char *path, uid_t owner, gid_t group);
```

**Povratna vrijednost (sve tri funkcije):** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`path`** / **`fd`** — datoteka identificirana putanjom (`chown`, `lchown`) ili otvorenim deskriptorom (`fchown`).
- **`owner`** — novi UID vlasnika; ako se zadaje `-1`, vlasnik se ne mijenja.
- **`group`** — novi GID grupe; ako se zadaje `-1`, grupa se ne mijenja.

Razlika između `chown` i `lchown` analogna je razlici između `stat` i `lstat`: `chown` slijedi simbolički link i mijenja vlasništvo ciljne datoteke, dok `lchown` mijenja vlasništvo samog linka.

Bitna napomena: na većini suvremenih UNIX sustava, **promjena vlasništva ograničena je na korisnika `root`**. Običan korisnik **ne može** "preuzeti" tuđu datoteku (jer bi to predstavljao sigurnosni rizik), niti svoju datoteku "predati" drugome (jer bi to omogućilo zaobilaženje kvota i drugih ograničenja). U praksi to znači da običan korisnik ove sistemske pozive uglavnom ne koristi izravno; oni su prvenstveno alat administratora. Ekvivalent u ljusci je naredba `chown` koja se također najčešće poziva uz `sudo`:

```
# sudo chown dkrst:users dat.txt    # dodjeljuje datoteku korisniku dkrst, grupi users
```

Zbog ograničenja primjene, u ovom poglavlju **ne dajemo zaseban C primjer** za `chown`-obitelji — sintaksa je ionako trivijalna, a izvršavanje bi tražilo root ovlasti.

## Linkovi

UNIX podržava dva različita načina da jedna te ista datoteka bude dostupna pod više imena: **čvrste linkove** (engl. *hard link*) i **simboličke linkove** (engl. *symbolic link* ili *soft link*). Iako oba mehanizma pružaju "alias" za neku datoteku, oni rade na suštinski različite načine.

### Čvrsti linkovi

**Čvrsti link** je dodatni zapis u direktoriju koji pokazuje na isti i-node kao već postojeća datoteka. Sjetimo se: u UNIX-u sve atribute datoteke (tip, prava, veličinu, vremena, lokaciju podataka na disku) jezgra čuva u i-node strukturi, a ime datoteke u nekom direktoriju je samo zapis koji to ime povezuje s i-node brojem. Stvaranjem čvrstog linka jednostavno se dodaje novi takav zapis koji pokazuje na **isti** i-node — pa s gledišta jezgre, dvije nemaju drukčije ime; obje su jednako vrijedne reference na istu datoteku.

Iz ovog mehanizma slijedi nekoliko važnih svojstava čvrstih linkova:

- **Svi linkovi pokazuju na isti i-node**, dijele identičan sadržaj i atribute. Promjena sadržaja kroz jedno ime vidljiva je odmah kroz drugo.
- **Svi linkovi moraju biti na istom datotečnom sustavu** (istoj particiji), jer i-node brojevi vrijede samo unutar pojedinog datotečnog sustava.
- **Brisanjem nekog imena samo se uklanja njegov zapis u direktoriju** i smanjuje brojač linkova (`st_nlink`) na i-nodeu. Sama datoteka briše se s diska tek kad brojač padne na 0 — odnosno kad je obrisano i zadnje ime.
- **Izvorno ime nije ničim "jače" od kasnije stvorenih linkova.** Brisanje izvornog imena je jednako brisanju bilo kojeg drugog linka.
- **Samo `root` smije stvarati čvrste linkove na direktorij**, jer bi obični korisnici mogli proizvoditi cikluse u stablu direktorija koje bi zbunile alate poput `find` ili `du`.

```c
#include <unistd.h>

int link(const char *oldpath, const char *newpath);
int unlink(const char *pathname);
```

**Povratna vrijednost (obje):** `0` u slučaju uspjeha, `-1` u slučaju greške.

`link()` stvara novo ime (`newpath`) za već postojeću datoteku (`oldpath`) — povećava brojač linkova na i-nodeu i atomski dodaje novi zapis u direktorij. `unlink()` radi obrnuto: uklanja zapis iz direktorija i smanjuje brojač. Ako brojač padne na 0 i nema otvorenih deskriptora na datoteku, jezgra fizički briše datoteku s diska.

- [**`makelink.c`**](makelink.c) — stvara čvrsti link između dvije zadane putanje.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>

  int main(int argc, char *argv[]) {
      if (argc != 3) {
          printf("koristenje: %s <postojeca_datoteka> <novi_link>\n", argv[0]);
          return 1;
      }

      if (link(argv[1], argv[2]) < 0) {
          perror("link");
          return 1;
      }

      printf("Stvoren hard link '%s' -> '%s'.\n", argv[2], argv[1]);
      return 0;
  }
  ```

  Pokrenimo cjeloviti scenarij koji ilustrira sva spomenuta svojstva. Stvorit ćemo datoteku, dodati joj dva čvrsta linka, pa pratiti brojač linkova dok ne dođe do nule:

  ```
  $ echo "vrijedan sadrzaj" > orig.txt
  $ ls -li orig.txt
  1002 -rw-r--r-- 1 dkrst users 17 May  5 15:00 orig.txt

  $ ./makelink orig.txt link1.txt
  Stvoren hard link 'link1.txt' -> 'orig.txt'.
  $ ./makelink orig.txt link2.txt
  Stvoren hard link 'link2.txt' -> 'orig.txt'.

  $ ls -li orig.txt link1.txt link2.txt
  1002 -rw-r--r-- 3 dkrst users 17 May  5 15:00 link1.txt
  1002 -rw-r--r-- 3 dkrst users 17 May  5 15:00 link2.txt
  1002 -rw-r--r-- 3 dkrst users 17 May  5 15:00 orig.txt
  ```

  Sva tri imena imaju **isti i-node 1002** (prvi stupac, koji se ispisuje opcijom `-i`) i **brojač linkova jednak 3** (četvrti stupac). Sad obrišimo izvorno ime:

  ```
  $ rm orig.txt
  $ ls -li link1.txt link2.txt
  1002 -rw-r--r-- 2 dkrst users 17 May  5 15:00 link1.txt
  1002 -rw-r--r-- 2 dkrst users 17 May  5 15:00 link2.txt

  $ cat link1.txt
  vrijedan sadrzaj
  ```

  Brojač je pao na 2, ali i-node, sadržaj i ostali atributi nepromijenjeni su — datoteka još uvijek živi pod druga dva imena. Brisanje "izvornog" imena nije imalo nikakve posebne posljedice. Pokušajmo dalje:

  ```
  $ rm link1.txt
  $ ls -li link2.txt
  1002 -rw-r--r-- 1 dkrst users 17 May  5 15:00 link2.txt

  $ cat link2.txt
  vrijedan sadrzaj

  $ rm link2.txt
  $ cat link2.txt
  cat: link2.txt: No such file or directory
  ```

  Tek kad je brisano i zadnje ime — i brojač pao na 0 — jezgra je fizički obrisala datoteku s diska. Ekvivalent C poziva `link()` u ljusci je naredba `ln`:

  ```
  $ ln orig.txt link1.txt              # cvrsti link
  ```

### Simbolički linkovi

**Simbolički link** je datoteka koja **sadrži tekst** — putanju do druge datoteke. Kad jezgra naiđe na simbolički link u nekoj operaciji nad datotekom, čita njegov sadržaj i operaciju izvršava nad datotekom čije je ime pročitano. Simbolički linkovi pružaju funkcionalnost sličnu čvrstim linkovima, ali rade na potpuno drugačiji način, što ima nekoliko važnih posljedica:

- **Simbolički linkovi mogu prelaziti granice datotečnih sustava.** Sadržaj je samo tekst (putanja), tako da nema problema kao kod čvrstih linkova.
- **Mogu pokazivati na nepostojeće datoteke** — u trenutku stvaranja jezgra ne provjerava da li ciljna datoteka postoji. Ako ciljna datoteka nikad nije stvorena, ili je obrisana, link postaje "razbijen" (engl. *dangling*).
- **Mogu pokazivati na direktorije** — bez ikakvih ograničenja koja vrijede za čvrste linkove na direktorije.
- **Brisanjem ciljne datoteke linkovi ostaju na mjestu**, ali postaju razbijeni. Obrnuto: brisanjem linka ciljnoj datoteci se ništa ne događa.
- **Imaju vlastite atribute** (vlasnika, vremena) odvojene od ciljne datoteke. Veličina linka je broj bajtova u stringu putanje koji sadrži.

```c
#include <unistd.h>

int symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *pathname, char *buf, size_t bufsize);
```

**`symlink()`** stvara simbolički link na putanji `linkpath` koji "pokazuje" na `target`. Vraća `0` u slučaju uspjeha, `-1` u slučaju greške.

**`readlink()`** čita sadržaj simboličkog linka — odnosno **tekst (putanju) zapisan u datoteci tipa simbolički link** — i smješta ga u zadani međuspremnik. Vraća broj stvarno pročitanih bajtova, ili `-1` u slučaju greške. Bitna posebnost: `readlink` **ne dodaje null-terminator** na kraj — pozivatelj ga mora dodati ručno.

- [**`makesymlink.c`**](makesymlink.c) — stvara simbolički link i odmah ga čita pozivom `readlink` da pokaže njegov sadržaj.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>

  int main(int argc, char *argv[]) {
      char buf[256];
      ssize_t n;

      if (argc != 3) {
          printf("koristenje: %s <postojeca_datoteka> <novi_link>\n", argv[0]);
          return 1;
      }

      if (symlink(argv[1], argv[2]) < 0) {
          perror("symlink");
          return 1;
      }

      printf("Stvoren simbolicki link '%s' -> '%s'.\n", argv[2], argv[1]);

      /* readlink ne dodaje null terminator, pa ga moramo sami dodati */
      n = readlink(argv[2], buf, sizeof(buf) - 1);
      if (n < 0) {
          perror("readlink");
          return 1;
      }
      buf[n] = '\0';

      printf("Sadrzaj linka (procitan readlink-om): \"%s\"\n", buf);
      return 0;
  }
  ```

  Pokretanje:

  ```
  $ echo "ovo je sadrzaj" > orig.txt
  $ ./makesymlink orig.txt sym.txt
  Stvoren simbolicki link 'sym.txt' -> 'orig.txt'.
  Sadrzaj linka (procitan readlink-om): "orig.txt"

  $ ls -li orig.txt sym.txt
  1003 -rw-r--r-- 1 dkrst users 15 May  5 15:00 orig.txt
  1004 lrwxrwxrwx 1 dkrst users  8 May  5 15:00 sym.txt -> orig.txt

  $ cat sym.txt
  ovo je sadrzaj
  ```

  Bitno: i-node simboličkog linka (`1004`) različit je od i-noda ciljne datoteke (`1003`); brojač linkova je `1` na svakom (nisu hard linkovi); tip prvog je `-` (regularna), drugog `l` (link); veličina linka je 8 bajtova (duljina stringa `"orig.txt"`). Pokušajmo obrisati izvornu datoteku:

  ```
  $ rm orig.txt
  $ ls -l sym.txt
  lrwxrwxrwx 1 dkrst users 8 May  5 15:00 sym.txt -> orig.txt
  $ cat sym.txt
  cat: sym.txt: No such file or directory
  ```

  Link je sam ostao netaknut — ali ciljna datoteka više ne postoji, pa je link sad razbijen. Boja imena u modernim shellovima ovo obično signalizira (npr. crveni naziv).

  Ekvivalent u ljusci je naredba `ln -s`:

  ```
  $ ln -s orig.txt sym.txt              # simbolicki link
  ```

## Vremena pristupa

Već smo upoznali tri vremenska polja u `struct stat`: `st_atime`, `st_mtime`, `st_ctime`. Jezgra ih ažurira automatski — pri svakom čitanju, pisanju ili promjeni atributa datoteke. Ipak, ponekad želimo eksplicitno postaviti `atime` i `mtime` na neku zadanu vrijednost — najčešće da bismo "potvrdili" da je datoteka svježa, ili da Build sustavi (`make`) misle da je novija od neke druge.

```c
#include <sys/types.h>
#include <utime.h>

int utime(const char *pathname, const struct utimbuf *times);

struct utimbuf {
    time_t actime;     // novo atime
    time_t modtime;    // novo mtime
};
```

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`pathname`** — datoteka kojoj mijenjamo vremena.
- **`times`** — pokazivač na strukturu s novim vremenima. Ako je `NULL`, oba vremena postavljaju se na **trenutno** vrijeme (specijalan slučaj koji ekvivalentan je naredbi `touch`).

- [**`dotakni.c`**](dotakni.c) — pojednostavljena inačica naredbe `touch`: ako datoteka ne postoji, stvara je praznu; potom postavlja njena vremena na trenutno vrijeme.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <utime.h>
  #include <sys/stat.h>

  int main(int argc, char *argv[]) {
      int fd;

      if (argc != 2) {
          printf("koristenje: %s <ime_datoteke>\n", argv[0]);
          return 1;
      }

      /* Ako datoteka ne postoji, stvori je praznu */
      fd = open(argv[1], O_WRONLY | O_CREAT, 0644);
      if (fd < 0) {
          perror("open");
          return 1;
      }
      close(fd);

      /* Postavi atime i mtime na trenutno vrijeme.
       * NULL kao drugi argument znaci "koristi trenutno vrijeme". */
      if (utime(argv[1], NULL) < 0) {
          perror("utime");
          return 1;
      }

      printf("Datoteka '%s' azurirana.\n", argv[1]);
      return 0;
  }
  ```

  Pokretanje:

  ```
  $ ls nova.txt
  ls: cannot access 'nova.txt': No such file or directory

  $ ./dotakni nova.txt
  Datoteka 'nova.txt' azurirana.

  $ ls -l nova.txt
  -rw-r--r-- 1 dkrst users 0 May  5 15:00 nova.txt

  $ ./dotakni nova.txt              # vec postoji - samo azurira vremena
  Datoteka 'nova.txt' azurirana.
  ```

  Ekvivalent u ljusci je naredba `touch`:

  ```
  $ touch nova.txt                  # ekvivalent gornjeg poziva
  ```

  Tipična uporaba `touch` (i `dotakni`) — "natjerati" `make` da iznova prevede neki fajl, čak i ako mu sadržaj nije promijenjen, samo postavljanjem `mtime` na sadašnji trenutak.

## Rad s direktorijima

### Stvaranje, brisanje i navigacija

```c
#include <unistd.h>
#include <sys/stat.h>

int mkdir(const char *pathname, mode_t mode);
int rmdir(const char *pathname);

char *getcwd(char *buf, size_t size);
int chdir(const char *pathname);
int fchdir(int fd);
```

**`mkdir()`** stvara novi prazan direktorij. U njemu jezgra automatski stvara dva posebna unosa: `.` (pokazivač na sam taj direktorij) i `..` (pokazivač na nadređeni direktorij).

**`rmdir()`** briše direktorij — ali samo ako je **prazan** (sadrži samo `.` i `..`). Za rekurzivno brisanje neprazna direktorija treba sami obići sadržaj i obrisati ga prije nego što pozovemo `rmdir`.

**`getcwd()`** vraća putanju do trenutnog radnog direktorija (CWD) procesa, upisujući je u međuspremnik `buf` veličine `size`. Vraća pokazivač na `buf` u slučaju uspjeha, `NULL` u slučaju greške (npr. ako je međuspremnik premali). Bitno je razumjeti da je radni direktorij **svojstvo procesa**, ne korisnika; svaki proces ima svoj vlastiti CWD koji je naslijedio od svog roditelja.

**`chdir()`** mijenja radni direktorij procesa na `pathname`; **`fchdir()`** isto, ali se direktorij identificira otvorenim file deskriptorom. Bitan suptilan detalj: `chdir` u programu mijenja radni direktorij **samo tom procesu** — ne i ljusci koja ga je pokrenula. Zato u ljusci ne postoji "vanjska" naredba `cd`: bila bi beskorisna jer bi mijenjala CWD svog vlastitog procesa, a ne ljuske. `cd` je ugrađena (*built-in*) naredba ljuske koja mijenja njen vlastiti CWD pozivom `chdir()` "iznutra".

### Čitanje sadržaja direktorija

Direktoriji su zapravo posebne datoteke koje sadrže popis imena i pripadnih i-node brojeva. Jezgra dopušta samo sebi da piše u njih — korisnički procesi smiju ih čitati, ali ne i izravno mijenjati. Čitanje se obavlja kroz skup funkcija iz `<dirent.h>`:

```c
#include <dirent.h>

DIR           *opendir(const char *pathname);
struct dirent *readdir(DIR *dp);
int            closedir(DIR *dp);
void           rewinddir(DIR *dp);
long           telldir(DIR *dp);
void           seekdir(DIR *dp, long loc);
```

**`opendir()`** otvara direktorij za čitanje i vraća pokazivač na **`DIR`** strukturu — neprozirni (*opaque*) tip koji stoji nasuprot file deskriptora i kojeg standard ne specificira detaljno; programer ga koristi samo kao apsolutnu referencu kroz ostale funkcije obitelji. Vraća `NULL` u slučaju greške.

**`readdir()`** vraća **pokazivač na sljedeći zapis** u direktoriju ili `NULL` po kraju (ili pri grešci). Bitno je da memoriju u koju pokazuje vraćeni pokazivač održava sama biblioteka; pozivatelj nikad ne smije pozvati `free()` na rezultatu. Sljedeći poziv `readdir`-a obično prepiše tu memoriju, pa ako trebamo zadržati vrijednost — treba je kopirati.

**`closedir()`** zatvara direktorij i oslobađa pripadne resurse.

**`rewinddir()`** vraća čitanje na početak direktorija (sljedeći `readdir` opet vraća prvi zapis).

**`telldir()`** vraća trenutnu poziciju u direktoriju kao neprozirnu vrijednost; **`seekdir()`** postavlja čitanje natrag na poziciju ranije zapamćenu `telldir`-om. (Ove dvije funkcije rijetko se koriste u praksi.)

**Veza s standardnom C bibliotekom:** uočite da je kombinacija `opendir → readdir → closedir` za direktorije analogna kombinaciji `fopen → fread → fclose` za regularne datoteke. U oba slučaja imamo "stream" apstrakciju — neprozirni objekt (`DIR *` odnosno `FILE *`) koji čuva interne podatke o napretku čitanja, plus funkcije za otvaranje, sekvencijalno čitanje i zatvaranje.

#### Struktura `struct dirent`

POSIX standard zahtijeva da `struct dirent` sadrži samo **dva polja**:

```c
struct dirent {
    ino_t  d_ino;       // i-node broj zapisa
    char   d_name[];    // ime datoteke (null-terminirano)
};
```

Polje `d_name` ima **neodređenu veličinu** — POSIX kaže samo "polje znakova koje sadrži ime od najviše `NAME_MAX` bajtova plus null-terminator". Različite implementacije strukturu dopunjuju vlastitim poljima (`d_off`, `d_reclen`, `d_type` na Linuxu/glibc), ali ta polja **nisu prenosiva**. Zbog ovoga vrijedi nekoliko praktičnih napomena:

- **`sizeof(d_name)` ne radi pouzdano** — neke implementacije deklariraju polje kao `char d_name[1]`, druge kao `char d_name[256]`. Za pravu duljinu uvijek koristimo `strlen(d_name)`.
- **`sizeof(struct dirent)` ne predstavlja nužno stvarnu veličinu zapisa** — iz istog razloga kao gore.
- **Polje `d_type`** (tip datoteke kao bajt — regularna/direktorij/link/...) postoji na Linuxu i nije POSIX. Za prenosiv kod treba pozvati `lstat` na ime datoteke i čitati `st_mode`.

- [**`mojls.c`**](mojls.c) — pojednostavljena inačica naredbe `ls`. Bez argumenta lista trenutni direktorij; s argumentom lista zadani.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <dirent.h>

  int main(int argc, char *argv[]) {
      DIR *dp;
      struct dirent *entry;
      const char *path;

      /* bez argumenta, listamo trenutni direktorij */
      path = (argc < 2) ? "." : argv[1];

      dp = opendir(path);
      if (dp == NULL) {
          perror("opendir");
          return 1;
      }

      /* readdir vraca po jedan zapis u svakom pozivu;
       * NULL znaci kraj direktorija (ili greska) */
      while ((entry = readdir(dp)) != NULL) {
          /* preskoci skrivene datoteke (ukljucujuci . i ..) */
          if (entry->d_name[0] == '.')
              continue;

          printf("%s\n", entry->d_name);
      }

      closedir(dp);
      return 0;
  }
  ```

  Glavna petlja je tipična UNIX-ova "while-readdir" konstrukcija — funkcija u istom pozivu javlja i podatke (kroz povratnu vrijednost) i kraj pretrage (vraćanjem `NULL`-a). Ovaj sažeti idiom karakterističan je za UNIX-ove `read*` pozive opće namjene.

  Pokretanje:

  ```
  $ ./mojls
  fileinfo.c
  fileinfo2.c
  Makefile
  mojls.c
  prava.c
  README.md
  ...

  $ ./mojls /etc
  passwd
  group
  hostname
  ...
  ```

  **Vježba za čitatelja:** trenutna verzija `mojls`-a ispisuje samo imena datoteka. Pravi `ls -l` daje znatno više informacija — tip datoteke, prava pristupa, vlasnika, veličinu, vrijeme zadnje izmjene. Pokušajte doraditi `mojls` tako da za svaki ulazni zapis pozove `lstat()` na ime datoteke, pa iz dobivenog `struct stat`-a ispiše atribute. Korisne funkcije: `getpwuid()` (UID → ime korisnika), `getgrgid()` (GID → ime grupe), `ctime()` (vrijeme → string).

## Prevođenje

Direktorij dolazi s priloženim `Makefile`-om koji prati iste konvencije kao i u ostalim poglavljima (varijable `CC`, `CFLAGS`, `LDFLAGS`, `TARGETS`; implicitno pravilo `.c.o`; pravila `default`, `all`, `clean`).

```sh
make all          # gradi sve primjere
make fileinfo     # gradi pojedinačni primjer
make clean        # čisti generirane datoteke
```
