# Prezentacije za predavanja

Prezentacije prate skriptu *Programiranje za UNIX* (Krstinić, Braović, FESB).
Gradivo poglavlja P01–P07 podijeljeno je na 13 predavanja.

## Popis predavanja

| # | Predavanje | Poglavlje | Izvor | Slajdovi |
|---|---|---|---|---|
| 1 | Osnove UNIX-a | P01 | [md](Predavanje01-Osnove_unixa.md) | [pdf](Predavanje01-Osnove_unixa.pdf) |
| 2 | Ljuska, procesi i shell skripte | P01 | [md](Predavanje02-Ljuska_i_procesi.md) | [pdf](Predavanje02-Ljuska_i_procesi.pdf) |
| 3 | Osnove programiranja: GCC, make, ar | P02 | [md](Predavanje03-Osnove_programiranja.md) | [pdf](Predavanje03-Osnove_programiranja.pdf) |
| 4 | `make` i biblioteke | P02 | — | — |
| 5 | Sistemski pozivi za rad s datotekama | P03 | — | — |
| 6 | I/O strukture i dijeljenje datoteka | P03 | — | — |
| 7 | Svojstva datoteka (`stat`) | P04 | — | — |
| 8 | Prava, linkovi i direktoriji | P04 | — | — |
| 9 | Okruženje procesa, `fork` i `wait` | P05 | — | — |
| 10 | `exec`, ograničenja resursa, zombiji | P05 | — | — |
| 11 | Signali | P06 | — | — |
| 12 | Cjevovodi i FIFO | P07 | — | — |
| 13 | Dijeljena memorija, semafori, redovi poruka | P07 | — | — |

Opcijski, izvan 13 termina: višenitno programiranje (P08) i socketi (P09).

## Struktura

```
Prezentacije/
├── README.md                      <- ovaj popis
├── build_slides.py                <- generiranje PDF-a
├── fesb_slides.tex                <- zajednicka Beamer tema (boje, podnozje, naslovnica)
├── slides_filter.lua              <- slajd sa samo slikom -> bez podnozja
├── Predavanje01-Osnove_unixa.md   <- izvor (pandoc markdown, H2 = novi slajd)
├── Predavanje01-Osnove_unixa.pdf  <- generirani slajdovi
├── Predavanje02-Ljuska_i_procesi.md
├── Predavanje02-Ljuska_i_procesi.pdf
├── Predavanje03-Osnove_programiranja.md
├── Predavanje03-Osnove_programiranja.pdf
├── slike/                         <- slike svih predavanja
└── OLD/                           <- stare prezentacije kolegija
```

## Generiranje PDF-a

Preduvjeti: `pandoc`, `xelatex`, `lmodern`, DejaVu fontovi.

```
./build_slides.py            # sve prezentacije
./build_slides.py 01         # samo Predavanje01
```

Za svaku datoteku `Predavanje*.md` generira se `.pdf` istog imena.

## Konvencije

- H1 (`#`) je naslov poglavlja, H2 (`##`) je novi slajd.
- Jedan slajd = jedna ideja; kod najviše ~15 redaka po slajdu.
- Slike i njihova numeracija ("Slika X.Y") preuzimaju se iz skripte.
- Slajdovi tipa **Demo** sadrže cilj, naredbe za tipkanje uživo i poantu.
- Zadaci za samostalan rad ne idu na slajdove --- daju se usmeno na predavanju.
- Imenovanje: `PredavanjeNN-Naziv.md`, dvoznamenkasti broj termina.
- Izgled se mijenja isključivo u `fesb_slides.tex` — nikada u pojedinoj prezentaciji.
