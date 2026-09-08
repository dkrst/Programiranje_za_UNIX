# Prezentacije za predavanja

Prezentacije prate skriptu *Programiranje za UNIX* (Krstinić, Braović, FESB).
Gradivo poglavlja P01–P07 podijeljeno je na 13 predavanja od po ~80 minuta.

## Popis predavanja

| # | Predavanje | Poglavlje | Izvor | Slajdovi |
|---|---|---|---|---|
| 1 | Osnove UNIX-a | P01 | [md](P01-Osnove_UNIXa/Predavanje1-Osnove_unixa.md) | [pdf](P01-Osnove_UNIXa/Predavanje1-Osnove_unixa.pdf) |
| 2 | Ljuska i shell skripte | P01 | — | — |
| 3 | Prevođenje i povezivanje, GCC | P02 | — | — |
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
├── README.md                        <- ovaj popis
├── build_slides.py                  <- generiranje PDF-a
├── fesb.tex                         <- zajednicka Beamer tema (boje, podnozje, naslovnica)
├── P01-Osnove_UNIXa/
│   ├── Predavanje1-Osnove_unixa.md  <- izvor (pandoc markdown, H2 = novi slajd)
│   ├── Predavanje1-Osnove_unixa.pdf <- generirani slajdovi
│   └── slike/
└── OLD/                             <- stare prezentacije kolegija
```

## Generiranje PDF-a

Preduvjeti: `pandoc`, `xelatex`, `lmodern`, DejaVu fontovi.

```
./build_slides.py            # sve prezentacije
./build_slides.py P01        # samo jedno poglavlje
```

Za svaku `.md` datoteku u direktoriju poglavlja generira se `.pdf` istog imena.

## Konvencije

- H1 (`#`) je naslov poglavlja, H2 (`##`) je novi slajd.
- Jedan slajd = jedna ideja; kod najviše ~15 redaka po slajdu.
- Slike i njihova numeracija ("Slika X.Y") preuzimaju se iz skripte.
- Slajdovi tipa **Demo** sadrže cilj, naredbe za tipkanje uživo i poantu.
- Izgled se mijenja isključivo u `fesb.tex` — nikada u pojedinoj prezentaciji.
