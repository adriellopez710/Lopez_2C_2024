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
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
/*==================[macros and definitions]=================================*/
//#define CONFIG_BLINK_PERIOD 1000
#define ON 0
#define OFF 1
#define TOG 2

/*==================[internal data definition]===============================*/


struct leds
{
    uint8_t n_led;      //indica el número de led a controlar
    uint8_t n_ciclos;   //indica la cantidad de ciclos de encendido/apagado
    uint16_t periodo;    //indica el tiempo de cada ciclo
    uint8_t mode;       //ON, OFF, TOGGLE
} my_leds;



/*==================[internal functions declaration]=========================*/

/** @brief Funcion ControlLed
 * Controla el encendido o apagado de leds (1,2,3) respondiendo al diagrama de flujo dado
 * en el problema.
 *
 * @param[in]
 * leds, contiene la informacion del led a controlar.
 *
 */

void ControlLed(struct leds *my_leds) {

	switch (my_leds->mode) {
	case ON:
	{
		switch (my_leds->n_led) {
		case 1:
			LedOn(LED_1);
			break;
		case 2:
			LedOn(LED_2);
			break;
		case 3:
			LedOn(LED_3);
			break;

		}
	}
		break;
	case OFF: {
		switch (my_leds->n_led) {
		case 1:
			LedOff(LED_1);
			break;
		case 2:
			LedOff(LED_2);
			break;
		case 3:
			LedOff(LED_3);
			break;

		}
	}
		break;

	case TOG:
	{
		for (int i = 0; i < my_leds->n_ciclos; i++) {

			switch (my_leds->n_led) {

			case 1:
				LedToggle(LED_1);
				break;
			case 2:
				LedToggle(LED_2);
				break;
			case 3:
				LedToggle(LED_3);
				break;

			}

			for (int j = 0; j < my_leds->periodo/100; j++)
            {
                vTaskDelay(100 / portTICK_PERIOD_MS);

            }
				;

		}
	}
	break;
	}
}




/*==================[external data definition]===============================*/


/*==================[external functions definition]==========================*/
 void app_main(void){
	/* initializations */
	struct leds led;
	LedsInit();
	led.mode = TOG;
	led.n_ciclos =10;
	led.periodo = 500;
	led.n_led =1;
	ControlLed(&led);
    while(1)
    {
		/* main loop */

    }

}

/*==================[end of file]============================================*/

