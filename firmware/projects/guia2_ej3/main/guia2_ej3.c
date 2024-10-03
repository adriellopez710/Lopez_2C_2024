/** 
 * @file guia2_ej3.c
 * @brief Código para la actividad 3 del proyecto: Medidor de distancia con interrupciones y puerto serie.
 *
 * Actividad 3 - Proyecto: Medidor de distancia por ultrasonido c/interrupciones y puerto serie
 *
 * Consigna: Cree un nuevo proyecto en el que modifique la actividad del punto 2 agregando ahora el puerto serie. 
 * Envíe los datos de las mediciones para poder observarlos en un terminal en la PC, siguiendo el siguiente formato:
 * 
 * 3 dígitos ascii + 1 carácter espacio + dos caracteres para la unidad (cm) + cambio de línea “\r\n”
 * 
 * Además, debe ser posible controlar la EDU-ESP de la siguiente manera:
 * Con las teclas “O” y “H”, replicar la funcionalidad de las teclas 1 y 2 de la EDU-ESP.
 * 
 * OPCIONAL: 
 * - Usar “I” para cambiar la unidad de trabajo de “cm” a “pulgadas”.
 * - Usar “M” para implementar la visualización del máximo.
 */
/** 
 * @mainpage Medición de Distancia con Sensor HC-SR04, con interrupciones y puerto serie
 *
 * @section desc Descripción General
 * Este proyecto es una extensión de la actividad 2, donde se integran funcionalidades adicionales de 
 * comunicación con el puerto serie y control remoto del sistema.
 * 
 * 
 * El sistema también permite el control remoto del dispositivo EDU-ESP a través de las teclas del terminal:
 * - Tecla "O": Enciende el sistema (equivalente a la tecla 1 de la EDU-ESP).
 * - Tecla "H": Apaga el sistema (equivalente a la tecla 2 de la EDU-ESP).
 * - Tecla "I": Cambia la unidad de medida de centímetros a pulgadas.
 * - Tecla "M": Muestra el valor máximo registrado en las mediciones.
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
#include "led.h"
#include "hc_sr04.h"
#include "gpio_mcu.h"
#include "lcditse0803.h"
#include "switch.h"
#include "timer_mcu.h"
#include "uart_mcu.h"

/*==================[macros and definitions]=================================*/
#define REFRESCO_MEDICION 1000000  ///< Período de refresco para la tarea de medición (us)
#define REFRESCO_DISPLAY 1000000   ///< Período de refresco para la tarea de mostrar (us)

/*==================[internal data definition]===============================*/
TaskHandle_t Medir_task_handle = NULL;   ///< Objeto para manejar la tarea de medición
TaskHandle_t Mostrar_task_handle = NULL; ///< Objeto para manejar la tarea de mostrar
uint16_t distancia = 0;  ///< Variable global que almacena la distancia medida
bool hold = false;       ///< Bandera de estado de mantener (hold)
bool encendido = true;   ///< Bandera de estado de encendido (on)

/*==================[internal functions declaration]=========================*/

/**
 * @brief Envía la distancia al monitor serie en tiempo real.
 */
void escribirDistanciaEnPc(void) {
    UartSendString(UART_PC, "Distancia: ");
    UartSendString(UART_PC, (char *)UartItoa(distancia, 10));
    UartSendString(UART_PC, " cm\r\n");
}

/**
 * @brief Tarea encargada de medir la distancia con el sensor ultrasónico.
 */
void TaskSensar(void *pvParameters) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  ///< Espera una notificación
        if (encendido) {
            distancia = HcSr04ReadDistanceInCentimeters();  ///< Lee la distancia
        }
    }
}

/**
 * @brief Tarea encargada de mostrar la distancia en el display y controlar los LEDs.
 */
