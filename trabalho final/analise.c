#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_LINHA 1000
#define MAX_FUNCOES 100
#define MAX_LOOPS 100
#define MAX_RECURSAO 100
#define MAX_TOKEN 100

typedef enum {
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_IF,
    TOKEN_RETURN,
    TOKEN_IDENTIFICADOR,
    TOKEN_NUMERO,
    TOKEN_OPERADOR,
    TOKEN_CHAVE_ABRE,
    TOKEN_CHAVE_FECHA,
    TOKEN_PAREN_ABRE,
    TOKEN_PAREN_FECHA,
    TOKEN_PONTO_VIRGULA,
    TOKEN_DESCONHECIDO,
    TOKEN_FIM
} TipoToken;

typedef struct {
    TipoToken tipo;
    char valor[MAX_TOKEN];
    int linha;
} Token;

typedef struct {
    int nivel;
    int linha;
    char tipo[10];
    char variavel[50];
    bool depende_de_n;
} InfoLoop;

typedef struct {
    char nome_funcao[100];
    int cont_chamadas;
    int linhas[MAX_RECURSAO];
    bool tem_caso_base;
    int linha_caso_base;
} InfoRecursao;

typedef struct {
    char nome_funcao[100];
    int linha_inicio;
    int linha_fim;
    InfoLoop loops[MAX_LOOPS];
    int cont_loops;
    int profundidade_maxima;
    InfoRecursao recursao;
    bool eh_recursiva;
    char complexidade[50];
} AnaliseFuncao;

AnaliseFuncao funcoes[MAX_FUNCOES];
int cont_funcoes = 0;
char arquivo_atual[256];

void limpar(char *texto) {
    char *inicio = texto;
    while (*inicio && isspace(*inicio)) {
        inicio++;
    }
    if (*inicio == 0) {
        texto[0] = 0;
        return;
    }
    char *fim = texto + strlen(texto) - 1;
    while (fim > inicio && isspace(*fim)) {
        fim--;
    }
    fim[1] = 0;
    memmove(texto, inicio, fim - inicio + 2);
}

bool contem(const char *texto, const char *subtexto) {
    return strstr(texto, subtexto) != NULL;
}

int contar_char(const char *texto, char caractere) {
    int contador = 0;
    while (*texto) {
        if (*texto == caractere) {
            contador++;
        }
        texto++;
    }
    return contador;
}

TipoToken identificar_palavra_chave(const char *palavra) {
    if (!strcmp(palavra, "for")) {
        return TOKEN_FOR;
    }
    if (!strcmp(palavra, "while")) {
        return TOKEN_WHILE;
    }
    if (!strcmp(palavra, "if")) {
        return TOKEN_IF;
    }
    if (!strcmp(palavra, "return")) {
        return TOKEN_RETURN;
    }
    return TOKEN_IDENTIFICADOR;
}

Token pegar_proximo_token(const char *linha, int *posicao) {
    Token token;
    token.tipo = TOKEN_DESCONHECIDO;
    token.valor[0] = 0;

    while (linha[*posicao] && isspace(linha[*posicao])) {
        (*posicao)++;
    }

    if (linha[*posicao] == 0) {
        token.tipo = TOKEN_FIM;
        return token;
    }

    if (isdigit(linha[*posicao])) {
        int indice = 0;
        while (isdigit(linha[*posicao])) {
            token.valor[indice++] = linha[(*posicao)++];
        }
        token.valor[indice] = 0;
        token.tipo = TOKEN_NUMERO;
        return token;
    }

    if (isalpha(linha[*posicao]) || linha[*posicao] == '_') {
        int indice = 0;
        while (isalnum(linha[*posicao]) || linha[*posicao] == '_') {
            token.valor[indice++] = linha[(*posicao)++];
        }
        token.valor[indice] = 0;
        token.tipo = identificar_palavra_chave(token.valor);
        return token;
    }

    char caractere = linha[*posicao];
    (*posicao)++;

    if (caractere == '{') {
        token.tipo = TOKEN_CHAVE_ABRE;
    }
    else if (caractere == '}') {
        token.tipo = TOKEN_CHAVE_FECHA;
    }
    else if (caractere == '(') {
        token.tipo = TOKEN_PAREN_ABRE;
    }
    else if (caractere == ')') {
        token.tipo = TOKEN_PAREN_FECHA;
    }
    else if (caractere == ';') {
        token.tipo = TOKEN_PONTO_VIRGULA;
    }
    else {
        token.tipo = TOKEN_OPERADOR;
    }

    token.valor[0] = caractere;
    token.valor[1] = 0;

    return token;
}

