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

### Primjer: `pozdrav1`

- [**`pozdrav1.c`**](pozdrav1.c) — glavna nit (u kodu označena kao *prva nit*) stvara novu nit i ispisuje dvije svoje poruke prije izlaska iz `main`-a. Nova nit (*druga nit* u kodu) nakon kratke pauze ispisuje svoje dvije poruke.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <pthread.h>

  void *pozdrav(void *arg) {
      sleep(1);
      printf("Pozdrav iz druge niti!\n");
      printf("Druga nit izlazi!\n");
      return NULL;
  }

  int main(void) {
      pthread_t nit;

      if (pthread_create(&nit, NULL, pozdrav, NULL) != 0) {
          perror("pthread_create");
          return 1;
      }

      printf("Pozdrav iz prve niti.\n");
      printf("Prva nit nit izlazi!\n");
      return 0;
  }
  ```

  Ispis:

  ```
  $ ./pozdrav1
  Pozdrav iz prve niti.
  Prva nit nit izlazi!
  ```

  Iako bismo očekivali da se nakon pokretanja programa "jave" obje niti — prvo prva, a zatim, nakon kratke pauze, i druga — drugi pozdrav se nikada ne dogodi. Razlog ne bi smio biti prekid izvršavanja prve niti: sve niti unutar jednog procesa su, kao što smo ranije naveli, ravnopravne i predstavljaju nezavisne tokove izvršavanja unutar istog spremnika, pa završetak jedne niti ne povlači i završetak ostalih. Međutim, sjetimo se na trenutak značenja poziva `return` iz funkcije `main`: dok u svim drugim funkcijama u programu `return` znači povratak u funkciju pozivatelja, poziv `return` iz funkcije `main` po C standardu ekvivalentan je pozivu `exit` — prekidu izvršavanja procesa (više detalja vidjeti u poglavlju [P05 – Okruženje procesa, sekcija "Životni ciklus procesa"](../P05-Okruzenje_procesa/README.md#životni-ciklus-procesa)).

  Isto značenje `return` ima i u našem primjeru: `return` iz funkcije `main` je `exit`, a `exit` terminira **cijeli proces**, uključujući sve preostale niti, neovisno o tome jesu li završile svoj posao ili nisu. U trenutku kad bi se druga nit probudila iz `sleep(1)`, proces više ne postoji.

  Postoje različiti načini da ovo popravimo. U sljedeća dva primjera vidjet ćemo dva pristupa.

### Primjer: `pozdrav2`

- [**`pozdrav2.c`**](pozdrav2.c) — najjednostavnije rješenje: prva nit prije izlaska iz `main`-a pozove `pthread_join(nit, NULL)` i tako čeka da druga nit završi. Kod se od `pozdrav1.c` razlikuje samo u jednom dodanom retku.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <pthread.h>

  void *pozdrav(void *arg) {
      sleep(1);
      printf("Pozdrav iz druge niti!\n");
      printf("Druga nit izlazi!\n");
      return NULL;
  }

  int main(void) {
      pthread_t nit;

      if (pthread_create(&nit, NULL, pozdrav, NULL) != 0) {
          perror("pthread_create");
          return 1;
      }

      printf("Pozdrav iz prve niti.\n");
      pthread_join(nit, NULL);          /* novo: cekaj drugu nit prije izlaska */
      printf("Prva nit nit izlazi!\n");
      return 0;
  }
  ```

  `pthread_join(nit, NULL)` blokira prvu nit dok druga ne završi. Tek kad druga ispiše obje svoje poruke i vrati se iz polazne funkcije, prva nit nastavlja s ispisom svoje druge poruke i poziva `return 0`. U tom trenutku `exit` terminira proces — ali sad je sve već odrađeno.

  Ispis:

  ```
  $ ./pozdrav2
  Pozdrav iz prve niti.
  Pozdrav iz druge niti!
  Druga nit izlazi!
  Prva nit nit izlazi!
  ```

  Pažljivim pogledom na redoslijed vidimo da je prva nit zaista čekala drugu — između njezine prve i druge poruke ubacile su se obje poruke druge niti.

