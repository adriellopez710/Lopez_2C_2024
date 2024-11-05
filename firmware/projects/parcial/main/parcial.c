/** 
 * @file parcial.c
 * @brief Código para la actividad 2 del proyecto 2: Medidor de distancia con interrupciones.
 * @mainpage Medición de Distancia con Sensor HC-SR04, con interrupciones
 * @section desc Descripción General
 * 
 * Parcial de Electronica Programable
 * 
 * @section changelog Historial de Cambios
 * | Fecha       | Descripción                                    |
 * |:-----------:|:-----------------------------------------------|
 * | 4/11/2024  | Creación del documento                         |
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
#include "portmacro.h"
#include "uart_mcu.h"
#include <math.h>
#include "i2c_mcu.h"
#include "mpu6050.h"
#include "buzzer.h"
#include "delay_mcu.h"
#include "pwm_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_SENSING_PERIOD_US_SENS 500000 ///< Periodo de sensado de 2 Hz (500 ms = 500000 us)
#define CONFIG_SENSING_PERIOD_US_ACEL 10000 ///< Periodo de sensado de 100 Hz (10 ms = 10000 us)
//#define CONFIG_FREERTOS_HZ 1000 //<1000 ticks por segundo
#define UMBRAL 4                // Umbral    
/*==================[internal data definition]===============================*/
TaskHandle_t sensor_task_handle = NULL;   ///< Handler para la tarea de sensado
TaskHandle_t acel_task_handle = NULL; 
TaskHandle_t led_task_handle = NULL;      ///< Handler para la tarea de control de LEDs

uint16_t distancia = 0;  ///< Distancia medida por el sensor

/*==================[internal functions definition]=========================*/
/**
 * @brief Función invocada en la interrupción del temporizador para sensar la distancia.
 * Notifica a la tarea de sensado de distancia.
 */
void FuncTimerSensor(void* param) {
    vTaskNotifyGiveFromISR(sensor_task_handle, pdFALSE);  // Notifica a la tarea de sensado
}

/**
 * @brief Función invocada en la interrupción del temporizador para detectar la aceleracion.
 * Notifica a la tarea de sensado de distancia.
 */
void FuncTimerAcel(void* param) {
    vTaskNotifyGiveFromISR(acel_task_handle, pdFALSE);  // Notifica a la tarea de aceleracion
}

/**
 * @brief Función de la tarea de sensado de distancia.
 * Esta tarea se notifica a través de interrupciones de temporizador.
 */
static void SensorTask(void *pvParameter) {
    while (true) {
        // Espera notificación del temporizador
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  

        // Mide la distancia con el sensor ultrasónico
        distancia = HcSr04ReadDistanceInCentimeters();
        distancia = distancia/100; // se pasa a metro
        // Una vez sensada una distancia, notifica a las tareas de LEDs 
        xTaskNotifyGive(led_task_handle);  
  
    }
}

/**
 * @brief Función de la tarea de control de LEDs.
 * Se ejecuta tras la notificación de la tarea de sensado.
 */
static void LedTask(void *pvParameter) {
    while (true) {

        // Espera notificación del temporizador
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  
        
        // Enviar estado de los LEDs por Bluetooth
        if (distancia < 3) {
            LedOn(LED_1);       // verde
            LedOn(LED_2);       // amarillo
            LedOn(LED_3);       // rojo

            // Enviar mensaje de peligro
            UartSendString(2, "Peligro, vehiculo cerca");
        
            // Enciende el buzzer de peligro
            BuzzerOn();
            vTaskDelay(500 / portTICK_PERIOD_MS);  // Espera cada 0,5 sengundos

        } else if ( distancia > 3 && distancia <5 ) {
            LedOn(LED_1);       // led verde
            LedOn(LED_2);       // led amarillo
            LedOff(LED_3);       

            // Enviar mensaje de precaucion
            UartSendString(2, "Precaución, vehiculo cerca");
        
            // Enciende el buzzer de precaucion
            BuzzerOn();
            vTaskDelay(1000 / portTICK_PERIOD_MS);  // Espera cada 1 segundo

        } else if (distancia > 5) {

            LedOn(LED_1);       // led verde
            LedOff(LED_2);   
            LedOff(LED_3);    

        } 

    }
}

static void AcelTask(void *pvParameter) {
    
    // Espera notificación del temporizador
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  
    
    float sumatoria = 0;
    // Leer aceleración y calcular sumatoria
    int16_t ax = MPU6050_getAccelerationX();
    int16_t ay = MPU6050_getAccelerationY();
    int16_t az = MPU6050_getAccelerationZ();

    sumatoria = abs(ax)+abs(ay)+abs(az);    

    while (true) {
        
        // Activar/desactivar la medición de distancia basado en el umbral
        if (sumatoria > UMBRAL) {
            
            // Enviar mensaje de caida
            UartSendString(2, "Caida detectada");
  
        }
    }
}


/**
 * @brief Inicializa el sistema, incluyendo periféricos, temporizadores y tareas.
 */
void iniciar(void) {
    // Inicialización de periféricos
    LedsInit();         ///< Inicializa los LEDs
    SwitchesInit();     ///< Inicializa las teclas
    HcSr04Init(GPIO_3, GPIO_2); ///< Configura los pines para el sensor
    BuzzerInit(2);       // Inicializacion del buzzer

    /* Configura el temporizador para el sensor de distancia */
    timer_config_t timer_sensor = {
        .timer = TIMER_A,
        .period = CONFIG_SENSING_PERIOD_US_SENS,  // Período de sensado de 100 Hz
        .func_p = FuncTimerSensor,
        .param_p = NULL
    };
    TimerInit(&timer_sensor);

    /* Configura el temporizador para el acelerometro */
    timer_config_t timer_acel = {
        .timer = TIMER_B,
        .period = CONFIG_SENSING_PERIOD_US_ACEL,  // Período de sensado de 100 Hz
        .func_p = FuncTimerAcel,
        .param_p = NULL
    };
    TimerInit(&timer_acel);


    /* Configuración del puerto serie */
    serial_config_t ControlarUart =
        {
            .port = 2,
            .baud_rate = 115200,
            .func_p = NULL,
            .param_p = NULL,
        };
    UartInit(&ControlarUart);


    /* Creación de las tareas */
    xTaskCreate(&SensorTask, "SensorTask", 2048, NULL, 5, &sensor_task_handle);   ///< Tarea de sensado
    xTaskCreate(&LedTask, "LedTask", 2048, NULL, 4, &led_task_handle);            ///< Tarea de control de LEDs
    xTaskCreate(&AcelTask, "AcelTask", 2048, NULL, 6, &acel_task_handle);          ///< Tarea para el acelerometro

    /* Inicia de los timer */
    TimerStart(timer_sensor.timer);
    TimerStart(timer_acel.timer);

}

/*==================[external functions definition]==========================*/
/**
 * @brief Función principal que configura e inicia el sistema con FreeRTOS.
 */
void app_main(void) {
    iniciar();  ///< Llama a la función de inicialización
}
