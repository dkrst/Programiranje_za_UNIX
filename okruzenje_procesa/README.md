# okruzenje_procesa

Primjeri uz poglavlje **Okruženje procesa** iz knjige *Programiranje za UNIX*.

U ovom poglavlju dani su primjeri koji demonstriraju upravljanje procesima na UNIX-u: dohvaćanje argumenata naredbenog retka i okruženja UNIX procesa, stvaranje procesa, pokretanje programa i upravljanje limitima. Svaki od ovih mehanizama vezan je uz jedan ili više sistemskih poziva — `fork()`, `exec` obitelj, `wait()`, `dup2()`, `setrlimit()` — koji zajedno tvore jezgru UNIX-ove filozofije upravljanja procesima. Primjeri su poredani tako da se teme grade postupno — od najjednostavnijih (ispis primljenih argumenata) prema složenijima (kombinacija `fork`, `exec`, `dup2` i `wait` u jednom programu).

## Sadržaj

### Argumenti naredbenog retka

Funkcija `main` prema ISO C standardu može imati dva osnovna oblika:

```c
int main(void);
int main(int argc, char *argv[]);
```

Prvi oblik koristi se kada program ne treba pristupati argumentima naredbenog retka, dok drugi prima informacije o tome kako je program pokrenut.

Prilikom pozivanja bilo koje naredbe u UNIX ljusci, ljuska analizira niz znakova kojim korisnik želi pokrenuti naredbu i dijeli ga na podnizove odvojene razmacima. Ovaj postupak nazivamo **tokenizacija**, a rezultat je niz **tokena** — stringova koji redom sadrže pozvanu naredbu i sve argumente koje je korisnik naveo. Ukoliko ovim stringovima želimo pristupiti iz našeg programa, koristimo drugi oblik funkcije `main`, kod kojeg funkcija prima dva argumenta: cjelobrojni `argc` u kojem je pohranjen ukupan broj stringova navedenih u naredbenom retku (uključujući i naredbu kojom je program pozvan), i polje pokazivača na znakovni niz `argv`, u kojem su ovi stringovi redom pohranjeni.

Argumenti naredbenog retka nalaze se na samom vrhu adresnog prostora procesa, kako je prikazano na slici:

<p align="center">
  <img src="slike/memorijska_slika.png" alt="Memorijska slika UNIX procesa" width="400">
</p>

Najjednostavniji način pristupa je iteriranje kroz polje pokazivača `argv`, od prvog elementa (indeks 0) koji pokazuje na samu naredbu, do posljednjeg s indeksom `argc-1`.

Uzmimo za primjer hipotetski program `mojprogram` kod kojeg korisnik može prilikom pokretanja zadati argumente naredbenog retka. Neka je naš program pozvan s:

```
./mojprogram prvi argument      drugi_argument
```

Vrijednost argumenta `argc` bila bi 4 i odgovarala bi broju tokena koji čine naredbeni redak. Polje pokazivača `argv` redom bi pokazivalo na adrese pri vrhu adresnog prostora procesa, na kojima bi bile sljedeće vrijednosti:

```
argv[0] = "./mojprogram"
argv[1] = "prvi"
argv[2] = "argument"
argv[3] = "drugi_argument"
argv[4] = NULL
```

Uočimo dva detalja: ljuska tokenizira naredbeni redak tako da tokene razdvaja temeljem razmaka (*space*) — praznog prostora između njih. Pri tom je svejedno koristimo li jedan ili više razmaka (više puta pritisnuta tipka *space* prilikom unosa). Dodatno, pored pokazivača `argv[0]` do `argv[argc-1]`, uvijek postoji i posljednji pokazivač u nizu s indeksom `argc`, koji pokazuje na vrijednost `NULL`, tj. `(void*)0`.

- **`argumenti.c`** — najjednostavniji mogući primjer rada s argumentima naredbenog retka. Program u petlji prolazi kroz polje `argv[0], ..., argv[argc-1]` i ispisuje indeks i vrijednost svakog argumenta. Koristi se za vizualnu provjeru kako ljuska prenosi naredbeni redak programu — posebno korisno za razumijevanje razdvajanja riječi po razmacima, ponašanja navodnika, ili kako `argv[0]` uvijek nosi ime kojim je program pokrenut.

  ```
  $ ./argumenti jedan dva tri
  0:	 ./argumenti
  1:	 jedan
  2:	 dva
  3:	 tri
  ```

