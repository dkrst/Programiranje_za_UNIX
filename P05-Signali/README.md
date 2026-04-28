# Signali

Primjeri uz poglavlje **Signali** iz knjige *Programiranje za UNIX*.

U ovom poglavlju upoznat ćemo **signale** — UNIX-ov primarni mehanizam za asinkronu komunikaciju s procesom. Signal je kratka poruka koju jezgra šalje procesu kao obavijest da se dogodio neki događaj: korisnik je pritisnuo Ctrl+C, proces je pokušao pristupiti nedopuštenoj memorijskoj adresi, istekao je timer, neki drugi proces je eksplicitno zatražio prekid, i tako dalje. Iz perspektive procesa, signal može stići u **bilo kojem trenutku** između dvije strojne instrukcije — proces nema mogućnost predvidjeti kada će se to dogoditi.

Proces na primljeni signal može reagirati na nekoliko načina: prepustiti zadanu reakciju jezgri (što za većinu signala znači prekid procesa), eksplicitno ignorirati signal, ili registrirati vlastitu funkciju — **rukovatelj signala** (engl. *signal handler*) — koja će se izvršiti svaki put kad takav signal stigne. U ovom poglavlju kroz nekoliko primjera ilustriramo kako se signali registriraju, kako se hvataju, kako se koriste za komunikaciju među procesima i koje je opasnosti potrebno izbjegavati pri pisanju rukovatelja.

## Sadržaj

### Hvatanje signala

