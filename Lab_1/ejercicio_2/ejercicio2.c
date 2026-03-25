#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "image.h"

#define SIZE 96

uint8_t output[SIZE][SIZE];

void app_main() {

    int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };

    int Gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    for (int i = 1; i < SIZE - 1; i++) {
        for (int j = 1; j < SIZE - 1; j++) {

            int sumX = 0;
            int sumY = 0;

            for (int ki = -1; ki <= 1; ki++) {
                for (int kj = -1; kj <= 1; kj++) {
                    int pixel = image[i + ki][j + kj];

                    sumX += pixel * Gx[ki + 1][kj + 1];
                    sumY += pixel * Gy[ki + 1][kj + 1];
                }
            }

            int magnitude = abs(sumX) + abs(sumY);

            if (magnitude > 255) magnitude = 255;

            output[i][j] = (uint8_t)magnitude;
        }
    }
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", output[i][j]);
        }
        printf("\n");
    }
}