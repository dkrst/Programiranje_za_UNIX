# Programiranje za UNIX

Skripta s primjerima iz kolegija **Programiranje za UNIX** (Damir Krstinić, Maja Braović, travanj 2026.), Sveučilište u Splitu, Fakultet elektrotehnike, strojarstva i brodogradnje (FESB).

Skripta je organizirana po poglavljima koja postupno uvode čitatelja u ključne koncepte UNIX sustava — od osnova rada u ljusci, preko C programiranja i sistemskih poziva, do procesa i signala. Svaki primjer ilustrira specifičnu funkcionalnost (ljuska, sistemski pozivi, rad s procesima, datotečni sustav, signali) i predviđen je za prevođenje standardnim `gcc`-om na proizvoljnom POSIX-kompatibilnom sustavu. Svaki direktorij ima vlastiti README s detaljnim opisom primjera i teorijskim uvodom u temu poglavlja.

## Preduvjeti

- POSIX-kompatibilan sustav (Linux, macOS, WSL na Windowsu, BSD varijante)
- `gcc` (ili drugi C prevodilac koji razumije GNU sintaksu Makefilea)
- `make`
- `bash` (za primjere shell skripti u P01)

## Struktura

Svaki direktorij odgovara jednom poglavlju skripte:

| Direktorij | Poglavlje |
|---|---|
| [`P01-Osnove_UNIXa/`](P01-Osnove_UNIXa/) | Osnove UNIX-a |
| [`P02-Osnove_programiranja/`](P02-Osnove_programiranja/) | Osnove programiranja u C-u |
| [`P03-Ulazno_izlazne_operacije/`](P03-Ulazno_izlazne_operacije/) | Ulazno/izlazne operacije |
| [`P04-Upravljanje_datotekama/`](P04-Upravljanje_datotekama/) | Upravljanje datotekama |
| [`P05-Okruzenje_procesa/`](P05-Okruzenje_procesa/) | Okruženje procesa |
| [`P06-Signali/`](P06-Signali/) | Signali |
| [`P07-Komunikacija_izmedju_procesa/`](P07-Komunikacija_izmedju_procesa/) | Komunikacija između procesa |

### [`P01-Osnove_UNIXa/`](P01-Osnove_UNIXa/)

Uvodno poglavlje koje upoznaje čitatelja s arhitekturom UNIX operacijskog sustava, organizacijom datotečnog sustava, pravima pristupa, naredbenom ljuskom, preusmjeravanjem ulaza i izlaza, ulančavanjem naredbi te osnovama pisanja shell skripti. Sadrži četiri **bash skripte** koje ilustriraju ključne koncepte: `pozdrav.sh`, `brojac.sh`, `backup.sh`, `trazi.sh`.

### [`P02-Osnove_programiranja/`](P02-Osnove_programiranja/)

Uvod u proces prevođenja i povezivanja C programa s primjerima. Demonstrira osnovnu strukturu C programa, uporabu zaglavlja i razlaganje koda na više prevodbenih jedinica. Detaljno se obrađuje korištenje alata `make` — od ručnog prevođenja preko jednostavnih pravila do potpunog Makefilea s varijablama, implicitnim pravilima i konvencionalnim pseudo-pravilima `default`, `all` i `clean`.

### [`P03-Ulazno_izlazne_operacije/`](P03-Ulazno_izlazne_operacije/)

Primjeri koji ilustriraju UNIX sistemske pozive za rad s datotekama (`open`, `creat`, `close`, `read`, `write`, `lseek`, `umask`, `dup`, `dup2`) i temeljni UNIX koncept *"sve je datoteka"* — datoteke, uređaji, terminali, cijevi i mrežni socketi koriste se kroz isto sučelje file deskriptora. Obrađuje se i dijeljenje datoteka među procesima i unutar istog procesa, s mehanizmom preusmjeravanja standardnih tokova.

### [`P04-Upravljanje_datotekama/`](P04-Upravljanje_datotekama/)

Upravljanje datotekama na razini datotečnog sustava: rad s metapodacima, prava pristupa, vlasništvo, vremenski žigovi, simboličke i tvrde veze, te navigacija direktorijima.

### [`P05-Okruzenje_procesa/`](P05-Okruzenje_procesa/)

Primjeri koji obrađuju životni ciklus UNIX procesa: argumente naredbenog retka, varijable okruženja, stvaranje novih procesa pozivom `fork()`, pokretanje programa funkcijama iz `exec` obitelji, čekanje završetka djece pomoću `wait()`, ograničavanje resursa kroz `setrlimit()`, te problematiku zombi i osirotjelih procesa.

### [`P06-Signali/`](P06-Signali/)

Primjeri koji obrađuju signale — UNIX-ov primarni mehanizam za asinkronu komunikaciju s procesom. Pokrivaju hvatanje signala (`signal`, `sigaction`), korištenje `SIGALRM` za vlastite alarme, signalnu komunikaciju između procesa, blokiranje i ignoriranje signala, te pokupljanje djece preko `SIGCHLD` rukovatelja.

### [`P07-Komunikacija_izmedju_procesa/`](P07-Komunikacija_izmedju_procesa/)

Mehanizmi međuprocesne komunikacije (IPC): anonimni i imenovani cjevovodi (`pipe`, FIFO), POSIX dijeljena memorija (`shm_open`, `mmap`), sinkronizacija pomoću semafora, POSIX redovi poruka, mapiranje datoteka u memoriju, te kratki uvod u System V IPC. Poglavlje uvodi i koncepte race condition-a i atomskih operacija u kontekstu paralelnog rada više procesa.

## Prevođenje i pokretanje

Svaki direktorij od P02 nadalje sadrži vlastiti `Makefile` s istim konvencijama (varijable `CC`, `CFLAGS`, `LDFLAGS`, `TARGETS`; implicitno pravilo `.c.o`; pseudo-pravila `default`, `all`, `clean`). Pozicionirajte se u željeni direktorij i pokrenite:

```sh
make all       # prevede sve primjere iz direktorija
make clean     # briše generirane izvršne i objektne datoteke
```

Za prevođenje pojedinačnog primjera dovoljno je:

```sh
make <ime_primjera>
```

Konkretne upute za pokretanje pojedinačnih primjera (uključujući očekivane argumente i izlaz) nalaze se u README datoteci svakog poglavlja.

P01 ne sadrži C kod nego shell skripte; one se pokreću izravno nakon dodjeljivanja prava izvršavanja:

```sh
chmod +x *.sh
./pozdrav.sh
```

## Licenca

Kod je objavljen pod licencom **GNU General Public License v3.0**. Detalji su u datoteci [`LICENSE`](LICENSE).
