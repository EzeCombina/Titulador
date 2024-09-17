/*==================[ Inclusiones ]============================================*/
#include "../include/adc.h"
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"

/*==================[Variables]==============================*/

static uint16_t adc_value;
float voltage = 0.0;
int smooth_value = 0;

valoresPH valores;

static const char *TAG_ADC = "ADC";

/*==================[Implementaciones]==============================*/

void adc_init()
{
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH));

    // Configuración del canal y atenuación del ADC
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN));
}

float adc_valor()
{
    // ---Realizar codigo referente a la lectura de la medicion del adc---

    // adc_value = adc1_get_raw(ADC_CHANNEL);

    // // Promedio para mejorar la medición 
    // for(int i = 0; i < SAMPLES; i++)
    // {
    //     smooth_value += adc_value = adc1_get_raw(ADC_CHANNEL);
    // }
    // smooth_value /= SAMPLES;

    // if(smooth_value > BITS12)
    // {
    //     smooth_value = BITS12;
    // }

    // // Recta para modificar el valor a lo que nos interesa 
    // smooth_value *= ((-0.13 * (smooth_value - 2047)) / 2048) + 1.13; 

    // voltage = ((smooth_value)*T_MOTOR) / BITS12;

    // // Imprimir el valor leído 
    // ESP_LOGI(TAG, "Valor Tensión: %.3f", voltage);
    // ESP_LOGI(TAG, "Valor leido: %d\n", smooth_value);

    // // Esperar 100 milisegundos antes de la siguiente lectura
    // vTaskDelay(pdMS_TO_TICKS(500));

    // return smooth_value;
    return 0;
}

float adc_calibracion()
{
    // ---Codigo referente a la lectura de medicion para hacer la calibracion---
    return 0;
}