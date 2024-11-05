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
#include "wifi.c"

/*==================[Definiciones]======================*/

#define T_TITULACION_MS_01ml    1410    // ---VER---(1150)
#define T_TITULACION_01ml       pdMS_TO_TICKS(T_TITULACION_MS_01ml)
#define T_TITULACION_MS_1ml     T_TITULACION_MS_01ml*10
#define T_TITULACION_1ml        pdMS_TO_TICKS(T_TITULACION_MS_1ml)
#define PROCESADORA             0
#define PROCESADORB             1
#define C_MEDICIONES            10
#define T_MEDICIONES_MS         1000
#define T_MEDICIONES            pdMS_TO_TICKS(T_MEDICIONES_MS)

// ---PWM---
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (GPIO_NUM_12)                               // Pin de salida del PWM 
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT                            // Resolución del PWM (8 bits)
#define LEDC_DUTY_PERCENT       50                                          // Duty porcentual
#define LEDC_DUTY               ((LEDC_DUTY_PERCENT*254)/100)               // Duty al 50%. (2 ** 8) * 50% = 128
#define LEDC_FREQUENCY          (10000)                                     // Frequency in Hertz. Set frequency at 10 kHz

/*==================[Variables globales]======================*/

gpio_int_type_t P_Agitador = GPIO_NUM_17;
gpio_int_type_t P_3_3      = GPIO_NUM_22;       // Salida 3.3 
gpio_int_type_t P_Motor    = GPIO_NUM_12;
gpio_int_type_t P_Giro     = GPIO_NUM_27;

static const char *TAG_MAIN = "MAIN";

valoresPH valoresCalibracion;       // Valores de tension
RectaRegresion valoresRecta;

// Keys para acceder a los valores guardados en la memoria flash
char *key_pendiente = "Pend";
char *key_ordenada = "Ord";

Limpieza limpieza_main;
float Vout_PH_Ant = 0.0, Volumen_Inflexion;
float dif = 0.0, dif_guardado = 0.0;

extern float Vout_PH;
extern int Volumen_Comp;
extern bool flag_Titular;

/*==================[Handles]==============================*/

QueueHandle_t S_Agitador    = NULL;
//SemaphoreHandle_t S_Limpieza = NULL;
QueueHandle_t S_Limpieza    = NULL;
QueueHandle_t S_Calibracion = NULL;
QueueHandle_t S_Titulacion  = NULL;
nvs_handle_t app_nvs_handle;

/*==================[Prototipos de funciones]======================*/

void TaskAgitador(void *taskParmPtr);
void TaskLimpieza(void *taskParmPtr); 
void TaskCalibracion(void *taskParmPtr);  
void TaskTitulacion(void *taskParmPtr);  
static void example_ledc_init(void); 

/*==================[Main]======================*/

void app_main(void)
{

    S_Agitador = xQueueCreate(1, sizeof(bool));
    //S_Limpieza = xSemaphoreCreateBinary();
    S_Limpieza = xQueueCreate(1, sizeof(limpieza_main));
    S_Calibracion = xQueueCreate(1, sizeof(char*));
    S_Titulacion = xQueueCreate(1, sizeof(bool));

    esp_rom_gpio_pad_select_gpio(P_3_3);
    gpio_set_direction(P_3_3, GPIO_MODE_OUTPUT);
    gpio_set_level(P_3_3, 1);

    // Set the LEDC peripheral configuration
    example_ledc_init();
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));

    // Iniciar flash 
    ESP_ERROR_CHECK(init_nvs());

    // Lectura de los valores guardados en la UART 
    read_nvs(key_pendiente, &valoresRecta.Pendiente_Guardado);
    read_nvs(key_ordenada, &valoresRecta.Ordenada_Guardado);

    valoresRecta.Pendiente = valoresRecta.Pendiente_Guardado / 10000.0;
    valoresRecta.Ordenada = valoresRecta.Ordenada_Guardado / 10000.0;

    ESP_LOGI(TAG_MAIN, "Pendiente -> %.03f", valoresRecta.Pendiente);
    ESP_LOGI(TAG_MAIN, "Ordenada -> %.03f", valoresRecta.Ordenada);

    // Iniciar Comunicacion Wifi
    wifi_init();

    // Iniciar UART
    init_uart();

    // Iniciar ADC
    adc_init();

    esp_rom_gpio_pad_select_gpio(P_3_3);
    gpio_set_direction(P_3_3, GPIO_MODE_OUTPUT);
    gpio_set_level(P_3_3, true);

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

    BaseType_t err4 = xTaskCreatePinnedToCore(
        TaskTitulacion,                    // Funcion de la tarea a ejecutar
        "TaskTitulacion",   	            // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 		// Cantidad de stack de la tarea
        NULL,                          	    // Parametros de tarea
        tskIDLE_PRIORITY+1,         	    // Prioridad de la tarea -> Queremos que este un nivel encima de IDLE
        NULL,                          		// Puntero a la tarea creada en el sistema
        PROCESADORA                         // Numero de procesador
    );

    // Gestion de errores
    if(err4 == pdFAIL)
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
        ESP_LOGI(TAG_MAIN, "Estado Agitador = %d\n", estado_agitador);
        
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

    // TickType_t xPeriodicity     = T_TITULACION_1ml; 
    // TickType_t xLastWakeTime    = xTaskGetTickCount();

    /*==================[Bucle]======================*/
    while(1)
    {
        xQueueReceive(S_Limpieza, &limpieza_main, portMAX_DELAY);
        gpio_set_level(P_Giro, limpieza_main.Giro_Limpieza);

        if(limpieza_main.Habilitador_Limpieza == true)
        {
            // Habilitar Enabler
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        }
        else if(limpieza_main.Habilitador_Limpieza == false)
        {
            // Desabiilitar Enabler
            ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0));
        }

        //xLastWakeTime = xTaskGetTickCount();
        //ESP_LOGI(TAG_MAIN, "Led Encendido");
        //ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        //vTaskDelayUntil(&xLastWakeTime, xPeriodicity);
        //ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0));
        //ESP_LOGI(TAG_MAIN, "Led Apagado");
    } 
}

