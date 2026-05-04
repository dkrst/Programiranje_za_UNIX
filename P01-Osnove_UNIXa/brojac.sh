#!/bin/bash
# Broji od 1 do N (N se daje kao argument, ili 5 ako nije zadan).

# Provjera broja argumenata
if [ $# -eq 0 ]; then
    n=5
else
    n=$1
fi

echo "Brojim od 1 do $n..."

# for petlja
for i in $(seq 1 $n); do
    echo "  $i"
done

echo "Gotovo!"
