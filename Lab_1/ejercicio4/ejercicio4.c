#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "esp_cpu.h"
#include "esp_heap_caps.h"

#define MAX_SIZE 1000

DRAM_ATTR static int vector_dram[MAX_SIZE];
DRAM_ATTR static int result_dram[MAX_SIZE];
DRAM_ATTR static int num_dram = 5;

IRAM_ATTR static int vector_iram[MAX_SIZE];
IRAM_ATTR static int result_iram[MAX_SIZE];
IRAM_ATTR static int num_iram = 5;

RTC_DATA_ATTR static int vector_rtc[MAX_SIZE];
RTC_DATA_ATTR static int result_rtc[MAX_SIZE];
RTC_DATA_ATTR static int num_rtc = 5;

// Flash no se puede escribir, se deja fijo
const __attribute__((section(".rodata"))) int vector_flash[MAX_SIZE] = {
    [0 ... MAX_SIZE-1] = 1  // GCC extension para inicializar todo a 1
};
const __attribute__((section(".rodata"))) int num_flash = 5;

void multiply_vector_scalar(const int *vector, int num, int *result, int size) {
    for (int i = 0; i < size; i++) {
        result[i] = vector[i] * num;
    }
}

void init_vector(int *v, int size) {
    for (int i = 0; i < size; i++) v[i] = i + 1;
}

#define MEASURE(label, vec, num, res, size)                                         \
    do {                                                                             \
        uint32_t _s = esp_cpu_get_cycle_count();                                    \
        multiply_vector_scalar((vec), (num), (res), (size));                        \
        uint32_t _e = esp_cpu_get_cycle_count();                                    \
        float cpb = (float)(_e - _s) / ((size) * sizeof(int));                     \
        printf("%-20s size: %4d | cycles: %6lu | cycles/byte: %.4f\n",             \
               label, size, (unsigned long)(_e - _s), cpb);                        \
    } while(0)

void app_main() {

    int sizes[] = {10, 20, 50, 100, 200, 500, 1000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < num_sizes; s++) {
        int sz = sizes[s];

        printf("\n=== SIZE: %d ===\n", sz);

        // --- ESTÁTICAS ---
        init_vector(vector_dram, sz);
        MEASURE("DRAM estática",  vector_dram, num_dram, result_dram, sz);

        init_vector(vector_iram, sz);
        MEASURE("IRAM estática",  vector_iram, num_iram, result_iram, sz);

        init_vector(vector_rtc, sz);
        MEASURE("RTC estática",   vector_rtc,  num_rtc,  result_rtc,  sz);

        int result_flash[MAX_SIZE];
        MEASURE("FLASH (rodata)", vector_flash, num_flash, result_flash, sz);

        // --- DINÁMICAS ---
        int *v_dram = (int *)malloc(sz * sizeof(int));
        int *r_dram = (int *)malloc(sz * sizeof(int));
        init_vector(v_dram, sz);
        MEASURE("DRAM dinámica",  v_dram, 5, r_dram, sz);
        free(v_dram); free(r_dram);

        int *v_iram = (int *)heap_caps_malloc(sz * sizeof(int), MALLOC_CAP_EXEC);
        int *r_iram = (int *)heap_caps_malloc(sz * sizeof(int), MALLOC_CAP_EXEC);
        if (v_iram && r_iram) {
            init_vector(v_iram, sz);
            MEASURE("IRAM dinámica", v_iram, 5, r_iram, sz);
            heap_caps_free(v_iram); heap_caps_free(r_iram);
        }

        int *v_psram = (int *)heap_caps_malloc(sz * sizeof(int), MALLOC_CAP_SPIRAM);
        int *r_psram = (int *)heap_caps_malloc(sz * sizeof(int), MALLOC_CAP_SPIRAM);
        if (v_psram && r_psram) {
            init_vector(v_psram, sz);
            MEASURE("PSRAM dinámica", v_psram, 5, r_psram, sz);
            heap_caps_free(v_psram); heap_caps_free(r_psram);
        }
    }
}