### Primjer: `pozdrav3`

Drugi primjer (`pozdrav2`) riješio je problem preuranjenog terminiranja procesa. U ovom primjeru, prva nit ostala je "glavna" — ona stvara novu nit, čeka na njezin završetak i završava proces. Na sljedećem primjeru pokazat ćemo da su niti zaista ravnopravne — ne postoji glavna nit koja stvara i povezuje (*join*) druge niti. Jednom kada se stvori nova nit, ili više njih, sve niti unutar jednog procesa su ravnopravne i bilo koja nit može povezati (*join*) bilo koju drugu.

- [**`pozdrav3.c`**](pozdrav3.c) — uloge `pthread_join`-a sad su zamijenjene. Prva nit nakon stvaranja druge ispisuje svoje dvije poruke i poziva `pthread_exit(NULL)`. Druga nit, nakon vlastite prve poruke, poziva `pthread_join(glavna, NULL)` na *prvoj* niti, i tek tada ispiše svoju zadnju poruku i završi.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <pthread.h>

  pthread_t glavna;

  void *pozdrav(void *arg) {
      sleep(1);
      printf("Pozdrav iz druge niti!\n");
      pthread_join(glavna, NULL);       /* druga nit ceka prvu */
      printf("Druga nit izlazi!\n");
      return NULL;
  }

  int main(void) {
      pthread_t nit;

      glavna = pthread_self();           /* spremi vlastiti ID u globalnu varijablu */
      if (pthread_create(&nit, NULL, pozdrav, NULL) != 0) {
          perror("pthread_create");
          return 1;
      }

      printf("Pozdrav iz prve niti.\n");
      printf("Prva nit nit izlazi!\n");
      pthread_exit(NULL);                /* terminira samo prvu nit, NE proces */
  }
  ```

  Dva su nova elementa u odnosu na prethodne primjere. Prvo, prva nit poziva `pthread_self()` da dohvati vlastiti identifikator i pohrani ga u globalnu varijablu `glavna`, vidljivu drugoj niti (sjetimo se da niti dijele cijeli adresni prostor procesa, uključujući globalne varijable). Funkcija `pthread_self` deklarirana je u `<pthread.h>`:

  ```c
  pthread_t pthread_self(void);
  ```

  Funkcija vraća identifikator niti koja ju je pozvala.

  Drugo, prva nit umjesto `return 0` poziva `pthread_exit(NULL)`. Time se gasi samo nit koja je pozvala `pthread_exit`, a proces ostaje živ jer u njemu još uvijek postoji druga nit. Druga nit nakon ispisa pozdrava povezuje prvu. Ovdje vidimo još jednu ključnu razliku u odnosu na procese, kod kojih postoji jasna hijerarhija roditelj-dijete, a dijete ne može prikupiti izlazni status roditelja. Kod niti hijerarhije nema — kao što druga nit u našem primjeru bez problema čeka i povezuje prvu, tako bilo koja nit u procesu može čekati bilo koju drugu. Nakon povratka iz `pthread_join`, druga nit ispiše zadnju poruku i završi.

  Ispis:

  ```
  $ ./pozdrav3
  Pozdrav iz prve niti.
  Prva nit nit izlazi!
  Pozdrav iz druge niti!
  Druga nit izlazi!
  ```

  Redoslijed ispisa je zanimljiv: prva nit ispiše svoje obje poruke i pozove `pthread_exit`; druga nit se u međuvremenu probudi iz `sleep(1)`, ispiše svoju prvu poruku, čeka prvu kroz `pthread_join` (koji u tom trenutku odmah uspijeva jer je prva već završila), pa ispiše svoju drugu poruku.

  Ovaj primjer pokazuje nekoliko važnih stvari odjednom. **Niti su zaista ravnopravne** — druga nit poziva `pthread_join` na *prvoj* niti, što bi u svijetu procesa bilo nezamislivo. **Prva nit (ona koja izvršava `main`) nije "posebna"** osim po tome što počinje izvršavati `main` i što `return` iz `main`-a terminira proces; ako umjesto `return` koristimo `pthread_exit`, ponaša se kao bilo koja druga nit. I konačno — **proces živi sve dok mu živi barem jedna nit**.

### Primjer: `kvadrat` — prijenos podataka u nit i natrag

U prethodnim primjerima glavna nit i dalje je nešto "radila" — ispisivala pozdrav. Da bismo vidjeli kako se podaci stvarno predaju u nit i kako se rezultat vraća, napravit ćemo program koji od nove niti traži da izračuna kvadrat broja zadanog kao argument naredbenog retka. Niti koja računa kvadrat broj predajemo kao četvrti argument funkcije `pthread_create`, a rezultat se vraća kao argument funkcije `pthread_exit`. Nit pozivatelj povratnu vrijednost prima kao drugi argument `pthread_join`.

- [**`kvadrat.c`**](kvadrat.c) — prvi pokušaj, s tipičnom početničkom pogreškom.

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <pthread.h>

  void *kvadrat(void *arg) {
      int broj = *(int*)arg;
      int r = broj*broj;
      pthread_exit((void*)&r);
  }

  int main(int argc, char **argv) {
      pthread_t nit;
      int broj;
      int *retval;

      if (argc < 2) {
          printf("koristenje: %s <broj>\n", argv[0]);
          return 0;
      }

      broj = atoi(argv[1]);
      if (pthread_create(&nit, NULL, kvadrat, &broj) != 0) {
          perror("pthread_create");
          return 1;
      }

      pthread_join(nit, (void**)&retval);
      printf("%d^2 = %d\n", broj, *retval);
      return 0;
  }
  ```

  Glavna nit pretvara argument naredbenog retka u cijeli broj i predaje funkciji `pthread_create` pokazivač na njega (varijabla `broj` je u glavnoj niti i ostaje živa do kraja `main`-a, pa je sigurno proslijediti njezinu adresu). U funkciji niti rezultat se izračuna u lokalnoj varijabli `r`, čija se adresa predaje `pthread_exit`-u. Glavna nit zatim u `retval` dobiva tu adresu i ispisuje vrijednost.

  Pokrenimo program nekoliko puta:

  ```
  $ ./kvadrat 7
  7^2 = -44044288
  $ ./kvadrat 7
  7^2 = -469766144
  $ ./kvadrat 7
  7^2 = -1574965248
  ```

  Rezultat je svaki put drugačiji i ni jednom točan. Razlog je upravo onaj koji smo spomenuli uz prototip `pthread_exit`-a: varijabla `r` živi na stogu niti koja računa kvadrat, a stog se oslobađa u trenutku kada nit završi. Kad glavna nit nakon povratka iz `pthread_join` dohvati `*retval`, čita iz područja memorije koje je nekad bilo stog te niti, ali sad sadrži nepoznat sadržaj.

  Logično pitanje koje se može postaviti: zašto pristup kroz pokazivač u glavnoj niti ne uzrokuje grešku `SIGSEGV`? Sjetimo se da niti dijele isti adresni prostor procesa, a stog jedne niti nije "izvan" adresnog prostora — to je memorija unutar njega, alocirana za potrebe te niti pri njenom stvaranju. Kad nit završi, sustav tu memoriju vraća u svoj bazen slobodnih stranica, ali sa stajališta hardvera adresa i dalje pripada procesu i moguć je pristup. Operacijski sustav nema načina znati da ono što čitamo više "ne pripada" niti koja je davno završila — pristup je tehnički legalan, samo je sadržaj smeće.

  Treba reći da povremeno možemo dobiti i `SIGSEGV` — primjerice ako sustav u međuvremenu vrati stranicu na kojoj je bio stog niti operacijskom sustavu, pa ona više nije mapirana u adresni prostor procesa. U ovom slučaju, pokušaj čitanja s adrese na koju pokazivač pokazuje izaziva segmentacijsku grešku, koja se manifestira na način da nam jezgra pošalje signal `SIGSEGV`.

  Iako ovo zvuči kontraintuitivno, takav ishod je u nekom smislu sretniji: program javlja grešku i odmah znamo da nešto ne valja. Mnogo je opasnija upravo ova tiha varijanta koju vidimo u našem primjeru — program *naizgled radi*, prolazi sve trivijalne provjere, ali rezultat je svaki put kriv. Greške ovog tipa mogu ostati neprimijećene jako dugo, sve dok se okolnosti ne poklope da ih netko slučajno otkrije.

  Ovo je važno pravilo koje vrijedi pamtiti: **nikad ne vraćajte iz niti pokazivač na lokalnu varijablu**. Isti princip vrijedi i za "obične" funkcije u C-u, ali se kod niti ova greška još teže otkriva jer ovisi o tome kad i kako sustav recklira memoriju oslobođenih stogova.

