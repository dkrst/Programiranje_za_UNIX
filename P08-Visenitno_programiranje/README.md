# Višenitno programiranje

Stvaranje novog procesa sistemskim pozivom `fork` moćan je mehanizam koji nam daje mogućnost da u naše programe ugradimo paralelno izvršavanje. Međutim, stvaranje novog procesa ujedno je i skup mehanizam: podrazumijeva kopiranje cijelog adresnog prostora postojećeg procesa, dodjelu identifikatora i ažuriranje struktura u jezgri koje čuvaju informacije o aktivnim procesima. Pored toga, komunikacija među procesima zahtijeva eksplicitne mehanizme — cjevovode, signale, dijeljenu memoriju, redove poruka, semafore i tako dalje.

U ovom poglavlju upoznajemo **niti** (engl. *thread*) — lakšu jedinicu izvršavanja koja postoji *unutar* procesa. U literaturi na hrvatskom jeziku ponekad se umjesto naziva "nit" koristi i termin "dretva"; oba naziva označavaju isto. U engleskoj literaturi niti se često nazivaju i *lightweight processes* — "lagani procesi" — što dobro opisuje njihovu prirodu: imaju mnoge mogućnosti kao i procesi (paralelno izvršavanje, vlastiti tok upravljanja), ali bez troška kopiranja cijelog adresnog prostora i bez potrebe za posebnim IPC mehanizmima. Niti su odgovor na pitanje: kako iskoristiti više jezgri suvremenih procesora za ubrzanje računski zahtjevnih programa, a istovremeno omogućiti da dijelovi programa međusobno jednostavno dijele podatke?


## Niti i procesi

Da bismo razumjeli odnos između niti i procesa, prisjetimo se što je proces. Proces je instanca programa u izvršavanju — apstrakcija koju operacijski sustav drži za svaku aktivnu izvršnu jedinicu. Svaki proces ima svoj jedinstveni identifikator (`PID`), vlastiti adresni prostor (memorija u kojoj su smješteni programski kod, statički podaci, hrpa i stog), vlastite deskriptore otvorenih datoteka, vlastitu tablicu rukovatelja signala, vlasnika i grupu, radni direktorij, varijable okruženja i niz drugih atributa. Svaki proces, kad ga jezgra raspoređuje na procesor, ima jedan tok izvršavanja — slijed instrukcija koji jezgra prati kroz brojač instrukcija (engl. *program counter*, PC) i sadržaj procesorskih registara.

Nit je upravo to — tok izvršavanja. Pri tom proces možemo promatrati kao "spremnik" resursa (adresni prostor, strojni kod programa, deskriptori otvorenih datoteka, ...), a niti su jedinice izvršavanja koje unutar tog spremnika žive. U svakom procesu imamo najmanje jednu nit — jedan tok izvršavanja, ali ih može biti i više, pri čemu sve niti dijele iste resurse i izvršavaju se unutar istog adresnog prostora.

Upravo ovo ključna je razlika između procesa i niti: procesi su međusobno izolirani, dok niti unutar istog procesa dijele zajedničke resurse.

| Resurs | Vlasništvo |
|---|---|
| PID (identifikator procesa) | zajednički za sve niti procesa |
| Programski kod (text segment) | dijele sve niti procesa |
| Globalne i statičke varijable | dijele sve niti procesa |
| Hrpa (memorija dobivena `malloc`-om) | dijele sve niti procesa |
| Otvoreni deskriptori datoteka | dijele sve niti procesa |
| Trenutni radni direktorij, `umask`, vlasnik | dijele sve niti procesa |
| Tablica rukovatelja signala | dijele sve niti procesa |
| **Stog (engl. *stack*)** | **vlastiti za svaku nit** |
| **Brojač instrukcija i procesorski registri** | **vlastiti za svaku nit** |
| **Identifikator niti** (`pthread_t`) | **jedinstven za svaku nit** |
| **Lokalna pohrana niti** (TLS) | **vlastita za svaku nit** |
| **Maska blokiranih signala** | **vlastita za svaku nit** |

Vlastiti stog svake niti je ključ za razumijevanje. Kad nit poziva funkciju, njezini argumenti, lokalne varijable i adresa povratka iz funkcije idu na njen stog — ne na neki "zajednički" stog procesa. Zato dvije niti mogu istovremeno biti unutar iste funkcije, svaka sa svojim privatnim lokalnim varijablama, bez ikakve interferencije. Globalne varijable, hrpa i ostale "zajedničke" strukture su mjesto gdje niti komuniciraju — i upravo zato im je potrebna sinkronizacija, ključna stavka za razumijevanje i upravljanje nitima u višenitnom procesu.

## Raspoređivač (scheduler) i niti

Promislimo malo o načinu na koji raspoređivač upravlja procesima: kada na sustavu imamo više procesa nego resursa (procesorskih jezgri), raspoređivač upravlja procesima na način da ih izvršava u dijeljenom vremenu (engl. *time sharing*) — svaki proces dobiva na određeno vrijeme potrebne resurse, nakon kojeg ih prepušta drugim procesima dok čeka svoj red da nastavi izvršavanje. Međutim, što ako proces ima više tokova izvršavanja, tj. više niti: što je jedinica koju raspoređivač zapravo raspoređuje — proces ili nit?

Na suvremenom Linuxu (kao i na većini modernih UNIX sustava) odgovor je nit: raspoređivač vidi niti kao osnovne jedinice izvršavanja i svakoj niti dodjeljuje procesorsko vrijeme neovisno o drugima. Razmislimo o značenju ovog podatka: ukoliko unutar svog procesa koristimo više niti, vjerojatno ćemo dobiti više procesorskog vremena u odnosu na programe koji imaju samo jedan tok izvršavanja — samo jednu nit. Još važnije: ako raspoređivač svaku nit promatra nezavisno, dobra je šansa da će se različite niti unutar našeg procesa izvršavati nezavisno na različitim procesorskim jezgrama (gotovo sva moderna računala imaju više procesorskih jezgri). Ovo osigurava istinski paralelizam, ali otvara i mogućnost da dvije (ili više) niti koje se istovremeno izvršavaju pristupe istom podatku u zajedničkoj memoriji — što je idealan scenarij za stanje trke (race condition), koji smo već spominjali u ranijim poglavljima skripte.

