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
 *
 */

/*==================[inclusions]=============================================*/

#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "gpio_mcu.h"

/*==================[macros and definitions]=================================*/
/*Escriba una función que reciba un dato de 32 bits,  la cantidad de dígitos de salida
  y dos vectores de estructuras del tipo  gpioConf_t.
  Uno  de estos vectores es igual al definido en el punto anterior y el otro vector mapea
  los puertos con el dígito del LCD a donde mostrar un dato:

● Dígito 1 -> GPIO1
● Dígito 2 -> GPIO3
● Dígito 3 -> GPIO5

La función deberá mostrar por display el valor que recibe.
Reutilice las funciones creadas en el punto 4 y 5.*/

typedef struct
	{
	uint8_t pin;  /*!< GPIO pin number */
	uint8_t dir;  /*!< GPIO direction ‘0’ IN; ‘1’ OUT */
	} gpioConf_t;

void BinaryToBcd (uint32_t data, uint8_t digits, uint8_t * bcd_number );

void BcdToLcd(uint8_t dig_Bcd , gpioConf_t *config);

void mostrarPorDisplay(uint32_t num, uint8_t digitos,  gpioConf_t *conf_lcd ,gpioConf_t *conf_gpio);

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


void BinaryToBcd (uint32_t data, uint8_t digits, uint8_t * bcd_number ){ //Se convierte cada digito de data en bcd y se lo almacena en bcd_number
	uint8_t i=0;
	for (i=0; i<digits; i++)
	{
		bcd_number[i]=data%10;
		data/=10;
	}
}


/** @brief Funcion BcdToLcd
 * Recibe como parámetro un dígito BCD y un vector de
 * estructuras del tipo gpioConf_t, que permite mapear cada digito BCD (4 bits)
 * al pin correspondiente del LCD (un bit por pin).
 *
 * @param[in]
 * ig_Bcd, es el dato en BCD.
 * gpioConf_t, struct que mapea el dato en BCD a los respectivos pines de salida en el LCD.
 *
 */

void BcdToLcd(uint8_t dig_Bcd, gpioConf_t *config){ //Mapea un dígito BCD en el puerto correspondiente del LCD.
	uint8_t i;
	for( i = 0; i<4; i++)
		{
			GPIOInit(config[i].pin,config[i].dir);
		}
	for(i=0; i<4; i++)
	{
		if(dig_Bcd & 1){
			GPIOOn(config[i].pin);
		}
		else{
			GPIOOff(config[i].pin);
		}
	dig_Bcd=dig_Bcd>>1;
	}
}


/** @brief Funcion mostrarPorDisplay
 * Con la ayuda de las funciones de los puntos anteriores, define una nueva estructura,
 * para determinar la cofiguracion de salida del BCD por el display
 *
 * @param[in]
 * datoDecimal, dato en decimal
 * cantidadDigitos, cantidad de digitos del dato en decimal
 * configuracionLcd, estructura para configurar el BCD a Lcd (bit a bit)
 * configuracionDisplay, estructura para configurar la salida por display
 *
 *
 */

void mostrarPorDisplay(uint32_t num, uint8_t cant,  gpioConf_t *conf_lcd ,gpioConf_t *conf_gpio)
{
	uint8_t vector[cant];
	BinaryToBcd(num, cant, vector);

	uint8_t i;
	for( i = 0; i<3; i++)
		{
			GPIOInit(conf_gpio[i].pin,conf_gpio[i].dir);
		}

	for( i = 0; i<3; i++)
	{
		BcdToLcd(vector[i],conf_lcd);
		//Se prende:
		GPIOOn(conf_gpio[i].pin);
		//Se espera un tiempo y se apaga:
		GPIOOff(conf_gpio[i].pin);
	}

}


/*==================[external functions definition]==========================*/

void app_main(void)
{

	//Mapea los puertos del LCD para mostrar un dato, trabaja con las patitas
	gpioConf_t configuracionLcd[4]= {{GPIO_20, GPIO_OUTPUT},{GPIO_21, GPIO_OUTPUT},{GPIO_22, GPIO_OUTPUT},{GPIO_23, GPIO_OUTPUT}};
	//Mapea los puertos con el dígito del LCD, trabaja con cada display, son 3 en total asi que el maximo numero que puedo repr es el 999
	//Se cambia el orden de la configuracion de la consigna,para que salgade manera correcta el numero por el display
	gpioConf_t configuracionDisplay[3]= {{GPIO_9, GPIO_OUTPUT},{GPIO_18, GPIO_OUTPUT},{GPIO_19, GPIO_OUTPUT}};
	uint32_t datoDecimal= 123;
	uint8_t cantidadDigitos= 3;
	mostrarPorDisplay(datoDecimal, cantidadDigitos , configuracionLcd ,configuracionDisplay);


	while(1)
	    {
			/* main loop */
	    }

}

/*==================[end of file]============================================*/