- [**`kvadrat2.c`**](kvadrat2.c) — ispravljena verzija. Umjesto lokalne varijable na stogu, memoriju za rezultat alociramo na hrpi pozivom `malloc`-a. Hrpa je dio adresnog prostora procesa i memorija ondje ostaje validna sve dok je eksplicitno ne oslobodimo pozivom `free`.

  ```c
  void *kvadrat(void *arg) {
      int broj = *(int*)arg;
      int *r = (int*)malloc(sizeof(int));

      *r = broj*broj;
      pthread_exit((void*)r);
  }
  ```

  Sve ostalo u programu je identično `kvadrat.c`-u. Sad rezultat radi pouzdano:

  ```
  $ ./kvadrat2 7
  7^2 = 49
  ```

  Mali nedostatak ovog rješenja: glavna nit nakon ispisa rezultata trebala bi pozvati `free(retval)` da oslobodi alociranu memoriju. U našem trivijalnom programu to nije problem jer i tako odmah završavamo, ali u dugotrajnim programima izostavljen `free` znači curenje memorije. Iako smo navikli da memoriju alociranu s `malloc`-om oslobađamo s `free` u funkciji u kojoj je memorija alocirana (jer je pokazivač u kojem je pohranjena memorijska adresa najčešće lokalna varijabla), u ovom slučaju to nije moguće jer je funkcija u kojoj je memorija alocirana završila s izvršavanjem završetkom niti. U ovom slučaju, nit koja je primila rezultat drži pokazivač na memorijsku adresu, pa je njena odgovornost da istu i oslobodi.