void TaskLcd(void *pvParameters) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  ///< Espera una notificación
        if (encendido) {
            // Control de LEDs basado en la distancia
            if (distancia < 10) {
                LedsOffAll();
            } else if (distancia < 20) {
                LedOn(LED_1);
                LedOff(LED_2);
                LedOff(LED_3);
            } else if (distancia < 30) {
                LedOn(LED_1);
                LedOn(LED_2);
                LedOff(LED_3);
            } else {
                LedOn(LED_1);
                LedOn(LED_2);
                LedOn(LED_3);
            }

            // Actualiza el display si no está en modo 'hold'
            if (!hold) {
                LcdItsE0803Write(distancia);
            }

            escribirDistanciaEnPc();  ///< Muestra la distancia en el puerto serie
        } else {
            LcdItsE0803Off();  ///< Apaga el LCD cuando el sistema está apagado
            LedsOffAll();      ///< Apaga todos los LEDs
        }
    }
}

/**
 * @brief Cambia el estado de encendido del sistema.
 */
void cambiarEncendido(void) {
    encendido = !encendido;  ///< Cambia el estado de encendido/apagado
}

/**
 * @brief Cambia el estado de mantener (hold).
 */
void cambiarHold(void) {
    hold = !hold;  ///< Cambia el estado de hold
}

/**
 * @brief Procesa los comandos recibidos por el puerto serie para controlar el sistema.
 */
void procesarComandoSerie(void) {
    uint8_t comando;
    if (UartReadByte(UART_PC, &comando) == 1) {  ///< Lee el comando del puerto serie
        switch (comando) {
            case 'O':  ///< Cambia el estado de encendido
                cambiarEncendido();
                break;
            case 'H':  ///< Cambia el estado de hold
                cambiarHold();
                break;
        }
    }
}

/**
 * @brief Función del temporizador para notificar a la tarea de sensado.
 */
void FuncTimerMedir(void) {
    vTaskNotifyGiveFromISR(Medir_task_handle, pdFALSE);  ///< Notifica a la tarea de sensado
}

/**
 * @brief Función del temporizador para notificar a la tarea de mostrar.
 */
void FuncTimerMostrar(void) {
    vTaskNotifyGiveFromISR(Mostrar_task_handle, pdFALSE);  ///< Notifica a la tarea de mostrar
}


void iniciar(void) {
    // Inicialización de periféricos
    LedsInit();
    HcSr04Init(GPIO_3, GPIO_2);
    LcdItsE0803Init();
    SwitchesInit();

    // Configuración del puerto serie
    serial_config_t configSerial = {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = procesarComandoSerie,
        .param_p = NULL
    };
    UartInit(&configSerial);

    // Configuración del temporizador para la medición
    timer_config_t timer_medicion = {
        .timer = TIMER_A,
        .period = REFRESCO_MEDICION,
        .func_p = FuncTimerMedir,
        .param_p = NULL
    };
    TimerInit(&timer_medicion);

    // Configuración del temporizador para mostrar los resultados
    timer_config_t timer_mostrar = {
        .timer = TIMER_B,
        .period = REFRESCO_DISPLAY,
        .func_p = FuncTimerMostrar,
        .param_p = NULL
    };
    TimerInit(&timer_mostrar);

    // Configuración de las interrupciones de teclas
    SwitchActivInt(SWITCH_1, cambiarEncendido, NULL);
    SwitchActivInt(SWITCH_2, cambiarHold, NULL);

    // Creación de tareas para sensar y mostrar
    xTaskCreate(TaskSensar, "TaskSensar", 2048, NULL, 5, &Medir_task_handle);
    xTaskCreate(TaskLcd, "TaskLcd", 2048, NULL, 5, &Mostrar_task_handle);

    // Iniciar los temporizadores
    TimerStart(timer_medicion.timer);
    TimerStart(timer_mostrar.timer);
}

/*==================[external functions definition]==========================*/

void app_main(void) {
   
    iniciar();  ///< Inicializa el sistema

}
