#include <stdio.h>
#include <nizfn.h>
#define N_EL 10

int main() {
    int i, mx, mi;
    int niz[N_EL];

    slucajni_niz(niz, N_EL);
    printf("Elementi niza:\n");
    printf("----------------\n");

    for (i = 0; i < N_EL; i++)
        printf("%3d. element: %12d\n", i + 1, niz[i]);

    mx = najveci_element(niz, N_EL);
    mi = najmanji_element(niz, N_EL);
    printf("\n\nMax: %d; Min: %d\n", mx, mi);

    return 0;
}