### Primjer: `argumenti` — više niti s različitim argumentima

Često nam je potrebno stvoriti niz niti i svakoj predati različit argument (npr. njezin redni broj, ulazne podatke za obradu, ili index u nekom polju). U sljedećem primjeru stvorit ćemo pet niti i svakoj predati njezin redni broj kroz argument `pthread_create`-a. Cilj je da svaka nit ispiše broj 0, 1, 2, 3 i 4 — svaka svoj.

- [**`argumenti.c`**](argumenti.c)

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
      int       podaci;
      int       i;

      for (i = 0; i < BROJ_NITI; i++) {
          podaci = i;
          if (pthread_create(&niti[i], NULL, radnik, &podaci) != 0) {
              perror("pthread_create");
              return 1;
          }
      }

      for (i = 0; i < BROJ_NITI; i++)
          pthread_join(niti[i], NULL);

      printf("Sve niti su zavrsile.\n");
      return 0;
  }
  ```

  Logika je očita: u svakoj iteraciji petlje upišemo trenutnu vrijednost `i` u varijablu `podaci`, te niti predamo njezinu adresu kao argument. Pokrenimo program i pogledajmo što se događa:

  ```
  $ ./argumenti
  Nit 3 pocinje rad
  Nit 3 pocinje rad
  Nit 3 pocinje rad
  Nit 3 pocinje rad
  Nit 4 pocinje rad
  Nit 3 zavrsila
  Nit 3 zavrsila
  Nit 3 zavrsila
  Nit 3 zavrsila
  Nit 4 zavrsila
  Sve niti su zavrsile.
  ```

  Umjesto očekivanih `0, 1, 2, 3, 4`, vidimo da nekoliko niti misli da im je `id` jednak `3` (ili neki drugi broj, ovisno o pokretanju). Što je pošlo po krivu?

  Sjetimo se da nit, kad je stvorimo, ne počinje *odmah* izvršavati svoju polaznu funkciju — između poziva `pthread_create` i početka izvršavanja funkcije `radnik` može proći određeno vrijeme dok raspoređivač ne odluči pokrenuti novostvorenu nit. U međuvremenu, glavna nit nastavlja izvršavati petlju i u svakoj iteraciji **prepisuje vrijednost varijable `podaci`**. Sjetimo se da nitima prenosimo adresu varijable (pokazivač), ne njezinu vrijednost — u našem slučaju sve niti su dobile pokazivač na istu varijablu (istu adresu u memoriji), koju "glavna" nit mijenja u petlji. Kada niti pokušaju pročitati vrijednost s adrese koju su dobile kao argument, vide trenutnu vrijednost — posljednju upisanu, ili onu koja se u tom trenutku tamo zatekla.

  Ovo je tipična zamka u radu s nitima: argument koji predajemo niti mora "preživjeti" do trenutka kad ga nit pročita i ne smije se u međuvremenu mijenjati. Ukoliko niti predamo pokazivač na varijablu koja se nakon poziva `pthread_create` mijenja — gotovo je sigurno da će barem neke od niti dobiti pogrešnu vrijednost.

- [**`argumenti2.c`**](argumenti2.c) — ispravljena verzija. Umjesto jedne varijable koju u petlji prepisujemo, koristimo polje, gdje svaka nit dobiva pokazivač na *svoj* zasebni element. Tih pet elemenata polja zadržavaju svoje vrijednosti sve do kraja `main`-a, kad sve niti već odavno čitaju svoje argumente.

  ```c
  int main(void) {
      pthread_t niti[BROJ_NITI];
      int       podaci[BROJ_NITI];     /* zasebna kopija ID-a za svaku nit */
      int       i;

      for (i = 0; i < BROJ_NITI; i++) {
          podaci[i] = i;
          if (pthread_create(&niti[i], NULL, radnik, &podaci[i]) != 0) {
              perror("pthread_create");
              return 1;
          }
      }

      for (i = 0; i < BROJ_NITI; i++)
          pthread_join(niti[i], NULL);

      printf("Sve niti su zavrsile.\n");
      return 0;
  }
  ```

  Funkcija `radnik` je identična kao u prethodnoj verziji. Razlika je samo u `main`-u: `podaci` je sad polje od `BROJ_NITI` elemenata, i `i`-toj niti predajemo `&podaci[i]` — pokazivač koji za nju ostaje stabilan jer nitko više ne dira upravo taj element polja.

  ```
  $ ./argumenti2
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

  Niti sad pravilno vide svoje brojeve. Usput primjećujemo i jednu zanimljivu osobinu: niti su završile u drugačijem redoslijedu nego što su počele. Ovo nije slučajno — **redoslijed izvršavanja niti nije unaprijed određen** i može se mijenjati od pokretanja do pokretanja, ovisno o tome kako se raspoređivač odluči ponašati u trenutku izvršavanja.