- **`zbroji.c`** — program koji dva argumenta naredbenog retka tumači kao cijele brojeve i ispisuje njihov zbroj. Uvodi praksu **provjere broja argumenata** na početku programa (`argc < 3` → ispis upute za korištenje i izlaz) te funkciju `atoi()` iz standardne C biblioteke za konverziju stringa u `int`. Uobičajeni UNIX obrazac: poruka o korištenju uvijek koristi `argv[0]` kako bi točno odražavala naziv pod kojim je program pozvan.

  ```
  $ ./zbroji
  koristenje: ./zbroji <1.broj> <2.broj>
  $ ./zbroji 17 25
  17 + 25 = 42
  ```

### Environment varijable

- **`readenv.c`** — čita vrijednost jedne environment varijable čije se ime zadaje kao argument naredbenog retka, pozivom `getenv()` iz standardne C biblioteke. `getenv()` vraća pokazivač na string s vrijednošću varijable, ili `NULL` ako varijabla nije postavljena. Program razlikuje ta dva slučaja i prikazuje odgovarajuću poruku. Ilustrira temeljni način na koji program pristupa okolini koju je naslijedio od ljuske — varijable poput `HOME`, `PATH`, `USER` ili vlastite varijable postavljene naredbom `export`.

  ```
  $ ./readenv HOME
  HOME = /home/dkrst
  $ ./readenv NEPOSTOJECA
  NEPOSTOJECA: environment varijabla ne postoji
  ```

- **`listenv.c`** — ispisuje **sve** environment varijable procesa. Demonstrira treći, nestandardni argument funkcije `main` koji većina udžbenika ne spominje:

  ```c
  int main(int argc, char *argv[], char *environ[]);
  ```

  Umjesto korištenja globalne varijable `environ` (deklarirane u `<unistd.h>`), ovdje se okolina prima izravno kao parametar — polje pokazivača na stringove oblika `"IME=vrijednost"` završeno `NULL`-om. Petlja iterira kroz polje sve dok ne naiđe na taj `NULL`. Funkcionalno je ekvivalentan UNIX naredbi `env`:

  ```
  $ ./listenv
  SHELL=/bin/bash
  HOME=/home/dkrst
  PATH=/usr/local/bin:/usr/bin:/bin
  LANG=hr_HR.UTF-8
  ...
  ```

### Stvaranje novog procesa (`fork`)

- **`novi.c`** — minimalni primjer sistemskog poziva `fork()`. **`fork()` je jedini način na koji u UNIX-u može nastati novi proces.** Ova funkcija u osnovi radi kopiju memorijske slike postojećeg procesa — proces dijete (*child*) je klon procesa roditelja (*parent*). Činjenica da nasljeđuje memorijsku sliku roditeljskog procesa znači da nasljeđuje i stanje svih varijabli, heap i stog — tj. da nastavlja izvršavanje od prve naredbe koja u kodu slijedi iza `fork`-a, kao da su se do tog trenutka oba procesa izvršavala paralelno, na potpuno jednak način, bez ikakve razlike u stanju varijabli, okruženja procesa, ili dinamički alocirane memorije na hrpi. Od tog trenutka dva procesa počinju živjeti odvojene živote — promjena bilo koje varijable u jednom od njih nikad ne utječe na drugoga, jer svaki ima potpuno svoju kopiju memorije.

  Povratna vrijednost `fork()`-a je jedini mehanizam po kojem dva procesa mogu razlikovati svoju ulogu: roditelj dobiva PID djeteta, a dijete dobiva 0. Program to iskorištava za granjanje logike — istim `printf`-om ispisuje različit tekst ovisno o tome tko ga izvršava:

  ```
  $ ./novi
  PARENT (25017)	 PID: 25016	 PPID: 14567
  CHILD  (0)	 PID: 25017	 PPID: 25016
  ```

  PID djeteta (25017) koji je roditelj dobio kao povratnu vrijednost `fork()`-a točno odgovara PID-u koji dijete prijavljuje pozivom `getpid()`; istovremeno dijete vidi PPID jednak PID-u roditelja, što potvrđuje odnos roditelj–dijete u procesnom stablu. Redoslijed ispisa između roditelja i djeteta nije određen — ovisi o raspoređivaču (*scheduleru*) koji odlučuje koji će od dva spremna procesa dobiti procesor prvi.

