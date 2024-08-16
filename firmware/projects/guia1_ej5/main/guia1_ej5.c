/*! @mainpage Blinking
 *
 * \section genDesc General Description
 *
 * This example makes LED_1 blink.
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Name Lopez Adriel
 *
 *  Escribir una función que reciba como parámetro un dígito BCD y un vector de
 *  estructuras del tipo gpioConf_t. Incluya el archivo de cabecera gpio.h
 *
 *
 * #include gpio.h
 * typedef struct
 * {
 * 	gpio_t pin;  	!< GPIO pin number
 *	io_t dir; 		!< GPIO direction '0' IN; '1' OUT
 * } gpioConf_t;
 *
 *
 * Defina un vector que mapee los bits de la siguiente manera:
 *
 * b0 -> GPIO_20
 * b1 -> GPIO_21
 * b2 -> GPIO_22
 * b3 -> GPIO_23
 *
 * La función deberá establecer en qué valor colocar cada bit del dígito BCD e
 * indexando el vector anterior, operar sobre el puerto y pin que corresponda.
 */


/*==================[inclusions]=============================================*/

#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "led.h"
#include "gpio_mcu.h"


/*==================[macros and definitions]=================================*/

// Es para saber que patita voy a encender o apagar del lcd
typedef struct
	{
	uint8_t pin;  /*!< GPIO pin number, es la patita */
	uint8_t dir;  /*!< GPIO direction ‘0’ IN; ‘1’ OUT, en nuestro caso es siempre de salida, o sea out */
	} gpioConf_t;


void BinaryToBcd (uint32_t data, uint8_t digits, uint8_t * bcd_number );

void BcdToLcd(uint8_t dig_Bcd , gpioConf_t *config);

/*==================[internal functions declaration]=========================*/

/** @brief Funcion BcdToLcd
 * Reciba como parámetro un dígito BCD y un vector de
 * estructuras del tipo gpioConf_t, que permite mapear EL unico digito BCD (4 bits)
 * al pin correspondiente del LCD (un bit por pin).
 *
 * @param[in]
 * ig_Bcd, es el dato en BCD(previa conversion de decimal a BCD)
 * gpioConf_t, struct que mapea el dato en BCD al respectivo pin para la salida por el LCD
 *
 */

void BinaryToBcd (uint32_t data, uint8_t digits, uint8_t * bcd_number ){
	uint8_t i;
	for (i=0; i<digits; i++)
	{
		bcd_number[i]=data%10;  				//Se obtiene el resto de la división entre data y 10 y lo almacena en la posición i del arreglo bcd_number. El residuo de la división entre data y 10 es el equivalente en BCD del último dígito del número original.
		data/=10;                               //Se divide "data" entre 10 quedandose sin la parte decimal y se guarda el resultado en la misma variable "data", para eliminar el último dígito que ya ha sido procesado
	}
}

//Para la primer iterazion del decimal 123, el digito Bcd es 3, que en binario es 0011,
//sin embargo al analizar el binario bit a bit se hace de manera invertida, es decir: 1, 1, 0 y 0
void BcdToLcd(uint8_t dig_Bcd, gpioConf_t *config){

	//Se mapea el pin correspondiente para cada bit
	uint8_t i;
	for( i = 0; i<4; i++)
		{
			GPIOInit(config[i].pin,config[i].dir);// notar que en todas las patitas salen el mismo numero
		}

	for(i=0; i<4; i++) 			//Se utiliza un ciclo for para recorrer los 4 bits del dígito BCD
	{
		if(dig_Bcd & 1){		//Se verifica si está activado (1) o desactivado (0) utilizando la operación AND (&) y la máscara "1"
			//En este caso se pone en 1 el pin correspondiente
			GPIOOn(config[i].pin);
		}
		else{
			//En este caso se pone en 0 el pin correspondiente
			GPIOOff(config[i].pin);
		}

	dig_Bcd=dig_Bcd>>1; 		//Se desplaza dig_Bcd un bit a la derecha (>>) para examinar el siguiente bit en la siguiente iteración del ciclo for.

	}

}


/*==================[external data definition]===============================*/


/*==================[external functions definition]==========================*/

void app_main(void)
{
	gpioConf_t configuracion[4]= {{GPIO_20, GPIO_OUTPUT},{GPIO_21, GPIO_OUTPUT},{GPIO_22, GPIO_OUTPUT},{GPIO_23, GPIO_OUTPUT}};

	uint32_t datoDecimal=123;		  			//Dato en decimal, a convertir de binario a bcd
	uint8_t cantidadDigitos=3; 					//Cantidad de dígitos de salida
	uint8_t arregloBcd[cantidadDigitos];     	//Arreglo de digitos bcd a cargar
	BinaryToBcd(datoDecimal, cantidadDigitos, arregloBcd);

	uint8_t i;
	for (i=0; i<3; i++) 						//Se utiliza un ciclo for para recorrer los 5 digitos de la salida(1 al 3)
		{
			BcdToLcd(arregloBcd[i], configuracion);
        	vTaskDelay(10000 / portTICK_PERIOD_MS);
		}


	while(1)
	    {
			/* main loop */

	    }

}

/*==================[end of file]============================================*/