## Joinable i detached niti

Po defaultu, niti su **joinable** — njihovi resursi (struktura koja čuva povratnu vrijednost niti, identifikator, ...) ostaju u sustavu sve dok ih netko ne pokupi pozivom `pthread_join`. Ovo je analogno zombi procesima iz P05: ako ne pokupimo nit, imamo curenje resursa.

Često, međutim, ne želimo čekati rezultat niti — pokrenemo neki pozadinski zadatak i prepustimo ga sustavu. Za to služe **detached niti**: nakon završetka, njihovi resursi se **automatski oslobađaju**, bez potrebe za eksplicitnim `join`-om. Detached nit se ne smije i ne može joinati — pokušaj je greška.

Nit možemo napraviti detached na dva načina:

1. **Pri stvaranju** — postavljanjem atributa `PTHREAD_CREATE_DETACHED` u `pthread_attr_t` strukturu koju predajemo `pthread_create`-u.
2. **Naknadno** — pozivom `pthread_detach(nit)` u bilo kojem trenutku nakon stvaranja.

Funkcije koje koristimo deklarirane su u `<pthread.h>`:

```c
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_detach(pthread_t thread);
```

`pthread_attr_init` inicijalizira strukturu atributa zadanim vrijednostima; `pthread_attr_setdetachstate` u toj strukturi postavlja stanje otkačenosti (`PTHREAD_CREATE_DETACHED` ili `PTHREAD_CREATE_JOINABLE`); `pthread_attr_destroy` oslobađa strukturu kad nam više nije potrebna. Sve tri vraćaju `0` u slučaju uspjeha, odnosno kod greške. `pthread_detach` je alternativni način — služi za odvajanje već stvorene niti, pa ga ne moramo koristiti ako smo nit stvorili kao detached već pri pozivu `pthread_create`.

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