- **`nproc.c`** — ilustrira ključnu posljedicu činjenice da novi proces dobiva vlastitu kopiju memorijskog prostora roditelja: uključujući kopiju svih varijabli na stogu i hrpi. Program u petlji tri puta poziva `fork()` i tek potom ispisuje PID i PPID. Naivan odgovor na pitanje koliko procesa nastaje bio bi 4 (jedan originalni plus tri stvorena u tri iteracije), no stvarni odgovor je **8**. Razlog leži u tome što varijabla `k` iz `for` petlje nakon `fork`-a postoji u **obje** kopije procesa, pa oba nastavljaju dalje kroz istu petlju — svaki sa svojom kopijom iste vrijednosti `k`.

  Razvoj kroz iteracije, uz broj procesa i vrijednost varijable `k` koja nakon `fork`-a postoji nezavisno u svakome:

  ```
  prije petlje:                          1 proces,  k=0

  iteracija k=0:
      fork() klonira postojeći proces  → 2 procesa, k=0 u svakom
      k++                              → 2 procesa, k=1 u svakom

  iteracija k=1:
      fork() klonira sva 2 procesa     → 4 procesa, k=1 u svakom
      k++                              → 4 procesa, k=2 u svakom

  iteracija k=2:
      fork() klonira sva 4 procesa     → 8 procesa, k=2 u svakom
      k++                              → 8 procesa, k=3 u svakom

  uvjet k<3 neistinit u svima          → izlazak iz petlje
  printf() izvršava se u svih 8
  ```

  Stablo procesa nakon tri iteracije (svaki `fork` je nova grana; izvorni proces nakon `fork`-a je uvijek "lijeva" grana, dijete je "desna"):

  ```
                              P
                             / \
                      (k=0) /   \
                           P     P'
                          /|     |\
                   (k=1) / |     | \
                        P  P'   P'  P''
                       /|  |\   /|  |\
                (k=2)/ |  | \ / |  | \
                    P P' P' P''P' P''P''P'''
  ```

  Formula za broj procesa nakon `n` uzastopnih `fork()` poziva je **2ⁿ** — svaki `fork()` udvostručuje broj aktivnih procesa. U ovom primjeru **u petlju ulazi 1 proces, a iz nje izlazi 8**. Iz ispisanih PPID-ova moguće je rekonstruirati cijelo stablo — svaki proces zna tko mu je neposredni roditelj.

### Zamjena programa u procesu (`exec`)

- **`myls.c`** — prvi primjer funkcija iz **`exec` obitelji**: poziv `execlp()` zamjenjuje trenutni programski kod procesa kodom nekog drugog programa — u ovom slučaju UNIX naredbe `ls`. Za razliku od `fork()`, koji stvara novi proces, `exec()` **ne stvara novi proces** — PID ostaje isti, promijeni se samo programski kod koji se izvršava.

  ```c
  execlp("ls", "ls", "-al", NULL);
  ```

  Valja primijetiti da su **prva dva argumenta poziva `execlp` identična**: prvi (`"ls"`) je ime izvršne datoteke koju treba naći i pokrenuti, a drugi (`"ls"`) je ono što će nova naredba dobiti kao svoj `argv[0]`. Ovo je neočigledno, ali nužno — podsjetimo se da po konvenciji svaka naredba u svom `argv[0]` očekuje vlastito ime (upravo iz tog razloga poruke o korištenju koriste `argv[0]`). Teoretski je moguće proslijediti i različito ime — tako bi program vidio da je pokrenut pod drugim imenom — ali u normalnoj uporabi oba se argumenta podudaraju.

  Sufiks `lp` u imenu funkcije sažima način predaje: **`l`** (*list*) — argumenti nove naredbe zadaju se kao lista završena s `NULL`; **`p`** (*path*) — za pronalazak izvršne datoteke koristi se varijabla okoline `PATH`, pa nije potrebno navoditi punu putanju. Ako `execlp()` uspije, poziv se nikad ne vraća — zato je `perror("execlp")` dosežljiv samo u slučaju greške (npr. naredba `ls` nije pronađena u `PATH`-u).

  ```
  $ ./myls
  ukupno 24
  drwxr-xr-x 2 dkrst users 4096 Apr 23 15:10 .
  drwxr-xr-x 8 dkrst users 4096 Apr 23 15:09 ..
  -rwxr-xr-x 1 dkrst users 8760 Apr 23 15:10 myls
  -rw-r--r-- 1 dkrst users  256 Apr 23 15:09 myls.c
  ...
  ```

