/**
 * @file main.c
 * @author Ezequiel Combina
 * @brief Codigo referente al manejo de el agitador y la limpieza de la bomba del titulador
 * @version 0.1
 * @date 2024-05-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */
/*==================[Inclusiones]======================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig.h"
#include "driver/gpio.h"
#include "pulsador.c"

/*==================[Definiciones]======================*/
#define T_LIMPIEZA_MS   1000
#define T_LIMPIEZA      pdMS_TO_TICKS(T_LIMPIEZA_MS)
#define PROCESADORA     0
#define PROCESADORB     1

/*==================[Variables globales]======================*/
gpio_int_type_t P_Agitador = GPIO_NUM_25;
gpio_int_type_t P_Motor    = GPIO_NUM_26;

//static const char *TAG = "MAIN";

/*==================[Prototipos de funciones]======================*/
void TaskAgitador(void *taskParmPtr);       //Prototipo de la función de la tarea
void TaskLimpieza(void *taskParmPtr); 
//void tarea_tecla(void* taskParmPtr);

/*==================[Main]======================*/
void app_main()
{
    // Crear tarea en freeRTOS
    // Devuelve pdPASS si la tarea fue creada y agregada a la lista ready
    // En caso contrario devuelve pdFAIL.
    inicializarPulsador();
    
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
        ESP_LOGI(TAG, "Error al crear la tarea.");
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
        ESP_LOGI(TAG, "Error al crear la tarea.");
        while(1);    // Si no pudo crear la tarea queda en un bucle infinito
    }
}

/*==================[Implementacion de la tarea]======================*/
void TaskAgitador(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/
    esp_rom_gpio_pad_select_gpio(P_Agitador);
    gpio_set_direction(P_Agitador, GPIO_MODE_OUTPUT);

    uint8_t estadoAgitador;

    /*==================[Bucle]======================*/
    while(1)
    {
        estadoAgitador = obtenerEstadoAgitador();

        switch(estadoAgitador)
        {
            case 1:
                gpio_set_level(P_Agitador, 1);
                break;
            
            case 0:
                gpio_set_level(P_Agitador, 0);
                break;

            default:
                break;
        }
    } 
}

void TaskLimpieza(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/
    esp_rom_gpio_pad_select_gpio(P_Motor);
    gpio_set_direction(P_Motor, GPIO_MODE_OUTPUT);

    uint8_t indice = LIMPIEZA;

    TickType_t xPeriodicity = T_LIMPIEZA; 
    TickType_t xLastWakeTime = xTaskGetTickCount();

    /*==================[Bucle]======================*/
    while(1)
    {
        xSemaphoreTake(pulsador[indice].semaforo, portMAX_DELAY);
        xLastWakeTime = xTaskGetTickCount();
        gpio_set_level(P_Motor, 1);
        vTaskDelayUntil(&xLastWakeTime, xPeriodicity);
        gpio_set_level(P_Motor, 0);
        /*
        switch(estadoLimpieza)
        {
            case 1:
                gpio_set_level(P_Motor, 1);
                vTaskDelay(T_LIMPIEZA);
                //vTaskDelayUntil(&xLastWakeTime, xPeriodicity);
                //gpio_set_level(P_Motor, 0);
                //EstadoLimpieza = 0;
                break;
            
            case 0:
                gpio_set_level(P_Motor, 0);
                break;

            default:
                break;
        }
        */
    } 
}