- [**`potvrdi.c`**](potvrdi.c) — minimalan primjer hvatanja signala koji uvodi sve ključne koncepte rada s njima. Program registrira vlastiti rukovatelj za signal `SIGINT` (signal koji se procesu šalje kad korisnik u terminalu pritisne `Ctrl+C`); kad korisnik pritisne `Ctrl+C` prvi put, program ne završi, nego ispiše poruku da treba pritisnuti `Ctrl+C` još jednom ako se zaista želi izaći. Ovo je obrazac kakav viđamo u stvarnim alatima (npr. `htop`, `vim`) i u skriptama koje se ne smiju nehotice prekinuti.

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>

  int brojac = 0;

  void uhvati(int signum) {
      brojac++;
  }

  int main() {
      signal(SIGINT, uhvati);

      while (brojac < 2) {
          pause();
          if (brojac == 1)
              printf("Pritisnite ponovo CTRL - C ukoliko zelite izaci\n");
      }

      return 0;
  }
  ```

  Za registraciju rukovatelja koristi se sistemski poziv `signal`:

  ```c
  #include <signal.h>

  void (*signal(int signum, void (*handler)(int)))(int);
  ```

  Na prvi pogled deklaracija djeluje zastrašujuće, ali zapravo je riječ o funkciji koja prima dva argumenta i vraća pokazivač:

  - **`signum`** — broj signala koji želimo hvatati (npr. `SIGINT`)
  - **`handler`** — pokazivač na funkciju koja će biti pozvana kad signal stigne. Funkcija mora primati jedan `int` argument (broj signala) i ne smije vraćati ništa (`void`).
  - **Povratna vrijednost** — pokazivač na *prethodno* registriranu funkciju, ili `SIG_ERR` u slučaju greške.

  Ovdje se prvi put susrećemo s **pokazivačem na funkciju** kao argumentom drugoj funkciji. Ideja je jednostavna: kako svaka funkcija, baš kao i svaka varijabla, u izvršnoj datoteci ima svoju adresu u memoriji, tako i njezino ime u kodu (bez zagrada) jednostavno označava upravo tu adresu. Drugim riječima, **ime funkcije je njezin pokazivač**. U našem programu funkcija `uhvati` definirana je tako da prima `int` i ne vraća ništa — što točno odgovara tipu argumenta `handler` — pa je dovoljno predati samo njezino ime: `signal(SIGINT, uhvati)`.

  Mehanizam je sljedeći. Funkcija `signal(SIGINT, uhvati)` jezgri kaže: *"od ovog trenutka, kad god mom procesu stigne signal `SIGINT`, ne primjenjuj zadanu reakciju (prekid procesa) nego pozovi funkciju `uhvati`."* Kad korisnik pritisne Ctrl+C, jezgra **prekine** trenutno izvršavanje glavnog programa točno tamo gdje je bilo, pozove `uhvati`, a kad se ona vrati, glavni program nastavlja izvršavanje od mjesta gdje je bio prekinut. Sav posao koji rukovatelj napravi — u ovom slučaju jednostavno povećanje varijable `brojac` — vidljiv je glavnom programu kroz globalnu varijablu, što je standardni način "razgovora" između main-a i rukovatelja.

  Glavni program koristi sistemski poziv `pause()`, koji uspava proces sve dok ne stigne **bilo koji** signal. Kad signal stigne i njegov rukovatelj završi izvršavanje, `pause()` se vrati i petlja nastavlja: provjeri trenutnu vrijednost `brojac`-a, ako je on `1` ispiše uputu, a u sljedećem prolazu petlje opet uđe u `pause()` čekajući novi signal. Tek kad `brojac` dosegne `2`, izlazi iz petlje i program uredno završava.

  Pokrenimo program i isprobajmo:

  ```
  $ ./potvrdi
  ^C
  Pritisnite ponovo CTRL - C ukoliko zelite izaci
  ^C
  Korisnik je potvrdio ozlazak - kraj programa!
  $
  ```

  Da bismo bolje uočili koliki je doprinos rukovatelja, predlažemo i sljedeći eksperiment: zakomentirajte redak `signal(SIGINT, uhvati);`, ponovno prevedite program i pokrenite ga. Pritisak `Ctrl+C` sada će dovesti do trenutnog prekida programa — nećete vidjeti ni poruku iz petlje, ni završnu poruku. Bez registriranog rukovatelja primjenjuje se **zadana reakcija** jezgre na `SIGINT`, a ona za ovaj signal podrazumijeva prekid procesa.

  Funkcionalno je program gotovo trivijalan, ali pokriva nekoliko važnih koncepata vrijednih da se odmah istaknu:

  - **Rukovatelj je vrlo kratak** — samo inkrementira brojač i ne pokušava ništa složenije od toga (npr. nema poziva `printf`-a). Ovo nije slučajno: rukovatelj signala se izvršava u posebnom kontekstu — može prekinuti glavni program u doslovno bilo kojem trenutku, uključujući i sredinu poziva drugih funkcija. Iz rukovatelja se zato smije pozivati samo vrlo ograničen skup funkcija (tzv. **`async-signal-safe`** funkcije). `printf` u taj skup ne spada — njegovo korištenje u rukovatelju može u rijetkim slučajevima dovesti do iznenađujućih grešaka. Detaljnije ćemo o ovome u kasnijem primjeru, ali već sad uvodimo dobru praksu: **rukovatelj postavlja zastavicu, glavni program reagira**.

  - **Komunikacija kroz globalnu varijablu** — rukovatelj i glavni program "razgovaraju" kroz `brojac`. Strogo gledano, takve dijeljene varijable trebale bi biti deklarirane s tipom `volatile sig_atomic_t` umjesto običnog `int`-a:
    - Ključna riječ `volatile` govori prevoditelju da vrijednost varijable može biti promijenjena "iza leđa" glavnog programa (od strane rukovatelja), pa optimizator ne smije njezinu vrijednost cache-irati u registru kroz iteracije petlje.
    - Tip `sig_atomic_t` jamči da se čitanje i pisanje varijable obavlja u jednoj nedjeljivoj operaciji — rukovatelj ne može uhvatiti glavni program "u sredini" upisa.

    Na većini modernih arhitektura (uključujući x86) u praksi će raditi i obični `int`.

  > **Napomena.** U nekim povijesnim verzijama UNIX-a rukovatelj signala resetirao bi se svaki put kada bi signal bio primljen, pa ga je bilo potrebno ponovno registrirati. U modernim verzijama UNIX-a ovo ponašanje gotovo sigurno nećete susresti.

### Vlastiti alarm — `SIGALRM` i `alarm()`

- [**`alarm_clock.c`**](alarm_clock.c) — primjer u kojem proces **sam sebi** zakaže signal. U svim dosadašnjim primjerima signal je dolazio izvana — od korisnika preko Ctrl+C ili od drugog procesa preko `kill`-a. UNIX, međutim, omogućuje i da proces zatraži od jezgre da mu nakon određenog broja sekundi pošalje signal `SIGALRM`. Tu funkcionalnost pruža sistemski poziv `alarm()`:

  ```c
  #include <unistd.h>

  unsigned int alarm(unsigned int seconds);
  ```

  Ovaj poziv jezgri kaže: *"za točno `seconds` sekundi pošalji mi `SIGALRM`."* Ako je već postojao prethodni alarm, on se zamjenjuje novim, a poziv vraća broj sekundi koje su preostale do isporuke prethodnog alarma. Pozivom `alarm(0)` aktivni alarm se otkazuje. Bitno je primijetiti da je `alarm` **jednokratan** — ako želimo periodičko "tikanje" svakih N sekundi, novi alarm moramo zakazati svaki put iznova.

  Upravo to radi naš primjer:

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>

  int brojac = 0;

  void alrm_handler(int signum) {
      brojac++;
      alarm(1);
  }

  int main() {
      signal(SIGALRM, alrm_handler);
      alarm(1);

      while (brojac < 5) {
          pause();
          printf("tik %d\n", brojac);
      }

      printf("kraj!\n");
      return 0;
  }
  ```

  Glavni program registrira rukovatelj za `SIGALRM`, postavlja prvi alarm na jednu sekundu i ulazi u petlju koja čeka pet "tikanja". Rukovatelj je vrlo kratak — samo poveća brojač i odmah zakaže sljedeći alarm — pa se ovaj ciklus odvija jednom u sekundi. Ispis "tik N" obavlja sam glavni program nakon što se vrati iz `pause()`-a, slijedeći obrazac koji smo uveli kod prethodnog primjera (*rukovatelj postavlja zastavicu, glavni program reagira*).

  Pokrenimo program:

  ```
  $ ./alarm_clock
  tik 1
  tik 2
  tik 3
  tik 4
  tik 5
  kraj!
  ```

  Cijeli ispis traje točno pet sekundi.

  Primijetite da smo rukovatelj imenovali `alrm_handler` umjesto naprosto `uhvati`, kao u prethodnom primjeru. Pri imenovanju funkcija dobra je praksa odabrati intuitivna imena koja odmah daju naslutiti što funkcija radi. U slučaju rukovatelja signala nije loše u ime ugraditi i ime samog signala — npr. `alrm_handler` za `SIGALRM`, ili `int_handler` za `SIGINT`, što bi u prethodnom primjeru bilo prikladnije ime od općenitog `uhvati`. Ovo je naravno samo sugestija autora; čitatelj ima punu slobodu imenovati svoje funkcije kako god mu odgovara.

