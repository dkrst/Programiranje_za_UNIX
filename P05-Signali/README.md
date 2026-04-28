# Signali

Primjeri uz poglavlje **Signali** iz knjige *Programiranje za UNIX*.

U ovom poglavlju upoznat ćemo **signale** — UNIX-ov primarni mehanizam za asinkronu komunikaciju s procesom. Signal je kratka poruka koju jezgra šalje procesu kao obavijest da se dogodio neki događaj: korisnik je pritisnuo Ctrl+C, proces je pokušao pristupiti nedopuštenoj memorijskoj adresi, istekao je timer, neki drugi proces je eksplicitno zatražio prekid, i tako dalje. Iz perspektive procesa, signal može stići u **bilo kojem trenutku** između dvije strojne instrukcije — proces nema mogućnost predvidjeti kada će se to dogoditi.

Proces na primljeni signal može reagirati na nekoliko načina: prepustiti zadanu reakciju jezgri (što za većinu signala znači prekid procesa), eksplicitno ignorirati signal, ili registrirati vlastitu funkciju — **rukovatelj signala** (engl. *signal handler*) — koja će se izvršiti svaki put kad takav signal stigne. U ovom poglavlju kroz nekoliko primjera ilustriramo kako se signali registriraju, kako se hvataju, kako se koriste za komunikaciju među procesima i koje je opasnosti potrebno izbjegavati pri pisanju rukovatelja.

## Najčešći signali

UNIX definira tridesetak signala, ali u praksi se najčešće susrećemo sa skupinom njih desetak. Tablica niže daje pregled onih s kojima ćemo raditi u ovom poglavlju i u tipičnim programima:

| Signal | Broj | Predefinirana akcija | Značenje |
|---|---|---|---|
| `SIGHUP`  |  1 | prekid procesa  | terminal je zatvoren ili je sesija prekinuta; često se koristi i kao "ponovno učitaj konfiguraciju" |
| `SIGINT`  |  2 | prekid procesa  | korisnik je pritisnuo Ctrl+C u terminalu |
| `SIGQUIT` |  3 | prekid + core   | korisnik je pritisnuo Ctrl+\ — kao SIGINT, ali generira i memory dump |
| `SIGILL`  |  4 | prekid + core   | proces je pokušao izvršiti nevažeću strojnu instrukciju |
| `SIGABRT` |  6 | prekid + core   | proces je sam sebe prekinuo pozivom `abort()` (npr. zbog `assert` greške) |
| `SIGFPE`  |  8 | prekid + core   | aritmetička greška (dijeljenje s nulom, prelijevanje) |
| `SIGKILL` |  9 | prekid procesa  | bezuvjetni prekid — **ne može se uhvatiti niti ignorirati** |
| `SIGSEGV` | 11 | prekid + core   | proces je pristupio nevažećoj memorijskoj adresi (*segmentation fault*) |
| `SIGPIPE` | 13 | prekid procesa  | proces je pisao u cijev kojoj je drugi kraj zatvoren |
| `SIGALRM` | 14 | prekid procesa  | istekao je timer postavljen pozivom `alarm()` |
| `SIGTERM` | 15 | prekid procesa  | uljudni zahtjev za prekid; pošiljatelj (npr. `kill <pid>`) može ga uhvatiti i obraditi |
| `SIGUSR1` | 10 | prekid procesa  | korisnički signal br. 1 — bez unaprijed definirane semantike, slobodan za vlastite svrhe |
| `SIGUSR2` | 12 | prekid procesa  | korisnički signal br. 2 — bez unaprijed definirane semantike, slobodan za vlastite svrhe |
| `SIGCHLD` | 17 | ignorira se     | dijete procesa je promijenilo stanje (završilo, zaustavljeno itd.) |
| `SIGSTOP` | 19 | zaustavi proces | bezuvjetno zaustavljanje — **ne može se uhvatiti niti ignorirati** |
| `SIGCONT` | 18 | nastavi proces  | nastavi izvršavanje zaustavljenog procesa |
| `SIGTSTP` | 20 | zaustavi proces | korisnik je pritisnuo Ctrl+Z u terminalu |
| `SIGXCPU` | 24 | prekid + core   | premašen je CPU limit postavljen `setrlimit()`-om (vidi P04) |
| `SIGXFSZ` | 25 | prekid + core   | premašen je file size limit postavljen `setrlimit()`-om |

Brojevi signala u tablici odgovaraju Linuxu na arhitekturi x86 — na drugim sustavima i arhitekturama mogu se razlikovati. U kodu se uvijek koriste simbolička imena (`SIGINT`, `SIGTERM`...), a brojevi se navode samo radi reference (npr. uz ispis "Prekid signalom, signal: 11" iz `pokreni2`-a). Potpuni popis dostupan je u priručniku: `man 7 signal`.

