# Ulazno/izlazne operacije

U ovom direktoriju nalaze se programi koji ilustriraju UNIX sistemske pozive za rad s datotekama: `open()`, `creat()`, `close()`, `read()`, `write()`, `lseek()` i `umask()`. Svi primjeri pisani su izravno korištenjem sistemskih poziva (bez uporabe funkcija standardne C biblioteke poput `fopen`, `fread`, `fprintf`), s ciljem da se prikaže kako operacijski sustav zapravo obavlja ulazno/izlazne operacije i na kojoj razini počiva koncept **"sve je datoteka"** — temeljno UNIX načelo prema kojem se datoteke, uređaji, terminali, cijevi i mrežni sockets svi koriste kroz isto skučeno sučelje file deskriptora.

Vrijedi napomenuti: funkcije standardne C biblioteke (`fopen`, `fread`, `fprintf`, `fputs`, ...) **na UNIX sustavima u konačnici se svode na ove iste sistemske pozive**. Funkcije više razine pružaju zgodne dodatke poput interno upravljanog međuspremnika za smanjenje broja sistemskih poziva, formatiran ulaz/izlaz (`fprintf`), ili portabilnost između operacijskih sustava — ali fizička komunikacija s jezgrom uvijek se odvija kroz `read()`, `write()`, `open()` i ostale poziva opisane u nastavku. Razumijevanje sistemskih poziva otkriva što se zapravo događa "ispod" funkcija standardne biblioteke koje ste do sada vjerojatno koristili.

## Deskriptori datoteka

Jezgra operacijskog sustava sve otvorene datoteke referencira pomoću nenegativnih cijelih brojeva koje nazivamo **deskriptorima datoteke** (engl. *file descriptors*). Deskriptor je apstraktna referenca koju proces dobiva u trenutku otvaranja datoteke i koristi za sve daljnje operacije na njoj (čitanje, pisanje, pozicioniranje), do njezinog zatvaranja.

Pri tom proces ne mora znati ništa o samoj datoteci otvorenoj na nekom deskriptoru: gdje se fizički nalazi, radi li se uopće o datoteci na disku, ili je možda riječ o standardnom izlazu na koji program ispisuje poruke korisniku, ili o otvorenom portu na mreži. Zahvaljujući konceptu *"sve je datoteka"*, svim tim resursima upravlja se na isti način, korištenjem malog skupa standardiziranih sistemskih poziva. Naravno, upisivanje u datoteku na lokalnom disku fizički se ne odvija na isti način kao komunikacija s udaljenim računalom putem mreže — jezgra čuva informacije o svakom otvorenom deskriptoru i na temelju toga odabire odgovarajuću komunikacijsku rutinu. Programer te detalje ne vidi: sučelje je jedinstveno za sve tipove datoteka.

Pri pokretanju novog programa, najčešće su već zauzeti file deskriptori 0, 1 i 2. Ovi deskriptori koriste se za komunikaciju s korisnikom putem naredbenog retka:

| Deskriptor | Naziv | Izvor / odredište |
|---|---|---|
| 0 | standardni ulaz (*standard input*) | naredbeni redak — unos korisnika putem tipkovnice |
| 1 | standardni izlaz (*standard output*) | naredbeni redak — ispis u korisnički terminal |
| 2 | standardni izlaz za greške (*standard error*) | naredbeni redak — ispis u korisnički terminal |

Ova konvencija datira iz ranih UNIX sustava 1970-ih i danas je neodvojiv dio POSIX standarda. Budući da su deskriptori 0, 1 i 2 pri pokretanju programa u pravilu već zauzeti, novokreirani deskriptori — oni koje vraća `open()` ili `creat()` — u pravilu imaju vrijednost 3 ili veću. Sama jezgra pri otvaranju nove datoteke uvijek dodjeljuje **najniži slobodan deskriptor**.

Vrijednosti deskriptora 0, 1 i 2 definirane su u zaglavlju `<unistd.h>` kao simboličke konstante `STDIN_FILENO`, `STDOUT_FILENO` i `STDERR_FILENO`. Preporuka je u kodu koristiti ove konstante umjesto golih brojeva 0, 1 i 2 — kod time postaje čitljiviji.

## Sistemski pozivi za rad s datotekama

### Funkcija `open()`

