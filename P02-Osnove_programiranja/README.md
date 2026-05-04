# Osnove programiranja

U ovom poglavlju dan je uvod u proces prevođenja i povezivanja C programa ilustriran jednostavnim primjerima.

### Najjednostavniji program

- [**`pozdrav.c`**](pozdrav.c) — program u jednoj datoteci koji ispisuje poruku na standardni izlaz. Najmanji smisleni C program koji se može prevesti i pokrenuti; služi za demonstraciju osnovne sintakse (`main`, `#include`, `printf`) i osnovnog toka prevođenja.

  ```c
  #include <stdio.h>

  int main() {
      printf("Dobar jutar!\n");

      return 0;
  }
  ```

### Program u više datoteka

Funkcionalno, primjer iz ovog odjeljka radi potpuno isto što i prethodni `pozdrav` — ispisuje istu poruku na standardni izlaz. Razlika je u tome što je kod razbijen u tri datoteke: zaglavlje s deklaracijom funkcije, C datoteku izvornog koda s njezinom definicijom i glavni program koji funkciju poziva. Ovaj umjetno složeniji oblik služi nam da objasnimo proces prevođenja i povezivanja u prisutnosti više prevodbenih jedinica. U realnim programima organizacija koda u više datoteka je gotovo univerzalno pravilo, pa je ovaj minimalni primjer dobra polazna točka za razumijevanje takvog načina rada.

- [**`funkcije.h`**](funkcije.h) — zaglavlje s deklaracijom funkcije `dobar_jutar()`. Pretprocesorska direktiva `#include` doslovno čita sadržaj navedene datoteke i "zalijepi" ga na svoje mjesto, prije nego što prevodilac počne s pravim prevođenjem. Zato bi se zaglavlje koje se u istoj prevodbenoj jedinici uključi više puta (npr. preko više drugih zaglavlja koja ga svako sa svoje strane uključuju) doslovno višestruko zalijepilo, što bi izazvalo greške višestruke deklaracije. Da bismo to spriječili, koristimo zaštitu od višestrukog uključivanja (engl. *include guards*) — par pretprocesorskih direktiva (`#ifndef _FUNKCIJE_H_` / `#define ... / #endif`) koji osigurava da se sadržaj zaglavlja ubaci samo prilikom prvog uključivanja, a sva kasnija ignoriraju.

  ```c
  #ifndef _FUNKCIJE_H_
  #define _FUNKCIJE_H_

  void dobar_jutar();

  #endif
  ```

- [**`funkcije.c`**](funkcije.c) — definicija (implementacija) funkcije `dobar_jutar()`.

  ```c
  #include <stdio.h>

  void dobar_jutar() {
      printf("Dobar jutar!\n");
  }
  ```

- [**`pozdrav_fn.c`**](pozdrav_fn.c) — glavni program koji uključuje `funkcije.h` i poziva `dobar_jutar()`.

  ```c
  #include "funkcije.h"

  int main() {
      dobar_jutar();

      return 0;
  }
  ```

Ovaj skup datoteka ilustrira osnovnu organizaciju projekta u više prevodbenih jedinica: zaglavlje `.h` sadrži deklaraciju i dijeli se između jedinica koje funkciju pozivaju i one koja ju definira, dok se `.c` datoteke prevode neovisno i kasnije povezuju u izvršni program.

## Prevođenje

### Ručno prevođenje u jednom koraku

Za `pozdrav.c`, koji ne ovisi o drugim datotekama izvornog koda, dovoljan je jedan poziv prevodioca:

```sh
gcc -Wall pozdrav.c -o pozdrav
```

Zastavica `-Wall` uključuje tipična upozorenja prevodioca; `-o pozdrav` određuje ime izlazne izvršne datoteke. Bez `-o`, rezultat bi se zvao `a.out`.

### Ručno prevođenje u dva koraka

Postupak generiranja izvršne datoteke zapravo se sastoji od dvije odvojene faze. U prvoj fazi (**prevođenje**, engl. *compilation*) prevodilac iz datoteke izvornog koda proizvodi datoteku objektnog koda. U drugoj fazi (**povezivanje**, engl. *linking*) jedna ili više objektnih datoteka spaja se u izvršni program, uz eventualno uključivanje simbola iz vanjskih biblioteka. Oba koraka mogu se pokrenuti odvojeno:

```sh
gcc -Wall -c pozdrav.c              # prevođenje: pozdrav.c -> pozdrav.o
gcc -Wall pozdrav.o -o pozdrav      # povezivanje: pozdrav.o -> pozdrav
```

Zastavica `-c` nalaže prevodiocu da stane nakon faze prevođenja i ne pokreće povezivanje.

### Ručno prevođenje programa iz više datoteka

Za `pozdrav_fn` potrebno je prevesti dvije izvorne datoteke u objektne, a zatim ih povezati u jedan izvršni program:

```sh
gcc -Wall -c pozdrav_fn.c           # -> pozdrav_fn.o
gcc -Wall -c funkcije.c             # -> funkcije.o
gcc -Wall pozdrav_fn.o funkcije.o -o pozdrav_fn
```

Ovdje se jasno vidi razlika između dviju faza koje smo ranije spomenuli — **prevođenja** i **povezivanja**. Prevođenje je sporiji proces u kojem prevodilac iz svake `.c` datoteke generira pripadnu `.o` (objektnu) datoteku — strojni kod te jedinice u oblik pripravan za povezivanje. Povezivanje je relativno brza operacija u kojoj se sve `.o` datoteke spajaju u izvršni program. Razdvajanje na dvije faze ima dvije važne koristi: **objektne datoteke možemo međusobno povezivati i u različitim kombinacijama** (ista `funkcije.o` može završiti u više različitih programa); još važnije, **kad u nekoj `.c` datoteci promijenimo neki redak, dovoljno je iznova prevesti samo tu jednu datoteku**, dok preostale `.o` datoteke ostaju kakve su bile. Povezivanje, međutim, uvijek treba ponoviti — ono je obavezni zadnji korak nakon svake izmjene.

Ovaj uvid je upravo razlog postojanja alata `make`. U projektu od nekoliko datoteka, ručno paziti koja se `.c` smije propustiti pri ponovnom prevođenju brzo postaje naporno i podložno greškama. `make` taj posao automatizira: na temelju vremena zadnje izmjene svake datoteke, sam odluči koje `.o` treba ponovno generirati, a koje su već svježe, i izvodi samo nužne korake.

### Korištenje alata `make`

`make` je alat koji automatizira upravo takav proces. Iz datoteke s pravilima (`Makefile`) čita koje ulazne datoteke čine projekt, kako ovise jedna o drugoj i kojim se naredbama iz njih generiraju izlazne datoteke. Na temelju vremena zadnje izmjene datoteka, `make` ponovno prevodi samo one koje su mijenjane od posljednjeg prevođenja, i ništa više. Pokreće se tako da se u direktoriju s `Makefile`-om zada:

```sh
make              # izvršava prvo pravilo u Makefileu
make ime_pravila  # izvršava navedeno pravilo
```

#### Struktura pravila

Svako pravilo ima oblik:

```
cilj: ovisnosti
	naredbe
```

- **cilj** je najčešće ime datoteke koja nastaje izvršavanjem pravila (u pravilu izvršna ili objektna datoteka).
- **ovisnosti** je popis datoteka o kojima cilj ovisi — ako je bilo koja od njih novija od cilja, pravilo se izvršava.
- **naredbe** su naredbe ljuske koje pravilo izvršava. **Moraju** biti uvučene tabulatorom, ne razmacima.

#### Gradnja Makefilea korak po korak

Krenut ćemo od najjednostavnije verzije i postupno je poboljšavati.

#### Korak 1: jednostavna pravila za svaki primjer

Prevedemo li logiku dvofaznog prevođenja iz prethodnog odjeljka u make pravila, za `pozdrav` dobivamo dva pravila — jedno za povezivanje (cilj `pozdrav`) i jedno za prevođenje (cilj `pozdrav.o`):

```make
pozdrav: pozdrav.o
	gcc -Wall pozdrav.o -o pozdrav

pozdrav.o: pozdrav.c
	gcc -Wall -c pozdrav.c
```

Isti pristup za `pozdrav_fn`, koji se povezuje iz dvije objektne datoteke:

```make
pozdrav_fn: pozdrav_fn.o funkcije.o
	gcc -Wall pozdrav_fn.o funkcije.o -o pozdrav_fn

pozdrav_fn.o: pozdrav_fn.c
	gcc -Wall -c pozdrav_fn.c

funkcije.o: funkcije.c
	gcc -Wall -c funkcije.c
```

