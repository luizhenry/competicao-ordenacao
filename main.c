#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// Limite maximo de intervalo para aplicacao do Counting Sort direto
#define MAX_COUNTING_RANGE 33554432

/**
 * Radix Sort LSB em Base 256 (processamento de 8 bits por passada).
 * Executa em tempo linear O(N) e eh utilizado quando o intervalo de valores
 * ultrapassa o limite do Counting Sort direto.
 */
static void radixSort256(int * restrict v, size_t n, int * restrict aux) {
    uint32_t *src = (uint32_t *)v;
    uint32_t *dst = (uint32_t *)aux;
    size_t count[4][256] = {0};

    // Construcao simultanea dos 4 histogramas em uma unica passada
    for (size_t i = 0; i < n; i++) {
        uint32_t val = src[i];
        count[0][ val        & 0xFF]++;
        count[1][(val >>  8) & 0xFF]++;
        count[2][(val >> 16) & 0xFF]++;
        count[3][(val >> 24) & 0xFF]++;
    }

    // 4 passadas de distribuicao (uma para cada byte)
    for (int pass = 0; pass < 4; pass++) {
        size_t *c = count[pass];

        // Se todos os elementos compartilham o mesmo byte, pula a passada
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

        // Alterna entre os vetores de origem e destino
        uint32_t *tmp = src;
        src = dst;
        dst = tmp;
    }

    // Se o resultado final ficou no vetor auxiliar, copia de volta para o vetor original
    if (src != (uint32_t *)v) {
        memcpy(v, src, n * sizeof(int));
    }
}

/**
 * Funcao principal de ordenacao hibrida.
 * Identifica o intervalo dos dados e seleciona a estrategia mais eficiente:
 * Counting Sort direto para intervalos densos ou Radix Sort Base 256.
 */
void ordenarVetor(int * restrict v, size_t n, int * restrict buffer_aux, uint32_t * restrict freq) {
    if (n < 2) return;

    int min_val = v[0];
    int max_val = v[0];
    for (size_t i = 1; i < n; i++) {
        if (v[i] < min_val) min_val = v[i];
        if (v[i] > max_val) max_val = v[i];
    }

    int64_t range = (int64_t)max_val - (int64_t)min_val + 1;

    // Se os valores sao nao-negativos e cabem no range pre-alocado, aplica Counting Sort direto
    if (min_val >= 0 && range <= MAX_COUNTING_RANGE) {
        memset(freq, 0, (size_t)range * sizeof(uint32_t));

        for (size_t i = 0; i < n; i++) {
            freq[v[i] - min_val]++;
        }

        size_t idx = 0;
        for (int64_t val = 0; val < range; val++) {
            uint32_t qtd = freq[val];
            int numero_real = (int)(val + min_val);
            while (qtd--) {
                v[idx++] = numero_real;
            }
        }
    } else {
        radixSort256(v, n, buffer_aux);
    }
}

/**
 * Verifica se o vetor esta ordenado em ordem crescente
 */
int verificarCorretude(const int *v, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (v[i - 1] > v[i]) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    const char *arquivo_entrada = (argc > 1) ? argv[1] : "ListahAhSerOrdenada.txt";

    // Leitura completa do arquivo para a memoria
    FILE *fp = fopen(arquivo_entrada, "rb");
    if (!fp) {
        perror("Erro ao abrir o arquivo de entrada");
        return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    long tamanho_arquivo = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *texto_bruto = (char *)malloc(tamanho_arquivo + 1);
    if (!texto_bruto) {
        fprintf(stderr, "Erro de alocacao de memoria para leitura.\n");
        fclose(fp);
        return EXIT_FAILURE;
    }

    size_t bytes_lidos = fread(texto_bruto, 1, tamanho_arquivo, fp);
    texto_bruto[bytes_lidos] = '\0';
    fclose(fp);

    // Alocacao inicial do vetor dinamico
    size_t capacidade = 10000000;
    int *vetor = (int *)malloc(capacidade * sizeof(int));
    size_t total_elementos = 0;

    // Conversao rapida do buffer de texto para inteiros
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
    free(texto_bruto);

    // Buffers auxiliares alocados previamente para nao interferir no cronometro
    int *buffer_aux = (int *)malloc(total_elementos * sizeof(int));
    uint32_t *freq  = (uint32_t *)malloc(MAX_COUNTING_RANGE * sizeof(uint32_t));

    printf("Arquivo carregado: %s\n", arquivo_entrada);
    printf("Total de elementos: %zu\n", total_elementos);

    // Medicao isolada do tempo de ordenacao
    clock_t inicio = clock();

    ordenarVetor(vetor, total_elementos, buffer_aux, freq);

    clock_t fim = clock();

    double tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("Tempo de ordenação: %f segundos (%f ms)\n", tempo_gasto, tempo_gasto * 1000.0);

    // Validacao da ordenacao
    if (verificarCorretude(vetor, total_elementos)) {
        printf("Status: Sucesso! Vetor ordenado corretamente.\n");
        printf("Primeiro elemento: %d | Ultimo elemento: %d\n", vetor[0], vetor[total_elementos - 1]);
    } else {
        printf("Status: Erro na ordenacao do vetor.\n");
    }

    // Liberacao de memoria
    free(freq);
    free(buffer_aux);
    free(vetor);

    return EXIT_SUCCESS;
}
