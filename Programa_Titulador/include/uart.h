#ifndef UART_H_
#define UART_H_

/*==================[Inclusiones]============================================*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "driver/uart.h"

/*==================[Definiciones]===================================*/

#define UART_NUM            UART_NUM_2
#define BUFFER_SIZE         1024
#define TASK_MEMORY         1024 * 2
#define TX                  GPIO_NUM_4
#define RX                  GPIO_NUM_5

uart_config_t uart_config = {

    .baud_rate  = 115200,
    .data_bits  = UART_DATA_BITS_MAX, // ---UART_DATA_8_BITS---
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB

};

typedef struct
{
    bool Habilitador_Limpieza;
    bool Giro_Limpieza;
}Limpieza;

typedef struct
{
    float lectura_PH4; 
    float lectura_PH7; 
    float lectura_PH11; 
}valoresPH;

/*==================[Prototipos de funciones]======================*/

void init_uart();

#endif