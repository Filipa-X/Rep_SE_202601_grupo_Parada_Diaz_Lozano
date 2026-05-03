#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"

//Conguración Punto de Acceso
#define AP_SSID     "AutoESP32"      
#define AP_PASS     "12345678"    
#define AP_CHANNEL  1
#define AP_MAX_CONN 4

//Pines
#define MOTOR_IZQ_IN1   GPIO_NUM_38
#define MOTOR_IZQ_IN2   GPIO_NUM_39
#define MOTOR_DER_IN3   GPIO_NUM_40
#define MOTOR_DER_IN4   GPIO_NUM_41

static const char *TAG = "AUTO_WEB";

//Comandos de Motor
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

//Página HTML
static const char *HTML_PAGE =
"<!DOCTYPE html><html lang='es'><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Control Auto ESP32</title>"
"<style>"
"  * { box-sizing: border-box; margin: 0; padding: 0; }"
"  body { background: #1a1a2e; display: flex; flex-direction: column;"
"         align-items: center; justify-content: center;"
"         min-height: 100vh; font-family: Arial, sans-serif; }"
"  h1 { color: #e0e0e0; margin-bottom: 30px; font-size: 1.5rem; letter-spacing: 2px; }"
"  #status { color: #00d4ff; font-size: 1.1rem; margin-bottom: 25px;"
"            font-weight: bold; min-height: 1.5em; }"
"  .grid { display: grid; grid-template-columns: repeat(3, 90px);"
"          grid-template-rows: repeat(3, 90px); gap: 10px; }"
"  .btn { background: #16213e; border: 2px solid #0f3460; border-radius: 12px;"
"         color: #e0e0e0; font-size: 2rem; cursor: pointer;"
"         display: flex; align-items: center; justify-content: center;"
"         user-select: none; transition: background 0.1s, transform 0.1s; }"
"  .btn:active, .btn.pressed {"
"         background: #0f3460; border-color: #00d4ff;"
"         transform: scale(0.95); color: #00d4ff; }"
"  .empty { visibility: hidden; }"
"</style></head><body>"
"<h1>Control Auto ESP32</h1>"
"<div id='status'>Listo</div>"
"<div class='grid'>"
"  <div class='empty'></div>"
"  <div class='btn' id='btn-adelante'"
"       onmousedown=\"send('adelante')\" onmouseup=\"send('stop')\""
"       ontouchstart=\"send('adelante')\" ontouchend=\"send('stop')\">▲</div>"
"  <div class='empty'></div>"
"  <div class='btn' id='btn-izquierda'"
"       onmousedown=\"send('izquierda')\" onmouseup=\"send('stop')\""
"       ontouchstart=\"send('izquierda')\" ontouchend=\"send('stop')\">◀</div>"
"  <div class='btn' id='btn-stop'"
"       onmousedown=\"send('stop')\" ontouchstart=\"send('stop')\">⏹</div>"
"  <div class='btn' id='btn-derecha'"
"       onmousedown=\"send('derecha')\" onmouseup=\"send('stop')\""
"       ontouchstart=\"send('derecha')\" ontouchend=\"send('stop')\">▶</div>"
"  <div class='empty'></div>"
"  <div class='btn' id='btn-atras'"
"       onmousedown=\"send('atras')\" onmouseup=\"send('stop')\""
"       ontouchstart=\"send('atras')\" ontouchend=\"send('stop')\">▼</div>"
"  <div class='empty'></div>"
"</div>"
"<script>"
"const nombres = {adelante:'Adelante',atras:'Atrás',"
"                 izquierda:'Izquierda',derecha:'Derecha',stop:'Stop'};"
"function send(cmd) {"
"  document.getElementById('status').textContent = nombres[cmd] || cmd;"
"  fetch('/cmd?action=' + cmd).catch(()=>{});"
"}"
"// Soporte teclado (para demos rápidas)"
"const keyMap = {ArrowUp:'adelante',ArrowDown:'atras',"
"                ArrowLeft:'izquierda',ArrowRight:'derecha',' ':'stop'};"
"document.addEventListener('keydown', e => {"
"  if (keyMap[e.key]) { e.preventDefault(); send(keyMap[e.key]); }"
"});"
"document.addEventListener('keyup', e => {"
"  if (keyMap[e.key] && keyMap[e.key] !== 'stop') send('stop');"
"});"
"</script></body></html>";

// HANDLER: GET /
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, HTML_PAGE, strlen(HTML_PAGE));
    return ESP_OK;
}

// HANDLER: GET /cmd?action=<accion>
static esp_err_t cmd_handler(httpd_req_t *req) {
    char query[64] = {0};
    char action[16] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "action", action, sizeof(action));
    }

    if      (strcmp(action, "adelante")  == 0) cmd_adelante();
    else if (strcmp(action, "atras")     == 0) cmd_atras();
    else if (strcmp(action, "izquierda") == 0) cmd_izquierda();
    else if (strcmp(action, "derecha")   == 0) cmd_derecha();
    else                                        cmd_stop();

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

//Servido HTTP
static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Error al iniciar el servidor HTTP");
        return NULL;
    }

    httpd_uri_t root_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t cmd_uri = {
        .uri      = "/cmd",
        .method   = HTTP_GET,
        .handler  = cmd_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &cmd_uri);

    ESP_LOGI(TAG, "Servidor HTTP iniciado");
    return server;
}

//Wifi
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Cliente conectado");
        (void)event_data;

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "Cliente desconectado");
        (void)event_data;
    }
}

void init_wifi(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();   

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, NULL, NULL);

    wifi_config_t ap_config = {
        .ap = {
            .ssid           = AP_SSID,
            .ssid_len       = strlen(AP_SSID),
            .channel        = AP_CHANNEL,
            .password       = AP_PASS,
            .max_connection = AP_MAX_CONN,
            .authmode       = WIFI_AUTH_WPA2_PSK,
        },
    };

    if (strlen(AP_PASS) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  Red WiFi creada: \"%s\"", AP_SSID);
    ESP_LOGI(TAG, "  Contraseña:      \"%s\"", AP_PASS);
    ESP_LOGI(TAG, "  Abre en el browser: http://192.168.4.1");
    ESP_LOGI(TAG, "==============================================");

    start_webserver();
}

//GPIO Motores
void init_gpio_motors(void) {
    gpio_config_t io_conf = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MOTOR_IZQ_IN1) |
                        (1ULL << MOTOR_IZQ_IN2) |
                        (1ULL << MOTOR_DER_IN3) |
                        (1ULL << MOTOR_DER_IN4),
    };
    gpio_config(&io_conf);
    cmd_stop();  
}

//main
void app_main(void) {
    ESP_LOGI(TAG, "Iniciando control web del auto...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    init_gpio_motors();
    init_wifi();
    ESP_LOGI(TAG, "Esperando conexión WiFi...");
}
