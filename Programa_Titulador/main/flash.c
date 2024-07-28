/*==================[ Inclusiones ]============================================*/
#include "../include/flash.h"
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"

/*==================[ Definiciones ]===================================*/



/*==================[Prototipos de funciones]======================*/



/*==================[Variables]==============================*/

static char *TAG_NVS = "NVS";

extern nvs_handle_t app_nvs_handle;

/*==================[Implementaciones]=================================*/

/**
 * @brief Inicializacion de la memoria flash
 * 
 * @return esp_err_t 
 */
esp_err_t init_nvs(void) {

    esp_err_t error;
    nvs_flash_init();

    error = nvs_open(TAG_NVS, NVS_READWRITE, &app_nvs_handle);

    if(error != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error en la apertura");
    }else{ESP_LOGI(TAG_NVS, "Apertura correcta");}

    return error;

}

/**
 * @brief Lectura de la memoria flash
 * 
 * @param key 
 * @param Valor 
 * @return esp_err_t 
 */
esp_err_t read_nvs(char *key, int32_t *Valor) {

    esp_err_t error;
    error = nvs_get_i32(app_nvs_handle, key, Valor);

    if(error != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error en la lectura");
    }else{ESP_LOGI(TAG_NVS, "Lectura correcta");}
    
    return error;

}

/**
 * @brief Escritura de la memoria flash
 * 
 * @param key 
 * @param Valor 
 * @return esp_err_t 
 */
esp_err_t write_nvs(char *key, int32_t Valor) {

    esp_err_t error;
    error = nvs_set_i32(app_nvs_handle, key, Valor);

    if(error != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error en la escritura");
    }else{ESP_LOGI(TAG_NVS, "Escritura correcta");}
    
    return error;

}

/**
 * @brief Borrar datos guardados en la flash 
 * 
 * @return esp_err_t 
 */
esp_err_t erase_nvs() {

    esp_err_t error;
    error = nvs_flash_erase();

    if(error != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error en el borrado de la memoria");
    }else{ESP_LOGI(TAG_NVS, "Borrado de la memoria realizado correctamente");}

    return error;

}