Što to znači za pisanje programa? Dva su važna zaključka. Prvo, **niti se izvršavaju paralelno**, ne sekvencijalno — kad pokrenemo deset niti, njihov međusobni redoslijed izvršavanja je nepredvidiv i može se mijenjati od pokretanja do pokretanja. Drugo, **raspoređivač može prekinuti bilo koju nit u bilo kojem trenutku** — i ne samo između "logičkih" instrukcija C koda nego doslovno između bilo koje dvije strojne instrukcije, što smo najbolje vidjeli na primjeru inkrementiranja cjelobrojne varijable (`i++`). To nas vodi u temu race conditiona, koju ćemo detaljno razraditi u nastavku.

> **Povijesna napomena.** Današnji 1:1 model upravljanja nitima (svaka nit u korisničkom prostoru odgovara jednoj niti jezgre) nije oduvijek bio jedini pristup. Postojali su sustavi gdje su sve niti procesa dijelile jednu "kvotu" procesorskog vremena — tzv. *M:1* model ili *user-level threads*. Tu je raspoređivač na razini jezgre vidio samo proces, a raspoređivanje među nitima radila je biblioteka u korisničkom prostoru. Prednost takvog pristupa bila je u brzini stvaranja niti i prebacivanja konteksta među njima (sve se događalo u korisničkom prostoru, bez sistemskih poziva), ali uz veliki nedostatak: cijeli proces nije mogao iskoristiti više procesorskih jezgri jer je jezgri u stvari bio jedan tok izvršavanja. Postojali su i tzv. *hibridni M:N modeli* koji su pokušavali kombinirati prednosti oba pristupa. Današnji Linux koristi isključivo 1:1 model, čime je razvoj višenitnih programa znatno pojednostavljen — sve odluke o raspoređivanju donosi jezgra, a program ne mora razmišljati o tome u kakvom je odnosu njegova "logička" nit prema niti jezgre.

## POSIX niti — pthreads

POSIX standard definira sučelje za rad s nitima koje se naziva **POSIX threads** ili kraće **pthreads**. Sve funkcije ovog API-ja imaju prefiks `pthread_` i deklarirane su u zaglavlju `<pthread.h>`. Implementacija pthreads-a na Linuxu naziva se **NPTL** (*Native POSIX Threads Library*) i dolazi kao dio biblioteke `libc`. Definitivna referenca za pthreads je *Programming with POSIX Threads*, Butenhof [1] — knjiga koja sustavno obrađuje svaki aspekt biblioteke, od osnova do graničnih slučajeva. Niti su također dobro pokrivene u *Advanced Programming in the UNIX Environment*, Stevens & Rago [2] (poglavlja 11 i 12), a šire koncepte niti kao osnovne apstrakcije operacijskog sustava (lightweight procesi, raspoređivanje niti, sinkronizacija) na hrvatskom jeziku obrađuje *Operacijski sustavi*, Budin, Golub, Jakobović & Jelenković [3].

Za prevođenje programa koji koriste pthreads potrebno je linkanje s pthread bibliotekom:

```sh
gcc program.c -lpthread -o program
```

Neke distribucije i neki kompajleri preferiraju oblik `-pthread` (s crticom, bez `l`), koji uz linkanje s bibliotekom postavlja i potrebne predprocesorske makroe (`_REENTRANT` ili `_POSIX_C_SOURCE`). Oba oblika funkcioniraju za naše primjere.


## Stvaranje i terminiranje niti

Tri osnovne funkcije za rad s nitima su `pthread_create` (stvaranje niti), `pthread_exit` (eksplicitno terminiranje niti uz povratnu vrijednost) i `pthread_join` (čekanje da nit završi i dohvaćanje njene povratne vrijednosti). Sve tri deklarirane su u zaglavlju `<pthread.h>`. Sintaksu i način korištenja obradit ćemo u dvije cjeline: prvo stvaranje, a zatim par koji upravlja životnim ciklusom — terminiranje i prikupljanje rezultata.

### Stvaranje niti — `pthread_create`

```c
#include <pthread.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
```

**Povratna vrijednost:** `0` u slučaju uspjeha. U slučaju greške *ne postavlja se* `errno` nego se vraća sam kod greške (npr. `EAGAIN` ako je sustav iscrpio resurse za stvaranje nove niti). Ovo je razlika u odnosu na uobičajeni UNIX stil i izvor čestih grešaka — provjera s `perror(...)` neće funkcionirati izravno na povratnoj vrijednosti pthreads funkcija; za ispis poruke o grešci treba koristiti `strerror(rezultat)`, gdje je `rezultat` povratna vrijednost funkcije `pthread_create`.

**Argumenti:**

- **`thread`** — pokazivač na varijablu tipa `pthread_t` u koju će se upisati identifikator nove niti.
- **`attr`** — atributi niti (veličina stoga, je li joinable ili detached, ...); ako predamo `NULL`, koriste se zadane vrijednosti.
- **`start_routine`** — pokazivač na funkciju koja će biti polazna točka nove niti; mora imati potpis `void *(*)(void *)`, tj. prima jedan generički pokazivač kao argument i vraća jedan generički pokazivač kao rezultat.
- **`arg`** — pokazivač na argumente koji će biti predani polaznoj funkciji nove niti.

Nova nit počinje izvršavanje pozivom `start_routine(arg)`.

