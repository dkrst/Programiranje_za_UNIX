# Upravljanje datotekama

U ovom poglavlju upoznat ćemo dublji UNIX pogled na datoteke: njihove **atribute**, kako jezgra organizira informacije o njima, te kako iz programa dohvatiti i mijenjati ta svojstva. Dok smo se u prethodnom poglavlju bavili **sadržajem** datoteka — čitanjem i pisanjem bajtova — ovdje stojimo "stepenicu više" i gledamo **metapodatke**: tip datoteke, vlasnika, prava pristupa, vremena pristupa, broj linkova, fizičku organizaciju na disku. Sve teme koje slijede klasično su pokrivene u *Advanced Programming in the UNIX Environment*, Stevens & Rago [1], i čitatelj koji želi šire i detaljnije obrade može tamo pronaći dodatne primjere i rasprave o specifičnim slučajevima.

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

Posebno se zaustavljamo na **simboličkim linkovima** jer ćemo ih sresti kroz cijelo poglavlje. Simbolički link je u osnovi obična datoteka koja sadrži tekst — doslovno putanju do druge datoteke. Pri tom datoteka na koju link pokazuje ne mora uopće postojati: link će svejedno čuvati string koji predstavlja tu putanju, čak i ako na toj putanji nema ničega. Ako se datoteka na koju link pokazuje stvori naknadno, link će na nju automatski pokazivati. Ovaj mehanizam temelji se isključivo na razrješavanju puta — jezgra svaki put kad pristupa linku iznova čita njegov sadržaj i koristi ga kao putanju.

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

- **`stat()`** — datoteka se zadaje putanjom; ako je putanja simbolički link, funkcija ga slijedi i vraća atribute datoteke na koju link pokazuje (umjesto atributa za simbolički link — datoteku koja je navedena kao argument funkcije).
- **`fstat()`** — atributi otvorene datoteke; umjesto putanje kao argument se zadaje deskriptor na kojem je datoteka otvorena.
- **`lstat()`** — kao `stat()`, ali ako je putanja simbolički link, ne slijedi ga nego vraća atribute samog linka (njegovu veličinu, vrijeme stvaranja itd.).

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

- **`st_mode`** — sadrži dvije informacije u istom polju: tip datoteke (regularna, direktorij, link...) i prava pristupa (devet bitova `rwxrwxrwx` plus posebni bitovi). Tip se ne čita izravno iz brojčane vrijednosti, nego pomoću posebnih makro funkcija opisanih u nastavku.
- **`st_ino`** — broj i-node zapisa, jedinstvene strukture u kojoj jezgra čuva sve atribute datoteke. Dvije datoteke s istim i-node brojem na istom datotečnom sustavu su zapravo **ista datoteka** (vidi hard linkove).
- **`st_dev`** — identifikator uređaja (datotečnog sustava) na kojem datoteka leži. Par (`st_dev`, `st_ino`) jedinstveno identificira datoteku na cijelom sustavu.
- **`st_rdev`** — koristi se samo za blok i karakter specijalne datoteke (uređaje); u njemu su kodirani tzv. *major* i *minor* brojevi uređaja, što izlazi izvan onoga što nas u ovom poglavlju zanima.
- **`st_nlink`** — broj hard linkova koji pokazuju na ovaj i-node. Datoteka može imati više imena u datotečnom sustavu i sva su ravnopravna — niti jedno nije "izvorno" ili "primarno", svaki je obični zapis u nekom direktoriju koji upućuje na isti i-node. Kad brojač padne na 0 (kad se obriše i zadnje ime), datoteka se fizički briše s diska.
- **`st_uid`, `st_gid`** — brojčani identifikatori vlasnika i grupe vlasnika. Tekstualna imena (npr. `dkrst`, `users`) dohvaćaju se naknadno iz sistemskih datoteka `/etc/passwd` i `/etc/group`.
- **`st_size`** — veličina sadržaja u bajtovima. Za regularne datoteke je to broj bajtova, za direktorije veličina ovisi o implementaciji, a za simboličke linkove — sjetimo se ranije napomene da je link u osnovi tekstualna datoteka — to je broj znakova u stringu putanje koji link sadrži.
- **`st_atime`** — vrijeme zadnjeg **pristupa sadržaju** (čitanja). Neke UNIX implementacije ne ažuriraju ovo polje pri svakom čitanju zbog performansi.
- **`st_mtime`** — vrijeme zadnje **izmjene sadržaja** (pisanja u datoteku).
- **`st_ctime`** — vrijeme zadnje promjene **statusa** datoteke: ne sadržaja, nego nekog atributa (prava pristupa, vlasništvo, broj linkova). Razlika prema `mtime` je suptilna ali važna: `chmod` mijenja `ctime` ali ne `mtime`.
- **`st_blksize`** — optimalna veličina jedinice I/O prijenosa za ovu datoteku; često 4096 B na suvremenim sustavima.
- **`st_blocks`** — broj 512-bajtnih blokova fizički alociranih datoteci. Kod *rijetkih* datoteka ("rupa") može biti znatno manji od `st_size / 512`.

#### Brojač linkova kod direktorija

Bilo bi prirodno očekivati da svaki novostvoreni direktorij ima `st_nlink` jednak 1 — ipak je riječ o samo jednom imenu u nadređenom direktoriju. Ipak, već u trenutku stvaranja, brojač linkova svakog direktorija je 2. Razlog je u dva posebna unosa koja jezgra automatski stvara unutar svakog direktorija: `.` (link na sam taj direktorij) i `..` (link na nadređeni direktorij). Tako svaki direktorij na samom početku ima dva imena koja na njega upućuju — jedno u direktoriju u kojem se promatrani direktorij nalazi (njegovo "stvarno" ime) i jedno unutar njega samoga (`.`).

Štoviše, svaki put kad se unutar nekog direktorija stvori novi poddirektorij, brojač linkova raste još za jedan jer poddirektorij sa sobom donosi i `..` koji pokazuje natrag. Pogledajmo to izravno u ljusci:

```
$ mkdir test_dir
$ cd test_dir
$ ls -al
total 8
drwxr-xr-x  2 dkrst users 4096 May  5 15:41 .
drwxr-xr-x  5 dkrst users 4096 May  5 15:41 ..
```

Tek što je stvoren, `test_dir` ima `nlink = 2` (drugi stupac u retku za `.`). Sad stvorimo poddirektorij:

```
$ mkdir poddir
$ ls -al
total 12
drwxr-xr-x  3 dkrst users 4096 May  5 15:41 .
drwxr-xr-x  5 dkrst users 4096 May  5 15:41 ..
drwxr-xr-x  2 dkrst users 4096 May  5 15:41 poddir
```

