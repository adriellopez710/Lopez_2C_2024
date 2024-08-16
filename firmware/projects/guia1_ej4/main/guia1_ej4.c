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
 * Escriba una función que reciba un dato de 32 bits,
 * la cantidad de dígitos de salida y un puntero a un arreglo donde se almacene los n dígitos.
 * La función deberá convertir el dato recibido a BCD,
 * guardando cada uno de los dígitos de salida en el arreglo pasado como puntero.
 *
 * int8_t  binaryToBcd (uint32_t data, uint8_t digits, uint8_t * bcd_number)
 * {
 *
 * }
 *
 */
 
/*==================[inclusions]=============================================*/

#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"

/*==================[macros and definitions]=================================*/

void BinaryToBcd (uint32_t data, uint8_t digits, uint8_t * bcd_number );

/*==================[internal data definition]===============================*/


/*==================[internal functions declaration]=========================*/

/** @brief BinaryToBcd
 * Convierte el dato recibido a BCD,
 * guardando cada uno de los dígitos de salida en el arreglo pasado como puntero.
 *
 * @param[in]
 * data, es el dato en decimal, a convertir de binario a bcd.
 * digits, es la cantidad de digitos a la salida.
 * bcd_number, es el arreglo de digitos para pasar a bcd.
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



/*==================[external data definition]===============================*/


/*==================[external functions definition]==========================*/

void app_main(void)
{
	uint32_t datoDecimal=123;  			        //Dato en decimal, a convertir de binario a bcd
	uint8_t cantidadDigitos=3; 					//Cantidad de dígitos de salida
	uint8_t arregloBcd[cantidadDigitos];     	//Arreglo de digitos bcd a cargar
	BinaryToBcd(datoDecimal, cantidadDigitos, arregloBcd);

	for (int i = 0; i < cantidadDigitos; i++) {
    	printf("%c", arregloBcd[i]);
	}

	while(1)
	{
			/* main loop */

	}

}

/*==================[end of file]============================================*/