### Terminiranje i prikupljanje rezultata niti — `pthread_exit` i `pthread_join`

Ove dvije funkcije čine logički par: jedna završava nit i ostavlja "iza sebe" povratnu vrijednost, druga čeka da nit završi i tu vrijednost pokupi.

```c
#include <pthread.h>

void pthread_exit(void *retval);
int  pthread_join(pthread_t thread, void **retval);
```

Nit može završiti na jedan od sljedećih načina:

- vraćanjem iz polazne funkcije (`return` s vrijednošću koja postaje povratna vrijednost niti);
- eksplicitnim pozivom `pthread_exit(retval)`;
- na zahtjev druge niti (`pthread_cancel`, vidi niže);
- ili završetkom cijelog procesa (npr. `exit` ili `return` iz `main`-a, što ubije sve niti, ili prekidom zbog signala).

`pthread_exit` završava pozivajuću nit i nikad se ne vraća, pa joj je tip `void`. Resursi pridruženi niti (struktura u jezgri, stog niti, ...) ostaju u sustavu dok ih netko ne pokupi pozivom `pthread_join`. Analogija s procesima je gotovo izravna: `pthread_exit` je za nit ono što je `exit` za proces, a `pthread_join` je za nit ono što je `wait` za proces (P05).

**Argument** za `pthread_exit`:

- **`retval`** — pokazivač koji postaje povratna vrijednost niti. Valja voditi računa da pokazivač koji proslijedimo funkciji `pthread_exit` ne pokazuje na varijablu na stogu! Svaka nit ima vlastiti stog, koji nestaje kada nit završi pa pokazivač na bilo koju lokalnu varijablu nije validan. Ovo je uobičajena greška, a umjesto pokazivača na lokalnu varijablu, nit tipično vraća pokazivač na memoriju alociranu na hrpi (engl. *heap*), pozivom `malloc`.

**Povratna vrijednost** za `pthread_join`: `0` u slučaju uspjeha, kod greške inače (ista konvencija kao kod `pthread_create`).

**Argumenti** za `pthread_join`:

- **`thread`** — identifikator niti koju čekamo (vrijednost koju nam je `pthread_create` ranije upisao u `pthread_t`).
- **`retval`** — pokazivač na pokazivač u koji `pthread_join` upisuje povratnu vrijednost niti (onu koju je nit predala `pthread_exit`-u, ili koju je vratila kroz `return` iz polazne funkcije). Ako nas povratna vrijednost ne zanima, možemo predati `NULL`. Razlog zašto je argument *pokazivač na pokazivač* je taj što funkcija mora u našu varijablu upisati adresu koja je predana funkciji `pthread_exit`. Stoga moramo proslijediti pokazivač na pokazivač — tj. pokazivač na varijablu tipa pokazivač, u koju će nova adresa biti upisana.

`pthread_join` blokira pozivajuću nit dok zadana nit ne završi. Nakon uspješnog poziva, sustav oslobađa sve resurse vezane za tu nit.

### Otkazivanje niti — `pthread_cancel`

Nit može poslati drugoj niti zahtjev za prekid izvršavanja pozivom `pthread_cancel`:

```c
#include <pthread.h>

int pthread_cancel(pthread_t thread);
```

Bitno je naglasiti da `pthread_cancel` nije bezuvjetna naredba — ona šalje **zahtjev** za otkazivanje, a hoće li i kad ciljana nit zaista završiti ovisi o njenom stanju otkaznosti i o tome dolazi li do tzv. *cancellation pointa* (poziva neke od funkcija koje POSIX definira kao mjesta na kojima se zahtjev za otkazivanje obrađuje, npr. `sleep`, `read`, `pthread_cond_wait`). Po defaultu su niti otkazne i obrada zahtjeva događa se na prvom cancellation pointu. Otkazivanje niti je relativno složena tema o kojoj nećemo dublje govoriti — koristit ćemo je samo u jednom primjeru kasnije, gdje glavna nit prekida pozadinske radnike koji izvode beskonačnu petlju.

### Primjer: prva nit