Osnovna UNIX funkcija za otvaranje (i po potrebi kreiranje) datoteka.

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int oflag, /* mode_t mode */);
```

**Povratna vrijednost:** file deskriptor otvorene datoteke (nenegativni cijeli broj), ili `-1` u slučaju greške.

**Argumenti:**

- **`pathname`** — putanja do datoteke koja se otvara ili kreira.
- **`oflag`** — kombinacija bitovnih konstanti koja specificira način otvaranja. Mora sadržavati **točno jednu** od sljedeće tri konstante:
  - `O_RDONLY` — otvori samo za čitanje,
  - `O_WRONLY` — otvori samo za pisanje,
  - `O_RDWR` — otvori za čitanje i pisanje.
  
  Uz tu obaveznu konstantu može se kombinacijom bitovnog *OR*-a (`|`) dodati jedna ili više opcionalnih:
  - `O_CREAT` — kreiraj datoteku ako ne postoji (zahtijeva treći argument `mode`),
  - `O_APPEND` — pri svakom pisanju pomakni offset na kraj datoteke, bez obzira na trenutni file offset,
  - `O_TRUNC` — ako je datoteka otvorena za pisanje, izbriši njezin sadržaj (dužina postaje 0),
  - `O_EXCL` — u kombinaciji s `O_CREAT`: vrati grešku ako datoteka već postoji; provjera i kreiranje izvode se kao **atomska operacija**,
  - `O_NONBLOCK` — postavi neblokirajući način rada za I/O operacije,
  - `O_SYNC` — čekaj da se svaka operacija pisanja fizički dovrši na disku.

  Tipične kombinacije zastavica koje ćete često sresti u UNIX kodu:

  | Kombinacija | Značenje |
  |---|---|
  | `O_WRONLY \| O_TRUNC` | otvori za pisanje i skrati na 0 (obriši postojeći sadržaj) |
  | `O_WRONLY \| O_CREAT` | otvori za pisanje; ako datoteka ne postoji, stvori je |
  | `O_WRONLY \| O_CREAT \| O_TRUNC` | otvori za pisanje i obriši postojeći sadržaj; ako datoteka ne postoji, stvori je |
  | `O_WRONLY \| O_CREAT \| O_EXCL` | stvori novu datoteku i otvori je za pisanje; ako datoteka već postoji, vrati grešku |
  | `O_WRONLY \| O_APPEND` | otvori za pisanje; svako pisanje dodaje se na kraj datoteke, bez obzira na trenutni file offset |

- **`mode`** — opcionalni treći argument koji se navodi pri kreiranju nove datoteke (uz zastavicu `O_CREAT`). Specificira prava pristupa novostvorene datoteke. Može se zadati kao troznamenkasti oktalni broj (svaka znamenka definira prava za vlasnika, grupu i ostale korisnike), ili kombinacijom konstanti tipa `mode_t` definiranih u zaglavlju `<sys/stat.h>`:

  | Konstanta | Oktalna vrijednost | Značenje |
  |---|---|---|
  | `S_IRUSR` | `0400` | čitanje za vlasnika |
  | `S_IWUSR` | `0200` | pisanje za vlasnika |
  | `S_IXUSR` | `0100` | izvršavanje za vlasnika |
  | `S_IRGRP` | `0040` | čitanje za grupu |
  | `S_IWGRP` | `0020` | pisanje za grupu |
  | `S_IXGRP` | `0010` | izvršavanje za grupu |
  | `S_IROTH` | `0004` | čitanje za ostale |
  | `S_IWOTH` | `0002` | pisanje za ostale |
  | `S_IXOTH` | `0001` | izvršavanje za ostale |

  Tako je primjerice `S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH` ekvivalentno oktalnom zapisu `0644` — vlasnik smije čitati i pisati, grupa i ostali samo čitati.

### Funkcija `creat()`

Namijenjena je kreiranju novih datoteka.

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int creat(const char *pathname, mode_t mode);
```

**Povratna vrijednost:** file deskriptor otvorene datoteke, ili `-1` u slučaju greške.

**Argumenti:**

- **`pathname`** — putanja do datoteke koja se kreira.
- **`mode`** — prava pristupa novostvorene datoteke (kombinacija konstanti tipa `mode_t`, kako je opisano kod `open()`-a).

Ova funkcija ekvivalentna je pozivu:

```c
open(pathname, O_WRONLY | O_CREAT | O_TRUNC, mode);
```

Iz definicije slijede dva ograničenja: `creat()` datoteku otvara **samo za pisanje**, a ako datoteka već postoji, njezin sadržaj se briše (`O_TRUNC`). Zato je `creat()` jednostavno kratki oblik za vrlo čest slučaj korištenja `open()`-a — u modernom kodu obično se ide direktno preko `open()`-a koji nudi punu kontrolu.

### Funkcija `close()`

Zatvara datoteku otvorenu na file deskriptoru `filedes`, čime jezgra oslobađa deskriptor za ponovnu upotrebu.

```c
#include <unistd.h>

int close(int filedes);
```

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`filedes`** — deskriptor otvorene datoteke koju treba zatvoriti.

Kada proces završi s radom, automatski se zatvaraju sve otvorene datoteke, pa eksplicitno pozivanje `close()` nije *strogo* nužno. Ipak, preporučuje se eksplicitno zatvaranje jer sustav ima ograničen broj deskriptora po procesu, a za neke tipove datoteka (npr. mrežne sockete) zatvaranje pokreće važne završne operacije.

### Funkcija `read()`

Čita podatke iz datoteke i upisuje ih u memorijski prostor na koji pokazuje `buff`.