Brojač linkova za `test_dir` (vidljiv u retku za `.`) sad je 3, jer mu je `poddir/..` dodao novu referencu. Sam `poddir` također kreće od 2. Općenito vrijedi da brojač linkova direktorija jednak je `2 + broj poddirektorija` — što ujedno objašnjava zašto naredba `find -type d -links 2` (ili slične formulacije) može poslužiti za pronalazak direktorija koji nemaju poddirektorija.

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

  #### Ponašanje na simboličkom linku

  Stvorimo iz ljuske simbolički link `drugoime.c` koji pokazuje na `fileinfo.c`:

  ```
  $ ln -s fileinfo.c drugoime.c
  $ ls -l fileinfo.c drugoime.c
  lrwxrwxrwx 1 dkrst users   10 May  5 16:14 drugoime.c -> fileinfo.c
  -rw-r--r-- 1 dkrst users 1300 May  5 15:22 fileinfo.c
  ```

  Iz ispisa `ls -l` vidimo da je `drugoime.c` zaista simbolički link (početno slovo `l` u prvom stupcu) koji pokazuje na `fileinfo.c`. Sad nad njim pokrenimo naš program:

  ```
  $ ./fileinfo drugoime.c
  Datoteka: drugoime.c
    tip:           regularna datoteka
    i-node broj:   63
    prava:         644
    vlasnik UID:   1000
    grupa GID:     1000
    velicina:      1300 B
    broj linkova:  1
    zadnji pristup:  Tue May  5 16:14:39 2026
    zadnja izmjena:  Tue May  5 15:22:01 2026
    zadnja promjena: Tue May  5 16:14:38 2026
  ```

  Iako smo u argumentu zadali `drugoime.c`, ispisani atributi pripadaju datoteci `fileinfo.c` — što vidimo po veličini od 1300 B (link sam ima samo 10 znakova) i tipu "regularna datoteka". To je posljedica činjenice da `stat()` **slijedi simbolički link** i vraća atribute datoteke na koju link pokazuje, ne samog linka.

### Razlika `stat` i `lstat`

Kako vidjeti atribute samog linka, a ne datoteke na koju on pokazuje? Tu na scenu stupa funkcija `lstat()`. Da bismo ilustrirali razliku, modificirajmo `fileinfo.c` tako da koristi `lstat` umjesto `stat` — sve ostalo neka ostane isto.

