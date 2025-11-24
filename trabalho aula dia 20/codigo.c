#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define INFINITO INT_MAX

typedef struct {
    int num_vertices;
    int **matriz_custo;
} Grafo;

int encontra_menor_distancia(int *dist, int *visitado, int V);
void imprime_caminho_recursivo(int *anterior, int j);
Grafo *carrega_grafo(const char *nome_arquivo);
void dijkstra(Grafo *g, int origem, int destino);

int main() {
    int origem = 0;
    int destino = 6;
    const char *arquivo_grafo = "grafo.txt";

    Grafo *meu_grafo = carrega_grafo(arquivo_grafo);
    if (meu_grafo == NULL) {
        return 1;
    }

    dijkstra(meu_grafo, origem, destino);

    for (int i = 0; i < meu_grafo->num_vertices; i++) {
        free(meu_grafo->matriz_custo[i]);
    }
    free(meu_grafo->matriz_custo);
    free(meu_grafo);

    return 0;
}

int encontra_menor_distancia(int *dist, int *visitado, int V) {
    int minimo = INFINITO;
    int indice_minimo = -1;

    for (int v = 0; v < V; v++) {
        if (visitado[v] == 0 && dist[v] < minimo) {
            minimo = dist[v];
            indice_minimo = v;
        }
    }
    return indice_minimo;
}

void imprime_caminho_recursivo(int *anterior, int j) {
    if (anterior[j] == -1) {
        printf("%d", j);
        return;
    }

    imprime_caminho_recursivo(anterior, anterior[j]);
    printf(" -> %d", j);
}

Grafo *carrega_grafo(const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        printf("Erro: abertura do arquivo: %s\n", nome_arquivo);
        return NULL;
    }

    int contador_numeros = 0;
    int valor_temp;
    while (fscanf(arquivo, "%d", &valor_temp) == 1) {
        contador_numeros++;
    }

    int N = 0;
    if (contador_numeros > 0) {
        rewind(arquivo);
        char linha[1024];
        if (fgets(linha, sizeof(linha), arquivo) != NULL) {
            int num;
            char *ptr = linha;
            while (sscanf(ptr, "%d", &num) == 1) {
                N++;
                while (*ptr && (*ptr != ' ' && *ptr != '\t' && *ptr != '\n')) ptr++;
                while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;
            }
        }
    }

    if (N == 0) {
        printf("Erro: arquivo vazio ou formato invalido.\n");
        fclose(arquivo);
        return NULL;
    }

    Grafo *g = (Grafo *)malloc(sizeof(Grafo));
    if (g == NULL) {
        printf("Erro: alocacao para grafo");
        fclose(arquivo);
        return NULL;
    }
    g->num_vertices = N;

    g->matriz_custo = (int **)malloc(N * sizeof(int *));
    if (g->matriz_custo == NULL) {
        printf("Erro: alocacao para linhas da matriz");
        free(g);
        fclose(arquivo);
        return NULL;
    }

    for (int i = 0; i < N; i++) {
        g->matriz_custo[i] = (int *)malloc(N * sizeof(int));
        if (g->matriz_custo[i] == NULL) {
            printf("Erro: alocacao para colunas da matriz");
            for (int k = 0; k < i; k++){
                free(g->matriz_custo[k]);
            }
            free(g->matriz_custo);
            free(g);
            fclose(arquivo);
            return NULL;
        }
    }

    rewind(arquivo);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (fscanf(arquivo, "%d", &g->matriz_custo[i][j]) != 1) {
                printf("Erro: leitura na posicao [%d][%d].\n", i, j);
                for (int k = 0; k < N; k++){
                    free(g->matriz_custo[k]);
                }
                free(g->matriz_custo);
                free(g);
                fclose(arquivo);
                return NULL;
            }
        }
    }

    fclose(arquivo);
    return g;
}

void dijkstra(Grafo *g, int origem, int destino) {
    int V = g->num_vertices;

    int *dist = (int *)malloc(V * sizeof(int));
    int *anterior = (int *)malloc(V * sizeof(int));
    int *visitado = (int *)malloc(V * sizeof(int));

    if (!dist || !anterior || !visitado) {
        printf("Erro: alocacao para vetores do Dijkstra");
        free(dist); free(anterior); free(visitado);
        return;
    }

    for (int i = 0; i < V; i++) {
        dist[i] = INFINITO;
        anterior[i] = -1;
        visitado[i] = 0;
    }

    dist[origem] = 0;

    for (int contador = 0; contador < V - 1; contador++) {
        int u = encontra_menor_distancia(dist, visitado, V);

        if (u == -1) break;

        visitado[u] = 1;

        if (u == destino) break;

        for (int v = 0; v < V; v++) {
            int custo_aresta = g->matriz_custo[u][v];

            if (!visitado[v] && custo_aresta > 0 && dist[u] != INFINITO &&
                dist[u] + custo_aresta < dist[v]) {

                dist[v] = dist[u] + custo_aresta;
                anterior[v] = u;
            }
        }
    }

    printf("\nResultado de Dijkstra:\n");

    if (dist[destino] == INFINITO) {
        printf("nao possui caminho do ponto %d até o %d.\n", origem, destino);
    } else {
        printf("Melhor caminho do ponto %d ao %d:\n", origem, destino);
        imprime_caminho_recursivo(anterior, destino);
        printf("\n\nCusto total do caminho: %d\n", dist[destino]);
    }

    free(dist);
    free(anterior);
    free(visitado);
}
