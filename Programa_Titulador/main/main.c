/**
 * @file main.c
 * @author Ezequiel Combina
 * @brief Agitador, Calibración y Limpieza de la bomba
 * @version 0.1
 * @date 2024-06-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "flash.c"
#include "uart.c"
#include "adc.c"

/*==================[Definiciones]======================*/

#define T_LIMPIEZA_MS       1150
#define T_LIMPIEZA          pdMS_TO_TICKS(T_LIMPIEZA_MS)
#define PROCESADORA         0
#define PROCESADORB         1
#define C_MEDICIONES        10
#define T_MEDICIONES_MS     50
#define T_MEDICIONES        pdMS_TO_TICKS(T_MEDICIONES_MS)
#define PH4                 4.0
#define PH7                 7.0
#define PH11                11.0

// ---PWM---
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (GPIO_NUM_12)                               // Pin de salida del PWM 
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT                            // Resolución del PWM (8 bits)
#define LEDC_DUTY_PERCENT       50                                          // Duty porcentual
#define LEDC_DUTY               ((LEDC_DUTY_PERCENT*254)/100)               // Duty al 50%. (2 ** 8) * 50% = 128
#define LEDC_FREQUENCY          (10000)                                     // Frequency in Hertz. Set frequency at 1 kHz

/*==================[Variables globales]======================*/

gpio_int_type_t P_Agitador = GPIO_NUM_17;
gpio_int_type_t P_Motor    = GPIO_NUM_12;
gpio_int_type_t P_Giro     = GPIO_NUM_27;

static const char *TAG_MAIN = "MAIN";

valoresPH valores_main;              // Valores de tension
RectaRegresion valoresRecta;

// Keys para acceder a los valores guardados en la memoria flash
char *key_pendiente = "Pend";
char *key_ordenada = "Ord";

Limpieza limpieza_main;

/*==================[Handles]==============================*/

QueueHandle_t S_Agitador = NULL;
//SemaphoreHandle_t S_Limpieza = NULL;
QueueHandle_t S_Limpieza = NULL;
QueueHandle_t S_Calibracion = NULL;
nvs_handle_t app_nvs_handle;

/*==================[Prototipos de funciones]======================*/

void TaskAgitador(void *taskParmPtr);
void TaskLimpieza(void *taskParmPtr); 
void TaskCalibracion(void *taskParmPtr);
static void example_ledc_init(void); 

/*==================[Main]======================*/

void app_main(void)
{

    S_Agitador = xQueueCreate(1, sizeof(bool));
    //S_Limpieza = xSemaphoreCreateBinary();
    S_Limpieza = xQueueCreate(1, sizeof(limpieza_main));
    S_Calibracion = xQueueCreate(1, sizeof(char));

    // Set the LEDC peripheral configuration
    example_ledc_init();
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));

    // Iniciar flash 
    ESP_ERROR_CHECK(init_nvs());

    // Lectura de los valores guardados en la UART 
    read_nvs(key_pendiente, &valoresRecta.Pendiente_Guardado);
    read_nvs(key_ordenada, &valoresRecta.Ordenada_Guardado);

    valoresRecta.Pendiente = valoresRecta.Pendiente_Guardado / 1000.0;
    valoresRecta.Ordenada = valoresRecta.Ordenada_Guardado / 1000.0;

    // Iniciar UART
    init_uart();

    // Iniciar ADC
    adc_init();

    BaseType_t err = xTaskCreatePinnedToCore(
        TaskAgitador,                     	// Funcion de la tarea a ejecutar
        "TaskAgitador",   	                // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 		// Cantidad de stack de la tarea
        NULL,                          	    // Parametros de tarea
        tskIDLE_PRIORITY+1,         	    // Prioridad de la tarea -> Queremos que este un nivel encima de IDLE
        NULL,                          		// Puntero a la tarea creada en el sistema
        PROCESADORA                         // Numero de procesador
    );

    // Gestion de errores
    if(err == pdFAIL)
    {
        ESP_LOGI(TAG_MAIN, "Error al crear la tarea.");
        while(1);    // Si no pudo crear la tarea queda en un bucle infinito
    }

    BaseType_t err2 = xTaskCreatePinnedToCore(
        TaskLimpieza,                     	// Funcion de la tarea a ejecutar
        "TaskLimpieza",   	                // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 		// Cantidad de stack de la tarea
        NULL,                          	    // Parametros de tarea
        tskIDLE_PRIORITY+1,         	    // Prioridad de la tarea -> Queremos que este un nivel encima de IDLE
        NULL,                          		// Puntero a la tarea creada en el sistema
        PROCESADORA                         // Numero de procesador
    );

    // Gestion de errores
    if(err2 == pdFAIL)
    {
        ESP_LOGI(TAG_MAIN, "Error al crear la tarea.");
        while(1);    // Si no pudo crear la tarea queda en un bucle infinito
    }

    BaseType_t err3 = xTaskCreatePinnedToCore(
        TaskCalibracion,                    // Funcion de la tarea a ejecutar
        "TaskCalibracion",   	            // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 		// Cantidad de stack de la tarea
        NULL,                          	    // Parametros de tarea
        tskIDLE_PRIORITY+1,         	    // Prioridad de la tarea -> Queremos que este un nivel encima de IDLE
        NULL,                          		// Puntero a la tarea creada en el sistema
        PROCESADORA                         // Numero de procesador
    );

    // Gestion de errores
    if(err3 == pdFAIL)
    {
        ESP_LOGI(TAG_MAIN, "Error al crear la tarea.");
        while(1);    // Si no pudo crear la tarea queda en un bucle infinito
    }

}

/*==================[Implementacion de la tarea]======================*/

