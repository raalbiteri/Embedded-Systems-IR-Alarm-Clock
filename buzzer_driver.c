/*
 * Raunel Albiter
 * CE2812-31 Embedded Systems II
 * Dr.Livingston
 * Project: Lab6Albiter
 * File: buzzer_driver.c
 * Revision 1: 1/25/16
 */

#include <stdio.h>
#include <stdbool.h>
#include "led_driver.h"
#include "buzzer_driver.h"

/*
 *	Function used to setup the Piezo speaker in order
 *	to be used to output Notes to it at a specified
 *	frequency
 *
 *	Param: none
 *	return: none
 */
void buzzer_init() {

	//Enable clock to TIM3
	*(RCC_APB1ENR) |= (1<<TIM3EN); //TIM3 is bit 1

	//Enable GPIOB
	*(RCC_AHB1ENR) |= (1<<GPIOBEN); //GPIOB is bit 1

	//Enable alternate function for PB4
	*(GPIOB_AFRL) = (0b0010<<16); //AF 2
	*(GPIOB_MODER) &= ~(0b11<<8);
	*(GPIOB_MODER) |= (0b10<<8);

	//Test Two frequencies TIM4
	//Enable clock to TIM3
	*(RCC_APB1ENR) |= (1<<TIM4EN); //TIM4 is bit 2

	//Enable alternate function for PB6
	*(GPIOB_AFRL) |= (0b0010<<24); //AF4
	*(GPIOB_MODER) &= ~(0b11<<12);
	*(GPIOB_MODER) |= (0b10<<12);
}

/*
 *	Function that is used to debug the piezo speaker
 *	if ever a song is not playing in order to make sure it
 *	is still functioning properly
 *
 *	Param: none
 *	return: none
 */
void playSimpleTone() {

	//2) ARR & CRR register will be at half period count
	//may want a different value for ARR if using other channels
	*(TIM3_ARR) = SIMPLETONE;
	*(TIM3_CCR1) = SIMPLETONE;

	//4) Select output mode -Toggle
	*(TIM3_CCMR1) = (0b011<<4); //OC1M = 011 - toggle on match

	//Compare output enable
	*(TIM3_CCER) = 1; //CC1E = 1

	for(int i = 0; i < 2; i++) {

		//5) Enable counter
		*(TIM3_CR1) = 1; //CEN = 1
		delay_ms(500);

		//Disable counter
		*(TIM3_CR1) = 0; //CEN = 0
	}
}

