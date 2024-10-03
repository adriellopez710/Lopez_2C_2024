/** 
 * @file guia2_ej2.c
 * @brief Código para la actividad 2 del proyecto 2: Medidor de distancia con interrupciones.
 *
 * Actividad 2 - Proyecto: Medidor de distancia por ultrasonido c/interrupciones
 *
 * Consigna: Cree un nuevo proyecto en el que modifique la actividad del punto 1 de manera de utilizar 
 * interrupciones para el control de las teclas y el control de tiempos (Timers).
 */
/** 
 * @mainpage Medición de Distancia con Sensor HC-SR04, con interrupciones
 * @section desc Descripción General
 * 
 * Este proyecto implementa un medidor de distancia utilizando un sensor ultrasónico HC-SR04, 
 * controlado mediante interrupciones para la gestión de teclas y temporización. El sistema 
 * realiza mediciones precisas de distancia y permite ajustar las unidades de medida (centímetros 
 * y pulgadas) a través de comandos recibidos por puerto serie desde un ordenador.
 * 
 * @section changelog Historial de Cambios
 * | Fecha       | Descripción                                    |
 * |:-----------:|:-----------------------------------------------|
 * | 13/09/2024  | Creación del documento                         |
 * 
 * @author
 * Adriel Lopez
 */
/*==================[inclusions]=============================================*/
#include "led.h"             ///< Biblioteca para control de LEDs
#include "switch.h"          ///< Biblioteca para el control de teclas
#include "lcditse0803.h"     ///< Biblioteca para el display LCD
#include "hc_sr04.h"         ///< Biblioteca para el sensor ultrasónico HC-SR04
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "gpio_mcu.h"
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_SENSING_PERIOD_US 1000000 ///< Se lleva a un millón para evitar el watchdog

/*==================[internal data definition]===============================*/
TaskHandle_t sensor_task_handle = NULL;   ///< Handler para la tarea de sensado
TaskHandle_t led_task_handle = NULL;      ///< Handler para la tarea de control de LEDs
TaskHandle_t lcd_task_handle = NULL;      ///< Handler para la tarea de actualización del LCD

bool encendido = true;   ///< Estado del sistema (encendido/apagado)
bool hold = false;       ///< Estado de pausa (activo/inactivo)
uint16_t distancia = 0;  ///< Distancia medida por el sensor

/*==================[internal functions definition]=========================*/
/**
 * @brief Función de la tarea de sensado de distancia.
 * Esta tarea se notifica a través de interrupciones de temporizador.
 */
static void SensorTask(void *pvParameter) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Espera notificación del temporizador

        if (encendido && !hold) {
            // Mide la distancia con el sensor ultrasónico
            distancia = HcSr04ReadDistanceInCentimeters();
        }

        // Notifica a las tareas de LEDs y LCD que hay una nueva distancia
        xTaskNotifyGive(led_task_handle);  // Notifica a la tarea de control de LEDs
        xTaskNotifyGive(lcd_task_handle);  // Notifica a la tarea de actualización del LCD
    }
}

/**
 * @brief Función de la tarea de control de LEDs.
 * Se ejecuta tras la notificación de la tarea de sensado.
 */
static void LedTask(void *pvParameter) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Espera notificación de la tarea de sensado

        if (encendido && !hold) {
            // Control de LEDs según la distancia medida
            if (distancia < 10) {
                LedOff(LED_1);
                LedOff(LED_2);
                LedOff(LED_3);
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
        }
    }
}

/**
 * @brief Función de la tarea de actualización del LCD.
 * Se ejecuta tras la notificación de la tarea de sensado.
 */
static void LcdTask(void *pvParameter) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Espera notificación de la tarea de sensado

        if (encendido && !hold) {
            // Actualiza la pantalla LCD con la distancia medida
            LcdItsE0803Write(distancia);
        }
    }
}

/**
 * @brief Función invocada en la interrupción del temporizador para sensar la distancia.
 * Notifica a la tarea de sensado de distancia.
 */
void FuncTimerSensor(void* param) {
    vTaskNotifyGiveFromISR(sensor_task_handle, pdFALSE);  // Notifica a la tarea de sensado
}

/**
 * @brief Alterna el estado del sistema (encendido/apagado).
 * Callback de la interrupción de la tecla 1.
 */
void cambiarEncendido(void) {
    encendido = !encendido;  // Alterna el estado del sistema
}

/**
 * @brief Alterna el estado de hold (pausa).
 * Callback de la interrupción de la tecla 2.
 */
void cambiarHold(void) {
    hold = !hold;  // Alterna el estado de pausa
}

/**
 * @brief Inicializa el sistema, incluyendo periféricos, temporizadores y tareas.
 */
void iniciar(void) {
    // Inicialización de periféricos
    LcdItsE0803Init();  ///< Inicializa el display LCD
    LedsInit();         ///< Inicializa los LEDs
    SwitchesInit();     ///< Inicializa las teclas
    HcSr04Init(GPIO_3, GPIO_2); ///< Configura los pines para el sensor

    // Configuración de interrupciones de teclas
    SwitchActivInt(SWITCH_1, cambiarEncendido, NULL);  ///< Asigna la tecla 1 a cambiarEncendido
    SwitchActivInt(SWITCH_2, cambiarHold, NULL);       ///< Asigna la tecla 2 a cambiarHold

    /* Configura el temporizador para el sensor de distancia */
    timer_config_t timer_sensor = {
        .timer = TIMER_A,
        .period = CONFIG_SENSING_PERIOD_US,  // Período de sensado de 1 segundo
        .func_p = FuncTimerSensor,
        .param_p = NULL
    };
    
    TimerInit(&timer_sensor);

    /* Creación de las tareas */
    xTaskCreate(&SensorTask, "SensorTask", 2048, NULL, 5, &sensor_task_handle);   ///< Tarea de sensado
    xTaskCreate(&LedTask, "LedTask", 2048, NULL, 4, &led_task_handle);            ///< Tarea de control de LEDs
    xTaskCreate(&LcdTask, "LcdTask", 2048, NULL, 3, &lcd_task_handle);            ///< Tarea de actualización del LCD

    /* Inicia el temporizador */
    TimerStart(timer_sensor.timer);
}

/*==================[external functions definition]==========================*/
/**
 * @brief Función principal que configura e inicia el sistema con FreeRTOS.
 */
void app_main(void) {
    
    iniciar();  ///< Llama a la función de inicialización

}
