#include <stdio.h>

int resultado(int x);

int main() {
    int x;
    
    printf("informe um numero inteiro: ");
    scanf("%d", &x);

    int soma = resultado(x);

    printf("%d", soma);

    return 0;
}

/* ROTINES */

int resultado(int x){
    if (x>=10){
        return 1 + resultado(x/10);
    }

    return 1;
}