```c
#include <unistd.h>

ssize_t read(int filedes, void *buff, size_t nbytes);
```

**Povratna vrijednost:** broj stvarno pročitanih bajtova (može biti manji od `nbytes`), `0` po dosizanju kraja datoteke (*EOF*), ili `-1` u slučaju greške.

Povratni tip `ssize_t` (*signed size*) je POSIX-om definiran ekvivalent `size_t` koji može imati i negativnu vrijednost — neophodan da bi funkcija mogla vratiti `-1` kao oznaku greške.

**Argumenti:**

- **`filedes`** — deskriptor otvorene datoteke.
- **`buff`** — pokazivač na statički ili dinamički alociran memorijski blok u koji se upisuju pročitani podaci. `read()` ne alocira memoriju sam — to je zadaća programera.
- **`nbytes`** — maksimalan broj bajtova koji će se pokušati pročitati.

Čitanje počinje na trenutnom file offsetu. Ova vrijednost je nakon otvaranja datoteke uvijek `0`, tj. pokazuje na početak datoteke. Nakon svakog uspješnog poziva `read()`-a offset se automatski pomiče za broj pročitanih bajtova. Razlozi zbog kojih stvarno pročitani broj može biti manji od traženog uključuju dostignut kraj datoteke, čitanje s terminala (koji vraća redak koji je korisnik utipkao odjednom), ili čitanje s mrežnog socketa.

### Funkcija `write()`

Upisuje podatke iz memorijskog bloka u datoteku identificiranu deskriptorom `filedes`.

```c
#include <unistd.h>

ssize_t write(int filedes, const void *buff, size_t nbytes);
```

**Povratna vrijednost:** broj stvarno upisanih bajtova (po POSIX-u garantirano jednak `nbytes` u uspješnom slučaju za obične datoteke; može biti manji za neke tipove datoteka poput mrežnih socketa), ili `-1` u slučaju greške.

**Argumenti:**

- **`filedes`** — deskriptor otvorene datoteke.
- **`buff`** — pokazivač na memorijski blok koji se upisuje.
- **`nbytes`** — broj bajtova koji se piše.

Pisanje počinje na trenutnom file offsetu, a nakon uspješnog upisa offset se pomiče za broj upisanih bajtova. Iznimka je kad je datoteka otvorena s `O_APPEND` zastavicom — tada jezgra prije svakog pisanja postavlja offset na kraj datoteke.

## Primjeri

### Otvaranje, čitanje i pisanje

- [**`read_file.c`**](read_file.c) — otvara datoteku `moja_datoteka.txt` sistemskim pozivom `open()` u načinu samo za čitanje (`O_RDONLY`) i ispisuje njen sadržaj na standardni izlaz, čitajući znak po znak pozivima `read()` te ih prosljeđujući pozivima `write()`. Demonstrira osnovni slijed rada s datotekom (`open` → `read`/`write` → `close`) i rukovanje greškama preko povratnih vrijednosti sistemskih poziva.

  ```c
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <stdio.h>

  int main() {
      int n, fd;
      char s;

      fd = open("moja_datoteka.txt", O_RDONLY);
      if (fd == -1) {
          perror("open");
          return 1;
      }

      while ((n = read(fd, &s, 1)) > 0) {
          if (write(STDOUT_FILENO, &s, 1) != 1) {
              perror("write");
              return 1;
          }
      }

      close(fd);
      return 0;
  }
  ```

  ```sh
  ./read_file
  ```

- [**`io_copy.c`**](io_copy.c) — kopira standardni ulaz na standardni izlaz, čitajući u međuspremnik konstantne veličine (`BUFFSIZE`). Za razliku od `read_file.c` koji čita znak po znak, ovdje se u jednom pozivu `read()` pokušava pročitati cijeli blok bajtova, čime se višestruko smanjuje broj sistemskih poziva potrebnih za istu količinu prenesenih podataka. Primjer ujedno ilustrira koncept "sve je datoteka": pri pokretanju bez preusmjeravanja, standardni ulaz vezan je na tipkovnicu, a standardni izlaz na terminal — isti `read()` i `write()` koji bi radili s običnim datotekama na disku ovdje rade s terminalom. Svaki redak koji korisnik utipka program odmah ispiše natrag, a petlja se prekida pritiskom `Ctrl+D` (oznaka kraja ulaza, EOF):

  ```c
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <stdio.h>

  #define BUFFSIZE 1024

  int main() {
      int n;
      char buf[BUFFSIZE];

      while ((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0) {
          if (write(STDOUT_FILENO, buf, n) != n) {
              perror("write");
              return 1;
          }
      }

      if (n < 0)
          perror("read");

      return 0;
  }
  ```

  ```
  $ ./io_copy
  Prvi red teksta
  Prvi red teksta
  Drugi red teksta
  Drugi red teksta
  ^D
  $
  ```

  U kombinaciji s preusmjeravanjem standardnog izlaza, isti program može poslužiti kao jednostavan alat za upis teksta u datoteku — sve što korisnik utipka do `Ctrl+D` završi kao sadržaj datoteke, a terminal u međuvremenu ne prikazuje ništa dodatno jer je standardni izlaz preusmjeren:

  ```
  $ ./io_copy > datoteka.txt
  Prvi red teksta
  Drugi red teksta
  ^D
  $ cat datoteka.txt
  Prvi red teksta
  Drugi red teksta
  ```

