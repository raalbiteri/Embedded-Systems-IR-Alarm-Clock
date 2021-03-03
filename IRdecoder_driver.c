/*
 * Raunel Albiter
 * CE2812-31 Embedded Systems II
 * Dr.Livingston
 * Project: Lab8Albiter
 * File: IRdecoder_driver.c
 * Revision 1: 2/16/17
 */

#include <stdio.h>
#include <stdbool.h>
#include "IRdecoder_driver.h"
#include "realTimeClock_driver.h"
#include "led_driver.h"

uint32_t lastISR = 0;
int irState = 0;
uint32_t irCommand = 0;
int isrCounter = 0;
int commandReceived = 0;

/*
 *	Function used to setup IR sensor that is taking input in
 *	from the TEAC RC-505 remote controller through an ISR.
 *
 *	Param: none
 *	return: none
 */
void IRdecoder_init() {

	//Enable clock to TIM12
	*(RCC_APB1ENR) |= (1<<TIM12EN); //TIM12 is bit 6

	//Enable GPIOB
	*(RCC_AHB1ENR) |= (1<<GPIOBEN); //GPIOB is bit 1

	//Enable alternate function for PB4
	*(GPIOB_AFRH) &= ~(0b1111<<24);
	*(GPIOB_AFRH) = (0b1001<<24); //AF 9
	*(GPIOB_MODER) &= ~(0b11<<28);
	*(GPIOB_MODER) |= (0b10<<28);

	//1. Select active input: TIMxCCR1 must be linked to TI1
	*(TIM12_CCMR1) = 0b01; //CC1s = 01 - channel 1 configured as input on TI1

	//3. Select the edge of the active transition
	*(TIM12_CCER) |= (1<<1)|(0<<3); //(CC1NP)(CC1P) = 01 - inverted falling edge

	//4. Interrupt request on input captures channel
	*(TIM12_DIER) = (1<<1); //CC1E = 0b1 - interrupt enabled on capture 1

	//5. Auto-reload register (ARR). holds max value of count
	*(TIM12_ARR) = (ARR_MAX_VALUE); //maxValue = 65,535
	*(TIM12_CCR1) = 0;

	//6. Set prescaler
	*(TIM12_PSC) = (PRESCALER_VAL); //Prescaler value = 800

	//7. Connect to channel 1 & enable counter
	*(TIM12_CCER) |= (0b01);
	*(TIM12_CR1) = 1; //CEN = 1

	//Configure NVIC to enable TIM12 interrupt
	*(NVIC_ISER1) = (1<<11);
}

/*
 *	Function that blocks whenever the IR sensor is needed
 *	for user input. Will however unblock if an update is required
 *	on the clock
 *
 *	Param: none
 *	return: IR command
 */
uint8_t IRDecode() {

	int tempCom = 0;
	clearAll();
	while(commandReceived == 0 && getUpdateStatus() == 0) {}
	if(getUpdateStatus() == 1) {
		commandReceived = PAUSE;
	}
	tempCom = commandReceived;
	clearAll();
	return tempCom;
}

/*
 *	Function that translates what the IR command is
 *	depending on what button is needed.
 *
 *	Param: IR command
 *	return: Translated character
 */
char IRCodeTranslate(uint8_t irCode) {

	char code = '0';
	switch(irCode) {
	case(T_1):
		code = '1';
		break;
	case(T_2):
		code = '2';
		break;
	case(T_3):
		code = '3';
		break;
	case(T_4):
		code = '4';
		break;
	case(T_5):
		code = '5';
		break;
	case(T_6):
		code = '6';
		break;
	case(T_7):
		code = '7';
		break;
	case(T_8):
		code = '8';
		break;
	case(T_9):
		code = '9';
		break;
	case(T_0):
		code = '0';
		break;
	default:
		break;
	}
	return code;
}

/*
 *	Function that will take the translated command
 *	and output it it as a number typically an int
 *
 *	Param: none
 *	return: none
 */
uint8_t IRkeyToNum (char key) {

	return key - '0';
}

/*
 *	Helper function that will reset all global
 *	variables used in the ISR in order to correct any
 *	junk data that was previously inside them.
 *
 *	Param: none
 *	return: none
 */
void clearAll() {
	lastISR = 0;
	irState = 0;
	irCommand = 0;
	isrCounter = 0;
	commandReceived = 0;
}