- **`pokreni.c`** — uopćena verzija prethodnog primjera: program prima proizvoljnu naredbu s argumentima kao svoje argumente naredbenog retka i pokreće je pozivom `execvp()`. Sufiks **`v`** (*vector*) označava da se argumenti prenose kao jedno polje pokazivača na stringove, završeno `NULL`-om — točno u formi u kojoj ih funkcija `main` pokrenutog programa i dobije kao `argv`. Kako funkcija `main` već dobiva argumente u tom obliku, dovoljno je proslijediti pokazivač na `argv[1]`:

  ```c
  execvp(argv[1], &argv[1]);
  ```

  Izraz `&argv[1]` je pokazivačka aritmetika — `argv` je polje pokazivača na stringove, a `&argv[1]` daje adresu **drugog elementa** tog polja, tj. pokazivač na pod-polje koje počinje od `argv[1]` i ide dalje. Funkcija `execvp` će to pod-polje tretirati kao svoje vlastito `argv`: `argv[1]` iz perspektive `pokreni`-ja postaje `argv[0]` nove naredbe (njezino ime), `argv[2]` postaje `argv[1]`, i tako redom. Ovako jednostavnim obrascem dobivamo zametak vlastite ljuske — program koji može pokrenuti bilo koju drugu UNIX naredbu.

  Preporuka je pokušati razne kombinacije i promotriti ponašanje:

  ```
  $ ./pokreni ls
  $ ./pokreni ls -al
  $ ./pokreni ls -al /root
  $ ./pokreni asdhasd
  ```

  Prva dva poziva rade očekivano. Treći poziv (`ls -al /root`) ovisi o tome je li korisnik root — ako nije, `ls` će ispisati poruku o greški jer nema pravo pristupa direktoriju `/root`. Zanimljivo je odgovoriti na pitanje: **tko ispisuje tu poruku?** U ovom slučaju to nije `pokreni` — `pokreni` je svoj posao obavio kad je pozvao `execvp`, koji je uspješno zamijenio njegov kod kodom naredbe `ls`. `pokreni` u doslovnom smislu više ne postoji kao program u tom procesu; poruku o grešci ispisuje sama naredba `ls`, nad svojim otvorenim standardnim izlazom za greške. U četvrtom pozivu (`./pokreni asdhasd`), poruku o grešci ispisuje `pokreni` sam — `execvp` je neuspješno pokušao pronaći izvršnu datoteku `asdhasd`, vratio se u `pokreni`, i idući redak koda je upravo `perror("execvp")`:

  ```
  $ ./pokreni asdhasd
  execvp: No such file or directory
  ```

  Razlika je suptilna ali ključna: kad `exec` uspije, program koji je pozvao `exec` prestaje postojati; kad `exec` ne uspije, izvršavanje se nastavlja s naredbom iza njega.

  Zabavan primjer rekurzivne zamjene:

  ```
  $ ./pokreni ./pokreni ./pokreni ls -al
  ```

  Jedan isti proces redom mijenja svoj sadržaj tri puta: prvi `pokreni` kod je zamijenjen drugim pozivom `pokreni`, taj pozivom trećeg `pokreni`-a, i on na kraju pozivom `ls -al`. PID ostaje isti kroz sve četiri metamorfoze, a ono što korisnik vidi kao rezultat — ispis sadržaja direktorija — potpuno je nerazlučivo od običnog poziva `ls -al`. Ova rekurzija ilustrira što `exec` zapravo jest: ne pokretanje novog procesa, nego **zamjena tijela postojećeg procesa drugim kodom**.

