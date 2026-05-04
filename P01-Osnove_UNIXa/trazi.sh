#!/bin/bash
# Pretražuje sve tekstualne datoteke u zadanom direktoriju koje sadrže
# zadanu ključnu riječ i ispisuje broj pojavljivanja u svakoj datoteci.
#
# Korištenje: ./trazi.sh <direktorij> <kljucna_rijec>

if [ $# -ne 2 ]; then
    echo "Korištenje: $0 <direktorij> <kljucna_rijec>"
    exit 1
fi

dir=$1
rijec=$2

if [ ! -d "$dir" ]; then
    echo "Greška: '$dir' nije direktorij."
    exit 2
fi

echo "Tražim '$rijec' u tekstualnim datotekama unutar '$dir'..."
echo

ukupno=0
nadeno_u=0

# Pretraga svih .txt datoteka u direktoriju i poddirektorijima
for f in $(find "$dir" -type f -name "*.txt"); do
    broj=$(grep -c "$rijec" "$f")
    if [ "$broj" -gt 0 ]; then
        echo "  $f: $broj"
        ukupno=$((ukupno + broj))
        nadeno_u=$((nadeno_u + 1))
    fi
done

echo
echo "Ukupno pojavljivanja: $ukupno (u $nadeno_u datoteka)"
