#ifndef ADC_H
#define ADC_H

/*==================[Inclusiones]============================================*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_err.h"

/*==================[Definiciones]===================================*/

#define DIRECTO         0
#define FILTRADO        1
#define CONVERSION      2
#define GRAFICA         3

#define ADC_WIDTH           12                  // Puede ser un valor entre 9 (0 – 511) y 12 bits (0 – 4095).
#define FILTRO              1000
#define ADC_CHANNEL         ADC1_CHANNEL_6      // Corresponde al GPIO 32
#define ADC_ATTEN           ADC_ATTEN_DB_11     // Atenuación de 12 dB en el voltaje de entrada para usar el ADC
#define MUESTRAS            64                  // Muestras para obtener un promedio del valor del ADC
#define T_PH                14.0
#define BITS12              4096
#define T_MAX_ADC           3.3

/*==================[Variables]===================================*/

typedef struct
{
    float lectura_PH4; 
    float lectura_PH7; 
    float lectura_PH10; 
    float lectura;
}valoresPH;

typedef struct
{
    int N;
    float Sum_X;
    float Sum_Y;
    float MediaX;
    float MediaY; 
    float Suma_XY;
    float Suma_XSquare;
    float Pendiente;
    float Ordenada;
    int32_t Pendiente_Guardado;     // --VER MANERA DE CAMBIAR ESTO-- 
    int32_t Ordenada_Guardado; 
}RectaRegresion;

/*==================[Prototipos de funciones]======================*/

void adc_init();
void lectura(char valor_actual);
void adc_calibracion();
void volumen(float *ptr);

#endif