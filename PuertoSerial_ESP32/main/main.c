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
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "uart.c"

/*==================[Definiciones]======================*/

#define T_LIMPIEZA_MS       1000
#define T_LIMPIEZA          pdMS_TO_TICKS(T_LIMPIEZA_MS)
#define PROCESADORA         0
#define PROCESADORB         1
#define C_MEDICIONES        10
#define T_MEDICIONES_MS     50
#define T_MEDICIONES        pdMS_TO_TICKS(T_MEDICIONES_MS)

/*==================[Variables globales]======================*/

gpio_int_type_t P_Agitador = GPIO_NUM_25;
gpio_int_type_t P_Motor    = GPIO_NUM_26;

static const char *TAG_MAIN = "MAIN";

/*==================[Semaforos]==============================*/

SemaphoreHandle_t S_Agitador = NULL;
SemaphoreHandle_t S_Limpieza = NULL;
SemaphoreHandle_t S_Calibracion = NULL;

/*==================[Prototipos de funciones]======================*/

void TaskAgitador(void *taskParmPtr);
void TaskLimpieza(void *taskParmPtr); 
void TaskCalibracion(void *taskParmPtr); 

/*==================[Main]======================*/

void app_main(void)
{

    S_Agitador = xSemaphoreCreateBinary();
    S_Limpieza = xSemaphoreCreateBinary();

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

    /*==================[Bucle]======================*/
    while(1)
    {
        xSemaphoreTake(S_Agitador, portMAX_DELAY);
        if(estadoAgitador() == 1)
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

    uint8_t cont = 0;
    float Prom = 0.0;

    /*==================[Bucle]======================*/
    while(1)
    {
        xSemaphoreTake(S_Calibracion, portMAX_DELAY);
        switch(estadoCalibracion())
        {
            case '1':       // PH 4
                ESP_LOGI(TAG_MAIN, "Calibración PH4");
                for(int i = 0; i < C_MEDICIONES; i++)
                {
                    //cont = 3.9;
                    //cont = (4/obtenerMedicion());
                    Prom += (4/cont);
                    vTaskDelay(T_MEDICIONES);
                }
                float PH4 = Prom /= C_MEDICIONES;
                ESP_LOGI(TAG_MAIN, "Fin Calibración PH4");
                Prom = 0.0;
                break;

            case '2':       // PH 7
                for(int i = 0; i < C_MEDICIONES; i++)
                {
                    //cont = 6.95;
                    //cont = (7/obtenerMedicion());
                    Prom += (7/cont);
                    vTaskDelay(T_MEDICIONES);
                }
                float PH7 = Prom /= C_MEDICIONES;
                Prom = 0.0;
                break;

            case '3':       // PH 11
                for(int i = 0; i < C_MEDICIONES; i++)
                {
                    //cont = 10.9;
                    //cont = (11/obtenerMedicion());
                    Prom += (11/cont);
                    vTaskDelay(T_MEDICIONES);
                }
                float PH11 = Prom /= C_MEDICIONES;
                Prom = 0.0;
                break;

            case '4':       // RESULTADO FINAL
                // Guardadr valor en la flash
                float Calibracion_Final = (PH4 + PH7 + PH11) / 3;
                break;
            
            default:
                break;
        }
    } 
}