- [**`nit_pozdrav.c`**](nit_pozdrav.c) — najjednostavniji mogući primjer. Glavna nit stvara jednu pomoćnu nit koja ispiše poruku i odmah završi.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <pthread.h>

  void *pozdrav(void *arg) {
      (void)arg;
      printf("Pozdrav iz niti!\n");
      return NULL;
  }

  int main(void) {
      pthread_t nit;

      if (pthread_create(&nit, NULL, pozdrav, NULL) != 0) {
          perror("pthread_create");
          return 1;
      }

      pthread_join(nit, NULL);

      printf("Glavna nit zavrsava.\n");
      return 0;
  }
  ```

  Ispis:

  ```
  $ ./nit_pozdrav
  Pozdrav iz niti!
  Glavna nit zavrsava.
  ```

  Da `pthread_join` nije bio pozvan, glavna nit bi mogla završiti prije nego što pomoćna stigne ispisati svoju poruku. Kad glavna nit (ona koja izvršava `main`) izađe pozivom `return` iz `main`-a, cijeli proces se gasi — uključujući sve preostale niti, neovisno o tome jesu li završile svoj posao. Ako nas to ne smeta (npr. pomoćne niti rade dijagnostiku koja smije nestati u trenutku izlaska), `pthread_join` možemo izostaviti. Inače je obavezan.

### Primjer: povratna vrijednost niti

- [**`nit_join.c`**](nit_join.c) — pomoćna nit računa kvadrat broja i vraća rezultat glavnoj.

  ```c
  void *kvadrat(void *arg) {
      int x = *(int *)arg;
      int *rezultat = malloc(sizeof(int));
      if (rezultat == NULL) return NULL;
      *rezultat = x * x;
      return rezultat;
  }

  int main(void) {
      pthread_t nit;
      int broj = 7;
      void *povratna_vrijednost;

      if (pthread_create(&nit, NULL, kvadrat, &broj) != 0) {
          perror("pthread_create");
          return 1;
      }

      pthread_join(nit, &povratna_vrijednost);

      int *rezultat = (int *)povratna_vrijednost;
      printf("%d^2 = %d\n", broj, *rezultat);
      free(rezultat);
      return 0;
  }
  ```

  Bitno je primijetiti da rezultat alociramo s `malloc`-om, a ne kao lokalnu varijablu unutar niti. Razlog je u sljedećem primjeru — lokalne varijable žive na stogu niti, a stog nestaje kad nit završi.

### Primjer: opasnost stoga

- [**`nit_stog.c`**](nit_stog.c) — demonstracija česte pogreške. Nit vraća pokazivač na svoju **lokalnu varijablu**, koja se nalazi na njenom stogu. Kad se nit terminira, njen stog se oslobađa, pa pristup kroz taj pokazivač iz druge niti čita nevažeću memoriju — *undefined behavior*.

  ```c
  void *lose(void *arg) {
      (void)arg;
      int lokalna = 42;     /* na stogu ove niti! */
      return &lokalna;      /* OPASNO: stog ce nestati */
  }
  ```

  Već nas i kompajler upozorava (`warning: function returns address of local variable`), što je signal da je nešto fundamentalno krivo. Na ovom konkretnom programu većina sustava pokazat će **segmentacijsku grešku** pri pristupu — što je sretan ishod, jer nas operacijski sustav glasno upozorava da nešto nije u redu. Mnogo opasniji slučajevi su oni gdje program *naizgled radi*, ali povremeno vraća pogrešne podatke jer memoriju koju je nekad zauzimao stog niti u međuvremenu zauzme nešto drugo. Pravilno rješenje vidjeli smo u prethodnom primjeru — alocirati memoriju na hrpi (`malloc`), koja preživi terminiranje niti.

### Primjer: više niti s različitim argumentima

- [**`nit_args.c`**](nit_args.c) — stvaramo pet niti odjednom, svakoj predajemo njezin redni broj.

  ```c
  #define BROJ_NITI 5

  void *radnik(void *arg) {
      int id = *(int *)arg;
      printf("Nit %d pocinje rad\n", id);
      sleep(1);
      printf("Nit %d zavrsila\n", id);
      return NULL;
  }

  int main(void) {
      pthread_t niti[BROJ_NITI];
      int       podaci[BROJ_NITI];

      for (int i = 0; i < BROJ_NITI; i++) {
          podaci[i] = i;
          pthread_create(&niti[i], NULL, radnik, &podaci[i]);
      }

      for (int i = 0; i < BROJ_NITI; i++)
          pthread_join(niti[i], NULL);

      return 0;
  }
  ```

  Pažnja na suptilnu zamku: svakoj niti predajemo pokazivač na **zaseban** element polja `podaci[]`, a ne pokazivač na varijablu petlje `i`. Da smo predali `&i`, sve niti bi gledale u istu varijablu, čija se vrijednost u međuvremenu mijenja — niti bi mogle pročitati `i` "kasno", kad smo već povećali brojač. To je tipičan obrazac početničke pogreške s nitima.

  Ispis pokazuje važnu osobinu: **redoslijed izvršavanja niti nije unaprijed određen**. U jednom pokretanju dobivamo:

  ```
  $ ./nit_args
  Nit 0 pocinje rad
  Nit 1 pocinje rad
  Nit 2 pocinje rad
  Nit 3 pocinje rad
  Nit 4 pocinje rad
  Nit 0 zavrsila
  Nit 4 zavrsila
  Nit 3 zavrsila
  Nit 2 zavrsila
  Nit 1 zavrsila
  Sve niti su zavrsile.
  ```

  Niti su završile u drugačijem redoslijedu nego što su počele — što je tipično ponašanje. Sljedeće pokretanje moglo bi dati drugačiji raspored.

## Odnos među nitima — nema "glavne"

Kad smo govorili o procesima u P05, postojao je jasan odnos roditelj-dijete: proces `A` stvori dijete `B` pozivom `fork`, i samo `A` može pokupiti zombi `B`-a preko `wait`. Roditelj i dijete nisu simetrični.

Kod niti, situacija je drugačija. **Sve niti unutar procesa su međusobno ravnopravne** — ne postoji formalni hijerarhijski odnos. Bilo koja nit može pozvati `pthread_join(nit_X, ...)` da pokupi rezultat bilo koje druge niti, neovisno o tome koja je koju stvorila. Glavna nit (ona koja izvršava `main`) nije ničim posebno povezana s nitima koje je stvorila — može završiti prije njih, može biti joinana od strane neke pomoćne niti, ili može jednostavno pozvati `pthread_exit()` da zavrsi sama, ostavljajući druge niti da nastave. Jedina razlika je da povratak iz `main`-a gasi cijeli proces zbog konvencije C-a, a ne zbog "posebnosti" glavne niti.

Praktična posljedica: u pisanju višenitnih programa nema potrebe za centraliziranom strukturom gdje "neki nadređeni" čeka sve podređene. Niti se mogu organizirati u proizvoljne grafove ovisnosti — dvije pomoćne niti mogu čekati treću, jedna nit može joinati niz drugih, i tako dalje. Glavna nit je važna samo po konvenciji (jer je u njoj `main`) i jer njen povratak gasi proces.


## Joinable i detached niti

Po defaultu, niti su **joinable** — njihovi resursi (struktura koja čuva povratnu vrijednost niti, identifikator, ...) ostaju u sustavu sve dok ih netko ne pokupi pozivom `pthread_join`. Ovo je analogno zombi procesima iz P05: ako ne pokupimo nit, imamo curenje resursa.

Često, međutim, ne želimo čekati rezultat niti — pokrenemo neki pozadinski zadatak i prepustimo ga sustavu. Za to služe **detached niti**: nakon završetka, njihovi resursi se **automatski oslobađaju**, bez potrebe za eksplicitnim `join`-om. Detached nit se ne smije i ne može joinati — pokušaj je greška.

Nit možemo napraviti detached na dva načina:

1. **Pri stvaranju** — postavljanjem atributa `PTHREAD_CREATE_DETACHED` u `pthread_attr_t` strukturu koju predajemo `pthread_create`-u.
2. **Naknadno** — pozivom `pthread_detach(nit)` u bilo kojem trenutku nakon stvaranja.

- [**`nit_detached.c`**](nit_detached.c) — primjer detached niti kao "ispali i zaboravi" zadataka.

  ```c
  void *pozadinski_posao(void *arg) {
      int id = *(int *)arg;
      free(arg);
      printf("Detached nit %d: radim svoj posao...\n", id);
      sleep(2);
      printf("Detached nit %d: gotovo.\n", id);
      return NULL;
  }

  int main(void) {
      pthread_t      nit;
      pthread_attr_t atribut;

      pthread_attr_init(&atribut);
      pthread_attr_setdetachstate(&atribut, PTHREAD_CREATE_DETACHED);

      for (int i = 0; i < 3; i++) {
          int *id = malloc(sizeof(int));
          *id = i;
          pthread_create(&nit, &atribut, pozadinski_posao, id);
      }

      pthread_attr_destroy(&atribut);

      sleep(3);    /* daj nitima vremena da odrade posao */
      return 0;
  }
  ```

  Primijetite da ne pozivamo `pthread_join` ni za jednu nit — sustav će sam pospremiti njihove resurse kad završe. Moramo, međutim, dati nitima vremena da završe prije nego `main` izađe (čime se proces gasi i niti nasilno prekidaju). U pravim aplikacijama detached niti se često koriste u poslužiteljima: glavna nit prima dolazne zahtjeve i za svaki stvara detached nit koja obrađuje taj zahtjev, bez potrebe da glavna nit prati svakoga.

## Race condition

Vratimo se na problem iz P07. U sekciji o semaforima smo objasnili da operacija `brojac++` nije atomska — na strojnoj razini sastoji se od tri koraka: učitaj vrijednost u registar, povećaj registar, vrati u memoriju. Ako između koraka raspoređivač prebaci kontrolu na drugu nit (ili drugi proces), može doći do gubitka inkrementacija.

U P07 smo to demonstrirali na dva procesa koji preko dijeljene memorije pristupaju zajedničkom brojaču. Ovdje radimo isto, samo s nitima jednog procesa. Bitna razlika: kod niti **dijeljenje memorije dolazi automatski**, jer sve niti dijele cijeli adresni prostor svog procesa. Obična globalna varijabla je istovremeno vidljiva svim nitima, bez potrebe za `shm_open` i `mmap`.

- [**`nit_race.c`**](nit_race.c) — više niti istovremeno inkrementira zajedničku globalnu varijablu.

  ```c
  #define BROJ_NITI 4
  #define ITERACIJA 100000

  static long brojac = 0;

  void *radnik(void *arg) {
      (void)arg;
      for (int i = 0; i < ITERACIJA; i++) {
          long temp = brojac;       /* 1. ucitaj iz memorije */
          sched_yield();             /* pustimo drugu nit da nas pretekne */
          temp = temp + 1;           /* 2. povecaj */
          brojac = temp;             /* 3. vrati u memoriju */
      }
      return NULL;
  }
  ```

  Da bi race bio jasno vidljiv, eksplicitno razlažemo inkrementaciju u tri koraka i između prvog i posljednjeg ubacujemo `sched_yield()` koji raspoređivaču nudi mogućnost da promijeni aktivnu nit. Bez toga, sekvenca `mov / add / mov` na modernim procesorima izvršava se tako brzo da je teško uloviti race u realnoj demonstraciji — ali u stvarnim programima sa složenijim kritičnim sekcijama race se pojavljuje sasvim sam, i upravo zato je opasan.

  Rezultat:

  ```
  $ for i in 1 2 3 4 5; do ./nit_race; done
  Brojac = 100001  (ocekivano: 400000)
  Brojac = 100000  (ocekivano: 400000)
  Brojac = 100000  (ocekivano: 400000)
  Brojac = 100000  (ocekivano: 400000)
  Brojac = 100001  (ocekivano: 400000)
  ```

  Gotovo sve inkrementacije iz triju niti su izgubljene — krajnji rezultat je oko 100 000 umjesto 400 000. Ovo je gotovo isti race koji smo vidjeli u P07, samo unutar jednog procesa.

## Mutex

Rješenje race conditiona kod niti je **mutex** (engl. *mutual exclusion lock*). Konceptualno je identičan binarnom semaforu iz P07 — sinkronizacijska primitiva koja osigurava da samo jedna nit u danom trenutku može izvršavati zaštićeni dio koda (**kritičnu sekciju**). Razlika je u tome što je mutex optimiziran za niti unutar istog procesa — implementacija je u korisničkom prostoru kad nema sporova (samo atomski test-and-set u memoriji), pa je znatno brža od POSIX semafora.

```c
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;     /* staticka inicijalizacija */

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
```

Statički alocirani mutex (npr. globalna varijabla) inicijalizira se makro vrijednošću `PTHREAD_MUTEX_INITIALIZER`. Mutex alocirani dinamički (na hrpi) treba inicijalizirati pozivom `pthread_mutex_init` i kasnije osloboditi pozivom `pthread_mutex_destroy`.

Operacije:

- **`pthread_mutex_lock`** — pokušava zaključati mutex. Ako je već zaključan od strane druge niti, blokira pozivajuću nit dok mutex ne postane slobodan.
- **`pthread_mutex_unlock`** — otpušta mutex. Smije ga otpustiti samo nit koja ga drži (ovisno o tipu mutexa — postoji više varijanti, ali za naše potrebe zadani tip je dovoljan).

- [**`nit_mutex.c`**](nit_mutex.c) — rješenje race conditiona iz `nit_race.c` korištenjem mutexa.

  ```c
  static long              brojac = 0;
  static pthread_mutex_t  mutex  = PTHREAD_MUTEX_INITIALIZER;

  void *radnik(void *arg) {
      (void)arg;
      for (int i = 0; i < ITERACIJA; i++) {
          pthread_mutex_lock(&mutex);     /* udji u kriticnu sekciju */
          long temp = brojac;
          sched_yield();                   /* dok smo unutra, nitko drugi nece uci */
          temp = temp + 1;
          brojac = temp;
          pthread_mutex_unlock(&mutex);   /* izadji iz kriticne sekcije */
      }
      return NULL;
  }
  ```

  Struktura niti je gotovo identična kao u `nit_race.c` — ista razložena inkrementacija s `sched_yield`, samo sad sve unutar kritične sekcije omeđene s `lock`/`unlock`. Rezultat:

  ```
  $ for i in 1 2 3; do ./nit_mutex; done
  Brojac = 400000  (ocekivano: 400000)
  Brojac = 400000  (ocekivano: 400000)
  Brojac = 400000  (ocekivano: 400000)
  ```

  Uvijek točno. Cijena je značajan pad performansi (svaki ulazak/izlazak iz kritične sekcije ima trošak), ali u zamjenu dobivamo ispravan rezultat — kompromis koji u 99% slučajeva itekako vrijedi.

### Deadlock

Mutexi rješavaju jedan problem ali otvaraju drugi — **deadlock** (engl. *zaglavljenje*, *uzajamna blokada*). Klasičan scenarij: nit `A` drži mutex `M1` i pokušava zaključati mutex `M2`; istovremeno nit `B` drži `M2` i pokušava zaključati `M1`. Obje niti čekaju onu drugu — nijedna nikad neće završiti.

Najjednostavniji način izbjegavanja deadlocka je **konzistentan redoslijed zaključavanja**: ako svi dijelovi koda koji trebaju oba mutexa uvijek zaključavaju `M1` prije `M2`, deadlock je nemoguć. U većim programima ovo zahtjeva pažljiv dizajn. Za detaljniju obradu deadlockova i drugih sinkronizacijskih problema, čitatelj se može obratiti specijaliziranoj literaturi o paralelnom programiranju.

## Kondicijske varijable

Mutex rješava problem isključivog pristupa, ali postoji i druga klasa problema — **čekanje na uvjet**. Razmotrimo klasični problem **proizvođač-potrošač** (engl. *producer-consumer*): jedna nit proizvodi podatke i stavlja ih u ograničeni međuspremnik, druga ih vadi i obrađuje. Pitanja koja se postavljaju:

- Što kad je međuspremnik **prazan**? Potrošač mora čekati da netko nešto stavi.
- Što kad je međuspremnik **pun**? Proizvođač mora čekati da netko nešto izvadi.

Naivno rješenje bi bilo aktivno čekanje (engl. *busy wait*) u petlji — *"provjeravaj uvjet svaki put, dok ne bude istinit"*. Ali to troši procesorsko vrijeme bez stvarnog napretka. Trebamo mehanizam da nit "zaspi" dok netko drugi ne signalizira promjenu stanja.

Taj mehanizam je **kondicijska varijabla** (engl. *condition variable*). Nit može pozivom `pthread_cond_wait` zaspati cekajući signal, a druga nit pozivom `pthread_cond_signal` (ili `pthread_cond_broadcast`) može probuditi jednu (ili sve) niti koje čekaju.

```c
#include <pthread.h>