bool eh_declaracao_funcao(const char *linha, char *nome) {
    if (!contem(linha, "(") || !contem(linha, ")")) {
        return false;
    }
    if (contem(linha, "//") || contem(linha, "/*") || contem(linha, "#")) {
        return false;
    }
    if (contem(linha, "if (") || contem(linha, "for (") || contem(linha, "while (")) {
        return false;
    }

    char temp[MAX_LINHA];
    strcpy(temp, linha);

    char *ponteiro = strchr(temp, '(');
    if (!ponteiro) {
        return false;
    }

    *ponteiro = 0;
    char *ultima = NULL;
    char *palavra = strtok(temp, " \t*&");

    while (palavra) {
        ultima = palavra;
        palavra = strtok(NULL, " \t*&");
    }

    if (ultima && isalpha(ultima[0])) {
        strcpy(nome, ultima);
        return true;
    }

    return false;
}

bool detectar_loop(const char *linha, InfoLoop *loop, int num_linha) {
    char temp[MAX_LINHA];
    strcpy(temp, linha);
    limpar(temp);

    if ((contem(temp, "for") || contem(temp, "for ")) && strchr(temp, '(')) {
        char *pos_for = strstr(temp, "for");
        if (pos_for && (pos_for == temp || !isalpha(*(pos_for-1)))) {
            
            strcpy(loop->tipo, "for");
            loop->linha = num_linha;
            loop->variavel[0] = 0;

            char *ponteiro = strchr(temp, '(');
            if (ponteiro) {
                char *ponto_virgula = strchr(ponteiro + 1, ';');
                if (ponto_virgula) {
                    char inicializacao[100];
                    int tamanho = ponto_virgula - ponteiro - 1;
                    strncpy(inicializacao, ponteiro + 1, tamanho);
                    inicializacao[tamanho] = 0;

                    char *igual = strchr(inicializacao, '=');
                    if (igual) {
                        char *var_fim = igual - 1;
                        while (var_fim > inicializacao && isspace(*var_fim)) {
                            var_fim--;
                        }
                        char *var_inicio = var_fim;
                        while (var_inicio > inicializacao && (isalnum(*(var_inicio-1)) || *(var_inicio-1) == '_')) {
                            var_inicio--;
                        }
                        int len = var_fim - var_inicio + 1;
                        if (len > 0) {
                            strncpy(loop->variavel, var_inicio, len);
                            loop->variavel[len] = 0;
                        }
                    }
                }
            }

            if (contem(linha, " n") || contem(linha, "size") || contem(linha, "length")) {
                loop->depende_de_n = true;
            } else {
                loop->depende_de_n = false;
            }
            return true;
        }
    }

    if ((contem(temp, "while") || contem(temp, "while ")) && strchr(temp, '(')) {
        char *pos_while = strstr(temp, "while");
        if (pos_while && (pos_while == temp || !isalpha(*(pos_while-1)))) {
            
            strcpy(loop->tipo, "while");
            loop->linha = num_linha;
            if (contem(linha, " n") || contem(linha, "size") || contem(linha, "length")) {
                loop->depende_de_n = true;
            } else {
                loop->depende_de_n = false;
            }
            strcpy(loop->variavel, "condicao");
            return true;
        }
    }

    return false;
}

bool detectar_recursao(const char *linha, const char *nome, int num_linha, InfoRecursao *rec) {
    if (contem(linha, nome) && contem(linha, "(")) {
        char temp[MAX_LINHA];
        strcpy(temp, linha);
        limpar(temp);

        if (!contem(temp, "void") && !contem(temp, "int") &&
            !contem(temp, "float") && !contem(temp, "double") &&
            !contem(temp, "char")) {
            if (rec->cont_chamadas < MAX_RECURSAO) {
                rec->linhas[rec->cont_chamadas++] = num_linha;
                return true;
            }
        }
    }
    return false;
}