- **`pokreni2.c`** — ključan korak prema pravoj ljusci: **kombinacija `fork()` i `exec()`**. U prethodnom primjeru `pokreni` je nakon `execvp()` prestao postojati kao `pokreni` — njegov je kod zamijenjen kodom pokrenute naredbe, pa nakon završetka naredbe nema ničeg na što bi se vratilo. U realnoj ljusci želimo zadržati roditelja živim kako bi mogao primiti sljedeću naredbu, a za svaku naredbu pokrenuti novi proces.

  Obrazac koji slijedi je upravo onaj koji **svaka UNIX ljuska koristi svaki put kad korisnik utipka naredbu**: ljuska pozivom `fork` stvori novi proces (kopiju same sebe), potom u tom novom procesu pozivom `exec` pokrene traženu naredbu, a u roditeljskom procesu čeka njen završetak pozivom `wait`. Ljuska ovo radi u petlji — nakon svakog `wait`-a korisniku daje novu ljuskinu oznaku (*prompt*) i mogućnost da unese sljedeću naredbu.

  Roditelj iz poziva `wait()` preuzima statusni broj kroz koji jezgra pakira informaciju o tome je li dijete završilo normalno i s kojim izlaznim statusom. Makro `WEXITSTATUS(s)` iz `<sys/wait.h>` izdvaja pravu povratnu vrijednost (onu koju je dijete predalo kroz `return` iz `main`-a ili `exit()`-om). Povratna vrijednost 127, koju `pokreni2` koristi u grani kad `execvp()` ne uspije, je UNIX konvencija za "naredba nije pronađena".

  Isprobajmo s različitim naredbama:

  ```
  $ ./pokreni2 ls
  argumenti  argumenti.c  listenv  listenv.c  ...
  Noramaln izlaz, izlazni status: 0

  $ ./pokreni2 asdasdasd
  execvp: No such file or directory
  Noramaln izlaz, izlazni status: 127

  $ ./pokreni2 ls sdfsdfsdf
  ls: cannot access 'sdfsdfsdf': No such file or directory
  Noramaln izlaz, izlazni status: 2
  ```

  **Zašto se izlazni status razlikuje** među tri slučaja? U prvom slučaju `ls` je uspješno obavio svoj posao i `exit`-ao s 0 — to je UNIX konvencija "sve je prošlo dobro". U drugom slučaju `execvp` u djetetu nije uspio (naredba `asdasdasd` ne postoji u `PATH`-u), pa se dijete vratilo na idući redak koda — `perror` i zatim `return 127`. Broj 127 je namjerno odabran jer po POSIX konvenciji upravo on označava "naredba nije pronađena". U trećem slučaju `ls` se uspješno pokrenuo, ali je pri otvaranju datoteke `sdfsdfsdf` utvrdio da ne postoji, ispisao poruku o grešci na standardni izlaz za greške i vratio 2 — što je specifična konvencija same naredbe `ls` za "ozbiljnija greška".

  **Zašto uopće `pokreni2` vraća ovaj status?** Zato što nam je `wait(&s)` iz djeteta predao statusni broj: za `pokreni2` je svaka od ove tri situacije uobičajeno ponašanje koje se razlikuje samo vrijednošću. Ljuska koju svakodnevno koristimo radi upravo to isto — izlazni status posljednje naredbe dostupan je preko posebne varijable `$?`, što omogućuje pisanje skripti koje granjaju svoju logiku ovisno o ishodu prethodnog poziva.

