#include <stdlib.h>
#include <time.h>

/*
 * Generira slucajni niz duzine broj_elemenata
 */
void slucajni_niz(int *niz, int broj_elemenata) {
    int i;
    srand(time(NULL));
    for (i = 0; i < broj_elemenata; i++)
        niz[i] = rand();
}

/*
 * Pronalazi najveci element u nizu
 */
int najveci_element(int *niz, int broj_elemenata) {
    int i, maxel = niz[0];

    for (i = 1; i < broj_elemenata; i++) {
        if (maxel < niz[i])
            maxel = niz[i];
    }

    return maxel;
}

/*
 * Pronalazi najmanji element u nizu
 */
int najmanji_element(int *niz, int broj_elemenata) {
    int i, minel = niz[0];

    for (i = 1; i < broj_elemenata; i++) {
        if (minel > niz[i])
            minel = niz[i];
    }

    return minel;
}
