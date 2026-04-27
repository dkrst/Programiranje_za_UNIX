# osnove_programiranja

Primjeri uz poglavlje **Osnove programiranja u C-u** iz knjige *Programiranje za UNIX*.

Primjeri u ovom direktoriju služe kao minimalni praktični uvod u proces prevođenja C programa: kako se jedna ili više datoteka izvornog koda pretvaraju u izvršni program, te kako se kod organizira u više prevodbenih jedinica povezanih preko zaglavlja.

## Sadržaj

### Najjednostavniji program

- **`pozdrav.c`** — program u jednoj datoteci koji ispisuje poruku na standardni izlaz. Najmanji smisleni C program koji se može prevesti i pokrenuti; služi za demonstraciju osnovne sintakse (`main`, `#include`, `printf`) i osnovnog toka prevođenja.

### Program u više datoteka

- **`funkcije.h`** — zaglavlje s deklaracijom funkcije `dobar_jutar()`. Uključuje standardne *include guards* (`#ifndef _FUNKCIJE_H_` / `#define ... / #endif`) koji sprječavaju višestruko uključivanje istog zaglavlja.
- **`funkcije.c`** — definicija (implementacija) funkcije `dobar_jutar()`.
- **`pozdrav_fn.c`** — glavni program koji uključuje `funkcije.h` i poziva `dobar_jutar()`.

Ovaj skup datoteka ilustrira osnovnu organizaciju projekta u više prevodbenih jedinica: zaglavlje `.h` sadrži deklaraciju i dijeli se između jedinica koje funkciju pozivaju i one koja ju definira, dok se `.c` datoteke prevode neovisno i kasnije povezuju u izvršni program.

## Ručno prevođenje

### Prevođenje u jednom koraku

Za `pozdrav.c`, koji ne ovisi o drugim datotekama izvornog koda, dovoljan je jedan poziv prevodioca:

```sh
gcc -Wall pozdrav.c -o pozdrav
```

Zastavica `-Wall` uključuje tipična upozorenja prevodioca; `-o pozdrav` određuje ime izlazne izvršne datoteke. Bez `-o`, rezultat bi se zvao `a.out`.

### Prevođenje u dva koraka

Postupak generiranja izvršne datoteke zapravo se sastoji od dvije odvojene faze. U prvoj fazi (**prevođenje**, engl. *compilation*) prevodilac iz datoteke izvornog koda proizvodi datoteku objektnog koda. U drugoj fazi (**povezivanje**, engl. *linking*) jedna ili više objektnih datoteka spaja se u izvršni program, uz eventualno uključivanje simbola iz vanjskih biblioteka. Oba koraka mogu se pokrenuti odvojeno:

```sh
gcc -Wall -c pozdrav.c              # prevođenje: pozdrav.c -> pozdrav.o
gcc -Wall pozdrav.o -o pozdrav      # povezivanje: pozdrav.o -> pozdrav
```

Zastavica `-c` nalaže prevodiocu da stane nakon faze prevođenja i ne pokreće povezivanje.

### Prevođenje programa iz više datoteka

Za `pozdrav_fn` potrebno je prevesti dvije izvorne datoteke u objektne, a zatim ih povezati u jedan izvršni program:

```sh
gcc -Wall -c pozdrav_fn.c           # -> pozdrav_fn.o
gcc -Wall -c funkcije.c             # -> funkcije.o
gcc -Wall pozdrav_fn.o funkcije.o -o pozdrav_fn
```

Ovakav radni tok — izmjena izvornog koda, ponovno prevođenje svih izvornih datoteka, povezivanje — brzo postaje naporan čim projekt naraste na nekoliko datoteka, a s vremenom i neefikasan: nakon izmjene u jednoj `.c` datoteci nema potrebe ponovno prevoditi ostale, već se prevodi samo ta jedna i ponavlja povezivanje.

## Alat `make`

`make` je alat koji automatizira upravo takav proces. Iz datoteke s pravilima (`Makefile`) čita koje ulazne datoteke čine projekt, kako ovise jedna o drugoj i kojim se naredbama iz njih generiraju izlazne datoteke. Na temelju vremena zadnje izmjene datoteka, `make` ponovno prevodi samo one koje su mijenjane od posljednjeg prevođenja, i ništa više. Pokreće se tako da se u direktoriju s `Makefile`-om zada:

```sh
make              # izvršava prvo pravilo u Makefileu
make ime_pravila  # izvršava navedeno pravilo
```

### Struktura pravila

Svako pravilo ima oblik:

```
cilj: ovisnosti
	naredbe
```

- **cilj** je najčešće ime datoteke koja nastaje izvršavanjem pravila (u pravilu izvršna ili objektna datoteka).
- **ovisnosti** je popis datoteka o kojima cilj ovisi — ako je bilo koja od njih novija od cilja, pravilo se izvršava.
- **naredbe** su naredbe ljuske koje pravilo izvršava. **Moraju** biti uvučene tabulatorom, ne razmacima.

### Gradnja Makefilea korak po korak

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

Ostaje još nekoliko praktičnih poboljšanja.

**`default`** je konvencionalno ime za pravilo koje se izvršava kad se `make` pozove bez argumenata. `make` tada izvršava prvo pravilo u datoteci, pa se `default` smješta na vrh. Obično nema vlastitih naredbi — u popisu ovisnosti navodi se cilj koji se treba izgraditi po zadanom:

```make
default: pozdrav_fn
```

**`all`** izgrađuje sve izvršne datoteke odjednom. Ovisi o svim ciljevima; za lakše održavanje koristimo varijablu `TARGETS` koja popisuje sve izvršne datoteke:

```make
TARGETS = pozdrav pozdrav_fn

all: $(TARGETS)
```

**`clean`** briše izvršne, objektne i privremene datoteke. Nema ovisnosti (izvršava se bezuvjetno) niti stvara datoteku istog imena — to je takozvano *phony* pravilo:

```make
clean:
	rm -f $(TARGETS) *.o *~ a.out
```

### Konačni Makefile

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