bool detectar_caso_base(const char *linha, int num_linha, InfoRecursao *rec) {
    if (contem(linha, "return") && (contem(linha, "if") || contem(linha, "=="))) {
        rec->tem_caso_base = true;
        rec->linha_caso_base = num_linha;
        return true;
    }
    return false;
}

void calcular_complexidade(AnaliseFuncao *funcao) {
    
    if (funcao->eh_recursiva) {
        if (funcao->recursao.cont_chamadas >= 2 && funcao->cont_loops == 0) {
            strcpy(funcao->complexidade, "O(2^n)");
            return;
        } 
        
        if (funcao->recursao.cont_chamadas >= 2 && funcao->cont_loops > 0) {
            strcpy(funcao->complexidade, "O(n log n)");
            return;
        }

        if (funcao->recursao.cont_chamadas == 1 && funcao->cont_loops == 0) {
             strcpy(funcao->complexidade, "O(n)");
             return;
        }
    }

    int profundidade = 0;
    for (int i = 0; i < funcao->cont_loops; i++) {
        if (funcao->loops[i].depende_de_n) {
            if (funcao->loops[i].nivel > profundidade) {
                profundidade = funcao->loops[i].nivel;
            }
        }
    }

    if (profundidade == 0) {
        strcpy(funcao->complexidade, "O(1)");
    } else if (profundidade == 1) {
        strcpy(funcao->complexidade, "O(n)");
    } else if (profundidade == 2) {
        strcpy(funcao->complexidade, "O(n²)");
    } else if (profundidade == 3) {
        strcpy(funcao->complexidade, "O(n³)");
    } else {
        sprintf(funcao->complexidade, "O(n^%d)", profundidade);
    }
}

void analisar_arquivo(const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("\nerro ao abrir arquivo '%s'\n", nome_arquivo);
        return;
    }

    strcpy(arquivo_atual, nome_arquivo);

    char linha[MAX_LINHA];
    int num_linha = 0, chaves = 0;
    int indice = -1;
    int pilha_loops[MAX_LOOPS];
    int profundidade = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        num_linha++;

        char nome[100];
        if (eh_declaracao_funcao(linha, nome) && contem(linha, "{")) {
            if (cont_funcoes < MAX_FUNCOES) {
                indice = cont_funcoes++;
                AnaliseFuncao *funcao = &funcoes[indice];
                strcpy(funcao->nome_funcao, nome);
                funcao->linha_inicio = num_linha;
                funcao->cont_loops = 0;
                funcao->profundidade_maxima = 0;
                funcao->eh_recursiva = false;
                funcao->recursao.cont_chamadas = 0;
                funcao->recursao.tem_caso_base = false;
                strcpy(funcao->recursao.nome_funcao, nome);
                chaves = 1;
            }
            continue;
        }

        if (indice == -1) {
            continue;
        }

        chaves += contar_char(linha, '{') - contar_char(linha, '}');

        InfoLoop loop;
        if (detectar_loop(linha, &loop, num_linha)) {
            AnaliseFuncao *funcao = &funcoes[indice];
            if (funcao->cont_loops < MAX_LOOPS) {
                loop.nivel = profundidade + 1;
                funcao->loops[funcao->cont_loops++] = loop;
                profundidade++;
                if (profundidade > funcao->profundidade_maxima) {
                    funcao->profundidade_maxima = profundidade;
                }
                pilha_loops[profundidade - 1] = chaves;
            }
        }

        while (profundidade > 0 && chaves < pilha_loops[profundidade - 1]) {
            profundidade--;
        }

        if (detectar_recursao(linha, funcoes[indice].nome_funcao, num_linha,
                             &funcoes[indice].recursao)) {
            funcoes[indice].eh_recursiva = true;
        }

        detectar_caso_base(linha, num_linha, &funcoes[indice].recursao);

        if (chaves == 0) {
            funcoes[indice].linha_fim = num_linha;
            calcular_complexidade(&funcoes[indice]);
            indice = -1;
            profundidade = 0;
        }
    }

    fclose(arquivo);
}