- [**`fileinfo2.c`**](fileinfo2.c) — identičan programu `fileinfo.c`, jedina razlika je u tome što umjesto `stat()` koristi `lstat()`.

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

      if (lstat(argv[1], &st) < 0) {
          perror("lstat");
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

  Usporedimo izlaz oba programa nad našim simboličkim linkom `drugoime.c`:

  ```
  $ ./fileinfo drugoime.c
  Datoteka: drugoime.c
    tip:           regularna datoteka
    i-node broj:   63
    prava:         644
    vlasnik UID:   1000
    grupa GID:     1000
    velicina:      1300 B
    broj linkova:  1
    zadnji pristup:  Tue May  5 16:14:39 2026
    zadnja izmjena:  Tue May  5 15:22:01 2026
    zadnja promjena: Tue May  5 16:14:38 2026

  $ ./fileinfo2 drugoime.c
  Datoteka: drugoime.c
    tip:           simbolicki link
    i-node broj:   81
    prava:         777
    vlasnik UID:   1000
    grupa GID:     1000
    velicina:      10 B
    broj linkova:  1
    zadnji pristup:  Tue May  5 16:14:39 2026
    zadnja izmjena:  Tue May  5 16:14:39 2026
    zadnja promjena: Tue May  5 16:14:39 2026
  ```

  Razlika je sad očita. `fileinfo` (koristi `stat`) slijedi simbolički link i pokazuje atribute datoteke `fileinfo.c` — i-node 63, veličina 1300 B, tip "regularna datoteka". `fileinfo2` (koristi `lstat`) ostaje na samom linku — i-node 81, veličina 10 B (koliko je dugačak string `"fileinfo.c"`), tip "simbolicki link".

  Koju ćemo funkciju koristiti, `stat` ili `lstat`, ovisi o tome želimo li dobiti informaciju da je određena datoteka simbolički link te njezine atribute, ili informaciju o datoteci na koju simbolički link pokazuje. Odabir ovisi o namjeni našeg programa.

  Međutim, potrebno je voditi računa o jednoj posebnoj situaciji koja se javlja kod programa koji pretražuju datotečni sustav (npr. `find` ili rekurzivni obilazak stabla direktorija). Ako u nekom direktoriju postoji simbolički link koji pokazuje natrag na taj isti direktorij — ili na bilo kojeg pretka u stablu — `stat()` će takav link slijediti i naš će program ponovno "ući" u već obiđeni direktorij. Pri svakom novom ulasku link je opet tu, pa program ponovno ulazi, i tako u nedogled. Da bismo ovakvu beskonačnu petlju izbjegli, prilikom pretraživanja datotečnog sustava treba koristiti `lstat()` — koji će za simbolički link to upravo i prijaviti, pa naš program može odlučiti da ga ne slijedi.

## Korisnici, grupe i prava pristupa

UNIX je višekorisnički sustav, što znači da na istom računalu istovremeno može raditi više korisnika. Svakom korisniku pridružen je jedinstveni cjelobrojni **UID** (*User ID*), a svaki korisnik pripada jednoj ili više **grupa**, od kojih svaka ima svoj **GID** (*Group ID*). Mapiranje imena u brojeve nalazi se u sistemskim datotekama `/etc/passwd` (korisnici) i `/etc/group` (grupe).

Kao što smo upoznali u prvom poglavlju, prava pristupa svakoj datoteci dijele se u tri razine — vlasnik (*user*), grupa (*group*), ostali (*others*) — i tri tipa prava: čitanje (`r`), pisanje (`w`), izvršavanje (`x`). Sveukupno **devet bitova** koji se zapisuju u `st_mode` polje strukture `stat`.

### Procesi i njihovo vlasništvo

Vlasništvo nije samo svojstvo datoteka — nego i procesa. Uostalom, već smo naučili da je na UNIX-u sve datoteka. Vidjeli smo u prvom poglavlju da standardni izlaz jednog procesa možemo preusmjeriti na standardni ulaz drugog i tako efektivno "pisati" u proces. Stoga je sasvim prirodno da i procesi imaju svog vlasnika i grupu kojoj pripadaju.

Svaki proces u sustavu ima pridruženu skupinu identifikatora koji određuju "u čije ime" radi. Najvažnija dva su:

- **stvarni** (*real*) **UID i GID** — tko je doslovno pokrenuo proces; preuzima se od ljuske u kojoj je naredba upisana.
- **efektivni** (*effective*) **UID i GID** — koje ovlasti proces zapravo ima u trenutku provjere.

Stvarni i efektivni vlasnik, kao i stvarno i efektivno grupno vlasništvo, u najvećem se broju slučajeva ne razlikuju. Međutim, postoje određene situacije u kojima se efektivno i stvarno vlasništvo mogu razlikovati. Važno je zapamtiti da jezgra prilikom pristupa datotekama uvijek provjerava prava **efektivnog** vlasnika i efektivne grupe koji su vlasnici procesa.

Logika pridruživanja vlasnika novom procesu prirodno slijedi iz toga kako razmišljamo o korištenju alata općenito: kada koristimo neki alat, sve posljedice — dobre i loše — našeg rada s tim alatom idu nama, nikako ne proizvođaču alata. **Stvarni vlasnik procesa u memoriji stoga je uvijek onaj tko je proces pokrenuo**, ne onaj tko je napisao izvršnu datoteku. Efektivni vlasnik je gotovo uvijek također onaj tko je proces pokrenuo, ali postoje specifične situacije u kojima efektivni vlasnik (i/ili efektivna grupa) može biti onaj korisnik sustava koji je vlasnik izvršne datoteke. Često je to `root` (superuser) koji ima neograničena prava na sustavu.

Klasičan primjer je promjena lozinke. Kad korisnik utipka naredbu `passwd`, ona mora upisati novu kriptiranu lozinku u datoteku `/etc/shadow`. Iz očiglednih sigurnosnih razloga, čitanje i pisanje u datoteku `/etc/shadow` dozvoljeno je samo root-u — niti jedan običan korisnik ne smije ni pročitati tuđe lozinke ni napisati novu:

```
$ ls -l /etc/shadow
-rw-r----- 1 root shadow 609 Apr 18 18:13 /etc/shadow
```

Ipak, svaki korisnik mora moći promijeniti **vlastitu** lozinku. Rješenje je **set-UID bit** — poseban bit u pravima izvršne datoteke koji jezgri kaže: *"kad se ovaj program pokrene, postavi efektivni UID procesa na UID vlasnika izvršne datoteke, ne na UID korisnika koji ga je pokrenuo"*. Set-UID bit u ispisu naredbe `ls -l` zauzima mjesto izvršnog bita za vlasnika i prikazuje se slovom `s`:

```
$ ls -l /usr/bin/passwd
-rwsr-xr-x 1 root root 64152 May 30  2024 /usr/bin/passwd
```

Datoteka `/usr/bin/passwd` u vlasništvu je root-a (četvrti i peti stupac) i ima postavljen set-UID bit (`s` umjesto `x` u korisničkim pravima). Posljedica je da se svaki put kad ju običan korisnik pokrene proces izvršava s **efektivnim UID-om root-a** (i može mijenjati `/etc/shadow`), dok **stvarni UID ostaje korisnikov** (pa program zna tko ga je pokrenuo i može mu mijenjati samo njegovu lozinku).

Set-UID bit moguće je postaviti i na izvršne datoteke koje sami napišemo — pomoću naredbe `chmod` (s kojom ćemo se pobliže upoznati malo kasnije u poglavlju). Pri tome treba biti **iznimno oprezan**: svaki nedostatak u programu s postavljenim set-UID bitom potencijalno se može iskoristiti tako da napadač izvrši proizvoljan kod s povišenim ovlastima. Zato se set-UID koristi samo na pažljivo provjerenim, minimalističkim alatima poput `passwd`-a, a u svakodnevnom programiranju se gotovo nikad ne koristi.

Kako će programi koji rade s povišenim ovlastima provjeriti smije li *stvarni* korisnik (onaj koji je proces pokrenuo) zaista raditi neku akciju? Tu na scenu stupa sistemski poziv `access`, kojeg upoznajemo u sljedećoj sekciji.

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

- **`path`** — putanja do datoteke (`chmod`).
- **`fd`** — otvoreni file deskriptor (`fchmod`).
- **`mode`** — nova prava pristupa, zadana kao bitovni `OR` konstanti tipa `mode_t` (`S_IRUSR | S_IWUSR | ...`) ili kao oktalni broj (`0644`, `0755`).

Da bi proces smio mijenjati prava pristupa neke datoteke, mora biti zadovoljen jedan od dva uvjeta: efektivni UID procesa mora biti jednak UID-u vlasnika datoteke, ili proces mora biti pokrenut kao root.

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

- **`path`** — putanja do datoteke (`chown`, `lchown`).
- **`fd`** — otvoreni file deskriptor (`fchown`).
- **`owner`** — novi UID vlasnika; ako se zadaje `-1`, vlasnik se ne mijenja.
- **`group`** — novi GID grupe; ako se zadaje `-1`, grupa se ne mijenja.

Razlika između `chown` i `lchown` analogna je razlici između `stat` i `lstat`: `chown` slijedi simbolički link i mijenja vlasništvo ciljne datoteke, dok `lchown` mijenja vlasništvo samog linka.

Bitna napomena: na većini suvremenih UNIX sustava, **promjena vlasništva ograničena je na korisnika `root`**. Običan korisnik **ne može** "preuzeti" tuđu datoteku (jer bi to predstavljao sigurnosni rizik), niti svoju datoteku "predati" drugome (jer bi mogućnost da nekome "uvalimo" kukavičje jaje također predstavljala sigurnosni rizik, ali i omogućilo zaobilaženje kvota i drugih ograničenja). U praksi to znači da običan korisnik ove sistemske pozive uglavnom ne koristi izravno; oni su prvenstveno alat administratora. Ekvivalent u ljusci je naredba `chown`. Pošto promjena vlasništva traži root ovlasti, naredba se najčešće poziva uz `sudo` — pomoćnu naredbu koja omogućuje da se ostatak naredbe izvrši s ovlastima root korisnika, pod uvjetom da korisnik ima takvo pravo (tipično definirano u datoteci `/etc/sudoers`):

```
$ sudo chown dkrst:users dat.txt    # dodjeljuje datoteku korisniku dkrst, grupi users
```

## Linkovi

UNIX podržava dva različita načina da jedna te ista datoteka bude dostupna pod više imena: **čvrste linkove** (engl. *hard link*) i **simboličke linkove** (engl. *symbolic link* ili *soft link*). Iako oba mehanizma pružaju "alias" za neku datoteku, oni rade na suštinski različite načine.

### Čvrsti linkovi

Promislimo na trenutak o ulozi direktorija u datotečnom sustavu. Najjednostavnije ih možemo zamisliti kao registre neke velike arhive u kojima piše gdje se koji podatak nalazi: *"ako tražiš tu i tu mapu, pogledaj na trećoj polici lijevo, druga kutija od kraja"*. Nema nikakvog razloga da ista informacija o tome gdje se neki podatak nalazi ne piše u više različitih registara. U svakom od tih registara podatak je možda drugačije zaveden (recimo, računovodstvo fakulteta možda sistematizira studente na jedan, a referada na drugi način). Međutim, u svakom od njih zabilježena je ista lokacija mape na polici, i bez obzira u kojem registru tražimo, doći ćemo do istih podataka.

Na sličan način, iako datoteku čini jedan jedinstveni sadržaj pohranjen na disku, ne postoji nikakav razlog da informaciju o tome gdje se taj sadržaj nalazi ne pohranimo u više direktorija, pod istim ili različitim imenima. Bez obzira koje ime koristimo, sva ona u konačnici pokazuju na iste fizičke podatke na disku.

Tehnički, **čvrsti link** je upravo to: dodatni zapis u nekom direktoriju koji povezuje neko ime s istim i-node brojem kao već postojeća datoteka. Sjetimo se da u UNIX-u sve atribute datoteke (tip, prava, veličinu, vremena, lokaciju podataka na disku) jezgra čuva u i-node strukturi, a ime datoteke u nekom direktoriju je samo zapis koji to ime povezuje s odgovarajućim i-node brojem. Stvaranjem čvrstog linka jednostavno se dodaje novi takav zapis koji pokazuje na isti i-node — pa s gledišta jezgre, sva imena su jednako vrijedne reference na istu datoteku.

Iz ovog mehanizma slijedi nekoliko važnih svojstava čvrstih linkova:

- **Svi (čvrsti) linkovi pokazuju na isti i-node**, dijele identičan sadržaj i atribute. Promjena sadržaja kroz jedno ime vidljiva je odmah kroz drugo.
- **Svi (čvrsti) linkovi moraju biti na istom datotečnom sustavu** (istoj particiji), jer i-node brojevi vrijede samo unutar pojedinog datotečnog sustava.
- **Brisanjem nekog imena samo se uklanja njegov zapis u direktoriju** i smanjuje brojač linkova (`st_nlink`) na i-nodeu. Sama datoteka briše se s diska tek kad brojač padne na 0 — odnosno kad je obrisano i zadnje ime, jer više ništa ne "pokazuje" na te podatke.
- **Izvorno ime nije ničim "jače" od kasnije stvorenih linkova.** Brisanje izvornog imena je jednako brisanju bilo kojeg drugog linka.
- **Samo `root` smije stvarati čvrste linkove na direktorij**. Stvaranje takvog linka kreiralo bi cikluse u stablu direktorija — primjerice, link `/foo/bar` koji pokazuje natrag na `/foo` značio bi da iz direktorija `/foo` možemo unedogled "spuštati" se kroz `bar/bar/bar/...` i uvijek smo u istom direktoriju. Alati poput `find` ili `du` koji rekurzivno obilaze stablo zaglavili bi u beskonačnoj petlji. Slično vrijedi i za posebne unose `.` i `..` koji su u biti čvrsti linkovi, ali njih jezgra automatski stvara i njima upravlja sama.

```c
#include <unistd.h>

int link(const char *oldpath, const char *newpath);
int unlink(const char *pathname);
```

**Povratna vrijednost (obje):** `0` u slučaju uspjeha, `-1` u slučaju greške.

`link()` stvara novo ime (`newpath`) za već postojeću datoteku (`oldpath`) — povećava brojač linkova na i-nodeu i atomski dodaje novi zapis u direktorij. `unlink()` radi obrnuto: uklanja zapis iz direktorija i smanjuje brojač. Ako brojač padne na 0 i nema otvorenih deskriptora na datoteku, jezgra fizički briše datoteku s diska.

Obje funkcije imaju izravne ekvivalente u ljusci: `link()` odgovara naredbi `ln`, a `unlink()` naredbi `unlink` ili — što je u praksi puno češće — naredbi `rm`. Naredba `rm` nudi sve što i `unlink`, ali i puno više: brisanje više datoteka odjednom, rekurzivno brisanje s opcijom `-r`, prisilno brisanje s `-f`. Ipak, s gledišta sustava, one rade istu temeljnu operaciju — uklanjaju zapis iz direktorija i smanjuju brojač linkova.

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
  $ ln orig.txt link2.txt

  $ ls -li orig.txt link1.txt link2.txt
  1002 -rw-r--r-- 3 dkrst users 17 May  5 15:00 link1.txt
  1002 -rw-r--r-- 3 dkrst users 17 May  5 15:00 link2.txt
  1002 -rw-r--r-- 3 dkrst users 17 May  5 15:00 orig.txt
  ```

  Drugi link je stvoren naredbom `ln` u ljusci — ekvivalentno pozivu `./makelink orig.txt link2.txt`.

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

  Tek kad je brisano i zadnje ime — i brojač pao na 0 — jezgra je fizički obrisala datoteku s diska.

### Simbolički linkovi

**Simbolički link** je datoteka koja sadrži tekst — putanju do druge datoteke. Ono što razlikuje simbolički link od bilo koje druge tekstualne datoteke jest njegov **tip**, zapisan u atributima datoteke (točnije, u `st_mode` polju i-noda). Po tom tipu jezgra zna da sadržaj ne treba čitati kao obične podatke — nego da tekst u datoteci treba interpretirati kao putanju do druge datoteke i operaciju nastaviti nad njom. Tako pri svakom otvaranju, čitanju ili pisanju kroz simbolički link, jezgra čita njegov sadržaj i operaciju izvršava nad datotekom čije je ime ondje pročitano.

Simbolički linkovi pružaju funkcionalnost sličnu čvrstim linkovima, ali rade na potpuno drugačiji način, što ima nekoliko važnih posljedica:

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

- [**`makesymlink.c`**](makesymlink.c) — stvara simbolički link na zadanoj putanji koji "pokazuje" na zadanu metu.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>

  int main(int argc, char *argv[]) {
      if (argc != 3) {
          printf("koristenje: %s <postojeca_datoteka> <novi_link>\n", argv[0]);
          return 1;
      }

      if (symlink(argv[1], argv[2]) < 0) {
          perror("symlink");
          return 1;
      }

      printf("Stvoren simbolicki link '%s' -> '%s'.\n", argv[2], argv[1]);
      return 0;
  }
  ```

  Pokretanje:

  ```
  $ echo "ovo je sadrzaj" > orig.txt
  $ ./makesymlink orig.txt sym.txt
  Stvoren simbolicki link 'sym.txt' -> 'orig.txt'.

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

  Link je sam ostao netaknut — ali ciljna datoteka više ne postoji, pa je link sad razbijen. Moderne UNIX ljuske razbijene (*dangling*) simboličke linkove često prikazuju drugom bojom kako bi odmah bilo vidljivo da putanja u linku pokazuje na nepostojeću datoteku.

  Ekvivalent u ljusci je opet naredba `ln`, ovaj put s opcijom `-s`:

  ```
  $ ln -s orig.txt sym.txt
  ```

  Naredba `ln` ima i niz drugih opcija — ovdje smo upoznali samo dvije najčešće (osnovni oblik za hard linkove i `-s` za simboličke). Za potpunu uputu i popis svih opcija, kao i kod svih ostalih UNIX naredbi, čitatelj se upućuje na priručnik koji se otvara naredbom `man ln`.

- [**`readsymlink.c`**](readsymlink.c) — čita sadržaj simboličkog linka, odnosno doslovni tekst (putanju) koji je u njemu zapisan, pomoću funkcije `readlink`.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>

  int main(int argc, char *argv[]) {
      char buf[256];
      ssize_t n;

      if (argc != 2) {
          printf("koristenje: %s <simbolicki_link>\n", argv[0]);
          return 1;
      }

      /* readlink ne dodaje null terminator, pa ga moramo sami dodati */
      n = readlink(argv[1], buf, sizeof(buf) - 1);
      if (n < 0) {
          perror("readlink");
          return 1;
      }
      buf[n] = '\0';

      printf("Sadrzaj linka '%s': \"%s\"\n", argv[1], buf);
      return 0;
  }
  ```

  Da bismo program isprobali, stvorimo jednu datoteku u poddirektoriju (kako bi imala nešto složeniju putanju), pa na nju usmjerimo dva simbolička linka pisana s različito formuliranom putanjom — jednog napravimo pozivom `makesymlink`-a, a drugog izravno iz ljuske naredbom `ln -s`:

  ```
  $ mkdir -p dokumenti
  $ echo "ovo je sadrzaj" > dokumenti/orig.txt

  $ ./makesymlink dokumenti/orig.txt sym1
  Stvoren simbolicki link 'sym1' -> 'dokumenti/orig.txt'.

  $ ln -s ./dokumenti/orig.txt sym2

  $ ls -l sym1 sym2
  lrwxrwxrwx 1 dkrst users 19 May  5 17:23 sym1 -> dokumenti/orig.txt
  lrwxrwxrwx 1 dkrst users 21 May  5 17:23 sym2 -> ./dokumenti/orig.txt
  ```

  Već iz `ls -l` ispisa vidimo da oba linka pokazuju na istu datoteku, ali se njihove veličine razlikuju — `sym1` zauzima 19 bajtova (duljina niza `"dokumenti/orig.txt"`), a `sym2` 21 bajt (duljina niza `"./dokumenti/orig.txt"` — dva bajta više za uvodno `./`). Time je vidljivo da svaki link doslovno čuva onaj tekst kojim je stvoren.

  Sad pročitajmo sadržaj oba linka pozivom `readsymlink`:

  ```
  $ ./readsymlink sym1
  Sadrzaj linka 'sym1': "dokumenti/orig.txt"

  $ ./readsymlink sym2
  Sadrzaj linka 'sym2': "./dokumenti/orig.txt"
  ```

  Vidimo da nam program ispisuje upravo onaj tekst koji smo proslijedili kao putanju pri stvaranju linka — nezavisno o tome koristi li se `symlink()` u C-u ili `ln -s` u ljusci. Različita formulacija putanje (`dokumenti/orig.txt` vs `./dokumenti/orig.txt`) doslovno se čuva u sadržaju linka — jezgra niti normalizira putanju niti provjerava simboličke linkove pri stvaranju. Provjera se događa tek pri svakom pristupu kroz link.

## Vremena pristupa

Već smo upoznali tri vremenska polja u `struct stat`: `st_atime`, `st_mtime`, `st_ctime`. Jezgra ih ažurira automatski — pri svakom čitanju, pisanju ili promjeni atributa datoteke. Ipak, ponekad želimo eksplicitno postaviti `atime` i `mtime` za određenu datoteku na neku zadanu vrijednost — najčešće da bismo "potvrdili" da je datoteka svježa, ili da Build sustavi (`make`) misle da je novija od neke druge.

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
- **`times`** — pokazivač na strukturu s novim vremenima. Ako je `NULL`, oba vremena postavljaju se na **trenutno** vrijeme (što je zapravo ekvivalent pozivu naredbe `touch` bez argumenata koji zadaju vrijeme).

Pogledajmo kako se ova funkcija koristi u praksi:

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

  Naredba `touch` u praksi je nešto bogatija od našeg programa. Po zadanom postavlja **oba vremena** (`atime` i `mtime`) na trenutni trenutak, ali nudi i niz opcija za finiju kontrolu:

  - **`-a`** mijenja samo `atime`, dok `mtime` ostaje netaknut,
  - **`-m`** mijenja samo `mtime`, dok `atime` ostaje netaknut,
  - **`-d`** ili **`-t`** omogućuju zadavanje **proizvoljnog** datuma i vremena umjesto trenutnog (npr. `touch -d "2024-01-15 10:30:00" dat.txt`),
  - **`-r`** preslikava vremena s neke druge datoteke kao referentne (`touch -r referenca.txt cilj.txt`),
  - **`-c`** sprečava stvaranje datoteke ako ne postoji.

  Funkcija `utime` u svojoj punoj verziji već ima podršku za sve ovo: drugi argument može biti pokazivač na `struct utimbuf` s eksplicitno zadanim vrijednostima `atime` i `mtime` (umjesto `NULL` koji znači "trenutno vrijeme"). Vrijednosti tipa `time_t` mogu se dobiti iz čovjeku čitljivog datuma pomoću funkcija `mktime()` ili `strptime()`. Naš `dotakni.c` dakle pokriva samo najjednostavniji slučaj; proširenje s navedenim opcijama može biti korisna **vježba za čitatelja** ako želi bolje razumjeti rad s vremenom u UNIX sustavima.

  Tipična uporaba `touch` (i `dotakni`) — "natjerati" `make` da iznova prevede neki fajl, čak i ako mu sadržaj nije promijenjen, samo postavljanjem `mtime` na sadašnji trenutak.

## Rad s direktorijima

U posljednjem dijelu poglavlja dat ćemo pregled funkcija koje u našim programima možemo koristiti za upravljanje datotečnim stablom — stvaranje i brisanje direktorija, kretanje kroz njih, te čitanje njihova sadržaja. Završit ćemo i s odgovarajućim primjerom korištenja kojim ćemo ilustrirati kako radi još jedna od standardnih UNIX naredbi koju iznimno često koristimo u svakodnevnom radu.

```c
#include <unistd.h>
#include <sys/stat.h>

int mkdir(const char *pathname, mode_t mode);
int rmdir(const char *pathname);

char *getcwd(char *buf, size_t size);
int chdir(const char *pathname);
int fchdir(int fd);
```

#### Funkcija `mkdir()`

Stvara novi prazan direktorij. U njemu jezgra automatski stvara dva posebna unosa: `.` (pokazivač na sam taj direktorij) i `..` (pokazivač na nadređeni direktorij).

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`pathname`** — putanja na kojoj se stvara novi direktorij.
- **`mode`** — prava pristupa za novi direktorij (npr. `0755`); konačna prava dobiju se kombinacijom s maskom procesa (`umask`).

#### Funkcija `rmdir()`

Briše direktorij — ali samo ako je **prazan** (sadrži samo `.` i `..`). Ukoliko želimo obrisati direktorij u kojem se nalaze druge datoteke i direktoriji, potrebno je ući u njega i rekurzivno obrisati cijeli njegov sadržaj, datoteku po datoteku (sistemskim pozivom `unlink`) te tek nakon toga obrisati i sam direktorij.

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`pathname`** — putanja do direktorija koji se briše.

#### Funkcija `getcwd()`

Vraća putanju do trenutnog radnog direktorija procesa (CWD - *Current Working Directory*). Bitno je razumjeti da je radni direktorij **svojstvo procesa**, ne korisnika; svaki proces ima svoj vlastiti CWD koji je naslijedio od svog roditelja u trenutku stvaranja (sistemskim pozivom `fork()`).

**Povratna vrijednost:** pokazivač na `buf` u slučaju uspjeha, `NULL` u slučaju greške (npr. ako je međuspremnik premali da primi punu putanju).

**Argumenti:**

- **`buf`** — međuspremnik u koji se upisuje putanja do trenutnog direktorija (kao null-terminirani string).
- **`size`** — veličina međuspremnika `buf` u bajtovima.

#### Funkcije `chdir()` i `fchdir()`

Mijenjaju radni direktorij procesa. `chdir` direktorij identificira putanjom, dok `fchdir` koristi otvoreni file deskriptor.

Bitan suptilan detalj: `chdir` u programu mijenja radni direktorij **samo tom procesu** — ne i ljusci koja ga je pokrenula. Zato naredba `cd` **nije implementirana kao "vanjska" naredba**, tj. ne postoji izvršna datoteka koja se poziva kad napišemo `cd` (za razliku od drugih naredbi, kao npr. `ls`, `touch` i brojnih drugih koje smo koristili). Ovako implementirana naredba bila bi beskorisna: pokretanje novog programa podrazumijeva i stvaranje novog procesa u kojem će se program izvršavati, dok ljuska čeka da on završi i prihvati sljedeću naredbu korisnika. Pozivanje funkcije `chdir()` u takvom novom procesu zapravo bi promijenilo trenutni radni direktorij (CWD) samo tog procesa, ne i ljuske koja je naredbu pozvala. Stoga je naredba `cd` realizirana kao ugrađena (*built-in*) naredba ljuske — ne izvršava se u novom procesu, nego mijenja CWD same ljuske u kojoj je pozvana, pozivom `chdir()` "iznutra".

**Povratna vrijednost (obje funkcije):** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`pathname`** — putanja do direktorija (`chdir`).
- **`fd`** — otvoreni file deskriptor (`fchdir`).

### Čitanje sadržaja direktorija

Direktoriji su zapravo posebne datoteke koje sadrže popis imena i pripadnih i-node brojeva. Jezgra dopušta samo sebi da piše u njih — korisnički procesi smiju ih čitati, ali ne i izravno mijenjati. Najčešći oblik rada s direktorijem — sekvencijalno čitanje svih zapisa u njemu — obavlja se trojkom funkcija iz `<dirent.h>`:

```c
#include <dirent.h>

DIR           *opendir(const char *pathname);
struct dirent *readdir(DIR *dp);
int            closedir(DIR *dp);
```

#### Funkcija `opendir()`

Otvara direktorij za čitanje i vraća pokazivač na `DIR` strukturu — **neprozirni** (*opaque*) tip, što znači da programer ne zna i ne treba znati kako struktura iznutra izgleda; samo je prosljeđuje ostalim funkcijama obitelji kao apstraktnu referencu, dok upravljanje sadržajem strukture u potpunosti preuzima biblioteka.

**Povratna vrijednost:** pokazivač na `DIR` strukturu u slučaju uspjeha, `NULL` u slučaju greške.

**Argumenti:**

- **`pathname`** — putanja do direktorija koji se otvara.

#### Funkcija `readdir()`

Vraća zapis koji se nalazi na **trenutnoj poziciji** u direktoriju, a zatim tu poziciju pomiče na sljedeći zapis. Pozivom `opendir()` pozicija se postavlja na **prvi zapis** u direktoriju, pa prvi poziv `readdir`-a vraća upravo njega. Svaki sljedeći poziv vraća sljedeći zapis i pomiče poziciju za jedno mjesto naprijed; kad više nema zapisa, `readdir` vraća `NULL`. Time u jednoj `while` petlji možemo prirodno pročitati cijeli direktorij od početka do kraja.

Mehanizam je analogan onomu što smo već vidjeli kod običnih datoteka: tamo `read()` čita podatke od trenutnog **offseta** u datoteci i pomiče ga za broj pročitanih bajtova; ovdje `readdir()` čita zapis s trenutne pozicije u direktoriju i pomiče tu poziciju na sljedeći zapis. Razlika je u tome što kod regularnih datoteka offsetom programer može slobodno upravljati funkcijom `lseek()`, dok je kod direktorija interna pozicija dio neprozirne `DIR` strukture i nije izravno dostupna programeru — on je može samo vratiti na početak pozivom `rewinddir()`, ili koristiti `telldir()` i `seekdir()` o kojima ćemo govoriti malo kasnije.

**Povratna vrijednost:** pokazivač na `struct dirent` strukturu sa zapisom, ili `NULL` na kraju direktorija ili u slučaju greške.

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
- **Polje `d_type`** (regularna/direktorij/link/...) postoji na Linuxu, ali nije dio POSIX standarda. Ukoliko želimo pisati prenosivi kod, sigurnije je koristiti `lstat` sa imenom datoteke (koje možemo dobiti iz `d_name` polja) i nakon toga iz `st_mode` saznati tip datoteke.

Vraćeni pokazivač pokazuje na strukturu koju je sama biblioteka prethodno alocirala u svojoj internoj memoriji (najčešće u statičkom međuspremniku); pozivatelj ju nije ni alocirao, pa ju ni ne smije osloboditi pozivom `free()`. Sljedeći poziv `readdir`-a nad istim direktorijem prepiše taj međuspremnik novim zapisom — povratni pokazivač iz prethodnog poziva nakon toga još uvijek vrijedi, ali sadržaj na koji pokazuje više nije isti. Ako iz nekog razloga trebamo zadržati podatke iz zapisa duže (npr. spremiti listu svih imena u direktoriju), moramo ih kopirati u vlastitu memoriju.

**Argumenti:**

- **`dp`** — pokazivač na otvorenu `DIR` strukturu, dobiven prethodnim pozivom `opendir`.

#### Funkcija `closedir()`

Zatvara direktorij i oslobađa pripadne resurse.

**Povratna vrijednost:** `0` u slučaju uspjeha, `-1` u slučaju greške.

**Argumenti:**

- **`dp`** — pokazivač na otvorenu `DIR` strukturu.

**Veza sa standardnom C bibliotekom:** uočite da je kombinacija `opendir → readdir → closedir` za direktorije analogna kombinaciji `fopen → fread → fclose` za regularne datoteke. U oba slučaja imamo "stream" apstrakciju — neprozirni objekt (`DIR *` odnosno `FILE *`) koji čuva interne podatke o napretku čitanja, plus funkcije za otvaranje, sekvencijalno čitanje i zatvaranje.

#### Pozicioniranje unutar direktorija

Pored osnovne trojke koju smo upravo opisali, `<dirent.h>` nudi i tri dodatne funkcije za izravno upravljanje internom pozicijom u direktoriju:

```c
#include <dirent.h>

void rewinddir(DIR *dp);
long telldir(DIR *dp);
void seekdir(DIR *dp, long loc);
```

#### Funkcija `rewinddir()`

Vraća čitanje na početak direktorija — sljedeći poziv `readdir`-a opet će vratiti prvi zapis.

**Povratna vrijednost:** nema (funkcija je tipa `void`).

**Argumenti:**

- **`dp`** — pokazivač na otvorenu `DIR` strukturu.

#### Funkcije `telldir()` i `seekdir()`

`telldir` vraća trenutnu poziciju u direktoriju kao neprozirnu vrijednost; `seekdir` postavlja čitanje natrag na poziciju ranije zapamćenu `telldir`-om. Ove dvije funkcije rijetko se koriste u praksi.

**Povratna vrijednost:**

- **`telldir`** — trenutna pozicija (kao `long` vrijednost), ili `-1` u slučaju greške.
- **`seekdir`** — nema povratne vrijednosti (`void`).

**Argumenti:**

- **`dp`** — pokazivač na otvorenu `DIR` strukturu.
- **`loc`** (samo `seekdir`) — pozicija ranije dobivena pozivom `telldir`.

Sada kad smo upoznali sve funkcije za rad s direktorijima, iskoristit ćemo ih u jednom konkretnom primjeru — implementaciji **pojednostavljene inačice standardne UNIX naredbe `ls`**, koja kao i izvorna naredba ispisuje sadržaj zadanog direktorija.

- [**`mojls.c`**](mojls.c) — pojednostavljena inačica naredbe `ls`. Bez argumenta lista trenutni direktorij; s argumentom lista zadani. Za svaki zapis ispisuje veličinu datoteke u bajtovima, te ime.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <sys/stat.h>
  #include <dirent.h>

  int main(int argc, char *argv[]) {
      DIR *dp;
      struct dirent *entry;
      struct stat st;
      char path[1024];
      const char *dir;

      /* bez argumenta, listamo trenutni direktorij */
      dir = (argc < 2) ? "." : argv[1];

      dp = opendir(dir);
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

          /* sastavi punu putanju "dir/ime" za poziv lstat-u */
          snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

          if (lstat(path, &st) < 0) {
              perror(path);
              continue;
          }

          printf("%10ld  %s\n", (long)st.st_size, entry->d_name);
      }

      closedir(dp);
      return 0;
  }
  ```

  Glavna petlja je tipična UNIX-ova "while-readdir" konstrukcija — funkcija u istom pozivu javlja i podatke (kroz povratnu vrijednost) i kraj pretrage (vraćanjem `NULL`-a). Ovakvu petlju, koja prolazi kroz niz podataka ili poziva zaključenih `NULL` pokazivačem, često ćete sresti u programima.

  Bitno: `entry->d_name` sadrži samo **ime zapisa unutar otvorenog direktorija**, ne njegovu punu putanju. `lstat()` međutim treba putanju — pa prije svakog poziva `snprintf`-om sastavljamo string oblika `"<dir>/<ime>"`. Bez ovog koraka `lstat` bi tražio `entry->d_name` u trenutnom radnom direktoriju procesa, što bi naravno bilo pogrešno čim listamo neki drugi direktorij osim `"."`. Format `"%10ld"` u `printf`-u poravnava broj u stupac širine 10 znakova radi urednijeg ispisa.

  Pokretanje:

  ```
  $ ./mojls
         564  prava.c
         373  makesymlink.c
         595  provjeri.c
         807  Makefile
         484  readsymlink.c
        1302  fileinfo2.c
         942  mojls.c
       58904  README.md
         677  dotakni.c
        1300  fileinfo.c
         361  makelink.c

  $ ./mojls /etc
        4096  cron.d
        4096  default
        4096  X11
        1111  passwd
         597  group
         609  shadow
         219  hosts
          11  hostname
       12813  services
         582  profile
         656  fstab
         122  resolv.conf
        ...
  ```

  Direktorij `/etc` u praksi sadrži znatno više datoteka i poddirektorija nego što je prikazano (na tipičnom sustavu desetke i stotine zapisa); ostatak ispisa koji ovdje nije prikazan iz prostornih razloga označen je s `...` na dnu.

  Direktoriji (`cron.d`, `default`, `X11`) uobičajeno imaju veličinu 4096 bajtova, što je tipična veličina bloka datotečnog sustava na Linuxu — jedan blok dovoljan je za pohranjivanje popisa imena i pripadnih i-node brojeva uobičajenog direktorija. Bitno je razumjeti da ova vrijednost **ne predstavlja zbroj veličina datoteka unutar direktorija**, kao što čitatelj možda intuitivno očekuje, nego veličinu prostora koji direktorij sam zauzima na disku radi vlastite organizacije. Većina direktorija stoga u `lstat()` ispisu pokazuje istih 4096 B bez obzira što se u njima nalazi (vrijednost može biti i veća kod direktorija s vrlo mnogo unosa, kad jedan blok više nije dovoljan). Za regularne datoteke (`passwd`, `services`, ...) `st_size` pokazuje stvarnu veličinu sadržaja u bajtovima.

  **Vježba za čitatelja:** ispisivanje pravog `ls -l` sadrži znatno više informacija — tip datoteke, prava pristupa, vlasnika i grupu, vrijeme zadnje izmjene. Sve te informacije su nam već dostupne u `struct stat` strukturi koju smo popunili pozivom `lstat()`. Pokušajte doraditi `mojls` tako da reproducira potpun `ls -l` ispis. Korisne funkcije iz standardne C biblioteke: `getpwuid()` (pretvara UID u ime korisnika, definirana u `<pwd.h>`), `getgrgid()` (pretvara GID u ime grupe, definirana u `<grp.h>`), te `ctime()` ili `strftime()` za pretvorbu `time_t` vrijednosti u tekstualni datum.

## Što smo zapravo radili

Vrijedi se na kraju ovog poglavlja na trenutak osvrnuti na ono što smo zapravo radili kroz dane primjere. Mali UNIX programi poput `dotakni`, `mojls`, `prava`, `makelink`, `makesymlink`, `readsymlink` — kao i `f_cat`, `f_strip` ili `f_write` iz prethodnog poglavlja — nisu tek umjetni vježbeni primjeri. Svaki od njih implementira temeljnu funkcionalnost neke od standardnih UNIX naredbi: `touch`, `ls`, `chmod`, `ln`, `ln -s`, `readlink`, `cat`. Time ilustriramo i jednu od najvažnijih osobina UNIX-a: skupina alata koje koristimo svaki dan zapravo je **tanki sloj iznad sistemskih poziva**. Kad ih sami napišemo u nekoliko desetaka linija C koda, vidimo iz prve ruke da iza naredbi koje na prvi pogled izgledaju "magično" stoje sasvim razumljivi pozivi `stat`, `chmod`, `link`, `symlink`, `utime` — a temeljne UNIX rutine i sistemski pozivi koji ih implementiraju isti su već gotovo pola stoljeća, što pokazuje koliko je dobro osmišljen UNIX sustav.

Ovo nam ukazuje na još jednu osobinu UNIX-a: koncepti na kojima UNIX počiva su složeni, ali kad ih jednom usvojite, naizgled kompleksne stvari možete realizirati na relativno jednostavan način, sa svega nekoliko sistemskih poziva. Ova osobina utkana je u temeljnu filozofiju izrade UNIX programa: *radi jednu stvar, ali radi je dobro!* Takav pristup potiče pisanje malih, modularnih programa specijaliziranih za obavljanje jedne konkretne zadaće, uz mogućnost njihova povezivanja u veće i složenije cjeline koristeći osnovno načelo UNIX-a — *sve je datoteka*.

## Prevođenje

Direktorij dolazi s priloženim `Makefile`-om koji prati iste konvencije kao i u ostalim poglavljima (varijable `CC`, `CFLAGS`, `LDFLAGS`, `TARGETS`; implicitno pravilo `.c.o`; pravila `default`, `all`, `clean`).

```sh
make all          # gradi sve primjere
make fileinfo     # gradi pojedinačni primjer
make clean        # čisti generirane datoteke
```

## Zadaci za samostalno rješavanje

### Zadatak 1 — `fileinfo3`

Modificirajte primjer [`fileinfo2.c`](fileinfo2.c) tako da umjesto ispisa svakog atributa u zasebnom retku sve podatke o datoteci ispiše u jednom retku, na sličan način kako ih ispisuje naredba `ls -l`: tip datoteke i prava pristupa kao niz od deset znakova (`-rw-r--r--`), zatim broj linkova, UID i GID vlasnika, veličinu, vrijeme zadnje izmjene i ime datoteke. Za simbolički link iza imena ispišite i ` -> ` te ime datoteke na koju link pokazuje.

Očekivano ponašanje:

```sh
$ ./fileinfo3 fileinfo.c
-rw-r--r-- 1 1000 1000 1300 May  5 15:22 fileinfo.c
$ ./fileinfo3 drugoime.c
lrwxrwxrwx 1 1000 1000   10 May  5 16:14 drugoime.c -> fileinfo.c
$ ls -l fileinfo.c drugoime.c
-rw-r--r-- 1 dkrst dkrst 1300 May  5 15:22 fileinfo.c
lrwxrwxrwx 1 dkrst dkrst   10 May  5 16:14 drugoime.c -> fileinfo.c
```

Jedina razlika u odnosu na `ls -l` je što umjesto imena vlasnika i grupe ispisujemo njihove brojčane identifikatore.

> **Mala pomoć:** Pri rješavanju zadatka koristite se primjerom [`readsymlink.c`](readsymlink.c), a za formatiranje ispisa datuma i vremena funkcijom `strftime()`.

**Dodatni zadatak:** korištenjem funkcija `getpwuid()` i `getgrgid()` umjesto UID-a i GID-a ispišite ime vlasnika i grupe.

### Zadatak 2 — `mojls2`

Modificirajte primjer [`mojls.c`](mojls.c) tako da za svaku stavku direktorija ispiše redak u istom obliku kao `fileinfo3` iz prethodnog zadatka, čime dobivate vlastitu inačicu naredbe `ls -l`. Ispis za jednu datoteku izdvojite u zasebnu funkciju koju zatim pozivate iz petlje po stavkama direktorija.

Očekivano ponašanje:

```sh
$ ./mojls2
drwxr-xr-x 2 1000 1000 4096 May  5 16:10 slike
-rw-r--r-- 1 1000 1000  412 May  5 15:20 Makefile
lrwxrwxrwx 1 1000 1000   10 May  5 16:14 drugoime.c -> fileinfo.c
-rw-r--r-- 1 1000 1000 1300 May  5 15:22 fileinfo.c
```

### Zadatak 3 — `noviji`

Napišite program `noviji` koji prima imena dviju datoteka, `izvor` i `odrediste`, te usporedi vremena njihove zadnje izmjene. Ako `odrediste` ne postoji ili je `izvor` od njega noviji, program ispiše da je odredište potrebno osvježiti; u protivnom ispiše da je odredište ažurno. Ako `izvor` ne postoji, program ispiše grešku.

Očekivano ponašanje:

```sh
$ ./noviji fileinfo.c fileinfo
Odrediste 'fileinfo' je azurno.
$ ./dotakni fileinfo.c
Datoteka 'fileinfo.c' azurirana.
$ ./noviji fileinfo.c fileinfo
Odrediste 'fileinfo' treba osvjeziti.
$ ./noviji fileinfo.c nepostojeca
Odrediste 'nepostojeca' treba osvjeziti.
```

Opisana tehnika usporedbe vremena izmjene izvorne i ciljane datoteke koristi se u alatu `make` prilikom odlučivanja koje dijelove koda treba ponovo prevesti.

> **Mala pomoć:** Za provjeru vremena zadnje izmjene koristite funkciju `stat()`.

**Dodatni zadatak:** ako je odredište potrebno osvježiti, kopirajte sadržaj izvora u odredište (po uzoru na [`io_copy.c`](../P03-Ulazno_izlazne_operacije/io_copy.c) iz poglavlja P03) i pomoću `utime()` odredištu postavite ista vremena pristupa i izmjene kakva ima izvor.

Doradite `Makefile` datoteku na način da u nju dodate pravila za prevođenje i povezivanje zadataka za vježbu.

## Bibliografija

[1] W. R. Stevens and S. A. Rago, *Advanced Programming in the UNIX Environment*, 3rd ed. Boston, MA, USA: Addison-Wesley Professional, 2013.