- **`preusmjeri.c`** — povezuje koncepte iz ovog poglavlja s mehanizmom preusmjeravanja ulaza/izlaza obrađenim u poglavlju o ulazno-izlaznim operacijama. Program prima dva ili više argumenata: prvi je ime datoteke u koju želimo preusmjeriti standardni izlaz, a ostatak je naredba koju želimo pokrenuti. Redoslijed koraka je:

  1. Pozivom `creat()` otvori se izlazna datoteka s pravima `0644`.
  2. Pozivom `dup2(fd, STDOUT_FILENO)` file deskriptor datoteke se duplicira na deskriptor 1 — upravo onaj koji proces koristi za standardni izlaz. Od tog trenutka svaki `write` ili `printf` kojemu je odredište `STDOUT_FILENO` efektivno piše u otvorenu datoteku.
  3. Stari deskriptor `fd` više nije potreban pa se zatvara, uz uobičajenu zaštitnu provjeru (`fd != STDOUT_FILENO`) za rijedak slučaj kad je `creat()` slučajno vratio baš 1.
  4. Pozivom `execvp()` program se zamjenjuje traženom naredbom. Ključno zapažanje: **otvoreni file deskriptori nasljeđuju se kroz `exec()`** — nova naredba naslijedi preusmjerenje na datoteku, iako ona sama nema pojma o tome.

  Dobivamo funkcionalni ekvivalent ljuskinog operatora `>`:

  ```
  $ ./preusmjeri izlaz.txt ls -la
  $ cat izlaz.txt
  ukupno 24
  drwxr-xr-x 2 dkrst users 4096 Apr 23 15:10 .
  ...
  ```

  Na istom principu radi i sama UNIX ljuska kad obrađuje `naredba > datoteka`: prije `exec`-a naredbe, ljuska otvori datoteku i pozivom `dup2` podmetne je na deskriptor 1.

### Ograničavanje resursa (`setrlimit`)