### Pozicioniranje unutar datoteke (`lseek`)

Sistemski poziv `lseek()` mijenja file offset otvorene datoteke. Koristi se kad želimo čitati ili pisati na specifičnoj poziciji bez sekvencijalnog prolaska.

```c
#include <sys/types.h>
#include <unistd.h>

off_t lseek(int filedes, off_t offset, int whence);
```

**Povratna vrijednost:** novi file offset (broj bajtova od početka datoteke), ili `(off_t)-1` u slučaju greške.

**Argumenti:**

- **`filedes`** — deskriptor otvorene datoteke.
- **`offset`** — pomak; tumači se relativno prema vrijednosti `whence`.
- **`whence`** — referentna točka:
  - `SEEK_SET` — pomak je apsolutni, mjeren od početka datoteke,
  - `SEEK_CUR` — pomak je relativan u odnosu na trenutni offset,
  - `SEEK_END` — pomak je relativan u odnosu na kraj datoteke (može biti i negativan da se vrati unazad).

`lseek()` se zove "seek" jer pomiče glavu za čitanje na proizvoljnu poziciju u datoteci — ali to "pomicanje" je samo logičko, na razini jezgrinog file offseta. Stvarni pristup disku događa se tek pri sljedećem `read()`-u ili `write()`-u.

- [**`f_strip.c`**](f_strip.c) — demonstrira `lseek()` s `SEEK_SET` (apsolutno pozicioniranje od početka datoteke). Program otvara datoteku za pisanje, upisuje prvi niz znakova, `lseek`-om postavlja offset na 15. bajt i upisivanjem drugog niza **prepisuje** dio postojećeg sadržaja. Rezultat pokazuje da se pozicioniranje unutar otvorene datoteke može slobodno kombinirati s čitanjem i pisanjem — offset koji jezgra pamti nije povezan s fizičkim rasporedom blokova na disku.

  ```c
  #include <stdio.h>
  #include <string.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>

  int main() {
      char buf1[] = "Prvi redak teksta";
      char buf2[] = "Drugi redak teksta";
      int fd;

      fd = open("file.strip", O_WRONLY | O_CREAT | O_TRUNC, (mode_t)0644);
      if (fd == -1) {
          perror("open");
          return 1;
      }

      if (write(fd, buf1, strlen(buf1)+1) != strlen(buf1)+1) {
          perror("write buf1");
          return 1;
      }

      if (lseek(fd, 15, SEEK_SET) == -1) {
          perror("lseek");
          return 1;
      }

      if (write(fd, buf2, strlen(buf2)+1) != strlen(buf2)+1) {
          perror("write buf2");
          return 1;
      }

      close(fd);
      exit(0);
  }
  ```

  Rezultat se može provjeriti UNIX naredbom `cat`, koja ispisuje sadržaj jedne ili više datoteka na standardni izlaz:

  ```
  $ ./f_strip
  $ cat file.strip
  Prvi redak teksDrugi redak teksta
  ```

  Prvih 15 bajtova (`Prvi redak teks`) ostalo je netaknuto iz prvog upisa, a od 15. bajta nadalje vidljiv je sadržaj drugog niza koji je `lseek` + `write` upisao preko ostatka prethodnog teksta.

