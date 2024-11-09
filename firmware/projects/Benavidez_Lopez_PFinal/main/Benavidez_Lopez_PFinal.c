/**
 * @file Benavidez_Lopez_PFinal.c
 * @brief Código para el proyecto final integrador.
 *
 * Este código implementa un sistema de monitoreo y control de aceleración en una ambulancia en movimiento.
 * Utiliza un sensor MPU6050 para sensar la aceleración en tres ejes y calcula su módulo. Si la aceleración
 * detectada supera un umbral predefinido, el sistema activa una bocina y envía los datos vía Bluetooth.
 * También permite pausar la medición a través de un botón de control, enviado como comando Bluetooth.
 * 
 * Funcionalidades adicionales incluyen el envío de datos de aceleración y estado de la bocina al dispositivo
 * receptor mediante Bluetooth. El código está optimizado para FreeRTOS, permitiendo un control preciso y 
 * continuo de la aceleración en ambientes móviles.
 *
 * OPCIONAL:
 * Agregar un segundo acelerómetro al modelo de ambulancia.
 *
 */
/** 
 * @mainpage Control de movimiento en ambulancia en traslado de paciente.
 * @section desc Descripción General
 * 
 * Este proyecto está diseñado para monitorear la aceleración dentro de una ambulancia en movimiento 
 * y, en caso de detectar valores peligrosos, activar alertas auditivas y enviar datos en tiempo real
 * vía Bluetooth. El sistema proporciona un control basado en un umbral preconfigurado y permite pausar 
 * el monitoreo mediante un botón de control.
 * 
 * @section changelog Historial de Cambios
 * | Fecha       | Descripción                                    |
 * |:-----------:|:-----------------------------------------------|
 * | 5/11/2024  | Creación del documento                         |
 * 
 * @author
 * Adriel Lopez
 * Jeronimo Benavidez
 */


/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_mcu.h"
#include "mpu6050.h"
#include "ble_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_ACC_PERIOD 50

/*==================[internal data definition]===============================*/
bool boton = false;            // Controla el estado del boton (c minuscula = false = apagado)
float umbral = 1.80;            // Umbral de medicion
float modulo = 0;              // Modulo de la aceleracion con sus tres ejes
/*==================[internal functions declaration]=========================*/
void sensarAceleracion();
void conexionBluetooth();

/*==================[internal functions definition]==========================*/
/**
 * @brief Función que sensa la aceleración y calcula su módulo
 */
void sensarAceleracion() {
    int16_t ax = MPU6050_getAccelerationX();
    int16_t ay = MPU6050_getAccelerationY();
    int16_t az = MPU6050_getAccelerationZ();

    // Calcular el módulo de la aceleración
    modulo = sqrt(ax*ax + ay*ay + az*az) / 16413.17;  // Normalizar a valores en g
    
}

/**
 * @brief Función que envía los datos por Bluetooth y controla la bocina y el botón
 */
void conexionBluetooth() {

    if (modulo > umbral) {  // Si el módulo de la aceleración supera el umbral
        
        if (boton) { // si el boton esta encendido (ON, C mayuscula)
            printf("Medición pausada.\n");   // ademas no se envia la orden para encender la bocina
    
        } else {        // Solo si el control esta apagado (OFF, c minuscula)
            printf("Bocina activada, aceleración: %.2f g, umbral: %.2f g\n", modulo, umbral);
            printf(">x1: %d, y1:%d,z1:%d\r\n", MPU6050_getAccelerationX(), MPU6050_getAccelerationY(), MPU6050_getAccelerationZ());

            // Enviar por bluetooth la orden para encender la bocina
            char msg_bocina[128];
            sprintf(msg_bocina, "*SV100*"); // Bocina con volumen al 100 %
            BleSendString(msg_bocina);

        }
    }
    
    
    // Enviar por bluetooth el umbral y el modulo
    char msg_datos[128];
    sprintf(msg_datos, "*G%.2f,%.2f*", umbral, modulo);
    BleSendString(msg_datos);
    
}

// Recibir por bluetooth el estado del boton
void read_data_boton(uint8_t * data, uint8_t length){
    switch(data[0]){ 
        case 'C':
            boton = true;
            break;
        case 'c':
            boton = false;
            break;
    }
}


/*==================[tasks definition]=======================================*/
void acc_task(void *pvParameter) {
    while (true) {
        vTaskDelay(CONFIG_ACC_PERIOD / portTICK_PERIOD_MS);
        // Sensar aceleración y calcular el módulo
        sensarAceleracion();
        // Conectar y enviar datos vía Bluetooth
        conexionBluetooth();
    }
}

/*==================[external functions definition]==========================*/
void app_main(void) {

    I2C_initialize(400000);
    MPU6050_initialize(MPU6050_ADDRESS_AD0_LOW);

    // Inicializar Bluetooth
    ble_config_t ble_configuration = {
        "ESP_AMBU",
        read_data_boton
    };

    BleInit(&ble_configuration);

    // Creamos tarea de acelerómetro 
    xTaskCreate(&acc_task, "ACC Task", 4096, NULL, 5, NULL);
}

/*==================[end of file]============================================*/