- **`limitraj.c`** i **`potrosac.c`** — ilustriraju sistemski poziv `setrlimit()`, kojim proces može postaviti ograničenje na različite resurse koje mu sustav dopušta koristiti. `limitraj` postavlja `RLIMIT_CPU` na 3 sekunde (i tvrdo i meko ograničenje), što znači da sljedeći proces smije potrošiti najviše 3 sekunde procesorskog vremena; nakon toga jezgra mu šalje signal koji ga prekida. Nakon postavljanja limita, `limitraj` pozivom `execvp()` pokreće naredbu zadanu argumentima. Kako se limiti resursa — kao i file deskriptori — **nasljeđuju kroz `exec()`**, pokrenuta naredba nasljeđuje i ograničenje. Ovo je osnovna ideja iza mehanizma ugrađene naredbe `ulimit` u ljusci, kao i alata poput `timeout` i različitih "sandbox" okruženja.

  Program `potrosac.c` je pratilac koji služi samo za demonstraciju djelovanja `limitraj`-a — sastoji se od jedne jedine beskonačne petlje `while(1);`, bez ijednog sistemskog poziva ili I/O operacije. Kad se pokrene samostalno, trošio bi procesor neograničeno dugo. Pokrenut kroz `limitraj`, nasljeđuje njegov CPU limit:

  ```
  $ ./limitraj ./potrosac
  Killed
  ```

  Ljuska nakon prekida procesa signalom ispisuje `Killed` (na Linux sustavima s engleskom lokalizacijom; na nekim drugim sustavima ispis može biti `CPU time limit exceeded` ili izostati sasvim). Bitno je zapaziti koliko dugo program traje — oko **tri sekunde**, a ne više.

  **Procesorsko i stvarno vrijeme.** Limit `RLIMIT_CPU` broji isključivo **procesorsko vrijeme** (*CPU time*) — ukupno vrijeme tijekom kojeg je proces zaista izvršavao instrukcije na nekom procesoru. To nije isto što i **stvarno vrijeme** (*wall-clock time* ili *real time*) koje mjeri koliko je proteklo vremena od pokretanja do kraja procesa, uključujući sve periode kad je proces bio pauziran od strane raspoređivača, kad je čekao na I/O, ili kad je sustav izvršavao druge procese. Ova dva vremena razlikuju se po tome da **procesorsko vrijeme nikad nije duže od stvarnog** — proces ne može utrošiti više procesorskih sekundi nego što je sekundi proteklo u stvarnosti. Najčešće je procesorsko vrijeme **kraće** od stvarnog: čim proces počne čekati na nešto (pritisak tipke, čitanje s diska, poruku preko mreže), stvarni sat nastavlja kucati, a procesorski stoji jer proces ne radi. U slučaju `potrosac`-a razlika između ta dva vremena je minimalna jer proces zaista cijelo vrijeme samo okreće procesor; kod realnih programa koji komuniciraju s korisnikom ili diskom razlika zna biti vrlo velika. Razlika je vidljiva i korisniku — UNIX naredba `time` ispisuje sva tri vremena (stvarno/*real*, korisničko procesorsko/*user*, sistemsko procesorsko/*sys*):

  ```
  $ time ./limitraj ./potrosac
  Killed

  real    0m3.012s
  user    0m3.000s
  sys     0m0.001s
  ```

### Životni ciklus procesa

- **`noparent.c`** — detaljniji primjer `fork()` mehanizma koji prikazuje što se dogodi kada **roditelj završi prije djeteta**. Program stvara stablo od tri procesa — `PARENT 1`, `CHILD 1` i `CHILD 2` — pri čemu `CHILD 2` (unuk po odnosu prema `PARENT 1`) namjerno spava duže od svog direktnog roditelja (`CHILD 1`). Zbog toga `CHILD 1` završi dok njegovo dijete još radi, a time `CHILD 2` postaje **osirotjeli proces** (*orphan*). U takvoj situaciji jezgra automatski preuzima ulogu novog roditelja — tradicionalno je to proces `init` s PID-om 1.

  Očekivani ispis s konkretnim PID-ovima (skraćeno; `SH` je PID ljuske iz koje je program pokrenut, npr. 14567; `PARENT 1` dobiva PID 25016, `CHILD 1` 25017, `CHILD 2` 25018):

  ```
  CHILD 2:	 PID: 25018	 PPID: 25017
  CHILD 1:	 PID: 25017	 PPID: 25016
  PARENT 1:	 PID: 25016	 izlazi: 25017
  PARENT 1:	 PID: 25016	 PPID: 14567
  CHILD 2:	 PID: 25018	 PPID: 1
  ```

  U prvom ispisu `CHILD 2` vidi svog pravog roditelja `CHILD 1` (PPID = 25017); nakon tri sekunde spavanja, njegov drugi ispis prijavljuje **PPID = 1** — u međuvremenu je `CHILD 1` završio, a jezgra je posvojenje prenijela na `init`.

  **Napomena o modernim sustavima.** Na suvremenim Linux distribucijama koje koriste `systemd` i korisničke sesije (`systemd --user`), posljednji PPID često neće biti 1 nego PID korisničkog `systemd` procesa. Razlog je mehanizam **subreapera** uveden u Linux jezgri 3.4 (2012.): pozivom `prctl(PR_SET_CHILD_SUBREAPER, 1)` proces se može registrirati kao "zamjenski init" za sve svoje potomke. Kad sirotan nastane, jezgra traži najbližeg pretka-subreapera umjesto da ga automatski pošalje na PID 1. Na desktop Linuxu sa `systemd`-om upravo je `systemd --user` takav subreaper, pa se tamo vidi PPID jednak njegovom PID-u. Konkretan iznos može se provjeriti naredbom `ps -p <ppid> -o pid,ppid,comm`. Klasično pravilo (*sirotan → PID 1*) i dalje vrijedi u odsutnosti subreapera — primjerice pri pokretanju iz SSH sesije na minimalnom serveru.

## Prevođenje

Direktorij dolazi s priloženim `Makefile`-om koji prati iste konvencije kao i Makefile datoteke u direktorijima `osnove_programiranja/` i `fileio/` (varijable `CC`, `CFLAGS`, `LDFLAGS`, `TARGETS`; implicitno pravilo `.c.o`; pravila `default`, `all`, `clean`). Detaljan opis strukture i korake gradnje Makefilea vidjeti u [`../osnove_programiranja/README.md`](../osnove_programiranja/README.md).

Tipična uporaba:

```sh
make              # gradi zadani cilj (novi)
make all          # gradi sve primjere
make pokreni2     # gradi samo zadani primjer
make clean        # briše izvršne i objektne datoteke
```