- [**`stoperica.c`**](stoperica.c) — primjer koji kombinira tehnike iz prethodna dva: koristi `SIGALRM` za odbrojavanje sekundi, a `SIGINT` (Ctrl+C) za zaustavljanje. Funkcionalno se ponaša kao jednostavna stoperica: nakon pokretanja svake sekunde ispiše "tik N", a kad korisnik pritisne Ctrl+C ispiše ukupno proteklo vrijeme i uredno završi.

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>

  int brojac = 0;
  int broji  = 1;

  void alrm_handler(int signum) {
      brojac++;
      alarm(1);
  }

  void int_handler(int signum) {
      broji = 0;
  }

  int main() {
      signal(SIGALRM, alrm_handler);
      signal(SIGINT,  int_handler);

      printf("Stoperica pokrenuta -- pritisnite Ctrl+C za zaustavljanje.\n");
      alarm(1);

      while (broji) {
          pause();
          if (broji)
              printf("tik %d\n", brojac);
      }

      printf("Proteklo: %d sekundi\n", brojac);
      return 0;
  }
  ```

  Program ima dvije globalne varijable kroz koje glavni program i rukovatelji "razgovaraju": `brojac` koji broji koliko je sekundi prošlo i `broji` koji upravlja izvršavanjem glavne petlje. Registrirana su dva rukovatelja s različitim ulogama: `alrm_handler` poveća brojač i ponovno zakaže alarm (točno kao u prethodnom primjeru), dok `int_handler` postavi `broji = 0` čime signalizira glavnoj petlji da treba završiti.

  Glavna petlja `while (broji)` u svakom prolazu pasivno čeka signal pomoću `pause()`. Kad signal stigne i odgovarajući rukovatelj završi, `pause()` se vrati i petlja nastavlja. Slijedi mali ali važan detalj: prije ispisa "tik N" provjeravamo da je `broji` i dalje istinit. Razlog je u tome što se `pause()` vraća za **bilo koji** primljeni signal, uključujući i `SIGINT`. Bez te provjere, posljednji ispis bio bi suvišan "tik N" prije završne poruke "Proteklo: N sekundi". Ovaj obrazac — *nakon `pause()`, ponovo provjeri stanje pa tek onda reagiraj* — tipičan je kad više signala utječe na istu petlju.

  Pokrenimo stopericu i nakon nekoliko sekundi pritisnimo Ctrl+C:

  ```
  $ ./stoperica
  Stoperica pokrenuta -- pritisnite Ctrl+C za zaustavljanje.
  tik 1
  tik 2
  tik 3
  ^CProteklo: 3 sekundi
  ```

  Imenovanjem rukovatelja prema signalu na koji odgovaraju (`alrm_handler` i `int_handler`), kod postaje samodokumentirajuć — već iz naziva je jasno koji rukovatelj reagira na koji signal, što je posebno korisno kad u programu imamo više signala koje hvatamo.

- [**`stoperica2.c`**](stoperica2.c) — funkcionalno identičan prethodnom programu, ali strukturno drukčiji: koristi **jedan zajednički rukovatelj** za oba signala umjesto dva odvojena. Ovaj primjer služi da uvedemo prvi argument koji rukovatelj prima — `signum`, broj signala koji je upravo isporučen procesu.

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>

  int brojac = 0;
  int broji  = 1;

  void rukovatelj(int signum) {
      switch (signum) {
          case SIGALRM:
              brojac++;
              alarm(1);
              break;
          case SIGINT:
              broji = 0;
              break;
      }
  }

  int main() {
      signal(SIGALRM, rukovatelj);
      signal(SIGINT,  rukovatelj);

      printf("Stoperica pokrenuta -- pritisnite Ctrl+C za zaustavljanje.\n");
      alarm(1);

      while (broji) {
          pause();
          if (broji)
              printf("tik %d\n", brojac);
      }

      printf("Proteklo: %d sekundi\n", brojac);
      return 0;
  }
  ```

  Do sada smo argument rukovatelja, iako ga je deklaracija zahtijevala, jednostavno ignorirali. U ovom primjeru njegova svrha postaje jasna: kad jedna funkcija obrađuje više različitih signala, jezgra joj kao argument predaje broj signala koji je upravo isporučen, pa funkcija na temelju toga može odlučiti što dalje. `switch` je u takvim slučajevima prirodniji od lanca `if/else if` jer se lakše proširuje dodavanjem novih `case` grana za nove signale.

  Pokretanje i ispis su identični prethodnom primjeru:

  ```
  $ ./stoperica2
  Stoperica pokrenuta -- pritisnite Ctrl+C za zaustavljanje.
  tik 1
  tik 2
  tik 3
  ^CProteklo: 3 sekundi
  ```

  Koji je pristup bolji — razdvojeni rukovatelji kao u `stoperica.c` ili zajednički kao u `stoperica2.c`? Stvar je ukusa i konteksta. Razdvojeni rukovatelji su pregledniji kad je logika za svaki signal značajno različita i kad u svakom rukovatelju ima više od nekoliko redaka koda. Zajednički rukovatelj je prikladan kad signali dijele zajedničke resurse ili pomoćne varijable, ili kad očekujemo da će se broj obrađivanih signala s vremenom povećavati. U realnim programima često su zastupljena oba pristupa istovremeno: na primjer, jedan zajednički rukovatelj za sve signale koji označavaju zahtjev za prekidom (`SIGINT`, `SIGTERM`, `SIGHUP`) i poseban rukovatelj za vremenske signale.

## Prevođenje

Direktorij dolazi s priloženim [`Makefile`](Makefile)-om koji prati iste konvencije kao i Makefile datoteke u prethodnim poglavljima (varijable `CC`, `CFLAGS`, `LDFLAGS`, `TARGETS`; implicitno pravilo `.c.o`; pravila `default`, `all`, `clean`). Detaljan opis strukture i korake gradnje Makefilea vidjeti u [`../P02-Osnove_programiranja/README.md`](../P02-Osnove_programiranja/README.md).

Tipična uporaba:

```sh
make              # gradi zadani cilj (potvrdi)
make all          # gradi sve primjere
make stoperica    # gradi samo zadani primjer
make clean        # briše izvršne i objektne datoteke
```
