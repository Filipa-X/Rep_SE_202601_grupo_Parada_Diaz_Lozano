#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define N_TRIALS_DENSE  10  
#define N_TRIALS_CONV    3   

//Ancho de Banda por Memoria
static float measure_memory_bw(int n_bytes)
{
    float *src = (float *)malloc(n_bytes);
    float *dst = (float *)malloc(n_bytes);
    if (!src || !dst) {
        printf("[ERROR] Sin memoria para BW test\n");
        free(src); free(dst);
        return -1.0f;
    }

    for (int i = 0; i < n_bytes / 4; i++) src[i] = (float)i;

    int64_t t0 = esp_timer_get_time();
    memcpy(dst, src, n_bytes);
    int64_t t1 = esp_timer_get_time();

    free(src); free(dst);

    float us   = (float)(t1 - t0);
    float mb_s = (n_bytes / 1e6f) / (us / 1e6f);
    return mb_s;
}

//Dense: y = W * x (multiplicación matriz por vector, float32)
static float measure_dense(int in_f, int out_f)
{
    float *W = (float *)malloc(in_f * out_f * sizeof(float));
    float *x = (float *)malloc(in_f          * sizeof(float));
    float *y = (float *)malloc(out_f         * sizeof(float));

    if (!W || !x || !y) {
        printf("[ERROR] Sin memoria Dense %d->%d\n", in_f, out_f);
        free(W); free(x); free(y);
        return -1.0f;
    }

    for (int i = 0; i < in_f * out_f; i++) W[i] = 0.01f * (i % 100);
    for (int i = 0; i < in_f;         i++) x[i] = 0.5f;

    int64_t total = 0;
    for (int t = 0; t < N_TRIALS_DENSE; t++) {
        int64_t t0 = esp_timer_get_time();
        for (int o = 0; o < out_f; o++) {
            float sum = 0.0f;
            const float *row = &W[o * in_f];
            for (int i = 0; i < in_f; i++) sum += row[i] * x[i];
            y[o] = sum;
        }
        total += esp_timer_get_time() - t0;
    }

    free(W); free(x); free(y);
    return (float)total / N_TRIALS_DENSE; 
}

//Conv2D: kernel 3x3, padding same, float32
static float measure_conv2d(int H, int W, int IC, int OC)
{
    const int K = 3, PAD = 1;
    float *inp = (float *)malloc(H * W * IC       * sizeof(float));
    float *out = (float *)malloc(H * W * OC       * sizeof(float));
    float *ker = (float *)malloc(OC * IC * K * K  * sizeof(float));

    if (!inp || !out || !ker) {
        printf("[ERROR] Sin memoria Conv %dx%d IC=%d OC=%d\n", H, W, IC, OC);
        free(inp); free(out); free(ker);
        return -1.0f;
    }

    for (int i = 0; i < H * W * IC;      i++) inp[i] = 0.5f;
    for (int i = 0; i < OC * IC * K * K; i++) ker[i] = 0.01f;

    int64_t total = 0;
    for (int t = 0; t < N_TRIALS_CONV; t++) {
        int64_t t0 = esp_timer_get_time();
        for (int oc = 0; oc < OC; oc++) {
            for (int row = 0; row < H; row++) {
                for (int col = 0; col < W; col++) {
                    float sum = 0.0f;
                    for (int ic = 0; ic < IC; ic++) {
                        for (int kr = 0; kr < K; kr++) {
                            int ir = row + kr - PAD;
                            if (ir < 0 || ir >= H) continue;
                            for (int kc = 0; kc < K; kc++) {
                                int ic2 = col + kc - PAD;
                                if (ic2 < 0 || ic2 >= W) continue;
                                sum += inp[(ir * W + ic2) * IC + ic]
                                     * ker[((oc * IC + ic) * K + kr) * K + kc];
                            }
                        }
                    }
                    out[(row * W + col) * OC + oc] = sum;
                }
            }
        }
        total += esp_timer_get_time() - t0;
    }

    free(inp); free(out); free(ker);
    return (float)total / N_TRIALS_CONV;   
}

//main
void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));   
    printf("Medicion de operaciones\n");

    //1. Ancho de Banda de Memoria
    printf("-- Ancho de banda de memoria SRAM --\n");
    float bw = measure_memory_bw(32 * 1024);
    if (bw > 0) printf("   memcpy 32 KB : %.2f MB/s\n", bw);
    printf("\n");

    //2. Dense
    printf("-- Dense (mat-vec): entrada -> salida  |  tiempo(us)  |  MFLOPS --\n");

    //Capas densas de los 5 modelos
    int dense[][2] = {
        {12288, 128},   //FC_Only:  Flatten(64x64x3) -> Dense(128) 
        {128,    64},   //FC_Only:  Dense(128) -> Dense(64)        
        {64,      2},   //salida de FC_Only                        
        {8192,    2},   //Tiny_CNN: Flatten -> Dense(2)            
        {8192,   64},   //CNN_FC:   Flatten -> Dense(64)          
        {64,     32},   //CNN_FC:   Dense(64) -> Dense(32)         
        {32,      2},   // Deep_CNN: Dense(32) -> Dense(2)         
    };
    int nd = sizeof(dense) / sizeof(dense[0]);

    for (int i = 0; i < nd; i++) {
        int in_f = dense[i][0], out_f = dense[i][1];
        float t = measure_dense(in_f, out_f);
        if (t < 0) continue;
        float mflops = (2.0f * in_f * out_f) / (t * 1e-6f) / 1e6f;
        printf("   %6d -> %-5d  |  %10.1f  |  %8.2f\n", in_f, out_f, t, mflops);
    }

    // 3. Conv2D
    printf("\n-- Conv2D 3x3 same: HxW IC->OC  |  tiempo(us)  |  MFLOPS --\n");

    //{H, W, IC, OC}: tamaños representativos de los modelos
    int conv[][4] = {
        {64, 64,  3,  8},   //imagen original, 8 filtros  
        {64, 64,  3, 16},   //imagen original, 16 filtros 
        {32, 32,  8, 16},   //tras 1 MaxPool               
        {32, 32, 16, 32},   //tras 1 MaxPool, 32 filtros   
        {16, 16, 16, 32},   //tras 2 MaxPool               
        { 8,  8, 32, 32},   //tras 3 MaxPool               
    };
    int nc = sizeof(conv) / sizeof(conv[0]);

    for (int i = 0; i < nc; i++) {
        int H = conv[i][0], W = conv[i][1], IC = conv[i][2], OC = conv[i][3];
        float t = measure_conv2d(H, W, IC, OC);
        if (t < 0) continue;
        float flops  = 2.0f * H * W * 3 * 3 * IC * OC;
        float mflops = flops / (t * 1e-6f) / 1e6f;
        printf("   %2dx%2d IC=%2d OC=%2d  |  %10.1f  |  %8.2f\n",
               H, W, IC, OC, t, mflops);
    }
    printf("Listo\n");
}