- [**`broji.c`**](broji.c) — osam niti istovremeno inkrementira zajedničku globalnu varijablu `count`. Svaka nit u petlji povećava brojač sto tisuća puta, pa očekujemo konačnu vrijednost `8 × 100 000 = 800 000`.

  ```c
  #include <stdlib.h>
  #include <pthread.h>
  #include <stdio.h>
  #include <unistd.h>
  #include <math.h>

  #define NTHREADS 8

  pthread_t thr_counter[NTHREADS];
  unsigned long count = 0;

  void *counter(void *arg) {
      int *c = (int *)arg;
      double d;
      printf("c: %d\n", *c);
      for (int k = 0; k < *c; k++) {
          /* petlja koja umjetno usporava nit kako bi se race lakse uocio */
          for (int j = 0; j < 50; j++)
              d = sqrt((double)j);

          count++;
      }
      (void)d;
      pthread_exit(NULL);
  }

  int main() {
      int cnt = 100000, k;

      for (k = 0; k < NTHREADS; k++) {
          pthread_create(&thr_counter[k], NULL, counter, (void *)&cnt);
      }

      for (k = 0; k < NTHREADS; k++) {
          pthread_join(thr_counter[k], NULL);
      }

      printf("Ukupno (%d * %d) = %lu\n", NTHREADS, cnt, count);
      return 0;
  }
  ```

  Unutar glavne petlje, prije svake inkrementacije brojača, umetnuli smo malu petlju koja računa korijene brojeva od 0 do 50. Ovaj račun ne mijenja `count` ni na koji način — postoji samo zato da nit provede malo vremena radeći "nešto" prije nego dođe do dijeljene varijable. To produžuje vrijeme provedeno u jednoj iteraciji, što povećava vjerojatnost da raspoređivač prebaci kontrolu drugoj niti baš u trenutku kada smo "u sredini" sekvence `mov / add / mov` koju kompajler generira za `count++`. Bez ovog usporavanja, sekvenca bi na modernim procesorima bila tako brza da bismo race teško uhvatili u demonstraciji.

  Pokrenimo program nekoliko puta:

  ```
  $ for i in 1 2 3; do ./broji 2>/dev/null | grep Ukupno; done
  Ukupno (8 * 100000) = 743521
  Ukupno (8 * 100000) = 698104
  Ukupno (8 * 100000) = 776892
  ```

  Konačna vrijednost `count`-a nije 800 000 i nije ista u svakom pokretanju. Niti su se preplitale "na različitim mjestima", pa je broj izgubljenih inkrementacija svaki put drugačiji. Ovisno o broju jezgri vašeg računala i opterećenju sustava, rezultat može biti puno bliži očekivanom (ako je preplitanje rijetko) ili puno dalji od njega — ali samo iznimno ćemo dobiti točno 800 000. Ovaj nedeterminizam upravo je ono što race condition čini opasnim: program može raditi savršeno tisuću puta, a tisuću prvi put dati pogrešan rezultat.

