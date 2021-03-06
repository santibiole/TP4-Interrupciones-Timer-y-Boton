#include <stdint.h>
#include "stm32f4xx.h"			// Header del micro
#include "stm32f4xx_gpio.h"		// Perifericos de E/S
#include "stm32f4xx_rcc.h"		// Para configurar el (Reset and clock controller)
#include "stm32f4xx_tim.h"		// Modulos Timers
#include "stm32f4xx_exti.h"		// Controlador interrupciones externas
#include "stm32f4xx_syscfg.h"	// configuraciones Generales
#include "misc.h"				// Vectores de interrupciones (NVIC)
#include "bsp.h"
#include "LIS3DSH.h"

#define LED_V GPIO_Pin_12
#define LED_N GPIO_Pin_13
#define LED_R GPIO_Pin_14
#define LED_A GPIO_Pin_15

#define BOTON GPIO_Pin_0

/* Puertos de los leds disponibles */
GPIO_TypeDef* leds_port[] = { GPIOD, GPIOD, GPIOD, GPIOD };
/* Leds disponibles */
const uint16_t leds[] = { LED_V, LED_R, LED_N, LED_A };

/* Leds disponibles PWM*/
uint32_t* const leds_pwm[] = { &TIM4->CCR1, &TIM4->CCR3, &TIM4->CCR2, &TIM4->CCR4 }; // Contiene punteros a variables del tipo unit32_t a distintas direcciones de memoria.

/*
	Prototipo de una funci�n externa, es decir, una frecuencia que va a estar implementada en alg�n otro
	lugar de nuestro proyecto. El linker es el que se va a encargar de ubicar donde est� implementada.
 */
extern void APP_ISR_sw (void);
extern void APP_ISR_1ms (void);

volatile uint16_t bsp_count_ms = 0; // Defino como volatile para que el compilador no interprete el while(bsp_count_ms) como un bucle infinito.

void led_on(uint8_t led) {
	GPIO_SetBits(leds_port[led], leds[led]);
}

void led_off(uint8_t led) {
	GPIO_ResetBits(leds_port[led], leds[led]);
}

void led_toggle(uint8_t led) {
	GPIO_ToggleBits(leds_port[led], leds[led]);
}

uint8_t sw_getState(void) {
	return GPIO_ReadInputDataBit(GPIOA, BOTON);
}

void led_set_bright (uint8_t led, uint8_t value){
	*leds_pwm[led] = 10000 * value / 100; // Porcentaje.
}

void bsp_delay_ms(uint16_t x){
	bsp_count_ms = x;
	while (bsp_count_ms);
}

/**
 * @brief Interrupcion llamada cuando se preciona el pulsador
 */
void EXTI0_IRQHandler(void) {

	if (EXTI_GetITStatus(EXTI_Line0) != RESET) { //Verificamos si es la del pin configurado.
		EXTI_ClearFlag(EXTI_Line0); // Limpiamos la Interrupcion.
		// Rutina:
		APP_ISR_sw();
		//GPIO_ToggleBits(leds_port[1], leds[1]);
	}

}

/**
 * @brief Interrupcion llamada al pasar 1ms
 */
void TIM2_IRQHandler(void) {

	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		// Rutina:
		APP_ISR_1ms();

		if (bsp_count_ms) { //Pregunto si bsp_count_ms es distinto de 0.
			bsp_count_ms--;
		}
	}
}

void bsp_led_init();
void bsp_sw_init();
void bsp_timer_config();

void bsp_init() {
	//bsp_led_init();
	bsp_pwm_init();
	bsp_sw_init();
	bsp_timer_config();
	LIS3DSH_Init();
	LIS3DSH_Set_Output(0X47); // 25MHz
}

float bsp_get_acc (char axis) {
	switch(axis){
	case 'X':
	case 'x':
		return LIS3DSH_Get_X_Out(LIS3DSH_Sense_2g);
		break;
	case 'Y':
	case 'y':
		return LIS3DSH_Get_Y_Out(LIS3DSH_Sense_2g);
	case 'Z':
	case 'z':
		return LIS3DSH_Get_Z_Out(LIS3DSH_Sense_2g);
	default:
		return -999.9;
	}
}

/**
 * @brief Inicializa Leds
 */
