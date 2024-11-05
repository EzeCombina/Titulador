#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/adc.h"      // Para el ADC
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WIFI_SSID "."
#define WIFI_PASS "43364336"
#define LED1_PIN GPIO_NUM_2    // Pin para el primer LED (modificar si usas otro)
#define LED2_PIN GPIO_NUM_19    // Pin para el segundo LED (modificar si usas otro)

static const char *TAG = "wifi_server";
static httpd_handle_t server = NULL;
static int led1_state = 0; // Estado del LED1 (0 = apagado, 1 = encendido)
static int led2_state = 0; // Estado del LED2 (0 = apagado, 1 = encendido)
static float voltage = 0.0;  // Variable global para almacenar el valor del voltaje

// Función para manejar la solicitud desde la página web
// esp_err_t root_get_handler(httpd_req_t *req) {
//     // Generar HTML con botones para encender/apagar los LEDs
//     char response[1024];
//     snprintf(response, sizeof(response),
//         "<html>"
//         "<h2>ESP32 Servidor WEB</h2>"
//         "<p>Usando Modo Estacion</p>"
//         "<p>LED1 Estado: %s</p>"
//         "<form action=\"/toggle_led1\"><button>%s</button></form>"
//         "<p>LED2 Estado: %s</p>"
//         "<form action=\"/toggle_led2\"><button>%s</button></form>"
//         "</html>",
//         led1_state ? "ON" : "OFF", led1_state ? "OFF" : "ON",
//         led2_state ? "ON" : "OFF", led2_state ? "OFF" : "ON");

//     // Enviar la respuesta HTML
//     httpd_resp_send(req, response, strlen(response));
//     return ESP_OK;
// }