- [**`f_hole.c`**](f_hole.c) — demonstrira `lseek()` s `SEEK_CUR` (relativno pomicanje od trenutne pozicije). Nakon upisa prvog niza, offset se pomiče 15 bajtova naprijed — **iza** trenutnog kraja datoteke — i tek se tada upisuje drugi niz. Petnaest bajtova između prve i druge linije ostaje kao **rupa**, područje koje pri čitanju iščitava kao bajtovi s vrijednošću 0 (null terminator), iako `write()` ondje ništa nije upisao.

  ```c
  #include <stdio.h>
  #include <string.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>

  int main() {
      char buf1[] = "Prvi redak teksta";
      char buf2[] = "Drugi redak teksta";
      int fd;

      fd = creat("file.hole", (mode_t)0644);
      if (fd == -1) {
          perror("creat");
          return 1;
      }

      if (write(fd, buf1, strlen(buf1)+1) != strlen(buf1)+1) {
          perror("write buf1");
          return 1;
      }

      if (lseek(fd, 15, SEEK_CUR) == -1) {
          perror("lseek");
          return 1;
      }

      if (write(fd, buf2, strlen(buf2)+1) != strlen(buf2)+1) {
          perror("write buf2");
          return 1;
      }

      close(fd);
      exit(0);
  }
  ```

  ```
  $ ./f_hole
  $ ls -l file.hole
  -rw-r--r-- 1 dkrst users 52 Apr 22 12:54 file.hole
  $ du -h file.hole
  4.0K    file.hole
  $ od -c file.hole
  0000000   P   r   v   i       r   e   d   a   k       t   e   k   s   t
  0000020   a  \0  \0  \0  \0  \0  \0  \0  \0  \0  \0  \0  \0  \0  \0  \0
  0000040  \0   D   r   u   g   i       r   e   d   a   k       t   e   k
  0000060   s   t   a  \0
  0000064
  ```

  Tri naredbe daju pogled na datoteku iz tri različita kuta:

  - `ls -l` prijavljuje **logičku veličinu** datoteke — 52 bajta. To je raspon od početka datoteke do posljednjeg upisanog bajta, uključujući i bajtove kroz rupu koje `write()` nikad nije dotaknuo. Isti broj vratio bi i `lseek(fd, 0, SEEK_END)` u programu.

  - `du -h` prijavljuje **stvarno zauzeće diska** — 4.0K (odnosno 4096 bajtova, tipična veličina bloka Linux datotečnih sustava poput *ext4*; na drugim sustavima može biti 512 B, 8 KB ili više). Razlog zašto datoteka od 52 bajta zauzima puni 4 KB blok leži u činjenici da su tvrdi diskovi u UNIX-u **blok specijalni uređaji** (vidi poglavlje *Osnove UNIX-a*): podaci se na njima prenose i adresiraju u blokovima fiksne veličine, ne bajt po bajt. Datotečni sustav slijedi tu granularnost — najmanja jedinica alokacije za svaku datoteku je jedan cijeli blok, pa i datoteka od jednog jedinog bajta zauzima koliko i puni blok.

  - `od -c` otkriva unutarnju strukturu — niz od 15 uzastopnih `\0` bajtova između dva upisana niza točno je područje rupe. S gledišta procesa, tih 15 bajtova postoji i čitanjem bi se dobile nule; jezgra ih pri čitanju transparentno popunjava bez da su ti podaci ikad bili fizički upisani na disk.

### Prava pristupa i maska (`umask`)

Sistemski poziv `umask()` postavlja masku kreiranja datoteka za trenutni proces. Maska predstavlja bitove **koji će biti uklonjeni** iz prava pristupa kada proces stvara nove datoteke (npr. preko `open()`-a s `O_CREAT` ili `creat()`-a).

```c
#include <sys/types.h>
#include <sys/stat.h>

mode_t umask(mode_t cmask);
```

**Povratna vrijednost:** prethodna vrijednost maske.

**Argumenti:**

- **`cmask`** — nova vrijednost maske; kombinacija istih konstanti kao za `mode` u `open()`-u.

Algoritam je jednostavan: za svako kreiranje datoteke, jezgra uzima `mode` koji je program tražio i **iz njega ukloni** sve bitove koji su postavljeni u maski. Tako je rezultantni način pristupa `mode & ~cmask`. Maska se nasljeđuje od roditelja procesa; tipična vrijednost u shellu je `0022` (oduzima pravo pisanja grupi i ostalima).

- [**`perm_mask.c`**](perm_mask.c) — ilustrira djelovanje maske kreiranja datoteke (`umask`) na prava pristupa pri pozivu `creat()`. Program dva puta stvara datoteku s istim zatraženim pravima (`rw-rw-r--`), jednom s maskom postavljenom na 0 (nikakva prava se ne oduzimaju), a jednom s maskom koja eksplicitno isključuje pisanje za grupu i čitanje/pisanje za ostale. Usporedbom stvarno dobivenih prava vidi se učinak maske:

  ```c
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <stdio.h>
  #include <stdlib.h>

  #define PRAVA S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH

  int main() {
      umask(0);
      if (creat("datoteka1", PRAVA) < 0)
          perror("creat datoteka1");

      umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
      if (creat("datoteka2", PRAVA) < 0)
          perror("creat datoteka2");

      return 0;
  }
  ```

  ```
  $ ./perm_mask
  $ ls -al datoteka1 datoteka2
  -rw-rw-r-- 1 dkrst users 0 Jan 16 16:23 datoteka1
  -rw------- 1 dkrst users 0 Jan 16 16:23 datoteka2
  ```

  Datoteka `datoteka1` zadržala je sva zatražena prava jer je maska bila 0, dok su u slučaju datoteke `datoteka2` maskom isključena sva prava za grupu i ostale korisnike.

### Argumenti naredbenog retka

Naredbe i programi na UNIX sustavu često prihvaćaju **argumente naredbenog retka** — niz tekstualnih riječi koje korisnik upisuje uz ime programa kako bi izmijenio njegovo ponašanje, ili da bi mu predao ulaz (npr. ime datoteke nad kojom program radi). Primjer iz svakodnevnog rada s ljuskom je `cp izvor.txt cilj.txt`, gdje `cp` prima dva argumenta. Da bi C program mogao iščitati te argumente, `main` se deklarira u proširenom obliku:

```c
int main(int argc, char *argv[])
```

Ovdje `argc` sadrži broj argumenata, a `argv` je polje nizova znakova u kojemu je svaki argument zaseban niz. Bitno je naglasiti da `argc` uključuje i samu naredbu, ne samo argumente koje korisnik dodaje uz nju. Po konvenciji, `argv[0]` je ime kojim je program pokrenut (uključujući i putanju ako je upisana), a stvarni argumenti slijede od `argv[1]` nadalje. Tako pri pozivu `./f_write izlaz.txt`, vrijednosti će biti `argc = 2`, `argv[0] = "./f_write"`, `argv[1] = "izlaz.txt"` — `argc` je 2 jer broji i samo ime programa i jedan argument koji mu je dan. Programi obično odmah na početku provjeravaju je li `argc` u očekivanom rasponu i ako nije, ispišu uputu o korištenju te uredno završe.

- [**`f_write.c`**](f_write.c) — čita sa standardnog ulaza i upisuje u novokreiranu datoteku čije se ime zadaje kao argument naredbenog retka. Odmah na početku programa provjerava se `argc != 2` — očekuje se točno jedan argument (ime izlazne datoteke) uz ime programa. Ako korisnik program pozove bez argumenta ili s previše njih, program ispisom poruke:

  ```c
  #include <stdio.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <unistd.h>

  #define FMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

  int main(int argc, char *argv[]) {
      int fd, n;
      char s;

      if (argc != 2) {
          printf("koristenje: %s <ime_datoteke>\n", argv[0]);
          return 0;
      }

      fd = creat(argv[1], FMODE);
      if (fd == -1) {
          perror("creat");
          return 1;
      }

      while ((n = read(STDIN_FILENO, &s, 1)) > 0)
          if (write(fd, &s, 1) < 0) {
              perror("write");
              return 1;
          }

      close(fd);
      return 0;
  }
  ```

  ```c
  printf("koristenje: %s <ime_datoteke>\n", argv[0]);
  ```

  javlja upute o korištenju i završava. Konverzija `%s` zamjenjuje se upravo vrijednošću `argv[0]` — imenom kojim je program pokrenut — pa poruka korisniku uvijek automatski odražava točan naziv pod kojim je program bio zvan, neovisno o tome je li preimenovan ili pozvan preko simboličke veze. Ovakva provjera ulaznih argumenata uobičajen je uzorak u svim UNIX programima.

  Program se poziva s imenom izlazne datoteke kao jedinim argumentom; nakon toga sve što korisnik utipka do oznake kraja ulaza (`Ctrl+D`) zapisuje se u tu datoteku:

  ```
  $ ./f_write izlaz.txt
  Prvi red teksta
  Drugi red teksta
  ^D
  $ cat izlaz.txt
  Prvi red teksta
  Drugi red teksta
  ```

- [**`f_cat.c`**](f_cat.c) — pojednostavljena implementacija UNIX naredbe `cat`. Osnovna petlja čitanja i pisanja enkapsulirana je u pomoćnoj funkciji `rw(fdin, fdout)` koja čita s jednog deskriptora i piše na drugi. Ponašanje programa ovisi o argumentima naredbenog retka:

  ```c
  #include <stdio.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <unistd.h>

  int rw(int fdin, int fdout) {
      int n;
      char s;
      while ((n = read(fdin, &s, 1)) > 0)
          write(fdout, &s, 1);

      return n;
  }

  int main(int argc, char **argv) {
      int k, fd;
      if (argc < 2) {
          rw(STDIN_FILENO, STDOUT_FILENO);
      } else {
          for (k = 1; k < argc; k++) {
              fd = open(argv[k], O_RDONLY);
              if (fd < 0)              /* ne mogu otvoriti */
                  perror("open");
              else {
                  rw(fd, STDOUT_FILENO);
                  close(fd);
              }
          }
      }

      return 0;
  }
  ```

  - **Bez argumenata** program poziva `rw(STDIN_FILENO, STDOUT_FILENO)` i tako praktički radi istu stvar kao `io_copy`: čita sa standardnog ulaza i ispisuje na standardni izlaz.
  - **S jednim ili više argumenata** program redom otvara svaku navedenu datoteku pozivom `open()` i njezin sadržaj prosljeđuje na standardni izlaz pozivom iste funkcije `rw()`, ali sad s file deskriptorom otvorene datoteke umjesto standardnog ulaza.

  Program time u bitnome reproducira funkcionalnost UNIX naredbe `cat` i ujedno sažima sve koncepte iz prethodnih primjera: otvaranje datoteka s provjerom grešaka, jedinstveno sučelje preko file deskriptora, rad s argumentima naredbenog retka, i uniformno korištenje istih sistemskih poziva bez obzira na izvor (datoteka ili standardni tok).

  Pokretanje:

  ```
  $ ./f_cat
  ```

  Bez argumenata, program se ponaša kao `io_copy` — čita sa standardnog ulaza i ispisuje na standardni izlaz, sve dok ne dobije EOF.

  ```
  $ ./f_cat f_cat.c
  ```

  S jednim argumentom, program ispisuje sadržaj te datoteke na standardni izlaz. U gornjem primjeru program ispisuje vlastiti izvorni kod — i to bez ičega posebnoga: jednako bi ispisao bilo koju drugu tekstualnu datoteku navedenu kao argument.

  ```
  $ ./f_cat datoteka1.txt datoteka2.txt
  ```

  Sa više argumenata, program ispisuje navedene datoteke jednu za drugom — u ovom slučaju najprije sadržaj `datoteka1.txt`, a odmah za njim sadržaj `datoteka2.txt` (naravno, pod uvjetom da obje datoteke postoje u radnom direktoriju). Time se reproducira ponašanje UNIX naredbe `cat` po kojoj je program i nazvan: spaja (engl. *concatenate*) sadržaje više datoteka u jedinstveni izlazni tok.

