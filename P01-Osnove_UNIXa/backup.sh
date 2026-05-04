#!/bin/bash
# Stvara komprimiranu arhivu zadanog direktorija s timestampom u imenu.
#
# Korištenje: ./backup.sh <direktorij>

# Provjera argumenata
if [ $# -ne 1 ]; then
    echo "Korištenje: $0 <direktorij>"
    exit 1
fi

dir=$1

# Provjera da direktorij postoji
if [ ! -d "$dir" ]; then
    echo "Greška: '$dir' nije direktorij ili ne postoji."
    exit 2
fi

# Ime arhive: ime-direktorija_godina-mjesec-dan_sat-min.tar.gz
basename=$(basename "$dir")
timestamp=$(date +"%Y-%m-%d_%H-%M")
arhiva="${basename}_${timestamp}.tar.gz"

echo "Stvaram arhivu '$arhiva' iz direktorija '$dir'..."
tar -czf "$arhiva" "$dir"

if [ $? -eq 0 ]; then
    velicina=$(du -h "$arhiva" | cut -f1)
    echo "Gotovo. Arhiva '$arhiva' (veličina: $velicina) uspješno stvorena."
else
    echo "Greška pri stvaranju arhive."
    exit 3
fi
