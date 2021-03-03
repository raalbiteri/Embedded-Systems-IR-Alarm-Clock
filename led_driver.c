/*
 * Raunel Albiter
 * CE2812-31 Embedded Systems II
 * Dr.Livingston
 * Project: Lab2Albiter
 * File: led_driver.c
 * Revision 1: 12/12/16
 */

#include <stdio.h>
#include <stdbool.h>
#include "led_driver.h"

int ledRealIndex;
void init_led();
void led_allon_off(_Bool);
void led_on(int);
void led_off(int);
void led_scan(int);
void led_flash(int);
void delay_ms(uint32_t);

/*
 * 	Function used to setup the LEDs in order to have them
 * 	ready for use. This is done by enabling clock for GPIO A and B
 * 	and setting corresponding LED pins to general output mode
 * 	Param: none
 * 	Return: none
 */
void init_led() {

	//Enable clocks for GPIOA & B
	*(RCC_AHB1ENR) |= (1<<GPIOAEN);
	*(RCC_AHB1ENR) |= (1<<GPIOBEN);

	//Set up all bottom leds (bits 14,16,18,20,22)
	*(GPIOA_MODER) &= (0xFF00CFFF);  // Clear moder bits for PA7-11
	*(GPIOA_MODER) |= (0b0101010101<<14);  // PA7-11 will be outputs
	//B moder
	*(GPIOB_MODER) &= (0xF0C0FFFF);  // Clear moder bits for PA8-10 & 12-13
	*(GPIOB_MODER) |= (0b010100010101<<16);  // PB8-10 & 12-13 will be outputs
}

/*
 * 	Function used to either turn all LEDs on or off depending on the boolean
 * 	parameter. True corresponds to all on while False is all off. LEDs are
 * 	turned on or off through the ODR.
 * 	Param: modeSwitch-switches between on and off
 * 	Return: none
 */
void led_allon_off(_Bool modeSwitch) {

	if(modeSwitch) {
		//Pass in 1 into the corresponding ODRx bit to turn on
		//Lower 5 LEDs *bits 7-11 (0b11111<<6) -> GPIOA_ODR
		*(GPIOA_ODR) |= (0b11111<<7);
		//Upper 5 LEDs *bits 8-10 * 12-13 (0b111011<<7) -> GPIOB_ODR
		*(GPIOB_ODR) |= (0b110111<<8);
	} else {
		//Pass in 0 into the corresponding ODRx bit to turn off
		//Lower 5 LEDs *bits 7-11 (0b11111<<6) -> GPIOA_ODR
		*(GPIOA_ODR) &= (0b00000<<7);
		//Upper 5 LEDs *bits 8-10 * 12-13 (0b111011<<7) -> GPIOB_ODR
		*(GPIOB_ODR) &= (0b000000<<8);
	}
}

/*
 * 	Helper Function used to turn specific LEDs on based on the index parameter
 * 	Param: ledIndex- turns on an LED from 1-10
 * 	Return: none
 */
void led_on(int ledIndex) {

	//Options for entry include 1-10 for corresponding LEDs
	if(ledIndex < 6) {
		ledRealIndex = ledIndex + 6;
		*(GPIOA_ODR) |= (1<<ledRealIndex);
	} else {
		//In case of missing PB11 bit for LED
		if(ledIndex > 8) {
			ledRealIndex = ledIndex + 3;
		} else {
			ledRealIndex = ledIndex + 2;
		}
		*(GPIOB_ODR) |= (1<<ledRealIndex);
	}
}

/*
 * 	Helper function used to turn specific LEDs off based on the index parameter
 * 	Param: ledIndex- turns off an LED from 1-10
 * 	Return: 0
 */
void led_off(int ledIndex) {

	//Options for entry include 1-10 for corresponding LEDs
	if(ledIndex < 6) {
		ledRealIndex = ledIndex + 6;
		*(GPIOA_ODR) &= ~(1<<ledRealIndex);
	} else {
		//In case of missing PB11 bit for LED
		if(ledIndex > 8) {
			ledRealIndex = ledIndex + 3;
		} else {
			ledRealIndex = ledIndex + 2;
		}
		*(GPIOB_ODR) &= ~(1<<ledRealIndex);
	}
}

/*
 * 	Function that is used to scan through the 10 LEDs in a kitt knight
 * 	rider style but with a desired frequency/speed.
 * 	Param: scanSpeed- delay between turning LED off in order to move
 * 					  on to the next LED
 * 	Return: none
 */
void led_scan(int scanSpeed) {

	//Options for speed include 0(slowest)-9(fastest)
	//Speeds are split into 50ms delays between LED scan(on-delay-off).
	//Slowest being 1000ms fastest being 100ms delays
	for(int i=1; i<11; i++) {
		led_on(i);
		delay_ms(scanSpeed);
		led_off(i);
	}
	for(int i=10; i>0; i--) {
		led_on(i);
		delay_ms(scanSpeed);
		led_off(i);
	}
}

/*
 * 	Function that is used to flash all 10 LEDs with a desired frequency
 * 	Param: flashSpeed- delay between turning LEDs on and off
 * 	Return: none
 */
void led_flash(int flashSpeed) {

	//Options for speed include 0(slowest)-9(fastest)
	//Speed are split into 50ms delays between LED flash(on-delay-off).
	//Slowest being 1000ms fastest being 100ms delays
	for(int i=1; i<6; i++) {
		led_allon_off(true);
		delay_ms(flashSpeed);
		led_allon_off(false);
		delay_ms(flashSpeed);
	}
}

/*
 * 	Delay processor by t_ms by polling the Systick timer.
 * 	Param: t_ms- the desired time delay in milliseconds
 * 	Return: none
 */
void delay_ms(uint32_t t_ms) {

	//1) Make sure the counter is off
	*(STK_CTRL) &= ~(1<<STK_ENABLE_FLAG);
	//t_ms time
	for (uint32_t i = 0; i < t_ms; i++) {

		//Load 16000 into STK_LOAD
		*(STK_LOAD) = 16000; //1ms
		//Turn on counter
		*(STK_CTRL) |= ((1<<STK_ENABLE_FLAG) | (1<<STK_CLKSOURCE_FLAG));
		//Wait for flag to go to '1'
		while (!(*(STK_CTRL) & (1<<STK_CNT_FLAG))) {}
		//Turn off counter
		*(STK_CTRL) &= ~(1<<STK_ENABLE_FLAG);
	}
}