Promotrimo strukturu i redoslijed izvođenja pravila. Kada korisnik zatraži `make pozdrav_fn`, `make` čita `Makefile` u potrazi za pravilom čiji je cilj `pozdrav_fn`. To pravilo kao ovisnosti navodi `pozdrav_fn.o` i `funkcije.o`. Kako ove dvije datoteke ne postoje, `make` traži pravila u kojima su one ciljevi, provjerava njihove ovisnosti, i izvršava zadane naredbe — najprije za `pozdrav_fn.o`, zatim za `funkcije.o` — i tek na kraju za `pozdrav_fn`.

Što ako neka od objektnih datoteka navedena u ovisnostima već postoji? U tom slučaju `make` u pravilu kojem je ta datoteka cilj provjerava ovisnosti, odnosno odgovarajuću datoteku izvornog koda: ako je datoteka izvornog koda (ovisnost) **novija** od objektne datoteke (cilj), naredbe iz pravila izvršit će se iznova kako bi objektna datoteka uključila izmjene u izvornom kodu nastale od posljednje primjene pravila. Ako je objektna datoteka novija od izvora — odnosno već "svježa" — `make` taj korak preskače. Na ovaj način `make` rekurzivno kroz `Makefile` provjerava jesu li pojedini koraci "zastarjeli", tj. postoje li izmjene u kodu od posljednje primjene pravila, i provodi samo one korake koji su nužni.

Ovakav Makefile je funkcionalan, ali pokazuje dva problema: (a) pravila za generiranje `.o` datoteka su praktički identična — razlikuju se samo po imenu datoteke, i (b) naredba `gcc -Wall` je ponovljena u svakom pravilu, pa promjena prevodioca ili zastavica traži izmjenu na više mjesta.

#### Korak 2: implicitna pravila

Postupak prevođenja `.c → .o` uvijek je isti: iz datoteke `ime.c` generira se `ime.o` istim pozivom prevodioca. Za takve unificirane postupke `make` nudi **implicitna pravila** koja se primjenjuju na temelju uzorka ekstenzije. Pravilo:

```make
.c.o:
	gcc -Wall -c $<
```

govori `make`-u kako iz bilo koje `.c` datoteke izgraditi istoimenu `.o` datoteku. Unutar naredbe, automatska varijabla `$<` zamjenjuje se imenom ulazne datoteke (one iz popisa ovisnosti). Isto se tako može koristiti `$@` za ime cilja.

Ovim jednim implicitnim pravilom zamijenjena su sva pojedinačna `.c → .o` pravila. Makefile sad izgleda ovako:

```make
pozdrav: pozdrav.o
	gcc -Wall pozdrav.o -o pozdrav

pozdrav_fn: pozdrav_fn.o funkcije.o
	gcc -Wall pozdrav_fn.o funkcije.o -o pozdrav_fn

.c.o:
	gcc -Wall -c $<
```

#### Korak 3: varijable

I dalje je prevodilac ugrađen u pravila kao `gcc`, a zastavica `-Wall` ponovljena na više mjesta. Uvođenjem **varijabli** se dio koji se ponavlja izvuče iz pravila i centralizira:

```make
CC = /usr/bin/gcc
CFLAGS = -Wall

pozdrav: pozdrav.o
	$(CC) $(CFLAGS) pozdrav.o -o pozdrav

pozdrav_fn: pozdrav_fn.o funkcije.o
	$(CC) $(CFLAGS) pozdrav_fn.o funkcije.o -o pozdrav_fn

.c.o:
	$(CC) $(CFLAGS) -c $<
```

Varijabla se deklarira imenom, znakom jednakosti i tekstualnom vrijednošću. Vrijednost se dohvaća kao `$(IME)`. Promjena prevodioca ili zastavica sada se svodi na izmjenu jednog retka.

Postoji konvencija iz GNU svijeta prema kojoj se zastavice za **prevođenje** drže u varijabli `CFLAGS`, a zastavice za **povezivanje** (npr. `-L/put/do/lib` ili biblioteke) u zasebnoj varijabli `LDFLAGS`. Naši primjeri u ovoj fazi ne trebaju posebne zastavice za linker, pa `LDFLAGS` ostaje prazna — ali ju uvodimo radi konzistentnosti i lakšeg proširivanja:

```make
CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS =

pozdrav: pozdrav.o
	$(CC) $(LDFLAGS) pozdrav.o -o pozdrav

pozdrav_fn: pozdrav_fn.o funkcije.o
	$(CC) $(LDFLAGS) pozdrav_fn.o funkcije.o -o pozdrav_fn

.c.o:
	$(CC) $(CFLAGS) -c $<
```

