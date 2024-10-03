/** 
 * @file guia2_ej4.c
 * @brief Código para la actividad 4 del proyecto: Osciloscopio por puerto serie.
 *
 * Actividad 4 - Proyecto: Osciloscopio
 *
 * Consigna: Diseñar e implementar una aplicación que digitalice una señal analógica y la transmita a un graficador de puerto serie en la PC. 
 * Se debe tomar la entrada CH1 del conversor AD y la transmisión se debe realizar por la UART conectada al puerto serie de la PC.
 * 
 * Requisitos:
 * - Disparar la conversión AD a través de una interrupción periódica de timer.
 * - Utilizar una frecuencia de muestreo de 500Hz.
 * - Almacenar el valor del conversor y transmitirlo por la UART en formato ASCII.
 * - Compatible con graficador por puerto serie: "https://x-io.co.uk/serial-oscilloscope/".
 * - Formato de transmisión: "valor\r" por cada lectura.
 * 
 * Prueba:
 * - Utilizar una señal ECG convertida digitalmente y visualizada en el osciloscopio implementado.
 *
 * OPCIONAL:
 * - Tecla 1 / 'T' para incrementar la frecuencia (simula taquicardia).
 * - Tecla 2 / 'B' para disminuir la frecuencia (simula bradicardia).
 * - Tecla 'R' para retornar a la frecuencia original.
 */
/** 
 * @mainpage Osciloscopio Digital por Puerto Serie
 * @section desc Descripción General
 * 
 * Este proyecto implementa un osciloscopio digital que captura señales analógicas a través del conversor
 * AD de la entrada CH1 y las transmite a un graficador por puerto serie en una PC. Utiliza una interrupción 
 * periódica de timer para disparar la conversión AD a una frecuencia de muestreo de 500Hz, logrando una 
 * digitalización constante de la señal.
 * 
 * Los valores convertidos se envían por la UART al puerto serie de la PC en formato ASCII, compatibles 
 * con el software graficador "Serial Oscilloscope" (https://x-io.co.uk/serial-oscilloscope/). Cada transmisión
 * contiene el valor digitalizado seguido de un retorno de carro ("\r"). 
 * 
 * Este proyecto permite monitorear señales fisiológicas y analizarlas visualmente a través de una interfaz gráfica.
 * 
 * @section changelog Historial de Cambios
 * | Fecha       | Descripción                                    |
 * |:-----------:|:-----------------------------------------------|
 * | 2/10/2024  | Creación del documento                         |
 * 
 * @author
 * Adriel Lopez
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_mcu.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
#define TIEMPO_CONVERSION_AD 2000 /**< Tiempo de conversión para el conversor AD */
#define TIEMPO_CONVERSION_DA 4000 /**< Tiempo de conversión para el conversor DA */

/**
 * @def BUFFER_SIZE 
 * @brief Tamaño del vector que contiene los datos de un ECG
 */
#define BUFFER_SIZE 231

/*==================[internal data definition]===============================*/
/** 
 * @var ConversorAD_task_handle 
 * @brief Manejador de tarea para la conversión AD
 */
TaskHandle_t ConversorAD_task_handle = NULL;

/** 
 * @var ConversorDA_task_handle 
 * @brief Manejador de tarea para la conversión DA
 */
TaskHandle_t ConversorDA_task_handle = NULL;

/** 
 * @var valorAnalogico 
 * @brief Valor analógico leído
 */
uint16_t valorAnalogico = 0;

/** 
 * @var ecg 
 * @brief Datos de un ECG
 */
const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77
};

/*==================[internal functions declaration]=========================*/
/** 
 * @fn escribirValorEnPc 
 * @brief Envía el valor analógico por el monitor serie
 */
void escribirValorEnPc()
{
    UartSendString(UART_PC, (char *)UartItoa(valorAnalogico, 10));
    UartSendString(UART_PC, "\r");
}

/** 
 * @fn AD_conversor_task 
 * @brief Tarea para la conversión AD
 */
void AD_conversor_task()
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogInputReadSingle(CH1, &valorAnalogico);
        escribirValorEnPc();
    }
}

/** 
 * @fn DA_conversor_task 
 * @brief Tarea para la conversión DA
 */
void DA_conversor_task()
{
    uint8_t i = 0;
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogOutputWrite(ecg[i]);
        i++;
        if (i == BUFFER_SIZE - 1)
        {
            i = 0;
        }
    }
}

/** 
 * @fn FuncTimerConversionDA 
 * @brief Notifica la tarea DA para continuar la conversión
 */
void FuncTimerConversionDA()
{
    vTaskNotifyGiveFromISR(ConversorDA_task_handle, pdFALSE);
}

/** 
 * @fn FuncTimerConversionAD 
 * @brief Notifica la tarea AD para continuar la conversión
 */
void FuncTimerConversionAD()
{
    vTaskNotifyGiveFromISR(ConversorAD_task_handle, pdFALSE);
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    /* Configuración del puerto serie */
    serial_config_t ControlarUart =
        {
            .port = UART_PC,
            .baud_rate = 115200,
            .func_p = NULL,
            .param_p = NULL,
        };
    UartInit(&ControlarUart);

    /* Inicialización del timer de conversión DA */
    timer_config_t timer_conversionDA = {
        .timer = TIMER_A,
        .period = TIEMPO_CONVERSION_DA,
        .func_p = FuncTimerConversionDA,
        .param_p = NULL};
    TimerInit(&timer_conversionDA);

    /* Inicialización del timer de conversión AD */
    timer_config_t timer_conversionAD = {
        .timer = TIMER_B,
        .period = TIEMPO_CONVERSION_AD,
        .func_p = FuncTimerConversionAD,
        .param_p = NULL};
    TimerInit(&timer_conversionAD);

    /* Configuración del canal de entrada analógica */
    analog_input_config_t Analog_input = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = 0};
    AnalogInputInit(&Analog_input);
    AnalogOutputInit();

    /* Creación de tareas */
    xTaskCreate(&DA_conversor_task, "conversor DA", 512, NULL, 5, &ConversorDA_task_handle);
    xTaskCreate(&AD_conversor_task, "conversor AD", 4096, NULL, 5, &ConversorAD_task_handle);

    /* Inicio de los timers */
    TimerStart(timer_conversionAD.timer);
    TimerStart(timer_conversionDA.timer);
}