void bsp_led_init() {
	GPIO_InitTypeDef GPIO_InitStruct;

	// Arranco el clock del periferico
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_12;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; // (Push/Pull)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/**
 * @brief Inicializa SW
 */
void bsp_sw_init() {
	GPIO_InitTypeDef GPIO_InitStruct;

	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	// Arranco el clock del periferico
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Configuro interrupcion

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0); // Pin 0 del puerto A, lo hace interrupci�n.

	/* Configuro EXTI Line */
	EXTI_InitStructure.EXTI_Line = EXTI_Line0; // Interrupci�n en L�nea 0.
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; // Modo "Interrupci�n".
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // Interrupci�n por flanco ascendente.
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Habilito la EXTI Line Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn; // Qu� el canal sea el de la interrupci�n 0.
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // Prioridad.
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01; // Canal habilitado, habilito la interrupci�n.
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; // Habilito la interrupci�n.
	NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief Inicializa TIM2
 */
void bsp_timer_config(void) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
	NVIC_InitTypeDef NVIC_InitStructure;
	/* Habilito la interrupcion global del  TIM2 */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* TIM2 habilitado */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	/* Configuracion de la base de tiempo */
	TIM_TimeBaseStruct.TIM_Period = 1000; // 1 MHz bajado a 1 KHz (1 ms). �C�mo? Cuento 1us, aumento el contador, y cuando llego a mil tengo 1ms, es decir 1KHz.
	TIM_TimeBaseStruct.TIM_Prescaler = (2 * 8000000 / 1000000) - 1; // 8 MHz bajado a 1 MHz - Pre Escalador.
	TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1; // Divisor.
	TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up; // Como queremos que cuente.
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct); // Inicializamos timer.
	/* TIM habilitado */
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); // Inicializamos la interrupci�n.
	/* TIM2 contador habilitado */
	TIM_Cmd(TIM2, ENABLE);

}


void bsp_pwm_init (void) {

	/* Habilito el clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	/* Configuro leds como Segunda Funcion */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

/*
 * 	Inicializaci�n de Puerto D en modo Alternative Function y configuraci�n.
 */
	GPIO_InitTypeDef GPIO_Config;

	GPIO_Config.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Config.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Config.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Config.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Config.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOD, &GPIO_Config);

/*
 * 	Configuraci�n del Puerto D en modo Alternative Function como TIM4.
 */

	GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_TIM4);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_TIM4);

/*
 * 	Inicializaci�n y configuraci�n del TIMER.
 */
	TIM_TimeBaseInitTypeDef TIM_Config;

	TIM_Config.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_Config.TIM_ClockDivision = 0;
	TIM_Config.TIM_Period = 10000; // Cuantos TIC voy a contar.
	TIM_Config.TIM_Prescaler = 16-1; // 1us por TIC (-1 porque no arranca en cero).
	TIM_TimeBaseInit (TIM4, &TIM_Config);

/*
 * 	Inicializaci�n y configuraci�n del TIMER Output Compare en modo PWM1.
 */
	TIM_OCInitTypeDef TIM_OC_Config;

	// Configuraci�n del perif�rico
	TIM_OC_Config.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OC_Config.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC_Config.TIM_Pulse = 0;
	TIM_OC_Config.TIM_OCPolarity = TIM_OCPolarity_High;
	// CH1 del PWM
	TIM_OC1Init(TIM4, &TIM_OC_Config);
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);

	// ReConfiguraci�n del perif�rico por posible modificaci�n del puntero en el CH1 del PWM
	TIM_OC_Config.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC_Config.TIM_Pulse = 0;
	// CH2 del PWM
	TIM_OC2Init(TIM4, &TIM_OC_Config);
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);

	// ReConfiguraci�n del perif�rico por posible modificaci�n del puntero en el CH2 del PWM
	TIM_OC_Config.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC_Config.TIM_Pulse = 0;
	// CH3 del PWM
	TIM_OC3Init(TIM4, &TIM_OC_Config);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);

	// ReConfiguraci�n del perif�rico por posible modificaci�n del puntero en el CH3 del PWM
	TIM_OC_Config.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC_Config.TIM_Pulse = 0;
	// CH4 del PWM
	TIM_OC4Init(TIM4, &TIM_OC_Config);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);

	// Habilitar PreCargador
	TIM_ARRPreloadConfig(TIM4, ENABLE);

	// Habilitaci�n global del TIMER
	TIM_Cmd(TIM4, ENABLE);
}