Posebnu pažnju zaslužuju **`SIGKILL`** i **`SIGSTOP`** — to su jedina dva signala koja se ne mogu uhvatiti niti ignorirati. Razlog je praktičan: bez ova dva signala sustav ne bi imao apsolutni mehanizam za bezuvjetan prekid ili zaustavljanje "neposlušnog" procesa. Svaki drugi signal proces može uhvatiti i odlučiti kako reagirati — uključujući i `SIGTERM`, što neki programi iskorištavaju za uredno spremanje stanja prije izlaska.

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

### Signali kao međuprocesna komunikacija

U dosadašnjim primjerima signali su dolazili iz dva izvora: izvana, od korisnika preko Ctrl+C, ili iznutra, kad ih je proces sam sebi zakazao pozivom `alarm()`. UNIX, međutim, dopušta i da jedan proces eksplicitno **pošalje signal drugom procesu** — što čini signale jednim od najjednostavnijih oblika međuprocesne komunikacije (engl. *Inter-Process Communication*, IPC). Za to služi sistemski poziv `kill`:

```c
#include <signal.h>
#include <sys/types.h>

int kill(pid_t pid, int sig);
```

**Povratna vrijednost:** `0` u slučaju uspjeha; `-1` u slučaju greške (npr. ne postoji proces s navedenim PID-om, ili nemamo dovoljnu razinu ovlasti za slanje signala tom procesu).

Ime `kill` je povijesno — prvotno je sistemski poziv služio isključivo za prekid drugog procesa pomoću `SIGKILL` ili `SIGTERM`. Vremenom se njegova uloga proširila i danas se koristi za slanje **bilo kojeg** signala, ali ime je ostalo. Istog imena je i istoimena ugrađena naredba ljuske (`kill -<sig> <pid>`, npr. `kill -USR1 12345`), kojom korisnik može poslati proizvoljni signal nekom procesu iz terminala.

Većina UNIX signala ima predefinirano značenje. Međutim, kako programer može napisati vlastiti rukovatelj za većinu signala, time praktički može promijeniti njihovo predefinirano značenje — npr. iskoristiti `SIGSEGV` (kojim nas jezgra upozorava da smo s pokazivačem izašli izvan svog memorijskog prostora) za razmjenu poruka između roditelja i djeteta, kao u našem primjeru. Iako ovo nije nikakav problem implementirati, ideja je vrlo loša: što ako stvarno u programu napravimo grešku u rukovanju memorijom i jezgra nas pokuša na to upozoriti, a mi taj signal protumačimo kao poruku od drugog procesa?

Ideja da signalima pridijelimo posve drugačije značenje otprilike je jednako dobra kao da se studenti koji slušaju kolegij *Programiranje za UNIX* na FESB-u dogovore da crveno svjetlo na semaforu za njih znači "kreni", a zeleno "stani": dok su sami na cesti, sustav može funkcionirati, ali ako se pojavi bilo koji drugi vozač, posljedice su potencijalno katastrofalne.

Iz istog razloga treba poštivati predefinirana značenja signala, a za vlastite komunikacijske protokole između grupe procesa koristiti **slobodne signale `SIGUSR1` i `SIGUSR2`**, kao u našem primjeru. Sustav im ne pridružuje nikakvo standardno ponašanje — programer ih može slobodno koristiti za vlastite svrhe (npr. kao "ping" obavijest, signal za reload konfiguracije, ili sinkronizacijski mehanizam).

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

  Struktura programa razdvaja se odmah nakon `fork()`-a u dvije grane. **Dijete** registrira dva rukovatelja: `usr1_handler` koji broji primljene `SIGUSR1` signale, i `term_handler` koji postavlja `radi = 0` čime signalizira glavnoj petlji da treba završiti. Petlja je standardna `while (radi) pause()` konstrukcija s provjerom `if (radi)` prije `printf`-a, kao u `stoperica.c`-u, da se izbjegne suvišan ispis nakon `SIGTERM`-a. **Roditelj** zna PID djeteta jer mu ga je `fork()` vratio, pa može pozivati `kill(pid, SIGUSR1)` da mu šalje signale u željenim trenucima. Po završetku radne petlje šalje `SIGTERM`, a zatim `wait(NULL)` čeka da dijete uredno završi (čime se ono prestaje voditi kao zombi proces u tablici procesa, kao što smo objasnili u prethodnom poglavlju).

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

## Prevođenje

Direktorij dolazi s priloženim [`Makefile`](Makefile)-om koji prati iste konvencije kao i Makefile datoteke u prethodnim poglavljima (varijable `CC`, `CFLAGS`, `LDFLAGS`, `TARGETS`; implicitno pravilo `.c.o`; pravila `default`, `all`, `clean`). Detaljan opis strukture i korake gradnje Makefilea vidjeti u [`../P02-Osnove_programiranja/README.md`](../P02-Osnove_programiranja/README.md).

Tipična uporaba:

```sh
make              # gradi zadani cilj (potvrdi)
make all          # gradi sve primjere
make stoperica    # gradi samo zadani primjer
make clean        # briše izvršne i objektne datoteke
```