## Mutex

Rješenje race conditiona kod niti je **mutex** (engl. *mutual exclusion lock*). Konceptualno je identičan binarnom semaforu iz P07 — sinkronizacijska primitiva koja osigurava da samo jedna nit u danom trenutku može izvršavati zaštićeni dio koda (kritičnu sekciju). Razlika je u tome što je mutex optimiziran za niti unutar istog procesa — implementacija je u korisničkom prostoru kad nema sporova (samo atomski test-and-set u memoriji), pa je znatno brža od POSIX semafora.

Funkcije za rad s mutexima deklarirane su u `<pthread.h>`:

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;     /* staticka inicijalizacija */

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
```

Statički alocirani mutex (npr. globalna varijabla) inicijalizira se makro vrijednošću `PTHREAD_MUTEX_INITIALIZER`. Mutex alocirani dinamički (na hrpi) treba inicijalizirati pozivom `pthread_mutex_init` i kasnije osloboditi pozivom `pthread_mutex_destroy`.

Operacije nad mutexom:

- **`pthread_mutex_lock`** — pokušava zaključati mutex. Ako je već zaključan od strane druge niti, blokira pozivajuću nit dok mutex ne postane slobodan.
- **`pthread_mutex_unlock`** — otpušta mutex. Smije ga otpustiti samo nit koja ga drži (ovisno o tipu mutexa — postoji više varijanti, ali za naše potrebe zadani tip je dovoljan).

- [**`broji2.c`**](broji2.c) — rješenje race conditiona iz `broji.c` korištenjem mutexa. Struktura programa je gotovo identična, samo je inkrementacija brojača sad omeđena s `pthread_mutex_lock` i `pthread_mutex_unlock`.

  ```c
  pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

  void *counter(void *arg) {
      int *c = (int *)arg;
      double d;
      printf("c: %d\n", *c);
      for (int k = 0; k < *c; k++) {
          /* petlja koja simulira "neki posao" -- NE radi se nad
           * dijeljenom varijablom, pa ju drzimo IZVAN kriticne sekcije */
          for (int j = 0; j < 50; j++)
              d = sqrt((double)j);

          /* zakljucavamo mutex samo oko pristupa dijeljenoj varijabli */
          pthread_mutex_lock(&count_lock);
          count++;
          pthread_mutex_unlock(&count_lock);
      }
      (void)d;
      pthread_exit(NULL);
  }
  ```

  Rezultat:

  ```
  $ for i in 1 2 3; do ./broji2 2>/dev/null | grep Ukupno; done
  Ukupno (8 * 100000) = 800000
  Ukupno (8 * 100000) = 800000
  Ukupno (8 * 100000) = 800000
  ```

  Uvijek točno 800 000.

  Vrijedi razmisliti i o tome **gdje točno** postavljamo `lock` i `unlock`. Bilo bi tehnički ispravno staviti `pthread_mutex_lock` na sami početak vanjske petlje, a `pthread_mutex_unlock` na njen kraj — time bismo zaštitili i pristup brojaču i račun korijena. Ali to bi bilo izrazito loše po performanse: dok jedna nit izvodi svojih 50 sqrt operacija (koje uopće ne diraju zajedničku varijablu), sve ostale niti bi bespotrebno čekale. Račun korijena niti su mogli raditi paralelno bez ikakve interferencije; nepotrebno ih je serijalizirati.

  **Pravilo dobre prakse: mutex držimo zaključanim što kraće moguće.** Zaključavamo ga neposredno prije pristupa dijeljenom resursu, otključavamo neposredno nakon. Sve što ne mijenja dijeljene podatke ostaje izvan kritične sekcije, kako bi druge niti mogle raditi svoj posao paralelno. U našem `broji2.c` upravo to i radimo: petlja sa sqrt-om je *izvan* `lock`/`unlock` para, jer radi nad lokalnom varijablom `d`, dok je sama inkrementacija brojača (jedina operacija nad dijeljenim `count`) unutar mutexa.

  Cijena mutexa je značajan pad performansi (svaki ulazak/izlazak iz kritične sekcije ima trošak), ali u zamjenu dobivamo ispravan rezultat — kompromis koji u 99% slučajeva itekako vrijedi.

### Deadlock

Mutexi rješavaju jedan problem ali otvaraju drugi — **deadlock** (engl. *zaglavljenje*, *uzajamna blokada*). Da bismo razumjeli kako nastaje, zamislimo program s dva mutexa, `M1` i `M2`, koji štite dva različita resursa (npr. dvije strukture podataka). Nit `A` u nekom dijelu koda treba pristup objema strukturama, pa zaključava prvo `M1`, a zatim `M2`:

```
Nit A:                       Nit B:
  pthread_mutex_lock(&M1);    pthread_mutex_lock(&M2);
  pthread_mutex_lock(&M2);    pthread_mutex_lock(&M1);
  ...                          ...
  pthread_mutex_unlock(&M2);  pthread_mutex_unlock(&M1);
  pthread_mutex_unlock(&M1);  pthread_mutex_unlock(&M2);
