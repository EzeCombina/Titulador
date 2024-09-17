#ifndef ADC_H
#define ADC_H

/*==================[Inclusiones]============================================*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_err.h"

/*==================[Definiciones]===================================*/

#define ADC_WIDTH           12                 // Puede ser un valor entre 9 (0 – 511) y 12 bits (0 – 4095).
#define ADC_CHANNEL         ADC1_CHANNEL_0     // GPIO34 (ADC1_CHANNEL_6) en ESP32 
#define ADC_ATTEN           ADC_ATTEN_DB_12
#define SAMPLES             64
#define BITS8               255
#define BITS9               511
#define BITS12              4095
#define T_MOTOR             24.0

typedef struct
{
    float lectura_PH4; 
    float lectura_PH7; 
    float lectura_PH11; 
    float lectura;
}valoresPH;

void adc_init();
float adc_calibracion();
float adc_valor();

#endif