### Dijeljenje datoteka i preusmjeravanje (`dup`, `dup2`)

Da bismo razumjeli kako UNIX upravlja otvorenim datotekama — i kako više procesa ili više deskriptora unutar istog procesa može pristupati istoj datoteci — potrebno je upoznati interne strukture jezgre. Razumijevanje ovih koncepata bit će nam korisno i u kasnijim poglavljima jer se na te iste strukture stalno vraćamo kad govorimo o procesima i nasljeđivanju resursa.

Jezgra održava tri strukture u memoriji koje surađuju pri rukovanju otvorenim datotekama:

- **Tablica procesa** — jezgra svakom aktivnom procesu dodjeljuje jedan zapis u tablici procesa. Zapis sadrži sve informacije o procesu (identifikator, stanje, otvorene datoteke, masku `umask` itd.). U sklopu zapisa o procesu nalazi se **tablica deskriptora**: privatna lista svih deskriptora otvorenih unutar tog procesa. Tablicu procesa detaljnije ćemo razraditi u kasnijim poglavljima.
- **Tablica datoteka** — globalna struktura jezgre. U nju se upisuje novi zapis pri **svakom** pojedinom otvaranju datoteke. Zapis sadrži statusne zastavice (`O_RDONLY`, `O_APPEND`, ...), trenutni file offset, te pokazivač na zapis u v-node tablici.
- **v-node tablica** — apstrakcija fizičke datoteke. Svaki zapis opisuje samu datoteku (tip, lokaciju, ...). Više otvaranja iste datoteke pokazuju na isti v-node, ali svaki ima vlastiti zapis u tablici datoteka.

Slika u nastavku prikazuje tipičnu situaciju u kojoj dva procesa nezavisno otvaraju **istu** fizičku datoteku — svaki vlastitim pozivom `open()`:

![Dva procesa nezavisno otvaraju istu datoteku](slike/io_strukture_dva_procesa.png)

Iako se radi o istoj datoteci, jezgra svakom procesu dodjeljuje zaseban zapis u tablici datoteka — pa svaki ima vlastiti, neovisni file offset i vlastite zastavice. Dijeljenje se događa samo na razini v-node tablice. Posljedica: pomak jednog procesa u datoteci ne utječe na poziciju drugoga, ali eventualne izmjene sadržaja postaju vidljive obama procesima.

Drugi oblik dijeljenja događa se **na razini same tablice datoteka**: dva ili više deskriptora pokazuju na *isti* zapis u tablici datoteka, pa dijele i isti file offset i iste statusne zastavice. Do ovoga može doći na dva načina:

1. **nasljeđivanjem deskriptora** kad jedan proces pozivom `fork()` stvori novi (vidi poglavlje o procesima),
2. **dupliciranjem deskriptora** unutar istog procesa pozivom `dup()` ili `dup2()`.

![Dijeljenje na razini tablice datoteka](slike/io_strukture_dijeljenje_file_table.png)

Slika pokazuje oba slučaja: procesi P1 i P2 dijele zapis u tablici datoteka jer je P2 stvoren `fork()`-om iz P1; unutar procesa P3 dva deskriptora (3 i 4) pokazuju na isti zapis jer je jedan nastao dupliciranjem drugoga.

#### Dupliciranje deskriptora — `dup()` i `dup2()`

```c
#include <unistd.h>

int dup(int filedes);
int dup2(int filedes, int filedes2);
```

**Povratna vrijednost:** novi file deskriptor, ili `-1` u slučaju greške.

**Argumenti:**

- **`filedes`** — otvoreni deskriptor koji se duplicira.
- **`filedes2`** (samo za `dup2`) — ciljni deskriptor na koji se duplicira `filedes`. Ako je `filedes2` već otvoren, `dup2()` ga **atomski zatvara** prije dupliciranja. Ako je `filedes2 == filedes`, poziv nema učinka.

`dup()` duplicira `filedes` na **najniži slobodni deskriptor**. `dup2()` se razlikuje po tome što ciljni deskriptor zadajemo eksplicitno — i ta operacija je atomska.

