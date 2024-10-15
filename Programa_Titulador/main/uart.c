/*==================[ Inclusiones ]============================================*/
#include "../include/uart.h"
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"

/*==================[ Definiciones ]===================================*/

#define PROCESADORA         0
#define PROCESADORB         1
#define T_GUARDADO          10     // Timepo de espera entre datos leidos

/*==================[Prototipos de funciones]======================*/

esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config);
esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num);
esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t* uart_queue, int intr_alloc_flags);

void TaskUart(void *taskParmPtr);      // Prototipo de la función de la tarea

/*==================[Variables]==============================*/

static const char *TAG_UART = "UART";

extern float Vout_PH;
char valor[5];

bool flag_Agitador = false;
//bool flag_Limpieza = false;
//uint8_t flag_Calibracion = 0;

Limpieza limpieza;

//char *msg = "OK\r\n";
char *msg = "K";

const char *Calibrar_ON         = "A";
const char *Calibrar_OFF        = "B";
const char *Lectura_ADC         = "C";
const char *Buffer_4            = "D";
const char *Buffer_7            = "E";
const char *Buffer_10           = "F";
const char *Titular_ON          = "G";
const char *Lectura_PH          = "H";
const char *Titular_OFF         = "I";
const char *Titular_END         = "J";
const char *Limpieza_ON         = "K";
const char *Limpieza_OFF        = "L";
const char *Volumen             = "M";
const char *Guardar_Volumen     = "N";
const char *Estado_Agitador     = "O";
const char *Agitador_ON         = "P";
const char *Agitador_OFF        = "Q";

// int largo, largo2;

extern QueueHandle_t S_Agitador;
//extern SemaphoreHandle_t S_Limpieza;
extern QueueHandle_t S_Limpieza;
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

    BaseType_t err = xTaskCreatePinnedToCore(
        TaskUart,                     	// Funcion de la tarea a ejecutar
        "TaskUart",   	                // Nombre de la tarea como String amigable para el usuario
        TASK_MEMORY, 		            // Cantidad de stack de la tarea
        NULL,                          	// Parametros de tarea
        tskIDLE_PRIORITY+1,         	// Prioridad de la tarea -> Queremos que este un nivel encima de IDLE
        NULL,                           // Puntero a la tarea creada en el sistema
        PROCESADORA                     // Numero de procesador
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

    char *Dato = (char*) malloc(BUFFER_SIZE);     // Puntero toma el dato leido, con "malloc" se le asigna un espacio de memoria

    /*==================[Bucle]======================*/
    while(1)
    {
        bzero(Dato, BUFFER_SIZE); // Se borra el espacio de memoria que este en "Dato"

        int len = uart_read_bytes(UART_NUM, Dato, BUFFER_SIZE, pdMS_TO_TICKS(T_GUARDADO));

        if(len == 0)
        {
            continue;
        }

        // ---Le enviamos "OK" al ATMega cuando se recibe el dato---
        //uart_write_bytes(UART_NUM, (const char*)msg, sizeof(msg));

        // Recortamos el dato que se recibe
        Dato[1] = '\0';

        //ESP_LOGI(TAG_UART, "Largo Dato -> %d - %d", strlen(Dato), strlen(AI));
        ESP_LOGI(TAG_UART, "Dato recibido -> %s\n", Dato);

        if(strcmp(Dato, Agitador_ON) == 0)
        {
            //ESP_LOGI(TAG_UART, "Entrada a AI\n");
            // ---Le enviamos "OK" al ATMega cuando se recibe el dato---
            uart_write_bytes(UART_NUM, (const char*)msg, sizeof(msg));
            flag_Agitador = true;
            xQueueSend(S_Agitador, &flag_Agitador, portMAX_DELAY);
        }

        if(strcmp(Dato, Agitador_OFF) == 0)
        {
            // ---Le enviamos "OK" al ATMega cuando se recibe el dato---
            uart_write_bytes(UART_NUM, (const char*)msg, sizeof(msg));
            flag_Agitador = false;
            xQueueSend(S_Agitador, &flag_Agitador, portMAX_DELAY);
        }

        if(strcmp(Dato, Limpieza_ON) == 0)
        {
            // ---Le enviamos "OK" al ATMega cuando se recibe el dato---
            uart_write_bytes(UART_NUM, (const char*)msg, sizeof(msg));
            limpieza.Giro_Limpieza = 0;
            limpieza.Habilitador_Limpieza = true;
            xQueueSend(S_Limpieza, &limpieza, portMAX_DELAY);
        }

        if(strcmp(Dato, Limpieza_OFF) == 0)
        {
            // ---Le enviamos "OK" al ATMega cuando se recibe el dato---
            uart_write_bytes(UART_NUM, (const char*)msg, sizeof(msg));
            limpieza.Habilitador_Limpieza = false;
            //xQueueSend(S_Limpieza, &limpieza, portMAX_DELAY);
        }

        if(strcmp(Dato, Lectura_ADC) == 0)
        {
            // ---Le enviamos el valor actual de PH leido al ATMega cuando se recibe "C"---
            // ---Además, le enviamos "/" justo despues del valor---
            snprintf(valor, sizeof(valor), "%.02f", Vout_PH);
            uart_write_bytes(UART_NUM, valor, sizeof(valor));
            uart_write_bytes(UART_NUM, "/", sizeof(char));
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // if(strcmp(Dato, Agitador_ON) == 0)
        // {
        //     //flag_Calibracion = valor;
        //     char a = 'a';
        //     xQueueSend(S_Calibracion, &a, portMAX_DELAY);
        // }

        // ---No usamos Switch ya que no nos permite usar strings como condicion---

        // switch(Dato)
        // {
        //     //case 'A':
        //     //    xSemaphoreGive(S_Limpieza);
        //     //    break;
        //     case 'AI':
        //         // ---Agregar zona critica para la variable "flag_Agitador"---
        //         flag_Agitador = true;
        //         xQueueSend(S_Agitador, &flag_Agitador, portMAX_DELAY);
        //         break;

        //     case 'AF':
        //         flag_Agitador = false;
        //         xQueueSend(S_Agitador, &flag_Agitador, portMAX_DELAY);
        //         break;

        //     case 'LI':
        //         //flag_Agitador = true;
        //         limpieza.Habilitador_Limpieza = true;
        //         xQueueSend(S_Limpieza, &limpieza.Habilitador_Limpieza, portMAX_DELAY);
        //         break;

        //     case 'LF':
        //         //flag_Agitador = false;
        //         limpieza.Habilitador_Limpieza = false;
        //         xQueueSend(S_Limpieza, &limpieza.Habilitador_Limpieza, portMAX_DELAY);
        //         break;

        //     case '1':
        //         //flag_Calibracion = valor;
        //         xQueueSend(S_Calibracion, &valor, portMAX_DELAY);
        //         break;

        //     case '2':
        //         //flag_Calibracion = valor;
        //         xQueueSend(S_Calibracion, &valor, portMAX_DELAY);
        //         break;

        //     case '3':
        //         //flag_Calibracion = valor;
        //         xQueueSend(S_Calibracion, &valor, portMAX_DELAY);
        //         break;

        //     case '4':
        //         //flag_Calibracion = valor;
        //         xQueueSend(S_Calibracion, &valor, portMAX_DELAY);
        //         break;

        //     default:
        //         break;
        // }
    }
}
