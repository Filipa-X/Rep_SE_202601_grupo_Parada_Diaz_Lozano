#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_dsp.h"

#define MIC_ADC_CHANNEL     ADC_CHANNEL_3  
#define MOTOR_IZQ_IN1       GPIO_NUM_17
#define MOTOR_IZQ_IN2       GPIO_NUM_16
#define MOTOR_DER_IN3       GPIO_NUM_7
#define MOTOR_DER_IN4       GPIO_NUM_6

#define FREQ_ADELANTE       1000
#define FREQ_ATRAS          2000
#define FREQ_IZQUIERDA      3000
#define FREQ_DERECHA        4000
#define FREQ_TOLERANCIA     499

#define SAMPLE_RATE         8000
#define FFT_SIZE            512

static const char *TAG = "AUDIO_CAR";

static float samples[FFT_SIZE];
static float wind[FFT_SIZE];
static float fft_buf[FFT_SIZE * 2];

adc_oneshot_unit_handle_t adc_handle;

void init_adc(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    adc_oneshot_config_channel(adc_handle, MIC_ADC_CHANNEL, &config);
}

void init_gpio_motors(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MOTOR_IZQ_IN1) |
                        (1ULL << MOTOR_IZQ_IN2) |
                        (1ULL << MOTOR_DER_IN3) |
                        (1ULL << MOTOR_DER_IN4)
    };
    gpio_config(&io_conf);
}

void cmd_adelante(void) {
    ESP_LOGI(TAG, "ADELANTE");
    gpio_set_level(MOTOR_IZQ_IN1, 1); gpio_set_level(MOTOR_IZQ_IN2, 0);
    gpio_set_level(MOTOR_DER_IN3, 1); gpio_set_level(MOTOR_DER_IN4, 0);
}

void cmd_atras(void) {
    ESP_LOGI(TAG, "ATRAS");
    gpio_set_level(MOTOR_IZQ_IN1, 0); gpio_set_level(MOTOR_IZQ_IN2, 1);
    gpio_set_level(MOTOR_DER_IN3, 0); gpio_set_level(MOTOR_DER_IN4, 1);
}

void cmd_izquierda(void) {
    ESP_LOGI(TAG, "IZQUIERDA");
    gpio_set_level(MOTOR_IZQ_IN1, 0); gpio_set_level(MOTOR_IZQ_IN2, 1);
    gpio_set_level(MOTOR_DER_IN3, 1); gpio_set_level(MOTOR_DER_IN4, 0);
}

void cmd_derecha(void) {
    ESP_LOGI(TAG, "DERECHA");
    gpio_set_level(MOTOR_IZQ_IN1, 1); gpio_set_level(MOTOR_IZQ_IN2, 0);
    gpio_set_level(MOTOR_DER_IN3, 0); gpio_set_level(MOTOR_DER_IN4, 1);
}

void cmd_stop(void) {
    ESP_LOGI(TAG, "STOP");
    gpio_set_level(MOTOR_IZQ_IN1, 0); gpio_set_level(MOTOR_IZQ_IN2, 0);
    gpio_set_level(MOTOR_DER_IN3, 0); gpio_set_level(MOTOR_DER_IN4, 0);
}

void sample_audio(float *buf, int n, int sample_rate_hz) {
    int delay_us = 1000000 / sample_rate_hz;

    for (int i = 0; i < n; i++) {
        int raw = 0;
        adc_oneshot_read(adc_handle, MIC_ADC_CHANNEL, &raw);

        buf[i] = (float)raw - 2048.0f;
        esp_rom_delay_us(delay_us);
    }
}

float detect_dominant_frequency(float *buf, int n, int sample_rate) {

    dsps_wind_hann_f32(wind, n);

    for (int i = 0; i < n; i++) {
        buf[i] *= wind[i];
    }

    for (int i = 0; i < n; i++) {
        fft_buf[2*i] = buf[i];
        fft_buf[2*i + 1] = 0.0f;
    }

    dsps_fft2r_fc32(fft_buf, n);
    dsps_bit_rev_fc32(fft_buf, n);

    float max_mag = 0.0f;
    int max_idx = 0;

    for (int i = 1; i < n/2; i++) {
        float re = fft_buf[2*i];
        float im = fft_buf[2*i + 1];
        float mag = sqrtf(re*re + im*im);

        if (mag > max_mag) {   // ✅ FIX IMPORTANTE
            max_mag = mag;
            max_idx = i;
        }
    }

    float freq = (float)max_idx * sample_rate / n;

    ESP_LOGI(TAG, "Freq: %.1f Hz", freq);

    return freq;
}

void execute_command(float freq) {

    if (fabsf(freq - FREQ_ADELANTE) < FREQ_TOLERANCIA) {
        cmd_adelante();

    } else if (fabsf(freq - FREQ_ATRAS) < FREQ_TOLERANCIA) {
        cmd_atras();

    } else if (fabsf(freq - FREQ_IZQUIERDA) < FREQ_TOLERANCIA) {
        cmd_izquierda();

    } else if (fabsf(freq - FREQ_DERECHA) < FREQ_TOLERANCIA) {
        cmd_derecha();

    } else {
        cmd_stop();
    }
}

void audio_control_task(void *pvParam) {

    dsps_fft2r_init_fc32(NULL, FFT_SIZE);

    while (1) {
        sample_audio(samples, FFT_SIZE, SAMPLE_RATE);

        float freq = detect_dominant_frequency(samples, FFT_SIZE, SAMPLE_RATE);

        execute_command(freq);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Sistema de control por audio iniciado");

    init_adc();
    init_gpio_motors();

    xTaskCreate(audio_control_task, "audio_ctrl", 8192, NULL, 5, NULL);
}