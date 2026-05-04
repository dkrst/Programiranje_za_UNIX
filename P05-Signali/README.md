# Signali

U ovom poglavlju upoznat ćemo **signale** — UNIX-ov primarni mehanizam za asinkronu komunikaciju s procesom. Signal je kratka poruka koju jezgra šalje procesu kao obavijest da se dogodio neki događaj: korisnik je pritisnuo Ctrl+C, proces je pokušao pristupiti nedopuštenoj memorijskoj adresi, istekao je timer, neki drugi proces je eksplicitno zatražio prekid, i tako dalje. Iz perspektive procesa, signal može stići u **bilo kojem trenutku** između dvije strojne instrukcije — proces nema mogućnost predvidjeti kada će se to dogoditi.

Proces na primljeni signal može reagirati na nekoliko načina: prepustiti zadanu reakciju jezgri (što za većinu signala znači prekid procesa), eksplicitno ignorirati signal, ili registrirati vlastitu funkciju — **rukovatelj signala** (engl. *signal handler*) — koja će se izvršiti svaki put kad takav signal stigne. U ovom poglavlju kroz nekoliko primjera ilustriramo kako registriramo rukovatelj signala, kako se signali hvataju, kako se koriste za komunikaciju među procesima i o čemu je potrebno voditi računa pri pisanju rukovatelja.

## Najčešći signali

Signali su, gledano iz perspektive operacijskog sustava i programa, jednostavno **mali cjelobrojni identifikatori** — svaki signal ima svoj broj. Zbog čitljivosti i prenosivosti koda, u programima nikad ne baratamo izravno tim brojevima, nego koristimo **simboličke konstante** definirane u standardnoj zaglavnoj datoteci `<signal.h>` (npr. `SIGINT`, `SIGTERM`, `SIGKILL`...).

POSIX standard definira tridesetak signala, a u praksi se najčešće susrećemo sa skupinom njih desetak. Tablica niže daje pregled onih s kojima ćemo raditi u ovom poglavlju i u tipičnim programima:

| Signal | Broj | Predefinirana akcija | Značenje |
|---|---|---|---|
| `SIGHUP`  |  1 | prekid procesa            | terminal je zatvoren ili je sesija prekinuta; često se koristi i kao "ponovno učitaj konfiguraciju" |
| `SIGINT`  |  2 | prekid procesa            | korisnik je pritisnuo Ctrl+C u terminalu; programi ga često hvataju kako bi od korisnika zatražili potvrdu izlaska — "čisti" izlaz |
| `SIGQUIT` |  3 | prekid procesa + core file | korisnik je pritisnuo Ctrl+\ — kao SIGINT, ali generira i core file |
| `SIGILL`  |  4 | prekid procesa + core file | proces je pokušao izvršiti nevažeću strojnu instrukciju |
| `SIGABRT` |  6 | prekid procesa + core file | proces je sam sebe prekinuo pozivom `abort()` (npr. zbog `assert` greške) |
| `SIGFPE`  |  8 | prekid procesa + core file | aritmetička greška (dijeljenje s nulom, prelijevanje) |
| `SIGKILL` |  9 | prekid procesa            | bezuvjetni prekid — **ne može se uhvatiti niti ignorirati**; jezgra ga koristi kao zadnju mjeru kad se proces ogluši o `SIGTERM` |
| `SIGSEGV` | 11 | prekid procesa + core file | proces je pokušao pristupiti memorijskoj adresi kojoj nema pravo pristupa (*segmentation fault*) |
| `SIGPIPE` | 13 | prekid procesa            | pokušaj pisanja u cijevovod (*pipe*) ili socket čiji je drugi kraj zatvoren |
| `SIGALRM` | 14 | prekid procesa            | istekao je timer postavljen pozivom `alarm()` |
| `SIGTERM` | 15 | prekid procesa            | pristojan zahtjev za prekid; programi ga često koriste za uredno spremanje stanja i "čisti" izlaz. Ako se program ogluši o `SIGTERM`, jezgra ga može bezuvjetno prekinuti signalom `SIGKILL` |
| `SIGUSR1` | 10 | prekid procesa            | korisnički signal br. 1 — bez unaprijed definirane semantike, slobodan za vlastite svrhe |
| `SIGUSR2` | 12 | prekid procesa            | korisnički signal br. 2 — bez unaprijed definirane semantike, slobodan za vlastite svrhe |
| `SIGCHLD` | 17 | ignorira se               | dijete procesa je promijenilo stanje (završilo, zaustavljeno itd.) |
| `SIGSTOP` | 19 | zaustavi proces           | bezuvjetno zaustavljanje — **ne može se uhvatiti niti ignorirati** |
| `SIGCONT` | 18 | nastavi proces            | nastavi izvršavanje zaustavljenog procesa |
| `SIGTSTP` | 20 | zaustavi proces           | korisnik je pritisnuo Ctrl+Z u terminalu — svojevrsna "pauza" za pokrenuti program (zaustavlja ga privremeno; nastavak slanjem `SIGCONT`) |
| `SIGXCPU` | 24 | prekid procesa + core file | premašen je CPU limit postavljen `setrlimit()`-om (vidi [Limiti](../P04-Okruzenje_procesa/README.md#ograničavanje-resursa-setrlimit)) |
| `SIGXFSZ` | 25 | prekid procesa + core file | premašen je file size limit postavljen `setrlimit()`-om (vidi [Limiti](../P04-Okruzenje_procesa/README.md#ograničavanje-resursa-setrlimit)) |

Brojevi signala u tablici odgovaraju POSIX-u za osnovne signale i podudaraju se s vrijednostima na većini modernih UNIX sustava. Ipak, najsigurniji pristup je u kodu uvijek koristiti **simbolička imena** (`SIGINT`, `SIGTERM`...) umjesto numeričkih vrijednosti — time se izbjegavaju zabune i osigurava prenosivost koda na sustave gdje neki manje uobičajeni signali mogu imati drukčije brojeve. Brojeve navodimo samo radi reference (npr. uz ispis "Prekid signalom, signal: 11" iz `pokreni2`-a). Potpuni popis dostupan je u priručniku: `man 7 signal`.

Posebnu pažnju zaslužuju **`SIGKILL`** i **`SIGSTOP`** — to su jedina dva signala koja se ne mogu uhvatiti niti ignorirati. Razlog je praktičan: bez ova dva signala sustav ne bi imao apsolutni mehanizam za bezuvjetan prekid ili zaustavljanje "neposlušnog" procesa. Svaki drugi signal proces može uhvatiti i odlučiti kako reagirati — uključujući i `SIGTERM`, što neki programi iskorištavaju za uredno spremanje stanja prije izlaska.

**Što je core file?** Za neke signale predefinirana akcija nije samo prekid procesa, nego i stvaranje *core file*-a — datoteke koja sadrži snimku kompletnog memorijskog prostora procesa u trenutku kad je signal primljen (varijable na stogu, hrpi, registri procesora, otvoreni file deskriptori i drugi metapodatci). Datoteka se obično zove `core` ili `core.<pid>` i nastaje u trenutnom radnom direktoriju procesa. Ovo ponašanje vezano je uz signale koji uglavnom znače da smo u programu *nešto zabrljali* — `SIGSEGV` (pristup nevažećoj memoriji), `SIGFPE` (aritmetička greška), `SIGILL` (nevažeća instrukcija), `SIGABRT` (eksplicitan prekid pomoću `abort()`). Core file omogućuje *post-mortem* analizu: alatima poput `gdb` programer može učitati core file zajedno s izvršnom datotekom, vidjeti gdje je proces bio u trenutku pada, koje su vrijednosti varijabli imale, kakav je bio stack trace itd. Naravno, ako za to imamo volje i znanja — u suprotnom core file se može jednostavno obrisati.

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
  - **`handler`** — rukovatelj signala; pokazivač na funkciju koja će biti pozvana kada signal `signum` stigne. Rukovatelj signala kao argument prima jednu cjelobrojnu vrijednost (`int`) koja označava broj signala koji je pozvao rukovatelj (isti rukovatelj može se pozivati za više različitih signala). Rukovatelj signala ima povratni tip `void` — nema povratnu vrijednost.
  - **Povratna vrijednost** — pokazivač na *prethodno* registriranu funkciju, ili `SIG_ERR` u slučaju greške.

  Ovdje se prvi put susrećemo s **pokazivačem na funkciju** kao argumentom drugoj funkciji. Ideja je jednostavna: kako svaka funkcija, baš kao i svaka varijabla, u izvršnoj datoteci ima svoju adresu u memoriji, tako i njezino ime u kodu (bez zagrada) jednostavno označava upravo tu adresu. Drugim riječima, **ime funkcije je njezin pokazivač**. U našem programu funkcija `uhvati` definirana je tako da prima `int` i ne vraća ništa — što točno odgovara tipu argumenta `handler` — pa je dovoljno predati samo njezino ime: `signal(SIGINT, uhvati)`.

  Mehanizam je sljedeći. Funkcija `signal(SIGINT, uhvati)` jezgri kaže: *"od ovog trenutka, kad god mom procesu stigne signal `SIGINT`, ne primjenjuj zadanu reakciju (prekid procesa) nego pozovi funkciju `uhvati`."* Kad korisnik pritisne Ctrl+C, jezgra prekine trenutno izvršavanje glavnog programa točno tamo gdje je bilo, pozove `uhvati`, a kad se ona vrati, glavni program nastavlja izvršavanje od mjesta gdje je bio prekinut. Sav posao koji rukovatelj napravi — u ovom slučaju jednostavno povećanje varijable `brojac` — vidljiv je glavnom programu kroz globalnu varijablu, što je standardni način "razgovora" između main-a i rukovatelja.

  Glavni program koristi sistemski poziv `pause()`, koji uspava proces sve dok ne stigne bilo koji signal. Kad signal stigne i njegov rukovatelj završi izvršavanje, `pause()` se vrati i petlja nastavlja: provjeri trenutnu vrijednost `brojac`-a, ako je on `1` ispiše uputu, a u sljedećem prolazu petlje opet uđe u `pause()` čekajući novi signal. Tek kad `brojac` dosegne `2`, izlazi iz petlje i program uredno završava.

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

  - **Rukovatelj je vrlo kratak** — samo inkrementira brojač i ne pokušava ništa složenije od toga (npr. nema poziva `printf`-a). Ovo nije slučajno: rukovatelj signala se izvršava u posebnom kontekstu — može prekinuti glavni program u doslovno bilo kojem trenutku, uključujući i sredinu poziva drugih funkcija. Preporuka je iz rukovatelja pozivati samo tzv. **`async-signal-safe`** funkcije — funkcije za koje POSIX standard jamči da ih je sigurno pozvati iz signal handlera. U taj skup ne spada i `printf` — njegovo korištenje u rukovatelju može u rijetkim slučajevima dovesti do iznenađujućih grešaka. Detaljnije ćemo o ovome u kasnijem primjeru, ali već sad uvodimo dobru praksu: rukovatelj postavlja zastavicu, glavni program reagira.

  - **Komunikacija kroz globalnu varijablu** — rukovatelj i glavni program "razgovaraju" kroz `brojac`. Strogo gledano, takve dijeljene varijable trebale bi biti deklarirane s tipom `volatile sig_atomic_t` umjesto običnog `int`-a:
    - Ključna riječ `volatile` govori prevoditelju da vrijednost varijable može biti promijenjena "iza leđa" glavnog programa (od strane rukovatelja), pa optimizator ne smije njezinu vrijednost cache-irati u registru kroz iteracije petlje.
    - Tip `sig_atomic_t` jamči da se čitanje i pisanje varijable obavlja u jednoj nedjeljivoj operaciji — rukovatelj ne može uhvatiti glavni program "u sredini" upisa.

    Na većini modernih arhitektura (uključujući x86) u praksi će raditi i obični `int`.

  > **Napomena.** U nekim povijesnim verzijama UNIX-a rukovatelj signala resetirao bi se svaki put kada bi signal bio primljen, pa ga je bilo potrebno ponovno registrirati. U modernim verzijama UNIX-a ovo ponašanje gotovo sigurno nećete susresti.

### Vlastiti alarm — `SIGALRM` i `alarm()`

- [**`alarm_clock.c`**](alarm_clock.c) — primjer u kojem proces zakaže slanje signala samome sebi. Izvor signala najčešće dolazi izvan samog procesa, npr. ukoliko korisnik pritisne Ctrl+C, ili eksplicitno zatražimo slanje signala pozivom `kill` iz ljuske. UNIX, međutim, omogućuje i da proces zatraži od jezgre da mu nakon određenog broja sekundi pošalje signal `SIGALRM`. Tu funkcionalnost pruža sistemski poziv `alarm()`:

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

- [**`stoperica2.c`**](stoperica2.c) — funkcionalno identičan prethodnom programu, ali strukturno drukčiji: koristi **jedan zajednički rukovatelj** za oba signala umjesto dva odvojena. Na ovom primjeru ilustrirano je korištenje argumenta `signum` koji rukovatelj prima — broj signala koji je upravo isporučen procesu.

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

### Korištenje signala za komunikaciju između procesa

U dosadašnjim primjerima signali su dolazili iz dva izvora: izvana, od korisnika preko Ctrl+C, ili iznutra, kad ih je proces sam sebi zakazao pozivom `alarm()`. UNIX, međutim, dopušta i da jedan proces eksplicitno **pošalje signal drugom procesu** — što čini signale jednim od najjednostavnijih oblika međuprocesne komunikacije (engl. *Inter-Process Communication*, IPC). Za to služi sistemski poziv `kill`:

```c
#include <signal.h>
#include <sys/types.h>

int kill(pid_t pid, int sig);
```

**Povratna vrijednost:** `0` u slučaju uspjeha; `-1` u slučaju greške (npr. ne postoji proces s navedenim PID-om, ili nemamo dovoljnu razinu ovlasti za slanje signala tom procesu).

Ime `kill` je povijesno — prvotno je sistemski poziv služio isključivo za prekid drugog procesa pomoću `SIGKILL` ili `SIGTERM`. Vremenom se njegova uloga proširila i danas se koristi za slanje **bilo kojeg** signala, ali ime je ostalo. Istog imena je i istoimena ugrađena naredba ljuske (`kill -<sig> <pid>`, npr. `kill -USR1 12345`), kojom korisnik može poslati proizvoljni signal nekom procesu iz terminala.

- [**`razgovor.c`**](razgovor.c) — primjer u kojem dva procesa komuniciraju isključivo putem signala. Roditelj `fork`-a dijete, zatim mu u petlji svake sekunde šalje `SIGUSR1`. Dijete na svaki primljeni signal ispiše poruku i broji koliko ih je primilo. Nakon pet "dojava", roditelj djetetu pošalje `SIGTERM` da uredno završi, pa pričeka njegov završetak pozivom `wait()`.

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/wait.h>

  int radi = 1;
  int prosao = 0;

  void usr1_handler(int signum) {
      prosao++;
  }

  void term_handler(int signum) {
      radi = 0;
  }

  int main() {
      pid_t pid = fork();

      if (pid < 0) {
          perror("fork");
          return 1;
      }

      if (pid == 0) {
          /* CHILD */
          signal(SIGUSR1, usr1_handler);
          signal(SIGTERM, term_handler);

          while (radi) {
              pause();
              if (radi)
                  printf("[child]  primio sam SIGUSR1, prolaz %d\n", prosao);
          }

          printf("[child]  zavrsavam, ukupno prolaza: %d\n", prosao);
          return 0;
      }

      /* PARENT */
      printf("[parent] pokrenuo dijete s PID-om %d\n", pid);

      for (int k = 0; k < 5; k++) {
          sleep(1);
          printf("[parent] saljem SIGUSR1 djetetu\n");
          kill(pid, SIGUSR1);
      }

      sleep(1);
      printf("[parent] saljem SIGTERM djetetu\n");
      kill(pid, SIGTERM);

      wait(NULL);
      printf("[parent] dijete je zavrsilo, izlazim\n");
      return 0;
  }
  ```

  Struktura programa razdvaja se odmah nakon `fork()`-a u dvije grane. **Dijete** registrira dva rukovatelja: `usr1_handler` koji broji primljene `SIGUSR1` signale, i `term_handler` koji postavlja `radi = 0` čime signalizira glavnoj petlji da treba završiti. Petlja je standardna `while (radi) pause()` konstrukcija s provjerom `if (radi)` prije `printf`-a, kao u `stoperica.c`-u, da se izbjegne suvišan ispis nakon `SIGTERM`-a.

  Bitno je primijetiti da se rukovatelji **registriraju samo u dječjoj grani**, a ne i u roditelju. Razlog je u tome što roditelj ove signale uopće ne prima — on je **pošiljatelj**, a ne primatelj. Roditelj eksplicitno šalje `SIGUSR1` i `SIGTERM` djetetu pozivom `kill(pid, ...)`, a sam ne treba reagirati na te signale. Da smo registraciju rukovatelja stavili **prije** `fork()`-a, oba bi je procesa naslijedila — pa bi i roditelj imao registrirane rukovatelje koji se nikad ne bi izvršili, što ne bi bila greška, ali je nepotrebno. Stavljanjem registracije unutar dječje grane jasno odvajamo uloge: **dijete reagira, roditelj šalje**.

  **Roditelj** zna PID djeteta jer mu ga je `fork()` vratio, pa može pozivati `kill(pid, SIGUSR1)` da mu šalje signale u željenim trenucima. Po završetku radne petlje šalje `SIGTERM`, a zatim `wait(NULL)` čeka da dijete uredno završi (čime se ono prestaje voditi kao zombi proces u tablici procesa, kao što smo objasnili u prethodnom poglavlju).

  Pokrenimo program:

  ```
  $ ./razgovor
  [parent] pokrenuo dijete s PID-om 12345
  [parent] saljem SIGUSR1 djetetu
  [child]  primio sam SIGUSR1, prolaz 1
  [parent] saljem SIGUSR1 djetetu
  [child]  primio sam SIGUSR1, prolaz 2
  [parent] saljem SIGUSR1 djetetu
  [child]  primio sam SIGUSR1, prolaz 3
  [parent] saljem SIGUSR1 djetetu
  [child]  primio sam SIGUSR1, prolaz 4
  [parent] saljem SIGUSR1 djetetu
  [child]  primio sam SIGUSR1, prolaz 5
  [parent] saljem SIGTERM djetetu
  [child]  zavrsavam, ukupno prolaza: 5
  [parent] dijete je zavrsilo, izlazim
  ```

  Iz ispisa je vidljiv pravilan redoslijed događaja: roditelj svake sekunde signalizira djetetu, dijete trenutno reagira, i tako pet puta zaredom; konačno `SIGTERM` zatvara petlju u djetetu, a roditelj nakon `wait()`-a izlazi.

  Bitno je naglasiti da `kill(pid, sig)` samo "isporuči" signal djetetu — ne čeka da rukovatelj završi izvršavanje, a nema nikakve garancije da će signal biti obrađen prije nego roditelj nastavi sa svojim radom. Ovo je primjer **asinkronog** mehanizma: pošiljatelj i primatelj nisu uskladeni vremenski. Za pravu sinkroniziranu komunikaciju (gdje pošiljatelj čeka primateljev odgovor) postoje drugi UNIX IPC mehanizmi — cijevi, redovi poruka, dijeljena memorija — kojima ćemo se baviti u kasnijim poglavljima.

  > **Značenje signala.**
  >
  > Većina UNIX signala ima predefinirano značenje. Međutim, kako programer može napisati vlastiti rukovatelj za većinu signala, time praktički može promijeniti njihovo predefinirano značenje — npr. iskoristiti `SIGSEGV` (kojim nas jezgra upozorava da smo s pokazivačem izašli izvan svog memorijskog prostora) za razmjenu poruka između roditelja i djeteta, kao u našem primjeru. Iako ovo nije nikakav problem implementirati, ideja je vrlo loša: što ako stvarno u programu napravimo grešku u rukovanju memorijom i jezgra nas pokuša na to upozoriti, a mi taj signal protumačimo kao poruku od drugog procesa?
  >
  > Ideja da signalima pridijelimo posve drugačije značenje otprilike je jednako dobra kao da se studenti koji slušaju kolegij *Programiranje za UNIX* na FESB-u dogovore da crveno svjetlo na semaforu za njih znači "kreni", a zeleno "stani": dok su sami na cesti, sustav može funkcionirati, ali ako se pojavi bilo koji drugi vozač, posljedice su potencijalno katastrofalne.
  >
  > Iz istog razloga treba poštivati predefinirana značenja signala, a za vlastite komunikacijske protokole između grupe procesa koristiti slobodne signale `SIGUSR1` i `SIGUSR2`, kao u našem primjeru.

## Sistemski poziv `sigaction`

U svim dosadašnjim primjerima rukovatelje smo registrirali korištenjem funkcije `signal()`. Iako je `signal()` jednostavan i intuitivan, njegova povijest puna je nedosljednosti između UNIX implementacija. U starijim System V verzijama rukovatelj se nakon prve isporuke signala automatski poništavao (resetirao na zadanu reakciju), pa je sljedeća instanca istog signala — koja stigne prije nego rukovatelj stigne ponovno registrirati samog sebe — uzrokovala prekid procesa. Na BSD sustavima rukovatelj je ostajao registriran. Drugi izvor razlika bilo je ponašanje **prekinutih sistemskih poziva**: kad signal stigne procesu koji je u sporom sistemskom pozivu (`read`, `accept`, `pause`, `wait`...), BSD je sistemski poziv automatski nastavljao, dok je System V vraćao grešku s `errno = EINTR`. Treći problem je "atomarnost" registracije — bilo je moguće da signal stigne usred zamjene rukovatelja i pozove pogrešnu funkciju.

Da bi se ove razlike uklonile i ponašanje precizno definiralo, POSIX uvodi noviju funkciju `sigaction()`, koja u potpunosti zamjenjuje `signal()` i nudi dodatne mogućnosti: blokiranje drugih signala tijekom obrade rukovatelja, kontrolu nad nastavkom prekinutih sistemskih poziva, te alternativnu formu rukovatelja koja prima detaljne informacije o izvoru signala.

```c
#include <signal.h>

int sigaction(int signum, const struct sigaction *act,
              struct sigaction *oldact);

struct sigaction {
    void     (*sa_handler)(int);
    void     (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t   sa_mask;
    int        sa_flags;
    void     (*sa_restorer)(void);    /* obsolete, ne dirati */
};
```

Argumenti `sigaction()`-a su:

- **`signum`** — broj signala koji se hvata (kao i kod `signal()`-a),
- **`act`** — pokazivač na strukturu koja opisuje novu akciju za taj signal,
- **`oldact`** — pokazivač na strukturu u koju će se upisati **prethodna** akcija; korisno ako želimo privremeno preuzeti signal pa kasnije vratiti staro ponašanje. Ako nas prethodna akcija ne zanima, kao treći argument možemo koristiti `NULL` pokazivač.

Najvažnija polja strukture `struct sigaction`:

- **`sa_handler`** — pokazivač na funkciju koja će biti pozvana kad signal stigne, isto kao drugi argument funkcije `signal()`. Umjesto pokazivača na funkciju, polju se mogu dodijeliti i posebne vrijednosti `SIG_DFL` (vrati zadanu reakciju jezgre) ili `SIG_IGN` (ignoriraj signal).
- **`sa_mask`** — skup signala koje treba blokirati **dok se rukovatelj izvršava**. Po POSIX defaultu, dok se izvršava rukovatelj za neki signal `S`, taj isti signal `S` automatski je blokiran — ako tijekom obrade `S`-a stigne nova instanca, ona čeka u redu (engl. *pending*) i isporučuje se tek nakon što rukovatelj završi. Polje `sa_mask` proširuje ovo blokiranje: tu možemo navesti **dodatne** signale koji će biti blokirani tijekom izvršavanja rukovatelja. Ovo je iznimno korisno kad više signala dijele isti rukovatelj ili manipuliraju istim podacima — postavljanjem cross-maske između njih sprječavamo da jedan rukovatelj prekine drugog usred kritične sekcije.
- **`sa_flags`** — bit-maska zastavica koje fino podešavaju ponašanje. Najčešće korištene su:
  - `SA_RESTART` — automatski nastavi prekinute sistemske pozive (BSD ponašanje); bez ove zastavice, sistemski poziv prekinut signalom vraća grešku `EINTR`.
  - `SA_NOCLDWAIT` — kad se postavi za `SIGCHLD`, jezgra automatski uklanja zombije bez potrebe za `wait()`-om.
  - `SA_SIGINFO` — proširuje rukovatelj dodatnim informacijama o signalu (vidi opis polja `sa_sigaction` niže).

Polje `sa_sigaction` je alternativa polju `sa_handler` — koristi se uz zastavicu `SA_SIGINFO` i daje rukovatelju prošireni potpis `void f(int signum, siginfo_t *info, void *context)`. Drugi argument `info` je struktura s detaljnim podacima o signalu (PID procesa pošiljatelja, UID njegovog vlasnika, razlog isporuke...). Treći argument `context` daje pristup CPU registrima u trenutku prekida (rijetko se koristi izravno). Dva polja, `sa_handler` i `sa_sigaction`, na nekim su sustavima implementirana kao `union` — pa je dobra praksa koristiti samo jedno od njih i nikad oba istovremeno. Polje `sa_restorer` je interni Linux mehanizam i ne smije se eksplicitno postavljati. Zato je dobra praksa cijelu strukturu prije korištenja inicijalizirati `memset`-om, čime osiguravamo da neiskorištena polja imaju nulte vrijednosti.

**Atomarna zamjena rukovatelja.** Bitno je razumjeti da `sigaction()` postavlja novu akciju atomski: nije moguće da signal stigne usred izmjene, pronađe pola-staro-pola-novo stanje, i pozove pogrešan rukovatelj. To je važna garancija koju System V `signal()` nije pružao.

Postupak za registraciju rukovatelja `sigaction()`-om uvijek slijedi isti obrazac:

1. Deklarirati `struct sigaction sa`,
2. Nulirati strukturu pomoću `memset(&sa, 0, sizeof(sa))`,
3. Postaviti `sa.sa_handler` na pokazivač na rukovatelj,
4. Inicijalizirati masku pomoću `sigemptyset(&sa.sa_mask)` (ili dodati signale koje treba blokirati),
5. Postaviti `sa.sa_flags` na željenu kombinaciju zastavica (najčešće `0`),
6. Pozvati `sigaction(signum, &sa, NULL)`.

**Za sav novi kod preporučuje se `sigaction()` umjesto `signal()`.** U ostatku ovog poglavlja ipak smo se služili objema funkcijama: `signal()` u jednostavnijim primjerima gdje smo htjeli zadržati pregledan kod, a `sigaction()` ondje gdje su nam potrebne njegove dodatne mogućnosti.

- [**`potvrdi2.c`**](potvrdi2.c) — funkcionalno identičan primjeru `potvrdi.c` s početka poglavlja, ali rukovatelj se registrira pomoću `sigaction()`-a:

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>
  #include <string.h>

  int brojac = 0;

  void int_handler(int signum) {
      brojac++;
  }

  int main() {
      struct sigaction sa;

      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = int_handler;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = 0;

      sigaction(SIGINT, &sa, NULL);

      while (brojac < 2) {
          pause();
          if (brojac == 1)
              printf("Pritisnite ponovo CTRL - C ukoliko zelite izaci\n");
      }

      printf("Korisnik je potvrdio izlazak - kraj programa!\n");
      return 0;
  }
  ```

  Glavna petlja, rukovatelj i logika su nepromijenjeni u odnosu na `potvrdi.c` — razlika je samo u načinu registracije. Iako je ovo nešto više koda od jednostavnog `signal(SIGINT, int_handler)` poziva, zauzvrat dobivamo precizno definirano ponašanje koje vrijedi na svim POSIX sustavima.

  Pokretanje i ispis su identični prethodnom primjeru:

  ```
  $ ./potvrdi2
  ^C
  Pritisnite ponovo CTRL - C ukoliko zelite izaci
  ^C
  Korisnik je potvrdio izlazak - kraj programa!
  $
  ```

## Blokiranje i ignoriranje signala

Do sada smo signale uvijek "hvatali" — registrirali rukovatelja koji bi se pozvao kad signal stigne. UNIX, međutim, nudi i druge načine da odredimo što se događa kad signal stigne procesu. Dva najvažnija su **blokiranje** i **ignoriranje**, i između njih postoji važna razlika.

**Blokiranje** ne uklanja signal — samo odgađa njegovu isporuku. Svaki proces ima takozvanu **masku signala** (engl. *signal mask*): skup signala koji su trenutno blokirani. Maska signala je još jedan od podataka o procesu koje jezgra čuva u tablici procesa — uz sve drugo što smo dosad spominjali (PID, PPID, tablica otvorenih datoteka, izlazni status, limiti resursa). Postupno se vidi koliko različitih informacija jezgra mora držati za svaki proces, sve uredno upakirano u jedan slog tablice procesa. Kad signal stigne procesu, a taj signal je u njegovoj maski, jezgra ga pohrani u **red čekanja** (engl. *pending*). Tamo ostaje sve dok proces ne ukloni signal iz maske — tek tada se isporučuje, i tek tada se pokreće rukovatelj (ili zadana akcija). Blokiranje koristimo kad imamo dio koda koji ne smije biti prekinut signalom, ali ne želimo trajno izgubiti signal.

**Ignoriranje** je kvalitativno drugačije: signal se isporučuje, ali se odmah odbacuje. Proces ne saznaje da je signal stigao i nikad ne reagira na njega. Za ignoriranje koristimo specijalnu vrijednost `SIG_IGN` koju postavimo kao "rukovatelj" pomoću `sigaction()`-a (ili `signal()`-a). Ignoriranje koristimo kad nas određeni signal jednostavno ne zanima.

### Sistemski poziv `sigprocmask`

Maska signala glavnog programa mijenja se sistemskim pozivom `sigprocmask`:

```c
#include <signal.h>

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
```

Argumenti:

- **`how`** — što se radi s argumentom `set`:
  - `SIG_BLOCK` — dodaj signale iz `set` trenutnoj maski,
  - `SIG_UNBLOCK` — ukloni signale iz `set` iz trenutne maske,
  - `SIG_SETMASK` — postavi masku točno na `set`.
- **`set`** — skup signala kojim mijenjamo masku; ako je `NULL`, maska se ne mijenja (poziv samo dohvaća staru u `oldset`).
- **`oldset`** — pokazivač u koji se upisuje **prethodna** maska, korisno za kasnije vraćanje stare vrijednosti; ako nas to ne zanima, predaje se `NULL`.

Argumente tipa `sigset_t` koriste i drugi pozivi koje smo već vidjeli (polje `sa_mask` u `struct sigaction`). To je apstraktni skup signala kojim ne baratamo izravno, nego pomoću pomoćnih funkcija:

```c
int sigemptyset(sigset_t *set);                    /* prazan skup */
int sigfillset(sigset_t *set);                     /* svi signali */
int sigaddset(sigset_t *set, int signum);          /* dodaj signal u skup */
int sigdelset(sigset_t *set, int signum);          /* makni signal iz skupa */
int sigismember(const sigset_t *set, int signum);  /* je li u skupu? */
```

Funkcija `sigemptyset` inicijalizira `set` kao prazan skup, na koji potom novim signalima dodajemo pojedinačno pomoću `sigaddset`. Ovaj pristup je prikladan kada treba sastaviti masku s relativno malim brojem signala — npr. samo `SIGINT` u našem `maska.c` primjeru. Obrnuto, kada nam treba maska koja sadrži većinu signala, prirodnije je krenuti od potpunog skupa i ukloniti samo one koje ne želimo: `sigfillset` inicijalizira `set` kao skup koji sadrži sve signale, a `sigdelset` zatim uklanja pojedinačne signale po potrebi. Funkcija `sigismember` provjerava sadrži li `set` zadani signal — koristimo je npr. nakon poziva `sigpending` da bismo provjerili je li određeni signal u redu čekanja.

Postoji i poziv `sigpending(sigset_t *set)` koji u `set` upisuje signale koji su upravo **u redu čekanja** — stigli su procesu, ali su blokirani pa se još nisu isporučili. Korisno kad prije skidanja maske želimo provjeriti hoće li nešto biti isporučeno.

- [**`maska.c`**](maska.c) — kratak primjer blokiranja `SIGINT`-a tijekom kratke "kritične sekcije". Program registrira jednostavan rukovatelj koji ispiše poruku, blokira `SIGINT`, "radi" pet sekundi (`sleep`), pa skida masku.

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>
  #include <string.h>

  void int_handler(int signum) {
      printf("SIGINT obraden\n");
  }

  int main() {
      struct sigaction sa;
      sigset_t blok, prethodna;

      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = int_handler;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = 0;
      sigaction(SIGINT, &sa, NULL);

      sigemptyset(&blok);
      sigaddset(&blok, SIGINT);

      sigprocmask(SIG_BLOCK, &blok, &prethodna);
      sleep(5);
      sigprocmask(SIG_SETMASK, &prethodna, NULL);

      return 0;
  }
  ```

  Pokrenimo program i tijekom njegovog rada iz druge ljuske pošaljimo `SIGINT` (npr. `kill -INT <pid>`). Iako signal stiže odmah, proces neće reagirati dok ne završi `sleep` — rukovatelj se izvršava tek nakon poziva `sigprocmask(SIG_SETMASK, &prethodna, NULL)` koji vraća masku na prethodnu vrijednost (bez `SIGINT`-a). Iz ovoga je jasno da je signal sve to vrijeme čekao u redu.

  Bitan detalj: koliko god `SIGINT`-a pošaljemo tijekom blokade, rukovatelj će se izvršiti **samo jednom**. Razlog je u tome što jezgra za "klasične" UNIX signale ne vodi brojač pojavljivanja, samo zastavicu "u redu čekanja je / nije". Više instanci istog signala koje stignu tijekom blokade spojaju se u jednu jedinu isporuku. (POSIX-realtime signali, brojevi 32–64, imaju pravi red s brojačem, ali to je tema za posebnu raspravu.)

  **Pažljivi čitatelj će uočiti suptilan problem** s gornjim kodom. Redoslijed operacija je: prvo se pozivom `sigaction()` registrira rukovatelj, a tek zatim pozivom `sigprocmask()` blokira `SIGINT`. Što se događa ako `SIGINT` stigne u kratkom vremenskom prozoru između ova dva poziva? Signal će se isporučiti — rukovatelj će raditi svoj posao prije nego što program uopće uđe u "kritičnu sekciju". U našem benignom primjeru posljedica je samo blago netočan tajming, ali u stvarnom programu u kojem kritična sekcija mijenja zajedničke podatke, ovakav race condition može dovesti do korupcije podataka i neočekivanog ponašanja programa.

  Možda biste pomislili da problem rješavamo postavljanjem `SIGINT`-a u polje `sa.sa_mask` strukture `struct sigaction`. Međutim, **`sa_mask` blokira signale samo dok se rukovatelj izvršava** — kad se rukovatelj vrati, blokada nestaje, pa to ne pomaže za zaštitu naše kritične sekcije.

  Ostavljamo ovu situaciju kao **vježbu za čitatelja**: pokušajte preraditi `maska.c` tako da garantirano nijedan `SIGINT` ne može biti isporučen prije nego što program uđe u kritičnu sekciju, čak ni ako signal stigne u "neugodnom" trenutku. (Savjet: razmislite o redoslijedu poziva `sigaction()` i `sigprocmask()`.)

- [**`maska2.c`**](maska2.c) — funkcionalno drugačiji primjer, ali strukturom vrlo blizak prethodnom: umjesto da blokiramo `SIGINT`, ovdje ga **ignoriramo**. Rukovatelj nije potreban — kao "akciju" za signal postavljamo specijalnu vrijednost `SIG_IGN`:

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>
  #include <string.h>

  int main() {
      struct sigaction sa;

      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = SIG_IGN;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = 0;
      sigaction(SIGINT, &sa, NULL);

      sleep(5);
      printf("kraj programa - SIGINT-i su tijekom rada bili ignorirani\n");

      return 0;
  }
  ```

  Pokretanje izgleda slično `maska.c`-u — pet sekundi spavanja tijekom kojih program ne reagira na Ctrl+C, a na kraju ispiše završnu poruku koja nam jasno pokazuje da je program došao do kraja, tj. da `SIGINT` nije prekinuo izvršavanje. Kvalitativna razlika u odnosu na `maska.c` dolazi do izražaja kad usporedimo izlaze: `maska.c` će prije završne poruke iz `main`-a ispisati `SIGINT obraden` (ako smo poslali signal tijekom spavanja), dok `maska2.c` to neće ispisati nikad — signal je tiho odbačen u trenutku isporuke.

  Razlika između blokiranja i ignoriranja, sažeto:

  | | **Blokiranje (`sigprocmask`)** | **Ignoriranje (`SIG_IGN`)** |
  |---|---|---|
  | Što se događa kada signal stigne | jezgra ga pohrani u red čekanja | jezgra ga odbaci |
  | Kad se uvjet ukloni | signal se isporučuje, rukovatelj radi | ništa se ne događa, signal više ne postoji |
  | Više instanci istog signala | spajaju se u jednu isporuku | sve odbačene |
  | Tipičan use case | "ne sad, ali ne želim izgubiti signal" | "ovaj signal me zaista ne zanima" |

  Razlika postaje važna kad nas zanima da na signal ipak reagiramo, samo ne odmah. Klasičan primjer: tijekom kritične transakcije s bazom podataka, ne želimo prekid Ctrl+C-om, ali nakon završene transakcije želimo provjeriti je li korisnik tražio izlaz pa se uredno zatvoriti. Tu pomaže blokiranje; ignoriranje bi sve takve zahtjeve trajno izgubilo.

## Pokupljanje djece — `SIGCHLD`

U poglavlju o procesima ([P04](../P04-Okruzenje_procesa/README.md)) susreli smo se s pojmom **zombi procesa**: proces koji je završio izvršavanje, ali čiji zapis u tablici procesa jezgra još uvijek čuva, jer roditelj nije pozvao `wait()` da pokupi izlazni status. Tamo smo zaključili da je dobra programerska praksa za svako dijete obavezno pozvati `wait()`. Sada se postavlja pitanje: kako to napraviti elegantno ako roditelj u međuvremenu treba raditi nešto drugo, a ne samo blokirati u `wait()`-u?

Odgovor leži u signalu `SIGCHLD`. Svaki put kad proces dijete promijeni stanje (završi izvršavanje, bude zaustavljeno signalom, ili bude nastavljeno), jezgra šalje roditelju signal `SIGCHLD`. Po defaultu se ovaj signal ignorira — što je razlog zašto smo dosad mogli "zaboraviti" na njega. Međutim, ako registriramo rukovatelj za `SIGCHLD`, dobivamo elegantan obrazac u kojem roditelj pokuplja djecu **asinkrono**, kad god ona završe, dok glavni program nesmetano nastavlja sa svojim radom.

Ovo je jedan od najčešćih obrazaca u stvarnom UNIX programiranju — koriste ga UNIX ljuske, web serveri (npr. Apache, nginx za radne procese), baze podataka i mnogi drugi sustavi koji upravljaju većim brojem podređenih procesa.

- [**`nozombie.c`**](nozombie.c) — primjer u kojem roditelj forka tri djeteta s različitim trajanjem, a sam u glavnoj petlji broji sekunde. Pokupljanje djece obavlja se u SIGCHLD rukovatelju, asinkrono u odnosu na glavnu petlju.

  ```c
  #include <stdio.h>
  #include <signal.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <string.h>

  void chld_handler(int signum) {
      int status;
      pid_t pid;

      pid = wait(&status);
      printf("[parent] dijete %d pokupljeno (status %d)\n",
             pid, WEXITSTATUS(status));
  }

  int main() {
      struct sigaction sa;
      int trajanje[] = {3, 1, 2};
      int i, sek;

      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = chld_handler;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = SA_RESTART;
      sigaction(SIGCHLD, &sa, NULL);

      for (i = 0; i < 3; i++) {
          pid_t pid = fork();
          if (pid == 0) {
              printf("[child %d] PID %d, spavam %d s\n",
                     i+1, getpid(), trajanje[i]);
              sleep(trajanje[i]);
              return i+1;
          }
      }

      for (sek = 1; sek <= 5; sek++) {
          sleep(1);
          printf("[parent] sekunda %d\n", sek);
      }

      return 0;
  }
  ```

  Roditelj registrira rukovatelj `chld_handler` za signal `SIGCHLD`, a zatim u petlji forka tri djeteta. Svako dijete spava drukčiji broj sekundi (3, 1 ili 2) i pri završetku vraća svoj redni broj kao izlazni status. Glavna petlja roditelja jednostavno broji sekunde — pet puta odspava sekundu i ispiše broj. Dok roditelj broji, djeca jedno po jedno završavaju; svaki put kad proces dijete završi, jezgra roditelju isporuči `SIGCHLD`, rukovatelj se izvrši i pozove `wait(&status)` da pokupi gotovo dijete.

  Pokrenimo program:

  ```
  $ ./nozombie
  [child 2] PID 40, spavam 1 s
  [child 3] PID 41, spavam 2 s
  [child 1] PID 39, spavam 3 s
  [parent] sekunda 1
  [parent] dijete 40 pokupljeno (status 2)
  [parent] sekunda 2
  [parent] dijete 41 pokupljeno (status 3)
  [parent] sekunda 3
  [parent] dijete 39 pokupljeno (status 1)
  [parent] sekunda 4
  [parent] sekunda 5
  ```

  Iz ispisa se vidi vremenski tijek: nakon prve sekunde završava dijete 2 (spavalo je 1 sekundu), pa rukovatelj odmah javlja da je pokupljeno. Nakon druge sekunde isto se dogodi za dijete 3, a nakon treće za dijete 1. Glavni program ovo ne primjećuje — uredno nastavlja brojati sekunde do pet.

  **Zašto `SA_RESTART`?** Glavni program u svojoj petlji koristi `sleep(1)`. Kad SIGCHLD stigne tijekom `sleep`-a, jezgra prekida sistemski poziv i poziva rukovatelj. Po defaultu, kad se rukovatelj vrati, prekinuti sistemski poziv vraća grešku s `errno = EINTR`. Za `sleep` to znači: vraća se prije isteka tražene sekunde — brojač sekundi bio bi neispravan. Postavljanjem zastavice `SA_RESTART` u `sa_flags` jezgri kažemo: *"nakon obrade signala automatski nastavi prekinuti sistemski poziv"*. Tako naš `sleep` spava punu sekundu čak i ako tijekom toga stigne SIGCHLD.

  **Napomena o `printf`-u u rukovatelju.** Pažljivi čitatelj zapazit će da naš `chld_handler` poziva `printf` — što je u suprotnosti s preporukom koju smo uveli na samom početku poglavlja: u rukovatelju treba pozivati samo `async-signal-safe` funkcije, a `printf` to nije. U ovom primjeru smo se ipak odlučili za `printf` radi jasnoće — svrha je pokazati u kojem se trenutku rukovatelj zaista izvrši i koje dijete pokuplja, što ispis čini ključnim za razumijevanje primjera. U produkcijskom kodu ovo nije prihvatljivo: rukovatelj bi tipično samo postavio zastavicu ili upisao podatke u red iz kojeg ih glavni program kasnije izvuče i ispiše. Imajte to na umu kad ovaj uzorak budete prilagođavali za vlastite programe.

  **Pokupljanje više djece odjednom.** U ovom primjeru djeca završavaju jedno po jedno, s razmakom od jedne sekunde, pa svaka instanca SIGCHLD-a dovodi do pokupljanja točno jednog djeteta. Međutim, ako bi više djece završilo gotovo istovremeno, mogla bi se dogoditi situacija u kojoj je rukovatelj pozvan jednom, a u međuvremenu su dvije ili više djece spremne za pokupljanje. Razlog leži u tome što su signali "klasične" UNIX vrste — više instanci istog signala koje stignu blizu jedne drugoj spojaju se u jednu isporuku (kao što smo vidjeli kod blokiranja). U produkcijskom kodu rukovatelj zato obično poziva `waitpid(-1, &status, WNOHANG)` u petlji, sve dok funkcija ne vrati 0 ili −1, čime se garantira da su sva spremna djeca pokupljena. U ovom uvodnom primjeru nismo se bavili tom mogućnošću jer nam vremenski razmak između djece to ne nalaže.

  **Glavna pouka.** Roditelj nije ni jednom eksplicitno pozvao `wait()` u svojoj glavnoj petlji — sva djeca su uredno pokupljena. Petlja roditelja bavi se isključivo svojim "korisnim" poslom (brojanjem sekundi), a sustav za pokupljanje radi neovisno, kroz signale. Time se izbjegava i potreba za stalnim provjerama "je li završilo neko dijete", i opasnost da pokupljanje propustimo (što bi dovelo do gomilanja zombija).

## Prevođenje

Direktorij dolazi s priloženim [`Makefile`](Makefile)-om koji prati iste konvencije kao i Makefile datoteke u prethodnim poglavljima (varijable `CC`, `CFLAGS`, `LDFLAGS`, `TARGETS`; implicitno pravilo `.c.o`; pravila `default`, `all`, `clean`). Detaljan opis strukture i korake gradnje Makefilea vidjeti u [`../P02-Osnove_programiranja/README.md`](../P02-Osnove_programiranja/README.md).

Tipična uporaba:

```sh
make              # gradi zadani cilj (potvrdi)
make all          # gradi sve primjere
make stoperica    # gradi samo zadani primjer
make clean        # briše izvršne i objektne datoteke
```
