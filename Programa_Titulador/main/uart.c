/*==================[ Inclusiones ]============================================*/
#include "../include/uart.h"
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"

/*==================[ Definiciones ]===================================*/

#define T_GUARDADO  10     // Timepo de espera entre datos leidos

/*==================[Prototipos de funciones]======================*/

esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config);
esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num);
esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t* uart_queue, int intr_alloc_flags);

void TaskUart(void *taskParmPtr);      // Prototipo de la función de la tarea

/*==================[Variables]==============================*/

static const char *TAG_UART = "UART";

bool flag_Agitador = 0;
//uint8_t flag_Calibracion = 0;

extern QueueHandle_t S_Agitador;
extern SemaphoreHandle_t S_Limpieza;
extern QueueHandle_t S_Calibracion;

/*==================[Implementaciones]=================================*/

/**
 * @brief Inicializacion de la UART 
 * 
 */
void init_uart()
{
    uart_param_config(UART_NUM, &uart_config);

    uart_set_pin(UART_NUM, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    uart_driver_install(UART_NUM, BUFFER_SIZE, BUFFER_SIZE, 0, NULL, 0);

    BaseType_t err = xTaskCreate(
        TaskUart,                     	// Funcion de la tarea a ejecutar
        "TaskUart",   	                // Nombre de la tarea como String amigable para el usuario
        TASK_MEMORY, 		            // Cantidad de stack de la tarea
        NULL,                          	// Parametros de tarea
        tskIDLE_PRIORITY+1,         	// Prioridad de la tarea -> Queremos que este un nivel encima de IDLE
        NULL                            // Puntero a la tarea creada en el sistema
    );

    if(err == pdFAIL)
    {
        ESP_LOGI(TAG_UART, "Error al crear la tarea.");
        while(1);    // Si no pudo crear la tarea queda en un bucle infinito
    }

    ESP_LOGI(TAG_UART, "Inicialización de la UART completa");

}

/**
 * @brief Lectura del Puerto Serie
 * 
 * @param taskParmPtr 
 */
void TaskUart(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/

    uint8_t *Dato = (uint8_t*) malloc(BUFFER_SIZE);     // Puntero toma el dato leido, con "malloc" se le asigna un espacio de memoria

    /*==================[Bucle]======================*/
    while(1)
    {
        bzero(Dato, BUFFER_SIZE); // Se borra el espacio de memoria que este en "Dato"

        int len = uart_read_bytes(UART_NUM, Dato, BUFFER_SIZE, pdMS_TO_TICKS(T_GUARDADO)); 

        // Si el dato leido es dif de 0, lo escribe en el terminal y lo vuelve a mandar
        if(len == 0)
        {   
            continue;
        }

        //uart_write_bytes(UART_NUM,(const char*) Dato, len);

        ESP_LOGI(TAG_UART, "Dato recibido -> %s", Dato);

        for(size_t i = 0; i < len - 2; i++)
        {
            char valor = Dato[i];

            switch(valor)
            {
                case 'A':
                    xSemaphoreGive(S_Limpieza);
                    break;
                
                case 'L':
                    flag_Agitador = !flag_Agitador; 
                    xQueueSend(S_Agitador, &flag_Agitador, portMAX_DELAY);
                    break;
                
                case '1':
                    //flag_Calibracion = valor; 
                    xQueueSend(S_Calibracion, &valor, portMAX_DELAY);
                    break;
                
                case '2':
                    //flag_Calibracion = valor; 
                    xQueueSend(S_Calibracion, &valor, portMAX_DELAY);
                    break;
                
                case '3':
                    //flag_Calibracion = valor; 
                    xQueueSend(S_Calibracion, &valor, portMAX_DELAY);
                    break;

                case '4':
                    //flag_Calibracion = valor; 
                    xQueueSend(S_Calibracion, &valor, portMAX_DELAY);
                    break;    
                
                default:
                    break;
            }
        }
    } 
}
