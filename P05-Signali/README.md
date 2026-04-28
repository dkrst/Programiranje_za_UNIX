# Signali

Primjeri uz poglavlje **Signali** iz knjige *Programiranje za UNIX*.

U ovom poglavlju upoznat ćemo **signale** — UNIX-ov primarni mehanizam za asinkronu komunikaciju s procesom. Signal je kratka poruka koju jezgra šalje procesu kao obavijest da se dogodio neki događaj: korisnik je pritisnuo Ctrl+C, proces je pokušao pristupiti nedopuštenoj memorijskoj adresi, istekao je timer, neki drugi proces je eksplicitno zatražio prekid, i tako dalje. Iz perspektive procesa, signal može stići u **bilo kojem trenutku** između dvije strojne instrukcije — proces nema mogućnost predvidjeti kada će se to dogoditi.

Proces na primljeni signal može reagirati na nekoliko načina: prepustiti zadanu reakciju jezgri (što za većinu signala znači prekid procesa), eksplicitno ignorirati signal, ili registrirati vlastitu funkciju — **handler** — koja će se izvršiti svaki put kad takav signal stigne. U ovom poglavlju kroz nekoliko primjera ilustriramo kako se signali registriraju, kako se hvataju, kako se koriste za komunikaciju među procesima i koje je opasnosti potrebno izbjegavati pri pisanju handlera.

## Sadržaj

### Hvatanje signala

- [**`potvrdi.c`**](potvrdi.c) — minimalan primjer hvatanja signala koji uvodi sve ključne koncepte rada s njima. Program registrira vlastiti handler za signal `SIGINT` (signal koji se procesu šalje kad korisnik u terminalu pritisne `Ctrl+C`); kad korisnik pritisne `Ctrl+C` prvi put, program ne završi, nego ispiše poruku da treba pritisnuti `Ctrl+C` još jednom ako se zaista želi izaći. Ovo je obrazac kakav viđamo u stvarnim alatima (npr. `htop`, `vim`) i u skriptama koje se ne smiju nehotice prekinuti.

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

  Mehanizam je sljedeći. Funkcija `signal(SIGINT, uhvati)` jezgri kaže: *"od ovog trenutka, kad god mom procesu stigne signal `SIGINT`, ne primjenjuj zadanu reakciju (prekid procesa) nego pozovi funkciju `uhvati`."* Kad korisnik pritisne Ctrl+C, jezgra **prekine** trenutno izvršavanje glavnog programa točno tamo gdje je bilo, pozove `uhvati`, a kad se ona vrati, glavni program nastavlja izvršavanje od mjesta gdje je bio prekinut. Sav posao koji handler napravi — u ovom slučaju jednostavno povećanje varijable `brojac` — vidljiv je glavnom programu kroz globalnu varijablu, što je standardni način "razgovora" između main-a i handlera.

  Glavni program koristi sistemski poziv `pause()`, koji uspava proces sve dok ne stigne **bilo koji** signal. Kad signal stigne i njegov handler završi izvršavanje, `pause()` se vrati i petlja nastavlja: provjeri trenutnu vrijednost `brojac`-a, ako je on `1` ispiše uputu, a u sljedećem prolazu petlje opet uđe u `pause()` čekajući novi signal. Tek kad `brojac` dosegne `2`, izlazi iz petlje i program uredno završava.

  Pokrenimo program i isprobajmo:

  ```
  $ ./potvrdi
  ^C
  Pritisnite ponovo CTRL - C ukoliko zelite izaci
  ^C
  $
  ```

  Funkcionalno je program gotovo trivijalan, ali pokriva nekoliko važnih koncepata vrijednih da se odmah istaknu:

  - **Handler je vrlo kratak** — samo inkrementira brojač i ne pokušava ništa složenije od toga (npr. nema poziva `printf`-a). Ovo nije slučajno: signal handler se izvršava u posebnom kontekstu — može prekinuti glavni program u doslovno bilo kojem trenutku, uključujući i sredinu poziva drugih funkcija. Iz handlera se zato smije pozivati samo vrlo ograničen skup funkcija (tzv. **`async-signal-safe`** funkcije). `printf` u taj skup ne spada — njegovo korištenje u handleru može u rijetkim slučajevima dovesti do iznenađujućih grešaka. Detaljnije ćemo o ovome u kasnijem primjeru, ali već sad uvodimo dobru praksu: **handler postavlja zastavicu, glavni program reagira**.

  - **Komunikacija kroz globalnu varijablu** — handler i glavni program "razgovaraju" kroz `brojac`. Strogo gledano, takve dijeljene varijable trebale bi biti deklarirane s tipom `volatile sig_atomic_t` umjesto običnog `int`-a:
    - Ključna riječ `volatile` govori prevoditelju da vrijednost varijable može biti promijenjena "iza leđa" glavnog programa (od strane handlera), pa optimizator ne smije njezinu vrijednost cache-irati u registru kroz iteracije petlje.
    - Tip `sig_atomic_t` jamči da se čitanje i pisanje varijable obavlja u jednoj nedjeljivoj operaciji — handler ne može uhvatiti glavni program "u sredini" upisa.

    Na većini modernih arhitektura (uključujući x86) obični `int` u praksi radi, ali primjer ćemo u kasnijim verzijama čistiti i pisati kako se to zapravo treba pisati.

  - **Trajnost handlera** — funkcija `signal` se poziva **samo jednom**, prije ulaska u petlju, a handler ostaje registriran kroz oba pritiska Ctrl+C. To je ponašanje koje očekujemo na Linuxu, ali povijesno gledano nije bilo tako — na starijim System V sustavima `signal` je nakon svake isporuke signala automatski poništavao registraciju handlera, pa bi sljedeći signal opet ubio proces. Iz tog razloga POSIX preporučuje korištenje modernije funkcije `sigaction()` umjesto `signal()` — vratit ćemo se na to u kasnijim primjerima.

## Prevođenje

Svaki primjer u ovom direktoriju prevodi se trivijalnim pozivom prevoditelja, npr.:

```sh
gcc -Wall potvrdi.c -o potvrdi
```

Kad budemo imali više primjera, dodat ćemo i [`Makefile`](Makefile) koji prati iste konvencije kao i u prethodnim poglavljima.
