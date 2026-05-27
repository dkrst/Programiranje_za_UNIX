#!/bin/bash
# Provjerava je li skripta pozvana ispravno (tocno jedan argument)
# i postoji li zadana datoteka, pa ispisuje broj redaka i zadnjih 5
# redaka. Demonstrira koristenje izlaznog statusa: razliciti razlozi
# greske vracaju razlicite kodove ($? u bash, $status u csh).
#
# Koristenje: ./provjeri.sh <ime_datoteke>

# Provjeri je li zadan tocno jedan argument
if [ $# -ne 1 ]; then
    echo "Koristenje: $0 <ime_datoteke>"
    exit 1
fi

# Provjeri postoji li zadana datoteka
if [ ! -f "$1" ]; then
    echo "Greska: datoteka '$1' ne postoji."
    exit 2
fi

# Sve je u redu -- obradi datoteku
echo "Obrada datoteke: $1"
echo "Broj redaka: $(wc -l < $1)"
echo "Zadnjih 5 redaka:"
tail -5 "$1"

exit 0
