#include <stdint.h>
#include "bsp/bsp.h"

/**
 * @brief Delay por software
 *
 * @param nCount Numero de ciclos del delay
 */
void Delay(volatile uint32_t nCount);

/**
 * @brief Se encarga de prender un led y apagarlo luego de un tiempo
 *
 * @param led    Numero de led a pulsar
 * @param tiempo Numero de ciclos del delay entre prendido y apagado
 */
void pulsoLed(uint8_t led, uint32_t tiempo);

/**
 * @brief Aplicacion principal
 */
int main(void) {
	bsp_init();
	int brillo = 0;
	int flag = 0;
	float acc_x;
	float acc_y;
	float acc_z;

	while (1) {

		bsp_delay_ms(10);

//		led_set_bright(0, brillo);
//		led_set_bright(1, brillo);
//		led_set_bright(2, brillo);
//		led_set_bright(3, brillo);
//
//		if (brillo >= 100)
//			flag = 0;
//		if (brillo <= 0)
//			flag = 1;
//		if (flag)
//			brillo++;
//		else
//			brillo--;

		acc_x = 100*bsp_get_acc('x');
		acc_y = 100*bsp_get_acc('y');
		acc_z = 100*bsp_get_acc('z');

		if (acc_x<0){
			led_set_bright(1, 0);
			led_set_bright(0, acc_x*(-1));
		} else {
			led_set_bright(0, 0);
			led_set_bright(1, acc_x);
		}
	}
}


void pulsoLed(uint8_t led, uint32_t tiempo){
	led_on(led);
	Delay(tiempo);
	led_off(led);
}

void Delay(volatile uint32_t nCount) {
	while (nCount--) {
	}
}

void APP_ISR_sw (void){

}

void APP_ISR_1ms (void){
	static uint16_t count_1s = 1000; // static: es una variable que se declara una sola vez, se fija en 0, y luego cada vez que entramos en la funci�n, count conserva el valor anterior.
	count_1s--;
	if (!count_1s) {
		led_toggle(0);
		count_1s = 1000;
	}
}
