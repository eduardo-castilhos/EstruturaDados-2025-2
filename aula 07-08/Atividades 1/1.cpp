#include <stdio.h>
#include <iostream>
#include <string>

int main(){
    int num[10], max, min, med;

    std::cout << "Vamos começar a ler os números:" << std::endl;

    for(int i=0; i<10; i++){
        std::cout << "Informe o numero da posicao " << i+1 << std::endl;
        scanf("%d", &num[i]);
    }
}