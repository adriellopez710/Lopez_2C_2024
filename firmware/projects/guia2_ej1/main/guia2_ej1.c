/** 
 * @file guia2_ej1.c
 * @brief Código para la actividad 1 del proyecto 2.
 *
 * Actividad 1: Diseñar el firmware modelando con un diagrama de flujo de manera que cumpla con las siguientes funcionalidades:
 *
 * Mostrar distancia medida utilizando los leds de la siguiente manera:
 *
 * Si la distancia es menor a 10 cm, apagar todos los LEDs.
 * Si la distancia está entre 10 y 20 cm, encender el LED_1.
 * Si la distancia está entre 20 y 30 cm, encender el LED_2 y LED_1.
 * Si la distancia es mayor a 30 cm, encender el LED_3, LED_2 y LED_1.
 *
 * Mostrar el valor de distancia en cm utilizando el display LCD.
 * Usar TEC1 para activar y detener la medición.
 * Usar TEC2 para mantener el resultado (“HOLD”).
 * Refresco de medición: 1 s
 *
 */

/** 
 * @mainpage Medición de Distancia con Sensor HC-SR04
 * @section desc Descripción General
 * 
 * Este proyecto mide la distancia usando un sensor ultrasónico y muestra el resultado en LEDs y un LCD. 
 * Las teclas controlan el encendido y la pausa.
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_mcu.h"

#define CONFIG_BLINK_PERIOD 1000  ///< Periodo de parpadeo para el sistema

/*==================[macros and definitions]=================================*/

/** 
 * @brief Estado de pausa del sistema.
 * Si `hold` es verdadero, el sistema no realiza mediciones.
 */
bool hold = false;    

/** 
 * @brief Estado de encendido del sistema.
 * Si `encendido` es verdadero, el sistema está activo.
 */
bool encendido = true; 

/** 
 * @brief Distancia medida por el sensor en centímetros.
 */
uint16_t distancia = 0; 

/*==================[internal functions declaration]=========================*/

/**
 * @brief Inicializa los periféricos del sistema.
 *
 * Configura el display LCD, los LEDs, las teclas y el sensor ultrasónico.
 */
void iniciar() {
    LcdItsE0803Init();  ///< Inicializa el display LCD
    LedsInit();         ///< Inicializa los LEDs
    SwitchesInit();     ///< Inicializa las teclas
    HcSr04Init(GPIO_3, GPIO_2); ///< Configura los pines para el sensor
}

/**
 * @brief Tarea que realiza la medición de distancia.
 *
 * Si el sistema está encendido y no está en pausa, lee la distancia utilizando el sensor ultrasónico.
 *
 * @param[in] pvParameter Parámetro de la tarea (no utilizado).
 */
void sensar(void *pvParameter) {
    while (1) {
        if (encendido && !hold) {
            distancia = HcSr04ReadDistanceInCentimeters(); ///< Lee la distancia medida por el sensor
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);  ///< Espera 1 segundo entre mediciones
    }
}

/**
 * @brief Tarea que controla los LEDs según la distancia medida.
 *
 * Si el sistema está encendido y no está en pausa, ajusta el encendido de los LEDs basado en la distancia.
 *
 * @param[in] pvParameter Parámetro de la tarea (no utilizado).
 */
void led(void *pvParameter) {
    while (1) {
        if (encendido && !hold) {
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
        vTaskDelay(1000 / portTICK_PERIOD_MS);  ///< Espera 1 segundo para la actualización de LEDs
    }
}

/**
 * @brief Tarea que muestra la distancia medida en el LCD.
 *
 * Si el sistema está encendido y no está en pausa, muestra la distancia en centímetros en el display LCD.
 *
 * @param[in] pvParameter Parámetro de la tarea (no utilizado).
 */
void lcd(void *pvParameter) {
    while (1) {
        if (encendido && !hold) {
            LcdItsE0803Write(distancia);  ///< Muestra la distancia en el display LCD
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);  ///< Refresca la visualización cada 0.5 segundos
    }
}

/**
 * @brief Tarea que lee el estado de las teclas.
 *
 * Usa TEC1 para activar/detener el sistema y TEC2 para pausar/reanudar la medición.
 *
 * @param[in] pvParameter Parámetro de la tarea (no utilizado).
 */
void leerTecla(void *pvParameter) {
    while (1) {
        int8_t tecla = SwitchesRead();  ///< Lee el estado de las teclas
        if (tecla == SWITCH_1) {
            encendido = !encendido;  ///< Cambia el estado del sistema (encendido/apagado)
        } else if (tecla == SWITCH_2) {
            hold = !hold;  ///< Cambia el estado de pausa (medición/pausa)
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);  ///< Espera 1 segundo entre lecturas de teclas
    }
}

/*==================[external functions definition]==========================*/

/**
 * @brief Función principal del programa.
 *
 * Inicializa el sistema y crea las tareas de medición, control de LEDs, visualización en LCD y lectura de teclas.
 */
void app_main(void) {
    iniciar();  ///< Inicializa periféricos y configuraciones
    xTaskCreate(&sensar, "sensar", 2048, NULL, 5, NULL);  ///< Crea la tarea de sensado de distancia
    xTaskCreate(&led, "led", 2048, NULL, 5, NULL);        ///< Crea la tarea de control de LEDs
    xTaskCreate(&lcd, "lcd", 2048, NULL, 5, NULL);        ///< Crea la tarea de visualización en LCD
    xTaskCreate(&leerTecla, "leerTecla", 2048, NULL, 5, NULL); ///< Crea la tarea de lectura de teclas
}

/*==================[end of file]============================================*/
