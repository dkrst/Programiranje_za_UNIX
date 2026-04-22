# Programiranje za UNIX

Primjeri koda uz knjigu **Programiranje za UNIX** (Damir Krstinić, Maja Braović, siječanj 2025.), pisanu za kolegij istog imena na Fakultetu elektrotehnike, strojarstva i brodogradnje u Splitu (FESB).

Ovaj repozitorij sadrži izvorni kod svih primjera iz knjige, organiziran po poglavljima. Svaki primjer ilustrira specifičnu funkcionalnost UNIX sustava (ljuska, sistemski pozivi, rad s procesima, datotečni sustav) i predviđen je za prevođenje standardnim `gcc`-om na proizvoljnom POSIX-kompatibilnom sustavu.

## Preduvjeti

- POSIX-kompatibilan sustav (Linux, macOS, WSL na Windowsu, BSD varijante)
- `gcc` (ili drugi C prevodilac koji razumije GNU sintaksu Makefilea)
- `make`

## Struktura repozitorija

Direktoriji odgovaraju poglavljima knjige:

| Direktorij | Poglavlje knjige |
|---|---|
| `osnove_programiranja/` | Osnove programiranja u C-u |
| `fileio/` | Ulazno/izlazne operacije |

### `osnove_programiranja/`

Uvodni primjeri koji ilustriraju osnovnu strukturu C programa, uporabu zaglavlja i rastav koda na više prevodbenih jedinica.

- **`pozdrav.c`** — najjednostavniji program koji ispisuje poruku na standardni izlaz.
- **`pozdrav_fn.c`** + **`funkcije.c`** + **`funkcije.h`** — ista funkcionalnost, ali s pozivom funkcije definirane u zasebnoj prevodbenoj jedinici; demonstrira uporabu zaglavlja i povezivanje više objektnih datoteka.

### `fileio/`

Primjeri koji ilustriraju UNIX sistemske pozive za rad s datotekama (`open`, `creat`, `close`, `read`, `write`, `lseek`, `umask`) i temeljni UNIX koncept da se sve — datoteke, uređaji, terminali, cijevi — vide kroz isto sučelje file deskriptora.

- **`read_file.c`** — otvara `moja_datoteka.txt` pozivom `open()` i ispisuje njen sadržaj na standardni izlaz čitajući znak po znak.
- **`io_copy.c`** — kopira standardni ulaz na standardni izlaz koristeći međuspremnik konstantne veličine; pogodan za uporabu u cjevovodima (npr. `./io_copy < ulaz.txt > izlaz.txt`).
- **`f_cat.c`** — pojednostavljena implementacija naredbe `cat`: ispisuje sadržaj svih datoteka navedenih u argumentima, a u odsutnosti argumenata prepisuje standardni ulaz na izlaz.
- **`f_write.c`** — čita sa standardnog ulaza i upisuje u novostvorenu datoteku, čije se ime zadaje kao argument naredbenog retka.
- **`f_strip.c`** — demonstrira `lseek()` s `SEEK_SET`: upisuje prvi niz znakova, apsolutno pozicionira offset na 15. bajt i upisivanjem drugog niza prepisuje dio postojećeg sadržaja.
- **`f_hole.c`** — demonstrira `lseek()` s `SEEK_CUR`: pomicanjem offseta iza kraja datoteke nastaje "rupa" koja se pri čitanju popunjava nulama.
- **`perm_mask.c`** — ilustrira kako maska kreiranja datoteke (`umask`) utječe na prava pristupa pri pozivu `creat()`.
- **`moja_datoteka.txt`** — primjer ulazne datoteke za `read_file.c`.

## Prevođenje i pokretanje

Svaki direktorij sadrži vlastiti `Makefile`. Pozicionirajte se u željeni direktorij i pokrenite:

```sh
make all       # prevede sve primjere iz direktorija
make clean     # briše generirane izvršne i objektne datoteke
```

Za prevođenje pojedinačnog primjera dovoljno je:

```sh
make <ime_primjera>
```

npr. `make f_strip`.

Pokretanje iz istog direktorija (neki primjeri očekuju ulaznu datoteku u radnom direktoriju):

```sh
./read_file                         # čita moja_datoteka.txt
./io_copy < ulaz.txt > izlaz.txt    # preusmjeravanje
ls | ./io_copy                      # kao filter u cjevovodu
./f_cat datoteka1.txt datoteka2.txt
./f_strip                           # stvara file.strip
./f_hole                            # stvara file.hole
./perm_mask                         # stvara datoteka1 i datoteka2
```

## Licenca

Kod je objavljen pod licencom **GNU General Public License v3.0**. Detalji su u datoteci [`LICENSE`](LICENSE).
