#ifndef FLASH_H
#define FLASH_H

/*==================[Inclusiones]============================================*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "nvs_flash.h"

/*==================[Definiciones]===================================*/



/*==================[Prototipos de funciones]======================*/

esp_err_t init_nvs(void);
esp_err_t read_nvs(char *key, float *Valor);
esp_err_t write_nvs(char *key, float Valor);
esp_err_t erase_nvs();

#endif