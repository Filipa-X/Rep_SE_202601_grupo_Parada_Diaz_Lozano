#include <stdio.h>
#include <stdint.h>
#include "esp_timer.h"
#include "esp_cpu.h"

#define X 1000000

volatile int result_0, result_1, result_2, result_3, result_4;

void app_main() {

    int var_1 = 233;
    int var_2 = 128;

    int64_t start_time = esp_timer_get_time();

    uint32_t start_cycles = esp_cpu_get_cycle_count();

    for (int i = 0; i < X; i++) {
        result_0 = var_1 + var_2;
        result_1 = var_1 + 10;
        result_2 = var_1 % var_2;
        result_3 = var_1 * var_2;
        result_4 = var_1 / var_2;
    }

    int64_t end_time = esp_timer_get_time();

    uint32_t end_cycles = esp_cpu_get_cycle_count();

    int64_t total_time = end_time - start_time;
    uint32_t total_cycles = end_cycles - start_cycles;

    float cpu_freq_mhz = 160.0;

    float time_per_iteration = (float)total_time / X;
    float cycles_per_iteration = (float)total_cycles / X;

    float time_from_cycles = total_cycles / (cpu_freq_mhz * 1000000.0);

    if (result_0 == (233 + 128) &&
        result_1 == (233 + 10) &&
        result_2 == (233 % 128) &&
        result_3 == (233 * 128) &&
        result_4 == (233 / 128)) {

        printf("CHECK: PASS\n");
    } else {
        printf("CHECK: FAIL\n");
    }

    printf("Total time (us): %lld\n", total_time);
    printf("Total cycles: %lu\n", total_cycles);
    printf("Time per iteration (us): %f\n", time_per_iteration);
    printf("Cycles per iteration: %f\n", cycles_per_iteration);
    printf("Time from cycles (s): %f\n", time_from_cycles);
}