void TaskCalibracion(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/
    //esp_rom_gpio_pad_select_gpio(P_Motor);
    //gpio_set_direction(P_Motor, GPIO_MODE_OUTPUT);

    char *estado_calibracion;

    /*==================[Bucle]======================*/
    while(1)
    {
        xQueueReceive(S_Calibracion, &estado_calibracion, portMAX_DELAY);
        ESP_LOGI(TAG_MAIN, "%s", estado_calibracion);

        if(strcmp(estado_calibracion, "D") == 0)
        {
            ESP_LOGI(TAG_MAIN, "Calibración PH4");
            valoresCalibracion.lectura_PH4 = Vout_filtrada_corregida;
            ESP_LOGI(TAG_MAIN, "%.03f", valoresCalibracion.lectura_PH4);
            //lectura(estado_calibracion);
        }

        if(strcmp(estado_calibracion, "E") == 0)
        {
            ESP_LOGI(TAG_MAIN, "Calibración PH7");
            valoresCalibracion.lectura_PH7 = Vout_filtrada_corregida;
            ESP_LOGI(TAG_MAIN, "%.03f", valoresCalibracion.lectura_PH7);
        }

        if(strcmp(estado_calibracion, "F") == 0)
        {
            ESP_LOGI(TAG_MAIN, "Calibración PH10");
            valoresCalibracion.lectura_PH10 = Vout_filtrada_corregida;
            ESP_LOGI(TAG_MAIN, "%.03f", valoresCalibracion.lectura_PH10);

            adc_calibracion();

            // Conversion de la pendiente y la ordenada a variables enteras para poder guardarlas en la falsh 

            valoresRecta.Pendiente_Guardado = (int32_t) (valoresRecta.Pendiente * 10000.0);    // Multiplicamos por 1000 para guardar 3 decimales
            valoresRecta.Ordenada_Guardado = (int32_t) (valoresRecta.Ordenada * 10000.0);      // En caso de necesitar mayor presicion multiplicar por un numero mas grande 

            // Borrado previo a la escritura (NO Necesario)
            //erase_nvs();

            // Guardado en la memoria flash 
            write_nvs(key_pendiente, valoresRecta.Pendiente_Guardado);
            write_nvs(key_ordenada, valoresRecta.Ordenada_Guardado);
        }
    } 
}

void TaskTitulacion(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/
    //bool flag_Titulacion_main;

    // float dif = 0;
    // float *ptr_dif = &dif;
    float volumen_registrado = 0;

    TickType_t xPeriodicity_1ml     = T_TITULACION_1ml; 
    TickType_t xPeriodicity_01ml    = T_TITULACION_01ml; 
    TickType_t xLastWakeTime        = xTaskGetTickCount();

    /*==================[Bucle]======================*/
    while(1)
    {
        //xQueueReceive(S_Titulacion, &flag_Titulacion_main, portMAX_DELAY);
        if(flag_Titular == true)
        {
            if(dif < 0.2)   // Dif de PH 
            {
                Vout_PH_Ant = Vout_PH; // Guardamos el valor anterior de PH
                xLastWakeTime = xTaskGetTickCount();
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
                vTaskDelayUntil(&xLastWakeTime, xPeriodicity_1ml);
                ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0));
                volumen_registrado += 1;
                //vTaskDelay(pdMS_TO_TICKS(500)); // Tiempo de espera para que el electrodo ajuste su medicion
            }

            if(dif >= 0.2)   // Dif de PH 
            {
                Vout_PH_Ant = Vout_PH; // Guardamos el valor anterior de PH
                xLastWakeTime = xTaskGetTickCount();
                ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
                vTaskDelayUntil(&xLastWakeTime, xPeriodicity_01ml);
                ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0));
                volumen_registrado += 0.1;
                //vTaskDelay(pdMS_TO_TICKS(500)); // Tiempo de espera para que el electrodo ajuste su medicion
            }

            if(volumen_registrado >= Volumen_Comp)
            {
                fin_titulacion();
            }

            // Cálculo de la diferencia de PH 
            // Lo hacemos aca ya que en el proceso del ADC se esta midiendo constantemente, por lo tanto la dif siempre era muy pequeña
            dif = sqrt(pow((Vout_PH - Vout_PH_Ant), 2));

            // Calculo de volumen de corte 
            if(dif > dif_guardado)
            {
                dif_guardado = dif;
                Volumen_Inflexion = volumen_registrado;
            }

            ESP_LOGI(TAG_MAIN, "Dif -> %.02f", dif);

            vTaskDelay(T_MEDICIONES); // Tiempo de espera entre cada inyección 
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        //volumen(ptr_dif); 

        // ---MANDAR J## (## = VOLUMEN EN EL PUNTO DE INFLECCIÓN) CUANDO TERMINE LA TITULACION POR CUALQUIERA DE LOS DOS METODOS---
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