/*
 *	IR sensor ISR that is custom based to work with
 *	the IR input from the controller in order to translate
 *	the data into a 8 bit command. Command is returned in a
 *	global variable rather than from ISR
 *
 *	Param: none
 *	return: none
 */
void TIM8_BRK_TIM12_IRQHandler() {

	*(TIM12_SR) = 0; //Clear IRQ routine
	if(getAlarmStatus() != 1) {
		volatile int tempTimeStamp = 0;
		volatile int tempCommand = 0;
		volatile int tempState = 0;
		volatile int shiftAmount = 0;
		volatile int width = 0;

		//Find state function to go to
		switch(irState) {

		case(WAITING_FOR_START):
			//Store CCR1 to lastISR
			lastISR = *(TIM12_CCR1);

			//Change state to waiting for endstart
			irState = WAITING_FOR_ENDSTART;
			break;
		case(WAITING_FOR_ENDSTART):

			tempTimeStamp = lastISR;
			width = *(TIM12_CCR1) - tempTimeStamp;

			if(width > 240) { //width > 12 ms
				tempTimeStamp = 1;
			}
			if(width < 400) { //width < 15 ms
				//lastISR = 1;
				tempTimeStamp &= 1;
			} else if(width >= 400){
				//lastISR = 0;
				tempTimeStamp &= 0;
			}
			if(tempTimeStamp == 1) {
				//Store CCR1 to lastISR
				lastISR = *(TIM12_CCR1);
				irState = WAITING_FOR_BITN;
			} else {
				//Error: go back to waiting for start state
				lastISR = 0;
				irState = 0;
				irCommand = 0;
				isrCounter = 0;
				commandReceived = 0;
			}
			break;
		default:

			isrCounter++;
			if(isrCounter < 33) { //break out once all 32 bits are received

				//Get last time stamp
				tempTimeStamp = lastISR;
				width = *(TIM12_CCR1) - tempTimeStamp;

				if(width > 20) { //width > 1 ms
					tempTimeStamp = 1;
				}
				if(width < 30) { //width < 1.5 ms
					tempTimeStamp &= 1;
				} else if(width >= 30) {
					tempTimeStamp &= 0;
				}
				if(tempTimeStamp == 1) {
					//Store CCR1 to lastISR
					lastISR = *(TIM12_CCR1);
					//Set bit N to 0
					tempCommand = irCommand;
					tempState = irState;
					tempState -= 2; //get ORR shift amount
					shiftAmount = 0;
					shiftAmount = (shiftAmount<<tempState);
					tempCommand |= (shiftAmount); //set bit N to 0
					irCommand = tempCommand;

					//Change state to waiting for bitN+1 (state bitN + 1)
					tempState = irState;
					irState = (tempState + 1);

				} else if(tempTimeStamp != 1){
					if(width > 30) { //width > 1.5 ms
						tempTimeStamp = 1;
					}
					if(width < 60) { //width < 3 ms
						tempTimeStamp &= 1;
					} else if(width >= 60) {
						tempTimeStamp &= 0;
					}
					if(tempTimeStamp == 1) {
						//Store CCR1 to lastISR
						lastISR = *(TIM12_CCR1);
						//Set bit N to 0
						tempCommand = irCommand;
						tempState = irState;
						tempState -= 2; //get ORR shift amount
						shiftAmount = 1;
						shiftAmount = (shiftAmount<<tempState);
						tempCommand |= (shiftAmount); //set bit N to 0
						irCommand = tempCommand;

						//Change state to waiting for bitN+1 (state bitN + 1)
						tempState = irState;
						irState = (tempState + 1);
					} else {
						//Error: go back to waiting for start state
						lastISR = 0;
						irState = 0;
						irCommand = 0;
						isrCounter = 0;
						commandReceived = 0;
					}
				} else {
					//Error: go back to waiting for start state
					lastISR = 0;
					irState = 0;
					irCommand = 0;
					isrCounter = 0;
					commandReceived = 0;
				}
			} else {
				isrCounter = 0;
				tempCommand = irCommand;
				tempCommand = (tempCommand>>16);
				commandReceived = (tempCommand & 0x00FF);
				if(commandReceived != 0) {
					irCommand = 0; //reset command
					irState = 0; //reset state
					lastISR = 0; //reset isr time
				} else {
					commandReceived = 1;
				}
			}
			break;
		}
	} else {
		commandReceived = CLEAR;
	}
}
