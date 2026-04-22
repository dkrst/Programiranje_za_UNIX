# fileio

Primjeri uz poglavlje **Ulazno/izlazne operacije** iz knjige *Programiranje za UNIX*.

U ovom direktoriju nalaze se programi koji ilustriraju UNIX sistemske pozive za rad s datotekama: `open()`, `creat()`, `close()`, `read()`, `write()`, `lseek()` i `umask()`. Svi primjeri pisani su izravno nad ovim sistemskim pozivima (bez uporabe funkcija standardne C biblioteke poput `fopen`, `fread`, `fprintf`), s ciljem da se prikaže kako operacijski sustav zapravo obavlja ulazno/izlazne operacije i na kojoj razini počiva koncept **"sve je datoteka"** — temeljno UNIX načelo prema kojem se datoteke, uređaji, terminali, cijevi i mrežni sockets svi koriste kroz isto skučeno sučelje file deskriptora.

## Sadržaj

### Osnovno otvaranje, čitanje i pisanje

- **`read_file.c`** — otvara datoteku `moja_datoteka.txt` sistemskim pozivom `open()` u načinu samo za čitanje (`O_RDONLY`) i ispisuje njen sadržaj na standardni izlaz, čitajući znak po znak pozivima `read()` te ih prosljeđujući pozivima `write()`. Demonstrira osnovni slijed rada s datotekom (`open` → `read`/`write` → `close`) i rukovanje greškama preko povratnih vrijednosti sistemskih poziva.

  ```sh
  ./read_file
  ```

- **`io_copy.c`** — kopira standardni ulaz na standardni izlaz, čitajući u međuspremnik konstantne veličine (`BUFFSIZE`). Za razliku od `read_file.c` koji čita znak po znak, ovdje se u jednom pozivu `read()` pokušava pročitati cijeli blok bajtova, čime se višestruko smanjuje broj sistemskih poziva potrebnih za istu količinu prenesenih podataka. Primjer ujedno ilustrira koncept "sve je datoteka": pri pokretanju bez preusmjeravanja, standardni ulaz vezan je na tipkovnicu, a standardni izlaz na terminal — isti `read()` i `write()` koji bi radili s običnim datotekama na disku ovdje rade s terminalom. Svaki redak koji korisnik utipka program odmah ispiše natrag, a petlja se prekida pritiskom `Ctrl+D` (oznaka kraja ulaza, EOF):

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

- **`f_strip.c`** — demonstrira `lseek()` s `SEEK_SET` (apsolutno pozicioniranje od početka datoteke). Program otvara datoteku za pisanje, upisuje prvi niz znakova, `lseek`-om postavlja offset na 15. bajt i upisivanjem drugog niza **prepisuje** dio postojećeg sadržaja. Rezultat pokazuje da se pozicioniranje unutar otvorene datoteke može slobodno kombinirati s čitanjem i pisanjem — offset koji jezgra pamti nije povezan s fizičkim rasporedom blokova na disku.

  Rezultat se može provjeriti UNIX naredbom `cat`, koja ispisuje sadržaj jedne ili više datoteka na standardni izlaz:

  ```
  $ ./f_strip
  $ cat file.strip
  Prvi redak teksDrugi redak teksta
  ```

  Prvih 15 bajtova (`Prvi redak teks`) ostalo je netaknuto iz prvog upisa, a od 15. bajta nadalje vidljiv je sadržaj drugog niza koji je `lseek` + `write` upisao preko ostatka prethodnog teksta.

- **`f_hole.c`** — demonstrira `lseek()` s `SEEK_CUR` (relativno pomicanje od trenutne pozicije). Nakon upisa prvog niza, offset se pomiče 15 bajtova naprijed — **iza** trenutnog kraja datoteke — i tek se tada upisuje drugi niz. Petnaest bajtova između prve i druge linije ostaje kao **rupa**, područje koje pri čitanju iščitava kao bajtovi s vrijednošću 0 (null terminator), iako `write()` ondje ništa nije upisao.

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

- **`perm_mask.c`** — ilustrira djelovanje maske kreiranja datoteke (`umask`) na prava pristupa pri pozivu `creat()`. Program dva puta stvara datoteku s istim zatraženim pravima (`rw-rw-r--`), jednom s maskom postavljenom na 0 (nikakva prava se ne oduzimaju), a jednom s maskom koja eksplicitno isključuje pisanje za grupu i čitanje/pisanje za ostale. Usporedbom stvarno dobivenih prava vidi se učinak maske:

  ```
  $ ./perm_mask
  $ ls -al datoteka1 datoteka2
  -rw-rw-r-- 1 dkrst users 0 Jan 16 16:23 datoteka1
  -rw------- 1 dkrst users 0 Jan 16 16:23 datoteka2
  ```

  Datoteka `datoteka1` zadržala je sva zatražena prava jer je maska bila 0, dok su u slučaju datoteke `datoteka2` maskom isključena sva prava za grupu i ostale korisnike.

### Programi s argumentima naredbenog retka

- **`f_write.c`** — čita sa standardnog ulaza i upisuje u novokreiranu datoteku čije se ime zadaje kao argument naredbenog retka. Uz rad s datotekama, ovaj primjer uvodi i rad s **argumentima naredbenog retka**: funkcija `main` deklarirana je kao `int main(int argc, char *argv[])`, gdje `argc` sadrži broj argumenata, a `argv` polje nizova znakova s pojedinačnim argumentima. Po konvenciji, `argv[0]` je ime kojim je program pokrenut, a stvarni argumenti počinju od `argv[1]`. Odmah na početku programa provjerava se `argc != 2` — očekuje se točno jedan argument (ime izlazne datoteke) uz ime programa. Ako korisnik program pozove bez argumenta ili s previše njih, program ispisom poruke:

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

- **`f_cat.c`** — pojednostavljena implementacija UNIX naredbe `cat`. Osnovna petlja čitanja i pisanja enkapsulirana je u pomoćnoj funkciji `rw(fdin, fdout)` koja čita s jednog deskriptora i piše na drugi. Ponašanje programa ovisi o argumentima naredbenog retka:

  - **Bez argumenata** program poziva `rw(STDIN_FILENO, STDOUT_FILENO)` i tako praktički radi istu stvar kao `io_copy`: čita sa standardnog ulaza i ispisuje na standardni izlaz.
  - **S jednim ili više argumenata** program redom otvara svaku navedenu datoteku pozivom `open()` i njezin sadržaj prosljeđuje na standardni izlaz pozivom iste funkcije `rw()`, ali sad s file deskriptorom otvorene datoteke umjesto standardnog ulaza.

  Program time u bitnome reproducira funkcionalnost UNIX naredbe `cat` i ujedno sažima sve koncepte iz prethodnih primjera: otvaranje datoteka s provjerom grešaka, jedinstveno sučelje preko file deskriptora, rad s argumentima naredbenog retka, i uniformno korištenje istih sistemskih poziva bez obzira na izvor (datoteka ili standardni tok).

  ```sh
  ./f_cat                                  # ponasa se kao io_copy
  ./f_cat moja_datoteka.txt
  ./f_cat datoteka1.txt datoteka2.txt      # konkatenacija vise datoteka
  ```

### Ulazna datoteka

- **`moja_datoteka.txt`** — tekstualna datoteka koju koristi `read_file.c`. Ako datoteka ne postoji u radnom direktoriju, `read_file` će javiti grešku pozivom `perror()`.

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