esp_err_t root_get_handler(httpd_req_t *req) {
    // Generar HTML con estilo CSS y JavaScript para actualizar el voltaje
    char response[2048];
    snprintf(response, sizeof(response),
        "<html>"
        "<head>"
        "<style>"
        "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; }"
        "h2 { font-size: 2em; }"
        "p { font-size: 1.5em; }"
        "button { "
        "    display: inline-block; "
        "    width: 80%%; "
        "    padding: 15px; "
        "    font-size: 1.2em; "
        "    margin: 10px auto; "
        "    background-color: #333; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 8px; "
        "    cursor: pointer; "
        "}"
        "button:hover { background-color: #555; }"
        "#voltage-box { "
        "    margin-top: 20px; "
        "    font-size: 1.5em; "
        "    padding: 10px; "
        "    border: 2px solid #333; "
        "    display: inline-block; "
        "    border-radius: 8px; "
        "}"
        "@media (min-width: 600px) {"
        "    button { width: 300px; }"
        "}"
        "</style>"
        "<script>"
        "function updateVoltage() {"
        "    fetch('/voltage').then(response => response.text()).then(data => {"
        "        document.getElementById('voltage-box').innerText = 'Voltaje: ' + data + ' V';"
        "    });"
        "}"
        "setInterval(updateVoltage, 100);" // Tiempo de Act en ms 
        "</script>"
        "</head>"
        "<body>"
        "<h2>ESP32 Servidor WEB</h2>"
        "<p>Usando Modo Estacion</p>"
        "<p>LED1 Estado: %s</p>"
        "<form action=\"/toggle_led1\"><button>%s</button></form>"
        "<p>LED2 Estado: %s</p>"
        "<form action=\"/toggle_led2\"><button>%s</button></form>"
        "<div id='voltage-box'>Voltaje: -- V</div>"
        "</body>"
        "</html>",
        led1_state ? "ON" : "OFF", led1_state ? "OFF" : "ON",
        led2_state ? "ON" : "OFF", led2_state ? "OFF" : "ON");

    // Enviar la respuesta HTML
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// esp_err_t root_get_handler(httpd_req_t *req) {
//     // Generar HTML con estilo CSS y JavaScript para actualizar el voltaje y graficarlo
//     char response[4096];
//     snprintf(response, sizeof(response),
//         "<html>"
//         "<head>"
//         "<style>"
//         "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; }"
//         "h2 { font-size: 2em; }"
//         "p { font-size: 1.5em; }"
//         "button { "
//         "    display: inline-block; "
//         "    width: 80%%; "
//         "    padding: 15px; "
//         "    font-size: 1.2em; "
//         "    margin: 10px auto; "
//         "    background-color: #333; "
//         "    color: white; "
//         "    border: none; "
//         "    border-radius: 8px; "
//         "    cursor: pointer; "
//         "}"
//         "button:hover { background-color: #555; }"
//         "#voltage-box, #chart-container { "
//         "    margin-top: 20px; "
//         "    font-size: 1.5em; "
//         "    padding: 10px; "
//         "    border: 2px solid #333; "
//         "    display: inline-block; "
//         "    border-radius: 8px; "
//         "}"
//         "@media (min-width: 600px) {"
//         "    button { width: 300px; }"
//         "}"
//         "</style>"
//         "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>"
//         "<script>"
//         "let voltageData = [];"
//         "let timeLabels = [];"
//         "let chart;"

//         "function updateVoltage() {"
//         "    fetch('/voltage').then(response => response.text()).then(data => {"
//         "        document.getElementById('voltage-box').innerText = 'Voltaje: ' + data + ' V';"
//         "    });"
//         "}"

//         "function startGraphing() {"
//         "    voltageData = [];"
//         "    timeLabels = [];"
//         "    let startTime = new Date();"

//         "    const interval = setInterval(() => {"
//         "        fetch('/voltage').then(response => response.text()).then(data => {"
//         "            let elapsed = (new Date() - startTime) / 1000;"
//         "            timeLabels.push(elapsed.toFixed(1));"
//         "            voltageData.push(parseFloat(data));"
//         "            chart.update();"
//         "            if (elapsed >= 60) {"
//         "                clearInterval(interval);"
//         "            }"
//         "        });"
//         "    }, 1000);"
//         "}"

//         "window.onload = function() {"
//         "    const ctx = document.getElementById('voltageChart').getContext('2d');"
//         "    chart = new Chart(ctx, {"
//         "        type: 'line',"
//         "        data: {"
//         "            labels: timeLabels,"
//         "            datasets: [{"
//         "                label: 'Voltaje en el tiempo',"
//         "                data: voltageData,"
//         "                fill: false,"
//         "                borderColor: 'blue',"
//         "                tension: 0.1"
//         "            }]"
//         "        },"
//         "        options: {"
//         "            scales: {"
//         "                x: { title: { display: true, text: 'Tiempo (s)' } },"
//         "                y: { title: { display: true, text: 'Voltaje (V)' }, min: 0, max: 3.3 }"
//         "            }"
//         "        }"
//         "    });"
//         "}"
//         "</script>"
//         "</head>"
//         "<body>"
//         "<h2>ESP32 Servidor WEB</h2>"
//         "<p>Usando Modo Estacion</p>"
//         "<p>LED1 Estado: %s</p>"
//         "<form action=\"/toggle_led1\"><button>%s</button></form>"
//         "<p>LED2 Estado: %s</p>"
//         "<form action=\"/toggle_led2\"><button>%s</button></form>"
//         "<div id='voltage-box'>Voltaje: -- V</div>"
//         "<button onclick=\"startGraphing()\">Iniciar Gráfica de Voltaje</button>"
//         "<div id='chart-container'>"
//         "    <canvas id='voltageChart' width='400' height='200'></canvas>"
//         "</div>"
//         "</body>"
//         "</html>",
//         led1_state ? "ON" : "OFF", led1_state ? "OFF" : "ON",
//         led2_state ? "ON" : "OFF", led2_state ? "OFF" : "ON");

//     // Enviar la respuesta HTML
//     httpd_resp_send(req, response, strlen(response));
//     return ESP_OK;
// }

// Función para alternar el estado del LED1
esp_err_t toggle_led1_handler(httpd_req_t *req) {
    led1_state = !led1_state;
    gpio_set_level(LED1_PIN, led1_state);
    return root_get_handler(req);
}

// Función para alternar el estado del LED2
esp_err_t toggle_led2_handler(httpd_req_t *req) {
    led2_state = !led2_state;
    gpio_set_level(LED2_PIN, led2_state);
    return root_get_handler(req);
}

// Manejador para enviar el valor de voltaje
esp_err_t voltage_get_handler(httpd_req_t *req) {
    // Crear la respuesta con el voltaje actual
    char response[32];
    snprintf(response, sizeof(response), "%.2f", voltage);
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Iniciar servidor web y registrar manejadores de URI
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;  // Asignar más memoria de pila (valor en bytes)

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_get = {
            .uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_get);

        httpd_uri_t uri_toggle_led1 = {
            .uri = "/toggle_led1", .method = HTTP_GET, .handler = toggle_led1_handler, .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_toggle_led1);

        httpd_uri_t uri_toggle_led2 = {
            .uri = "/toggle_led2", .method = HTTP_GET, .handler = toggle_led2_handler, .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_toggle_led2);
    
        httpd_uri_t uri_root = {
            .uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_root);

        httpd_uri_t uri_voltage = {
            .uri = "/voltage", .method = HTTP_GET, .handler = voltage_get_handler, .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_voltage);
    }
    return server;
}

// Manejo de eventos Wi-Fi
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Intentando reconectar...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado con IP: " IPSTR, IP2STR(&event->ip_info.ip));
        start_webserver();  // Iniciar el servidor web al obtener IP
    }
}

// Inicializar Wi-Fi
void wifi_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}

// Configuración inicial de los LEDs
void init_leds(void) {
    gpio_set_direction(LED1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED1_PIN, led1_state);
    gpio_set_level(LED2_PIN, led2_state);
}

// Tarea para leer el ADC de manera constante
void adc_task(void *pvParameters) {
    while (1) {
        // Leer el valor del pin analógico (GPIO34, ADC1_CHANNEL_6)
        int adc_reading = adc1_get_raw(ADC1_CHANNEL_6);

        // Convertir el valor leído (0-4095) a un valor de tensión (0-3.3V)
        voltage = (adc_reading / 4095.0) * 3.3;

        // Imprimir el voltaje para depuración
        ESP_LOGI(TAG, "Voltaje leído: %.2f V", voltage);

        // Esperar 500 ms antes de leer de nuevo
        vTaskDelay(pdMS_TO_TICKS(450)); // Leer cada 450 ms
    }
}

void app_main(void) {
    // Inicializar almacenamiento no volátil
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    adc1_config_width(ADC_WIDTH_BIT_12); // Resolución de 12 bits
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // Atenuación para rango de 0-3.3V

    // Inicializar los LEDs
    init_leds();

    // Iniciar Wi-Fi
    wifi_init();

    xTaskCreate(adc_task, "adc_task", 2048, NULL, 5, NULL);
}