#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define INFINITY INT_MAX

typedef struct {
    int num_vertices;
    int **matriz_custo;
} Grafo;

int encontrar_minima_distancia(int *dist, int *visitado, int V);
void imprimir_caminho_recursivo(int *anterior, int j);
Grafo *aloca_grafo(const char *nome_arquivo);
void dijkstra(Grafo *g, int ponto_origem, int ponto_destino);

int main() {

    int ponto_origem = 0;
    int ponto_destino = 6;
    const char *arquivo_grafo = "grafo.txt"; // arquivo deve estar na mesma pasta origem do codigo

    Grafo *meu_grafo = aloca_grafo(arquivo_grafo);

    if (meu_grafo == NULL) {
        return 1;
    }

    printf("\nExecutando Dijkstra de Vertice %d para Vertice %d...\n", ponto_origem, ponto_destino);
    dijkstra(meu_grafo, ponto_origem, ponto_destino);

    for (int i = 0; i < meu_grafo->num_vertices; i++) {
        free(meu_grafo->matriz_custo[i]);
    }
    free(meu_grafo->matriz_custo);
    free(meu_grafo);
    
    return 0;
}

int encontrar_minima_distancia(int *dist, int *visitado, int V) {
    int min = INFINITY, min_index = -1;
    
    for (int v = 0; v < V; v++) {
        if (visitado[v] == 0 && dist[v] < min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

void imprimir_caminho_recursivo(int *anterior, int j) {
    if (anterior[j] == -1) {
        printf("%d", j);
        return;
    }
    
    imprimir_caminho_recursivo(anterior, anterior[j]);
    printf(" -> %d", j);
}

Grafo *aloca_grafo(const char *nome_arquivo) {

    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo: %s\n", nome_arquivo);
        return NULL;
    }

    int count_numbers = 0;
    int temp_val;
    while (fscanf(arquivo, "%d", &temp_val) == 1) {
        count_numbers++;
    }
    
    int N = 0;
    if (count_numbers > 0) {
        rewind(arquivo);
        char line[1024];
        if (fgets(line, sizeof(line), arquivo) != NULL) {
            int num;
            char *ptr = line;
            while (sscanf(ptr, "%d", &num) == 1) {
                N++;
                while(*ptr && (*ptr != ' ' && *ptr != '\t' && *ptr != '\n')) ptr++;
                while(*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;
            }
        }
    }

    if (N == 0) {
        fprintf(stderr, "Arquivo vazio ou formato invalido.\n");
        fclose(arquivo);
        return NULL;
    }
    
    Grafo *g = (Grafo *)malloc(sizeof(Grafo));
    if (g == NULL) {
        perror("Erro de alocacao para Grafo");
        fclose(arquivo);
        return NULL;
    }
    g->num_vertices = N;
    
    g->matriz_custo = (int **)malloc(N * sizeof(int *));
    if (g->matriz_custo == NULL) {
        perror("Erro de alocacao para linhas da matriz");
        free(g);
        fclose(arquivo);
        return NULL;
    }
    for (int i = 0; i < N; i++) {
        g->matriz_custo[i] = (int *)malloc(N * sizeof(int));
        if (g->matriz_custo[i] == NULL) {
            perror("Erro de alocacao para colunas da matriz");
            for (int k = 0; k < i; k++) free(g->matriz_custo[k]);
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
                fprintf(stderr, "Erro de leitura na posicao [%d][%d].\n", i, j);
                for (int k = 0; k < N; k++) free(g->matriz_custo[k]);
                free(g->matriz_custo);
                free(g);
                fclose(arquivo);
                return NULL;
            }
        }
    }

    fclose(arquivo);
    printf("Grafo lido com sucesso. Total de %d vertices.\n", N);
    return g;
}

void dijkstra(Grafo *g, int ponto_origem, int ponto_destino) {
    int V = g->num_vertices;
    
    int *dist = (int *)malloc(V * sizeof(int));
    int *anterior = (int *)malloc(V * sizeof(int));
    int *visitado = (int *)malloc(V * sizeof(int));
    
    if (!dist || !anterior || !visitado) {
        perror("Erro de alocacao para vetores de Dijkstra");
        free(dist); free(anterior); free(visitado);
        return;
    }

    for (int i = 0; i < V; i++) {
        dist[i] = INFINITY;
        anterior[i] = -1;
        visitado[i] = 0;
    }
    
    dist[ponto_origem] = 0;

    for (int count = 0; count < V - 1; count++) {
        int u = encontrar_minima_distancia(dist, visitado, V);
        
        if (u == -1) break;

        visitado[u] = 1;
        
        if (u == ponto_destino) break;

        for (int v = 0; v < V; v++) {
            int custo_aresta = g->matriz_custo[u][v];
            
            if (!visitado[v] && custo_aresta > 0 && dist[u] != INFINITY && 
                dist[u] + custo_aresta < dist[v]) {
                
                dist[v] = dist[u] + custo_aresta;
                anterior[v] = u;
            }
        }
    }

    printf("\n--- Resultado do Algoritmo de Dijkstra ---\n");
    
    if (dist[ponto_destino] == INFINITY) {
        printf("Nao ha caminho da ponto_origem %d para o ponto_destino %d.\n", ponto_origem, ponto_destino);
    } else {
        printf("Melhor Caminho (Indices) da ponto_origem %d ao ponto_destino %d:\n", ponto_origem, ponto_destino);
        imprimir_caminho_recursivo(anterior, ponto_destino);
        printf("\n\nCusto Total do Caminho: %d\n", dist[ponto_destino]);
        printf("\n**Mapeamento para nomes reais (ruas/esquinas):**\n");
        printf("Substitua os indices (0, 1, 2...) por nomes reais.\n");
    }

    free(dist);
    free(anterior);
    free(visitado);
}
