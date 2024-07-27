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
#include "flash.c"
#include "uart.c"

/*==================[Definiciones]======================*/

#define T_LIMPIEZA_MS       1000
#define T_LIMPIEZA          pdMS_TO_TICKS(T_LIMPIEZA_MS)
#define PROCESADORA         0
#define PROCESADORB         1
#define C_MEDICIONES        10
#define T_MEDICIONES_MS     50
#define T_MEDICIONES        pdMS_TO_TICKS(T_MEDICIONES_MS)
#define PH4                 4
#define PH7                 7
#define PH11                11

/*==================[Variables globales]======================*/

gpio_int_type_t P_Agitador = GPIO_NUM_25;
gpio_int_type_t P_Motor    = GPIO_NUM_26;

static const char *TAG_MAIN = "MAIN";

valoresPH valores;              // Valores de tension
RectaRegresion valoresRecta;

// Keys para acceder a los valores guardados en la memoria flash
char *key_pendiente = "Pend";
char *key_ordenada = "Ord";

/*==================[Handles]==============================*/

QueueHandle_t S_Agitador = NULL;
SemaphoreHandle_t S_Limpieza = NULL;
QueueHandle_t S_Calibracion = NULL;
nvs_handle_t app_nvs_handle;

/*==================[Prototipos de funciones]======================*/

void TaskAgitador(void *taskParmPtr);
void TaskLimpieza(void *taskParmPtr); 
void TaskCalibracion(void *taskParmPtr); 

/*==================[Main]======================*/

void app_main(void)
{

    S_Agitador = xQueueCreate(1, sizeof(bool));
    S_Limpieza = xSemaphoreCreateBinary();
    S_Calibracion = xQueueCreate(1, sizeof(char));

    // Iniciar flash 
    ESP_ERROR_CHECK(init_nvs());

    // Lectura de los valores guardados en la UART 
    read_nvs(key_pendiente, &valoresRecta.Pendiente);
    read_nvs(key_ordenada, &valoresRecta.Ordenada);

    // Iniciar UART
    init_uart();

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
        if(estado_agitador == 1)
        {
            gpio_set_level(P_Agitador, 0);
        }else{gpio_set_level(P_Agitador, 1);}
    } 
}

void TaskLimpieza(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/
    esp_rom_gpio_pad_select_gpio(P_Motor);
    gpio_set_direction(P_Motor, GPIO_MODE_OUTPUT);

    TickType_t xPeriodicity = T_LIMPIEZA; 
    TickType_t xLastWakeTime = xTaskGetTickCount();

    /*==================[Bucle]======================*/
    while(1)
    {
        xSemaphoreTake(S_Limpieza, portMAX_DELAY);
        xLastWakeTime = xTaskGetTickCount();
        //ESP_LOGI(TAG_MAIN, "Led Encendido");
        gpio_set_level(P_Motor, 1);
        vTaskDelayUntil(&xLastWakeTime, xPeriodicity);
        gpio_set_level(P_Motor, 0);
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
                // Lectura del valor en V
                //valores.lectura_PH7 = ...;
                break;

            case '3':       // PH 11
                // Lectura del valor en V
                //valores.lectura_PH11 = ...;
                break;

            case '4':       // RESULTADO FINAL
                // Guardadr valor en la flash
                // Calculo de recta de regresion 
                valoresRecta.N = 3;
                valoresRecta.MediaPH = (PH4 + PH7 + PH11) / valoresRecta.N;
                valoresRecta.MediaLectura = (valores.lectura_PH4 + valores.lectura_PH7 + valores.lectura_PH11) / valoresRecta.N;
                valoresRecta.VarianzaLectura = ((pow(valores.lectura_PH4, 2) + pow(valores.lectura_PH7, 2) + pow(valores.lectura_PH11, 2)) / valoresRecta.N) - pow(valoresRecta.MediaLectura, 2);
                valoresRecta.Covarianza = (((valores.lectura_PH4 * PH4) + (valores.lectura_PH7 * PH7) + (valores.lectura_PH11 * PH11)) / valoresRecta.N) - (valoresRecta.MediaPH * valoresRecta.MediaLectura);
                valoresRecta.Pendiente = (valoresRecta.Covarianza / valoresRecta.VarianzaLectura);
                valoresRecta.Ordenada = ((valoresRecta.Covarianza / valoresRecta.VarianzaLectura) * (valoresRecta.MediaLectura * (-1))) + valoresRecta.MediaPH;
                
                // Borrado previo a la escritura (NO Necesario)
                erase_nvs();

                // Guardado en la memoria flash 
                write_nvs(key_pendiente, valoresRecta.Pendiente);
                write_nvs(key_ordenada, valoresRecta.Ordenada);
                break;
            
            default:
                break;
        }
    } 
}