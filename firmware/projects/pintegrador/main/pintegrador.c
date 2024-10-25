/**
 * @file pintegrador.c
 * @brief Programa para el monitoreo de sacudidas en una ambulancia usando ESP32C6 y acelerómetro I2C.
 *
 * Este código mide la aceleración en los ejes X, Y, Z, detecta sacudidas excesivas, 
 * y envía los datos a un smartphone por Bluetooth.
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
float umbral = 0.8;

/*==================[internal functions declaration]=========================*/
void sensarAceleracion(float *modulo);
void conexionBluetooth(float modulo);

/*==================[internal functions definition]==========================*/
/**
 * @brief Función que sensa la aceleración y calcula su módulo
 */
void sensarAceleracion(float *modulo) {
    int16_t ax = MPU6050_getAccelerationX();
    int16_t ay = MPU6050_getAccelerationY();
    int16_t az = MPU6050_getAccelerationZ();

    // Calcular el módulo de la aceleración
    //*modulo = sqrt(ax*ax + ay*ay + az*az) / 16384.0;  // Normalizar a valores en g
    *modulo = sqrt(ax*ax + ay*ay + az*az);
}

/**
 * @brief Función que envía los datos por Bluetooth y controla la bocina y el botón
 */
void conexionBluetooth(float modulo) {

    if (modulo > umbral) {  // Si el módulo de la aceleración supera el umbral
        
        if (boton) { // si el boton esta encendido (ON, C mayuscula)
            printf("Medición pausada.\n");   // ademas no se envia la orden para encender la bocina
    
        } else {        // Solo si el control esta apagado (OFF, c minuscula)
            printf("Bocina activada, aceleración: %.2f g\n", modulo);
    		printf(">x1: %d, y1:%d,z1:%d\r\n", MPU6050_getAccelerationX(), MPU6050_getAccelerationY(), MPU6050_getAccelerationZ());

            // Enviar por bluetooth la orden para encender la bocina
            char msg_bocina[128];
            sprintf(msg_bocina, "*SV100*"); // Bocina con volumen al 100 %
            BleSendString(msg_bocina);

        }
    }
    

    // Enviar por bluetooth el modulo
    char msg_mod[128];
    sprintf(msg_mod, "*G%.2f", modulo);
    BleSendString(msg_mod);
    
    
    // Enviar por bluetooth el umbral
    char msg_umb[128];
    sprintf(msg_umb, "*G%.2f", umbral);
    BleSendString(msg_umb);

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
    float modulo = 0.0;
    while (true) {
        //vTaskDelay(CONFIG_ACC_PERIOD / portTICK_PERIOD_MS);
        vTaskDelay(CONFIG_ACC_PERIOD / 1000);
        // Sensar aceleración y calcular el módulo
        sensarAceleracion(&modulo);
        // Conectar y enviar datos vía Bluetooth
        conexionBluetooth(modulo);
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

    // Crear tarea de acelerómetro
    xTaskCreate(&acc_task, "ACC Task", 4096, NULL, 5, NULL);
}

/*==================[end of file]============================================*/