void TaskAgitador(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/
    esp_rom_gpio_pad_select_gpio(P_Agitador);
    gpio_set_direction(P_Agitador, GPIO_MODE_OUTPUT);

    bool estado_agitador;

    /*==================[Bucle]======================*/
    while(1)
    {
        xQueueReceive(S_Agitador, &estado_agitador, portMAX_DELAY);
        gpio_set_level(P_Agitador, estado_agitador);
        
        // if(estado_agitador == true)
        // {
        //   ESP_LOGI(TAG_MAIN, "Agitador Prendido\n");
        // }else{ESP_LOGI(TAG_MAIN, "Agitador Apagado\n");}
    } 
}

void TaskLimpieza(void *taskParmPtr)
{
    // ---La limpieza de la bomba va a ser regulabre y no por tiempo---
    // ---Se deben pasar DOS parámetros -> La direccion de giro y el on/off---

    /*==================[Configuraciones]======================*/
    //esp_rom_gpio_pad_select_gpio(P_Motor);
    esp_rom_gpio_pad_select_gpio(P_Giro);
    //gpio_set_direction(P_Motor, GPIO_MODE_OUTPUT);
    gpio_set_direction(P_Giro, GPIO_MODE_OUTPUT);

    TickType_t xPeriodicity = T_LIMPIEZA; 
    TickType_t xLastWakeTime = xTaskGetTickCount();

    /*==================[Bucle]======================*/
    while(1)
    {
        xQueueReceive(S_Limpieza, &limpieza_main, portMAX_DELAY);
        gpio_set_level(P_Giro, limpieza_main.Giro_Limpieza);
        //gpio_set_level(P_Motor, limpieza_main.Habilitador_Limpieza);
        xLastWakeTime = xTaskGetTickCount();
        //ESP_LOGI(TAG_MAIN, "Led Encendido");
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        vTaskDelayUntil(&xLastWakeTime, xPeriodicity);
        ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0));
        //ESP_LOGI(TAG_MAIN, "Led Apagado");
    } 
}

void TaskCalibracion(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/
    //esp_rom_gpio_pad_select_gpio(P_Motor);
    //gpio_set_direction(P_Motor, GPIO_MODE_OUTPUT);

    char estado_calibracion;

    /*==================[Bucle]======================*/
    while(1)
    {
        xQueueReceive(S_Calibracion, &estado_calibracion, portMAX_DELAY);
        switch(estado_calibracion)
        {
            case '1':       // PH 4
                ESP_LOGI(TAG_MAIN, "Calibración PH4");
                // Lectura del valor en V
                //valores.lectura_PH4 = ...;
                break;

            case '2':       // PH 7
                ESP_LOGI(TAG_MAIN, "Calibración PH7");
                // Lectura del valor en V
                //valores.lectura_PH7 = ...;
                break;

            case '3':       // PH 11
                ESP_LOGI(TAG_MAIN, "Calibración PH11");
                // Lectura del valor en V
                //valores.lectura_PH11 = ...;
                break;

            case '4':       // RESULTADO FINAL
                // Guardadr valor en la flash
                // Calculo de recta de regresion 
                ESP_LOGI(TAG_MAIN, "Calculo de recta de regresion");
                valoresRecta.N = 3;
                valoresRecta.MediaPH = (PH4 + PH7 + PH11) / valoresRecta.N;
                //ESP_LOGI(TAG_MAIN, "Media PH -> %f", valoresRecta.MediaPH);
                valoresRecta.MediaLectura = (valores_main.lectura_PH4 + valores_main.lectura_PH7 + valores_main.lectura_PH11) / valoresRecta.N;
                //ESP_LOGI(TAG_MAIN, "Media Lectura -> %f", valoresRecta.MediaLectura);
                valoresRecta.VarianzaLectura = ((pow(valores_main.lectura_PH4, 2) + pow(valores_main.lectura_PH7, 2) + pow(valores_main.lectura_PH11, 2)) / valoresRecta.N) - pow(valoresRecta.MediaLectura, 2);
                //ESP_LOGI(TAG_MAIN, "Varianza Lectura -> %f", valoresRecta.VarianzaLectura);
                valoresRecta.Covarianza = (((valores_main.lectura_PH4 * PH4) + (valores_main.lectura_PH7 * PH7) + (valores_main.lectura_PH11 * PH11)) / valoresRecta.N) - (valoresRecta.MediaPH * valoresRecta.MediaLectura);
                //ESP_LOGI(TAG_MAIN, "Covarianza -> %f", valoresRecta.Covarianza);
                valoresRecta.Pendiente = (valoresRecta.Covarianza / valoresRecta.VarianzaLectura);
                valoresRecta.Ordenada = ((valoresRecta.Covarianza / valoresRecta.VarianzaLectura) * (valoresRecta.MediaLectura * (-1))) + valoresRecta.MediaPH;
                ESP_LOGI(TAG_MAIN, "Pendiente -> %f - Ordenada -> %f", valoresRecta.Pendiente, valoresRecta.Ordenada);

                // Conversion de la pendiente y la ordenada a variables enteras para poder guardarlas en la falsh 

                valoresRecta.Pendiente_Guardado = (int32_t) (valoresRecta.Pendiente * 1000);    // Multiplicamos por 1000 para guardar 3 decimales
                valoresRecta.Ordenada_Guardado = (int32_t) (valoresRecta.Ordenada * 1000);      // En caso de necesitar mayor presicion multiplicar por un numero mas grande 

                // Borrado previo a la escritura (NO Necesario)
                //erase_nvs();

                // Guardado en la memoria flash 
                write_nvs(key_pendiente, valoresRecta.Pendiente_Guardado);
                write_nvs(key_ordenada, valoresRecta.Ordenada_Guardado);
                break;
            
            default:
                break;
        }
    } 
}

static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 1 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}