pthread_cond_t cv = PTHREAD_COND_INITIALIZER;     /* staticka inicijalizacija */

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);
```

Najvažnija funkcija je `pthread_cond_wait`. Ona prima dva argumenta — kondicijsku varijablu i **mutex koji pozivajuća nit drži zaključan**. `cond_wait` atomski **otpušta mutex i uspava nit**; kad neka druga nit signalom probudi ovu, mutex se opet zaključa prije povratka iz funkcije. Atomatičnost otpuštanja i uspavanja je ključna — bez nje bi nit mogla biti prekinuta upravo između tih dvaju koraka, i propustila bi signal koji je u međuvremenu poslan.

Standardni obrazac korištenja:

```c
pthread_mutex_lock(&mutex);
while (!uvjet)                          /* UVIJEK while, ne if! */
    pthread_cond_wait(&cv, &mutex);     /* atomski: otpusti mutex + cekaj */
/* sad smo budni, mutex je opet zakljucan, uvjet je istinit */
... napravi posao ...
pthread_mutex_unlock(&mutex);
```

`while` umjesto `if` je važno zbog tzv. **spurious wakeups** — sustav može probuditi nit i bez signala (zbog implementacijskih detalja jezgre, signala procesu, ...). Provjera uvjeta nakon buđenja štiti nas od ovih lažnih buđenja.

- [**`nit_cond.c`**](nit_cond.c) — proizvođač-potrošač s ograničenim međuspremnikom.

  Ovdje koristimo dva kondicijske varijable: jednu za "ima mjesta u međuspremniku" (proizvođač čeka na nju kad je pun) i jednu za "ima robe u međuspremniku" (potrošač čeka na nju kad je prazan).

  ```c
  #define VEL_BUFFERA 4
  #define BROJ_STAVKI 12

  static int             buffer[VEL_BUFFERA];
  static int             upis_idx = 0, cit_idx = 0, punjenje = 0;

  static pthread_mutex_t mutex      = PTHREAD_MUTEX_INITIALIZER;
  static pthread_cond_t  ima_mjesta = PTHREAD_COND_INITIALIZER;
  static pthread_cond_t  ima_robe   = PTHREAD_COND_INITIALIZER;

  void *proizvodjac(void *arg) {
      for (int i = 0; i < BROJ_STAVKI; i++) {
          pthread_mutex_lock(&mutex);
          while (punjenje == VEL_BUFFERA)
              pthread_cond_wait(&ima_mjesta, &mutex);

          buffer[upis_idx] = i;
          upis_idx = (upis_idx + 1) % VEL_BUFFERA;
          punjenje++;

          pthread_cond_signal(&ima_robe);
          pthread_mutex_unlock(&mutex);
          usleep(50000);
      }
      return NULL;
  }

  void *potrosac(void *arg) {
      for (int i = 0; i < BROJ_STAVKI; i++) {
          pthread_mutex_lock(&mutex);
          while (punjenje == 0)
              pthread_cond_wait(&ima_robe, &mutex);

          int v = buffer[cit_idx];
          cit_idx = (cit_idx + 1) % VEL_BUFFERA;
          punjenje--;

          pthread_cond_signal(&ima_mjesta);
          pthread_mutex_unlock(&mutex);
          usleep(120000);    /* potrosac sporiji */
      }
      return NULL;
  }
  ```

  Pošto proizvođač u primjeru radi brže od potrošača (50 ms naspram 120 ms po stavki), međuspremnik se postupno puni — vidi se u ispisu:

  ```
  $ ./nit_cond
  Proizvodjac: stavio 0 (punjenje 1)
                                  Potrosac: uzeo 0 (punjenje 0)
  Proizvodjac: stavio 1 (punjenje 1)
  Proizvodjac: stavio 2 (punjenje 2)
                                  Potrosac: uzeo 1 (punjenje 1)
  Proizvodjac: stavio 3 (punjenje 2)
  Proizvodjac: stavio 4 (punjenje 3)
                                  Potrosac: uzeo 2 (punjenje 2)
  ...
  ```

  Kad punjenje dosegne maksimum (4), proizvođač automatski blokira u `cond_wait(&ima_mjesta, ...)` i tako čeka dok potrošač ne izvadi nešto. Nakon nekoliko ciklusa, proizvođač je gotov sa svojim 12 stavkama, a potrošač ih sve uzima.


## Signali i niti

Signali iz P06 i niti su dvije neovisno razvijene apstrakcije UNIX-a, i njihova interakcija dolazi s nizom specifičnosti koje treba poznavati. Najvažnije pravilo je:

> Kad signal dolazi procesu, jezgra ga može dostaviti **bilo kojoj niti tog procesa koja taj signal trenutno ne blokira**. Sve dok ne specificiramo drugačije, ne možemo predvidjeti kojoj će niti signal stići.

Tablica rukovatelja signala je **dijeljena** među svim nitima procesa — ne postoji "moj rukovatelj `SIGINT`-a u nití `A`" različit od "rukovatelja `SIGINT`-a u niti `B`". Ali **maska blokiranih signala je privatna za svaku nit**. To znači da svaka nit može neovisno odrediti koje signale želi primati, a koje blokirati.

Funkcija `pthread_sigmask` je za niti ekvivalent `sigprocmask`-a iz P06:

```c
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);
```

Argumenti su isti kao kod `sigprocmask`-a: `how` je `SIG_BLOCK`, `SIG_UNBLOCK` ili `SIG_SETMASK`; `set` je maska signala; `oldset` izlazni argument za staru masku. **Bitno: u višenitnom programu treba koristiti `pthread_sigmask`, a ne `sigprocmask`** — ovaj drugi ima nedefinirano ponašanje u prisutnosti niti.

### Standardni obrazac: jedna nit obrađuje signale

U većini višenitnih programa najbolji pristup je centralizirati obradu signala u **jednoj niti**, dok sve ostale niti blokiraju signale. Tako izbjegavamo nepredvidivost koja nit primi signal. Obrazac:

1. Glavna nit prije stvaranja pomoćnih niti blokira sve signale koje želi obrađivati (npr. `SIGINT`).
2. Stvara pomoćne niti — one **naslijeđuju masku blokiranih signala** od glavne, pa i one blokiraju iste signale.
3. Glavna nit (ili posebna "signal nit") sinkrono čeka signale pozivom `sigwait`, i obrađuje ih.

`sigwait` je inverzna operacija od rukovatelja signala — umjesto asinkronog poziva funkcije, sinkrono čeka da signal stigne i vraća njegov broj. Time se izbjegavaju sve poteškoće pisanja "async-signal-safe" koda u rukovatelju (problem detaljno obrađen u P06).

- [**`nit_signal.c`**](nit_signal.c) — demonstracija obrasca.

  ```c
  #define BROJ_NITI 3

  void *radnik(void *arg) {
      int id = *(int *)arg;
      while (1) {
          printf("Radnik %d radi...\n", id);
          sleep(1);
      }
      return NULL;
  }

  int main(void) {
      pthread_t niti[BROJ_NITI];
      int       podaci[BROJ_NITI];
      sigset_t  maska;
      int       primljeni_signal;

      /* blokiraj SIGINT u glavnoj niti -- naslijedit ce ga sve pomocne */
      sigemptyset(&maska);
      sigaddset(&maska, SIGINT);
      pthread_sigmask(SIG_BLOCK, &maska, NULL);

      for (int i = 0; i < BROJ_NITI; i++) {
          podaci[i] = i;
          pthread_create(&niti[i], NULL, radnik, &podaci[i]);
      }

      printf("Pritisni Ctrl+C za izlaz...\n");

      /* sinkrono cekaj signal iz maske */
      sigwait(&maska, &primljeni_signal);
      printf("\nGlavna nit primila SIGINT, gasim radnike.\n");

      for (int i = 0; i < BROJ_NITI; i++)
          pthread_cancel(niti[i]);
      for (int i = 0; i < BROJ_NITI; i++)
          pthread_join(niti[i], NULL);

      return 0;
  }
  ```

  Pomoćne niti rade beskonačnu petlju ispisa. Kad korisnik pritisne `Ctrl+C`, signal `SIGINT` dolazi procesu — ali sve niti ga blokiraju osim implicitno glavna kroz `sigwait`. `sigwait` se vraća, glavna nit otkazuje pomoćne preko `pthread_cancel`, pokupi ih `pthread_join`-om i izlazi.

### Slanje signala specifičnoj niti

Za slanje signala procesu koristi se `kill(pid, sig)` iz P06. Za slanje signala specifičnoj niti unutar procesa postoji `pthread_kill`:

```c
int pthread_kill(pthread_t thread, int sig);
```

Ovo se rijetko koristi u praksi — kad imamo višenitni program, signali su uglavnom alat za vanjsku komunikaciju (od korisnika ili drugih procesa), pa nas obično ne zanima kojoj će točno niti stići. `pthread_kill` je tu kad nam stvarno treba precizna kontrola.

## Thread-safe i reentrant funkcije

Završna napomena koja je važna za pisanje stvarnih višenitnih programa. Mnoge funkcije iz standardne C biblioteke i POSIX-a nisu izvorno dizajnirane s nitima na umu — pretpostavljaju da unutar procesa postoji samo jedan tok izvršavanja. Kad ih pozovemo iz više niti istovremeno, mogu se dogoditi suptilni problemi.

Funkcija je **thread-safe** ako se može sigurno pozivati iz više niti istovremeno. Mnoge moderne implementacije pthreads-a su pažljivo dorađene da budu thread-safe — npr. `malloc` interno koristi mutexe da osigura ispravnost. Ali postoje i klasične "anti-primjeri":

- **`strtok`** — koristi internu statičku varijablu za pamćenje pozicije. Dvije niti koje istovremeno parsiraju različite stringove međusobno se "zarazu". Sigurna alternativa je **`strtok_r`** (sufiks `_r` od *reentrant*), kojem se eksplicitno predaje pointer za stanje.
- **`localtime`, `gmtime`, `asctime`** — vraćaju pokazivač na internu statičku strukturu, koja se prepisuje pri sljedećem pozivu. Sigurne alternative su **`localtime_r`**, **`gmtime_r`**, **`asctime_r`**.
- **`errno`** — bio bi problem da je obična globalna varijabla. Moderne implementacije čine ga *thread-local* (kao da svaka nit ima svoj `errno`) baš zbog ovog razloga, pa s njim u nitima radimo bez briga.

Kad pišete višenitni program, **provjerite man stranice funkcija koje koristite** — sekcija "ATTRIBUTES" navodi je li funkcija thread-safe, a ako nije, obično ukazuje na `_r` varijantu ili alternativu.

## Što smo zapravo radili

Niti su, uz signale i IPC, jedan od stupova suvremenog UNIX programiranja. Razumijevanjem niti otvaramo vrata cijelom svijetu paralelnih i konkurentnih programa — od jednostavnih web poslužitelja koji obrađuju više zahtjeva istovremeno, preko numeričkih programa koji iskorištavaju sve jezgre suvremenog procesora, do složenih distribuiranih sustava.

Najvažnije lekcije ovog poglavlja:

- Niti dijele adresni prostor procesa — to je istovremeno njihova najveća snaga (jednostavna komunikacija) i najveća opasnost (race conditioni).
- Pthreads sučelje je relativno jednostavno — nekoliko desetaka funkcija, ali bogato semantikom.
- **Sinkronizacija je teža od izvršavanja**. Pisanje koda koji se izvršava u više niti je relativno jednostavno; pisanje koda koji *točno* radi je gdje nastaju stvarni izazovi. Mutex za isključivost pristupa i kondicijske varijable za čekanje na uvjet pokrivaju ogromnu većinu sinkronizacijskih potreba.
- Klasični algoritmi (proizvođač-potrošač, čitatelji-pisci, ...) pojavljuju se iznova i iznova u stvarnom kodu. Vrijedi ih poznavati.

Tema koje smo se *namjerno klonili* uključuju otkazivanje niti (engl. *cancellation*) s pridruženim *cancellation points*, lokalnu pohranu niti (TLS — `pthread_key_create`), spin lockove i read-write lockove (`pthread_rwlock_*`), barijere (`pthread_barrier_*`), te razne atribute niti (prioriteti, policy raspoređivanja, veličina stoga). Sve to je dio pthreads-a, ali se rjeđe koristi u tipičnim programima — čitatelj koji nadje potrebu za tim će ih lako proučiti kad mu zatrebaju.

## Prevođenje

Direktorij dolazi s priloženim [`Makefile`](Makefile)-om koji prati iste konvencije kao i Makefile datoteke u prethodnim poglavljima. Posebnost je u tome da pthreads zahtijevaju linkanje s pthread bibliotekom (`-lpthread`), što je u Makefile-u već navedeno za svaki primjer.

```sh
make all          # gradi sve primjere
make nit_pozdrav  # gradi pojedinačni primjer
make clean        # čisti generirane datoteke
```

## Bibliografija

[1] D. R. Butenhof, *Programming with POSIX Threads*. Boston, MA, USA: Addison-Wesley Professional, 1997.

[2] W. R. Stevens and S. A. Rago, *Advanced Programming in the UNIX Environment*, 3rd ed. Boston, MA, USA: Addison-Wesley Professional, 2013.

[3] L. Budin, M. Golub, D. Jakobović, and L. Jelenković, *Operacijski sustavi*, 3. izd. Zagreb, Hrvatska: Element, 2013.