```

U drugom dijelu koda, nit `B` također treba obje strukture — ali iz nekog razloga (možda je drugi programer pisao tu funkciju, ili je redoslijed dolazio iz drugačijeg konteksta) zaključava ih u **obrnutom redoslijedu**: prvo `M2`, pa `M1`.

Sve dok se `A` i `B` ne izvršavaju istovremeno, sve je u redu. Problem nastaje kad raspoređivač isprepleće dvije niti baš na neugodan način:

1. Nit `A` zaključa `M1`.
2. Raspoređivač prebaci kontrolu na nit `B` (ili ona ionako radi paralelno na drugoj jezgri).
3. Nit `B` zaključa `M2`.
4. Nit `B` pokušava zaključati `M1` — blokira jer ga drži `A`.
5. Nit `A` pokušava zaključati `M2` — blokira jer ga drži `B`.

Obje niti sad zauvijek čekaju jedna drugu. Nijedna ne može otpustiti svoj mutex jer ne može dovršiti svoj posao, a posao ne može dovršiti jer čeka onaj drugi mutex. Proces je živ ali se nikad više neće dogoditi ništa — to je deadlock.

Klasična ilustracija ovog problema je *problem pet filozofa* (engl. *dining philosophers*): pet filozofa sjedi za okruglim stolom, između svaka dva filozofa nalazi se jedan štapić, a filozof treba *oba* štapića (lijevi i desni) da bi mogao jesti. Ako svi istovremeno uzmu lijevi štapić i čekaju desni, nijedan nikad neće početi jesti.

Najjednostavniji način izbjegavanja deadlocka u praksi je **konzistentan redoslijed zaključavanja**: ako se svi dijelovi koda koji trebaju oba mutexa dogovore da uvijek zaključavaju `M1` prije `M2` (recimo, prema adresama u memoriji, ili abecednom redu imena), deadlock je nemoguć. U većim programima ovo zahtijeva pažljiv dizajn — što su mutexi raspršeniji po kodu, to je teže jamčiti konzistentnost. Postoje i drugi pristupi (npr. `pthread_mutex_trylock` koji ne blokira nego odmah javlja neuspjeh, pa nit može otpustiti ono što već drži i pokušati ponovno), ali rasprava o njima nadilazi opseg ovog uvoda.

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
