#include <stdio.h>

int resultado(int x, int y);

int main() {
    int x, y;
    
    printf("informe um numero inteiro: ");
    scanf("%d", &x);
    printf("informe outro numero inteiro: ");
    scanf("%d", &y);

    int mdc = resultado(x, y);

    printf("%d", mdc);

    return 0;
}

/* ROTINES */

int resultado(int x, int y){
    if (y == 0) {
        return x;
    }
    return resultado(y, x % y);
}