void gerar_relatorio() {
    char saida[512];
    strcpy(saida, arquivo_atual);
    char *ponto = strrchr(saida, '.');
    if (ponto) {
        strcpy(ponto, "_relatorio.txt");
    }
    else {
        strcat(saida, "_relatorio.txt");
    }

    FILE *relatorio = fopen(saida, "w");

#define RELATORIO(...) do { \
    printf(__VA_ARGS__); \
    if(relatorio) { \
        fprintf(relatorio, __VA_ARGS__); \
    } \
} while(0)

    RELATORIO("\n");
    RELATORIO("RELATORIO\n");
    RELATORIO("------------------------------------------------------\n");
    RELATORIO("Foram encontradas %d funcoes pra analisar\n\n", cont_funcoes);

    for (int i = 0; i < cont_funcoes; i++) {
        AnaliseFuncao *funcao = &funcoes[i];

        RELATORIO("Funcao: %s\n", funcao->nome_funcao);
        RELATORIO("Fica entre as linhas %d e %d\n\n", funcao->linha_inicio, funcao->linha_fim);

        RELATORIO("O que tem nessa funcao: ");

        if (funcao->cont_loops > 0) {
            RELATORIO("Achei %d loop(s):\n", funcao->cont_loops);
            for (int j = 0; j < funcao->cont_loops; j++) {
                RELATORIO("- um %s na linha %d (profundidade %d)\n",
                     funcao->loops[j].tipo, funcao->loops[j].linha, funcao->loops[j].nivel);
                if (strlen(funcao->loops[j].variavel) > 0) {
                    RELATORIO("usa a variavel: %s\n", funcao->loops[j].variavel);
                }
                RELATORIO("depende do tamanho da entrada? %s\n",
                      funcao->loops[j].depende_de_n ? "sim" : "nao");
            }
        } else {
            RELATORIO("nao tem nenhum loop aqui\n");
        }

        if (funcao->eh_recursiva) {
            RELATORIO(" essa funcao e recursiva (chama ela mesma)\n");
            RELATORIO("se chama %d vez(es)\n", funcao->recursao.cont_chamadas);
            RELATORIO("nas linhas: ");
            for (int j = 0; j < funcao->recursao.cont_chamadas; j++) {
                RELATORIO("%d ", funcao->recursao.linhas[j]);
            }
            RELATORIO("\n");
            RELATORIO("tem caso base pra parar? %s", funcao->recursao.tem_caso_base ? "sim" : "nao");
            if (funcao->recursao.tem_caso_base) {
                RELATORIO(" (linha %d)", funcao->recursao.linha_caso_base);
            }
            RELATORIO("\n");
        }

        RELATORIO("\nComplexidade: %s\n", funcao->complexidade);
        RELATORIO("\n------------------------------------------------------\n\n");
    }

    int cont_o1=0, cont_logn=0, cont_n=0, cont_nlogn=0, cont_n2=0, cont_n3=0, cont_2n=0, outros=0;

    for (int i = 0; i < cont_funcoes; i++) {
        char *comp = funcoes[i].complexidade;
        if (contem(comp, "O(1)")) {
            cont_o1++;
        }
        else if (contem(comp, "O(log n)")) {
            cont_logn++;
        }
        else if (contem(comp, "O(n log n)")) {
            cont_nlogn++;
        }
        else if (contem(comp, "O(n³)")) {
            cont_n3++;
        }
        else if (contem(comp, "O(n²)")) {
            cont_n2++;
        }
        else if (contem(comp, "O(n)")) {
            cont_n++;
        }
        else if (contem(comp, "O(2^n)")) {
            cont_2n++;
        }
        else {
            outros++;
        }
    }

    if (relatorio) {
        fclose(relatorio);
        printf("\nrelatorio salvo\n");
    }

#undef RELATORIO
}

int main(int argc, char *argv[]) {
	
    if (argc < 2) {
        printf("por favor, use: %s <nome_do_arquivo.c>\n", argv[0]);
        return 1;
    }
    analisar_arquivo(argv[1]);
    gerar_relatorio();
    return 0;
}
