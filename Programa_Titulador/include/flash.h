#ifndef FLASH_H
#define FLASH_H

/*==================[Inclusiones]============================================*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "nvs_flash.h"

/*==================[Definiciones]===================================*/

typedef struct
{
    int N;
    float MediaPH; 
    float MediaLectura; 
    float VarianzaLectura; 
    float Covarianza; 
    float Pendiente;
    float Ordenada;
    int32_t Pendiente_Guardado;     // --VER MANERA DE CAMBIAR ESTO-- 
    int32_t Ordenada_Guardado; 
}RectaRegresion;

/*==================[Prototipos de funciones]======================*/

esp_err_t init_nvs(void);
esp_err_t read_nvs(char *key, int32_t *Valor);
esp_err_t write_nvs(char *key, int32_t Valor);
esp_err_t erase_nvs();

#endif