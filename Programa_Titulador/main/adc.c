/*==================[ Inclusiones ]============================================*/
#include "../include/adc.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/adc.h"

/*==================[Definiciones]==============================*/

#define PROCESADORA         0
#define PROCESADORB         1

#define PH4                 4.0
#define PH7                 7.0
#define PH10                10.0

/*==================[Prototipos de funciones]======================*/

void TaskADC(void *taskParmPtr);   

/*==================[Variables]==============================*/

int modo                        = DIRECTO;              // Elegir que tipo de técnica se le aplica a la señal del ADC para obtener un mejor valor
//int visualizar                  = GRAFICA;              // Forma de presentar los valores del ADC
int LecturaCruda                = 0;                    // Lectura del ADC
int LecturaSuavizada            = 0;                    // Valor promedio de la lectura cruda
int LecturaFiltrada             = 0;                    // VAlor filtrado de la lectura cruda
int Lectura                     = 0;
double filtro                   = 0;                    // Variable para generar el filtro
int veces[4096];                                        // Contar las cantidad de veces que aparece la misma lectura del ACD
uint64_t millisAnt              = 0;                    // Toma el valor del momento
float Vout_filtrada_corregida   = 0;
float Vout_PH                   = 0;

extern valoresPH valoresCalibracion;
extern RectaRegresion valoresRecta;

static const char *TAG_ADC = "ADC";

/*==================[Implementaciones]==============================*/

void adc_init()
{
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH));

    // Configuración del canal y atenuación del ADC
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN));

    BaseType_t err4 = xTaskCreatePinnedToCore(
        TaskADC,                     	        // Funcion de la tarea a ejecutar
        "TaskADC",   	                        // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*3, 		    // Cantidad de stack de la tarea
        NULL,                          	        // Parametros de tarea
        tskIDLE_PRIORITY+1,         	        // Prioridad de la tarea -> Queremos que este un nivel encima de IDLE
        NULL,                          		    // Puntero a la tarea creada en el sistema
        PROCESADORB                             // Numero de procesador
    );

    // Gestion de errores
    if(err4 == pdFAIL)
    {
        ESP_LOGI(TAG_ADC, "Error al crear la tarea.");
        while(1);    // Si no pudo crear la tarea queda en un bucle infinito
    }  
}

void TaskADC(void *taskParmPtr)
{
    /*==================[Configuraciones]======================*/
    // for(int i = 0; i <= 4096; i++)
    // {
    //     veces[i] = 0;
    // }

    /*==================[Bucle]======================*/
    while (1) {

        if(modo == DIRECTO)
        {
            LecturaCruda = adc1_get_raw(ADC_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(10));
            modo++;
        }
        else if(modo == FILTRADO)
        {
            filtro += ((double)LecturaCruda - filtro) / (double)FILTRO;
            LecturaFiltrada = (int)filtro;
            LecturaFiltrada *= ((-0.13 * (LecturaFiltrada - 2047)) / 2048) + 1.13;
            modo++;
        }           
        else if(modo == CONVERSION)
        {
            Vout_filtrada_corregida = (LecturaFiltrada * T_MAX_ADC) / BITS12;
            //Vout_PH = ((LecturaFiltrada)*T_PH) / BITS12; 
            //Vout_PH = Vout_PH*1.8529;
            //Vout_PH = -7.2286*Vout_filtrada_corregida + 24.662;
            Vout_PH = (Vout_filtrada_corregida - valoresRecta.Ordenada) / valoresRecta.Pendiente;
            //Vout_PH = (Vout_filtrada_corregida - 3.41) / (-0.13833);
            modo++;
        }

        //Visualizar el resultado de la conversión del ADC en función del modo escogido
        else if(modo == GRAFICA)
        {
            //Se muestra un punto en SerialPloter cada décima de segundo
            if(esp_timer_get_time() > millisAnt + 100*3000)
            {
                millisAnt = esp_timer_get_time();
                ESP_LOGI(TAG_ADC, "Bits: %d", LecturaFiltrada);
                ESP_LOGI(TAG_ADC, "Vout_filtrada_corregida: %.03f", Vout_filtrada_corregida);
                ESP_LOGI(TAG_ADC, "PH: %.01f", Vout_PH);
            }
            modo = 0;
        }
    }
}

void lectura(char valor_actual)
{
    // ---Codigo referente a la lectura del valor para calibra---
    // ---Aprox 2 min para que estabilice la medicion---
    // int n = 0;
    switch(valor_actual)
    {
        case 'D':       // PH 4
            // ---AGREGAR CODIGO DE APROXIMACIÓN---
            valoresCalibracion.lectura_PH4 = Vout_filtrada_corregida;
            break;
        
        case 'E':       // PH 7
            // ---AGREGAR CODIGO DE APROXIMACIÓN---
            valoresCalibracion.lectura_PH7 = Vout_filtrada_corregida;
            break;

        case 'F':       // PH 10
            // ---AGREGAR CODIGO DE APROXIMACIÓN---
            valoresCalibracion.lectura_PH10 = Vout_filtrada_corregida;
            break;
    }
}

void adc_calibracion()
{
    // ---Codigo referente a la lectura de medicion para hacer la calibracion---
    ESP_LOGI(TAG_ADC, "Calculo de recta de regresion");
    valoresRecta.N = 3;
    valoresRecta.Sum_X = PH4 + PH7 + PH10;
    valoresRecta.Sum_Y = valoresCalibracion.lectura_PH4 + valoresCalibracion.lectura_PH7 + valoresCalibracion.lectura_PH10;
    valoresRecta.MediaX = valoresRecta.Sum_X / valoresRecta.N;
    valoresRecta.MediaY = valoresRecta.Sum_Y / valoresRecta.N;
    valoresRecta.Suma_XY = (PH4 * valoresCalibracion.lectura_PH4) + (PH7 * valoresCalibracion.lectura_PH7) + (PH10 * valoresCalibracion.lectura_PH10);
    valoresRecta.Suma_XSquare = (pow(PH4, 2)) + (pow(PH7, 2)) + (pow(PH10, 2));
    valoresRecta.Pendiente = (valoresRecta.Suma_XY - ((valoresRecta.Sum_X * valoresRecta.Sum_Y) / valoresRecta.N)) / (valoresRecta.Suma_XSquare - (pow(valoresRecta.Sum_X, 2) / valoresRecta.N));
    valoresRecta.Ordenada = valoresRecta.MediaY - valoresRecta.Pendiente * valoresRecta.MediaX;
    ESP_LOGI(TAG_ADC, "Pendiente -> %f - Ordenada -> %f", valoresRecta.Pendiente, valoresRecta.Ordenada);
}