#### Korak 4: specijalna pravila `default`, `all`, `clean`

Ostaje još nekoliko praktičnih poboljšanja kojima se uvodi nekoliko konvencionalnih pravila. Bitno je odmah napomenuti: **imena `default`, `all` i `clean` nisu rezervirane ključne riječi** — pravilo `all` jednako bismo mogli nazvati `sve`, `clean` bismo mogli nazvati `ocisti`, i sve bi radilo identično. Ova imena su dogovorna konvencija koje se programeri drže kako bi `Makefile` bio čitljiv i predvidljiv svakome tko ga pogleda. Slično kao s imenima varijabli u kodu ili s imenima programa, dobra je praksa odabrati imena iz kojih se odmah vidi što pravilo radi; držanje navedene konvencije čini naš kod razumljivim drugima — a često i nama samima kad se vratimo svom kodu nakon izvjesnog vremena.

**`default`** — pravilo koje se izvršava kad korisnik pokrene `make` bez argumenata. `make` u tom slučaju jednostavno izvršava prvo pravilo navedeno u `Makefile`-u, bez obzira na njegovo ime — pa se po konvenciji to prvo pravilo zove `default` i smješta na vrh datoteke. Da smo mu dali bilo koje drugo ime, svejedno bi bilo pokrenuto kad korisnik upiše `make` bez argumenata.

```make
default: pozdrav_fn
```

Ovo pravilo ima samo cilj (`default`) i ovisnost (`pozdrav_fn`), ali nema naredbi. `make` provjerava ovisnosti i, prema ranije spomenutoj logici, traži pravilo u kojem je `pozdrav_fn` cilj te ga izvršava (rekurzivno, kako smo opisali — samo ako su pripadne datoteke zastarjele). Nakon toga `default` ne radi ništa jer nema vlastitih naredbi. Pravilo `default` koristi upravo taj trik da nas vodi do "korisnog" pravila koje stvarno gradi program na kojem trenutno radimo.

**`all`** — koristi isti trik za izgradnju svih ciljeva u direktoriju odjednom. Za lakše održavanje koristimo varijablu `TARGETS` koja popisuje sve izvršne datoteke:

```make
TARGETS = pozdrav pozdrav_fn

all: $(TARGETS)
```

I ovo pravilo ima samo ovisnosti, bez naredbi: `make` rekurzivno gradi svaku navedenu izvršnu datoteku, a samo `all` ne radi ništa.

**`clean`** — pravilo koje briše sve izvršne, objektne i privremene datoteke iz direktorija. Za razliku od `default` i `all`, ovdje imamo cilj i naredbe, ali **nema ovisnosti**. To znači da se pravilo izvršava bezuvjetno svaki put kad korisnik upiše `make clean` — neovisno o stanju datoteka u direktoriju.

```make
clean:
	rm -f $(TARGETS) *.o *~ a.out
```

I ovo pravilo bi se moglo nazvati drukčije (npr. `ocisti`), ali ime `clean` toliko je rasprostranjena konvencija da bi nestandardno ime samo zbunjivalo druge programere — pa ga zato i koristimo.

#### Konačni Makefile

Objedinimo sve navedeno u finalnu `Makefile` datoteku u ovom direktoriju:

```make
CC = /usr/bin/gcc
CFLAGS = -Wall
LDFLAGS = 
TARGETS = pozdrav pozdrav_fn

default: pozdrav_fn

all: $(TARGETS)

pozdrav: pozdrav.o
	$(CC) $(LDFLAGS) pozdrav.o -o pozdrav

pozdrav_fn: pozdrav_fn.o funkcije.o
	$(CC) $(LDFLAGS) pozdrav_fn.o funkcije.o -o pozdrav_fn

clean:
	rm -f $(TARGETS) *.o *~ a.out

.c.o:
	$(CC) $(CFLAGS) -c $<
```

Tipična uporaba:

```sh
make              # izvršava "default", tj. gradi pozdrav_fn
make all          # gradi oba primjera
make pozdrav      # gradi samo pozdrav
make clean        # briše izvršne i objektne datoteke
```

## Pokretanje

Izvršni programi pokreću se navođenjem relativne putanje (`./`) iz istog direktorija:

```sh
./pozdrav
./pozdrav_fn
```

Oba primjera ispisuju istu poruku; razlika je isključivo u unutarnjoj organizaciji koda.
