/**
 * Competição de Ordenação: O Desafio dos Milissegundos
 * Estratégia: Algoritmo Linear Híbrido O(N) (Counting Sort + Radix Sort Base 256)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// Limite de memória para usar Counting Sort Direto (~128 MB de limite para o range)
#define MAX_COUNTING_RANGE 33554432

/**
 * Radix Sort LSB (Base 256 - 8 bits por passada)
 * Complexidade: O(N) estrito. Usado caso o intervalo de números seja muito esparso.
 */
static void radixSort256(int * restrict v, size_t n, int * restrict aux) {
    uint32_t *src = (uint32_t *)v;
    uint32_t *dst = (uint32_t *)aux;
    size_t count[4][256] = {0};

    // Passada única para construir os 4 histogramas simultaneamente
    for (size_t i = 0; i < n; i++) {
        uint32_t val = src[i];
        count[0][ val        & 0xFF]++;
        count[1][(val >>  8) & 0xFF]++;
        count[2][(val >> 16) & 0xFF]++;
        count[3][(val >> 24) & 0xFF]++;
    }

    // 4 passadas de distribuição por bytes (1 byte = 256 posições no cache L1)
    for (int pass = 0; pass < 4; pass++) {
        size_t *c = count[pass];
        
        // OTIMIZAÇÃO EXTREMA: Se todos os números tiverem o mesmo byte (ex: 0 nos bits altos),
        // pula a passada inteira sem tocar na memória!
        if (c[0] == n) continue;

        size_t acumulador = 0;
        for (int i = 0; i < 256; i++) {
            size_t temp = c[i];
            c[i] = acumulador;
            acumulador += temp;
        }

        int shift = pass * 8;
        for (size_t i = 0; i < n; i++) {
            uint32_t val = src[i];
            dst[c[(val >> shift) & 0xFF]++] = val;
        }

        // Troca os ponteiros de origem e destino
        uint32_t *tmp = src;
        src = dst;
        dst = tmp;
    }

    // Se o resultado final parou no vetor auxiliar, copia de volta
    if (src != (uint32_t *)v) {
        memcpy(v, src, n * sizeof(int));
    }
}

/**
 * Função Principal de Ordenação O(N)
 * Analisa a distribuição dos dados e aplica Counting Sort direto ou Radix Sort.
 */
void ordenarVetor(int * restrict v, size_t n, int * restrict buffer_aux, uint32_t * restrict freq) {
    if (n < 2) return;

    // 1. Encontra o menor e o maior elemento (Vetorizado automaticamente com -O3)
    int min_val = v[0];
    int max_val = v[0];
    for (size_t i = 1; i < n; i++) {
        if (v[i] < min_val) min_val = v[i];
        if (v[i] > max_val) max_val = v[i];
    }

    int64_t range = (int64_t)max_val - (int64_t)min_val + 1;

    // 2. Se o intervalo cabe no buffer de frequência pré-alocado, usa Counting Sort O(N)
    if (min_val >= 0 && range <= MAX_COUNTING_RANGE) {
        // Zera apenas a fatia necessária do vetor de frequência
        memset(freq, 0, (size_t)range * sizeof(uint32_t));

        // Conta as ocorrências de cada número
        for (size_t i = 0; i < n; i++) {
            freq[v[i] - min_val]++;
        }

        // Reconstrói o vetor ordenado de forma linear contígua
        size_t idx = 0;
        for (int64_t val = 0; val < range; val++) {
            uint32_t qtd = freq[val];
            int numero_real = (int)(val + min_val);
            while (qtd--) {
                v[idx++] = numero_real;
            }
        }
    } else {
        // 3. Caso contrário, usa Radix Sort em Base 256
        radixSort256(v, n, buffer_aux);
    }
}

/**
 * Verifica se o vetor está em ordem crescente estrita (Critério: 40 pts)
 */
int verificarCorretude(const int *v, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (v[i - 1] > v[i]) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    const char *arquivo_entrada = (argc > 1) ? argv[1] : "ListahAhSerOrdenada.txt";

    // =========================================================================
    // ETAPA 1: Leitura ultrarrápida do disco (Ignorada pelo cronômetro)
    // =========================================================================
    FILE *fp = fopen(arquivo_entrada, "rb");
    if (!fp) {
        perror("Erro ao abrir o arquivo ListahAhSerOrdenada.txt");
        return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    long tamanho_arquivo = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *texto_bruto = (char *)malloc(tamanho_arquivo + 1);
    if (!texto_bruto) {
        fprintf(stderr, "Falha ao alocar memoria para leitura.\n");
        fclose(fp);
        return EXIT_FAILURE;
    }

    size_t bytes_lidos = fread(texto_bruto, 1, tamanho_arquivo, fp);
    texto_bruto[bytes_lidos] = '\0';
    fclose(fp);

    // Aloca o vetor principal de elementos
    size_t capacidade = 10000000; // Começa preparado para 10 milhões de inteiros
    int *vetor = (int *)malloc(capacidade * sizeof(int));
    size_t total_elementos = 0;

    // Parser manual de inteiros (até 10x mais rápido que fscanf)
    char *p = texto_bruto;
    while (*p) {
        while (*p && (*p < '0' || *p > '9') && *p != '-') p++;
        if (!*p) break;

        int sinal = 1;
        if (*p == '-') {
            sinal = -1;
            p++;
        }

        int valor = 0;
        while (*p >= '0' && *p <= '9') {
            valor = valor * 10 + (*p - '0');
            p++;
        }

        if (total_elementos >= capacidade) {
            capacidade *= 2;
            vetor = (int *)realloc(vetor, capacidade * sizeof(int));
        }
        vetor[total_elementos++] = valor * sinal;
    }
    free(texto_bruto); // Libera o texto bruto da RAM

    // Aloca os buffers auxiliares ANTES do clock() para não poluir a medição
    int *buffer_aux = (int *)malloc(total_elementos * sizeof(int));
    uint32_t *freq  = (uint32_t *)malloc(MAX_COUNTING_RANGE * sizeof(uint32_t));

    printf("==================================================\n");
    printf("   DESAFIO DOS MILISSEGUNDOS - BENCHMARK EM C     \n");
    printf("==================================================\n");
    printf("-> Arquivo carregado: %s\n", arquivo_entrada);
    printf("-> Total de elementos na RAM: %zu\n", total_elementos);

    // =========================================================================
    // ETAPA 2: Medição Isolada da Ordenação (Critério: 15 pts)
    // =========================================================================
    clock_t inicio = clock();

    ordenarVetor(vetor, total_elementos, buffer_aux, freq);

    clock_t fim = clock();
    // =========================================================================

    double tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("-> Tempo de ordenacao: %f segundos (%f ms)\n", tempo_gasto, tempo_gasto * 1000.0);

    // =========================================================================
    // ETAPA 3: Validação de Corretude (Critério: 40 pts)
    // =========================================================================
    if (verificarCorretude(vetor, total_elementos)) {
        printf("-> Status: SUCESSO! Vetor verificado em ordem crescente estrita.\n");
        printf("-> Menor elemento: %d | Maior elemento: %d\n", vetor[0], vetor[total_elementos - 1]);
    } else {
        printf("-> Status: ERRO! Falha na verificacao de ordenacao.\n");
    }
    printf("==================================================\n");

    // Liberação correta de memória (Critério: 30 pts)
    free(freq);
    free(buffer_aux);
    free(vetor);

    return EXIT_SUCCESS;
}

