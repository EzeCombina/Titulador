#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

uint8_t ValorAct = 0;

#define T 1000
static char *TAG = "NVS_TEST";

nvs_handle_t app_nvs_handle;

static esp_err_t init_nvs(void) {

    esp_err_t error;
    nvs_flash_init();

    error = nvs_open(TAG, NVS_READWRITE, &app_nvs_handle);

    if(error != ESP_OK)
    {
        ESP_LOGI(TAG, "Error en la apertura");
    }else{ESP_LOGI(TAG, "Apertura correcta");}

    return error;

}

static esp_err_t read_nvs(char *key, uint8_t *Valor) {

    esp_err_t error;
    error = nvs_get_u8(app_nvs_handle, key, Valor);

    if(error != ESP_OK)
    {
        ESP_LOGI(TAG, "Error en la lectura");
    }else{ESP_LOGI(TAG, "Lectura correcta -> %u", *Valor);}
    
    return error;

}

static esp_err_t write_nvs(char *key, uint8_t Valor) {

    esp_err_t error;
    error = nvs_set_u8(app_nvs_handle, key, Valor);

    if(error != ESP_OK)
    {
        ESP_LOGI(TAG, "Error en la escritura");
    }else{ESP_LOGI(TAG, "Escritura correcta -> %u", Valor);}
    
    return error;

}

void app_main(void) {
    
    char *key = "ValorNuevo";
    ESP_ERROR_CHECK(init_nvs());

    read_nvs(key, &ValorAct);

    while(1)
    {
        ValorAct++;
        vTaskDelay(T / portTICK_PERIOD_MS);
        write_nvs(key, ValorAct);
    }

    /*
    
    El valor de la variable "ValorAct" se va a guardar 
    en la memoria flash mientras vaya cambiando. Cuando se 
    desconecte el ESP32 el ultimo valor de la misma quedara
    en la memoria, cuando se vuelva a conectar la variable
    "ValorAct" tamará el valor guardado mediante la "key"

    */

}
