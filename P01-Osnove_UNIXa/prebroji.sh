#!/bin/bash
# Iterira po svim .c datotekama u trenutnom direktoriju, ispisuje
# broj redaka za svaku te ukupan zbroj. Demonstrira for petlju nad
# uzorkom datoteka, naredbenu supstituciju i aritmeticko zbrajanje
# u akumulator.
#
# Koristenje: ./prebroji.sh

ukupno=0

for datoteka in *.c; do
    # Provjeri postoji li uopce neka .c datoteka
    # (ako nema, for-petlja u bashu prolazi s doslovnim "*.c")
    if [ ! -f "$datoteka" ]; then
        echo "Nema .c datoteka u direktoriju."
        exit 1
    fi

    redci=$(wc -l < "$datoteka")
    echo "$datoteka: $redci redaka"
    (( ukupno += redci ))
done

echo "---"
echo "Ukupno: $ukupno redaka u svim .c datotekama"