Na prvi pogled nije jasno čemu unutar istog procesa služe dva deskriptora koja pokazuju na isti zapis u tablici datoteka. Praktična primjena je upravo **preusmjeravanje standardnog ulaza i izlaza** — mehanizam kojim ljuska ostvaruje preusmjeravanje pomoću operatora `<` i `>`.

- [**`dup_redirect.c`**](dup_redirect.c) — preusmjerava standardni izlaz na datoteku korištenjem `close()` + `dup()`. Otvorimo datoteku, zatvorimo deskriptor 1 (standardni izlaz, čime taj broj postane slobodan), pa pozovemo `dup(fd)`. Budući da `dup()` vraća **najniži slobodni** deskriptor, dobit ćemo upravo 1 — od ovog trenutka standardni izlaz pokazuje na našu datoteku, pa svaki `printf` u nastavku završava u datoteci, ne u terminalu.

  ```c
  #include <stdio.h>
  #include <unistd.h>
  #include <fcntl.h>

  int main() {
      int fd, newfd;

      fd = open("izlaz.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
          perror("open");
          return 1;
      }

      close(STDOUT_FILENO);    /* zatvori standardni izlaz */
      newfd = dup(fd);         /* dup vraća najnižu slobodnu vrijednost = 1 */

      if (newfd != fd)
          close(fd);

      printf("Ovaj tekst nece zavrsiti u terminalu!\n");

      return 0;
  }
  ```

  ```
  $ ./dup_redirect
  $ cat izlaz.txt
  Ovaj tekst nece zavrsiti u terminalu!
  ```

  Provjera `if (newfd != fd) close(fd)` štiti nas od posebnog slučaja kad bi `dup()` slučajno vratio isti broj kao stari `fd` (tada bi `close(fd)` zatvorio jedini preostali deskriptor).

  Iako program radi, sadrži suptilan problem: pozivi `close(1)` i `dup(fd)` su **dvije odvojene operacije**. U višenitnom programu druga nit bi između njih mogla pozvati `open()` ili `creat()` i preuzeti upravo oslobođeni deskriptor 1 za sebe. Race condition kakav smo opisali ranije, ali na razini niti unutar istog procesa.

- [**`dup2_redirect.c`**](dup2_redirect.c) — isti zadatak, samo s `dup2()`. Ovo je preferirana varijanta jer:
  - **atomski** zatvara odredišni deskriptor (`STDOUT_FILENO`) i postavlja novi na njegovo mjesto u jednoj nedjeljivoj operaciji,
  - eksplicitno zadajemo odredišni deskriptor pa se ne oslanjamo na pretpostavku da je 1 najniži slobodni.

  ```c
  #include <stdio.h>
  #include <unistd.h>
  #include <fcntl.h>

  int main() {
      int fd;

      fd = open("izlaz.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
          perror("open");
          return 1;
      }

      if (dup2(fd, STDOUT_FILENO) != fd)
          close(fd);

      printf("Ovaj tekst zavrsava u datoteci izlaz.txt!\n");

      return 0;
  }
  ```

  ```
  $ ./dup2_redirect
  $ cat izlaz.txt
  Ovaj tekst zavrsava u datoteci izlaz.txt!
  ```

  U produkcijskom kodu uvijek se koristi `dup2()` (ili moderniju `dup3()`). Pri pisanju programa koji preusmjeravaju ulaz/izlaz — npr. ljusku koja izvršava `program > datoteka` — ovaj uzorak je standardna tehnika.

  Bitna napomena: čak i kad u programu koristimo `printf`, koji je funkcija standardne C biblioteke, ona se u konačnici svodi na sistemski poziv `write()` s `STDOUT_FILENO` (tj. 1) kao prvim argumentom. Stoga preusmjeravanje deskriptora 1 utječe i na `printf` ispise — što je ono što i koristimo u gornjim primjerima.

## Prevođenje

Direktorij dolazi s priloženim `Makefile`-om koji prati iste konvencije kao i Makefile u `osnove_programiranja/` (varijable `CC`, `CFLAGS`, `LDFLAGS`, `TARGETS`; implicitno pravilo `.c.o`; pravila `default`, `all`, `clean`). Detaljan opis strukture i korake gradnje Makefilea vidjeti u [`../osnove_programiranja/README.md`](../osnove_programiranja/README.md).

Tipična uporaba:

```sh
make              # gradi zadani cilj (perm_mask)
make all          # gradi sve primjere
make f_strip      # gradi samo zadani primjer
make clean        # briše izvršne, objektne i generirane datoteke
```

Pravilo `clean` briše sve izvršne datoteke, objektne datoteke, privremene `*~` datoteke, `a.out`, kao i datoteke koje primjeri sami stvaraju pri pokretanju — `file.strip`, `file.hole`, `datoteka1`, `datoteka2` — kako bi se radni direktorij vratio u čisto stanje.
