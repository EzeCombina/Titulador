#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static char *TAG = "NVS_TEST";

static esp_err_t erase_nvs() {

    esp_err_t error;
    error = nvs_flash_erase();

    if(error != ESP_OK)
    {
        ESP_LOGI(TAG, "Error en el borrado de la memoria");
    }else{ESP_LOGI(TAG, "Borrado de la memoria realizado correctamente");}

    return error;

}

void app_main(void)
{

    erase_nvs();
    
}