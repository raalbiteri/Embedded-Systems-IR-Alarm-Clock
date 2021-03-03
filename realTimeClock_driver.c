/*
 * Raunel Albiter
 * CE2812-31 Embedded Systems II
 * Dr.Livingston
 * Project: Lab8Albiter
 * File: realTimeClock_driver.c
 * Revision 1: 2/16/17
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "IRdecoder_driver.h"
#include "realTimeClock_driver.h"
#include "buzzer_driver.h"
#include "lcd_driver.h"
#include "led_driver.h"

typedef struct {
	uint32_t CNTL;
	uint32_t LOAD;
	uint32_t VAL;
} STK;

static volatile STK* stk = (STK*) 0xE000E010;
static volatile int sysTickCount = 0;
static volatile int alarmTickCount = 0;
Clock homeClock = {1,2,0,0,AM};
volatile int activateAlarm = ALARM_DIS;
volatile int soundAlarm;
volatile int updateClock;

/*
 *	Function that allows for the initial home clock to be set in realtime
 *	as well as any other clock such as the alarm clock. Format for the
 *	clock is hh:mm AM/PM. *12 hour clock
 *
 *	Param: row & column position on lcd to place clock, Clock to be set, whether home
 *	clock or alarm clock is being set
 *	return: none
 */
void setClock(int lcdRow,int lcdCol,Clock* currentClock,int setMode) {

	lcd_home();
	if(setMode == CURRENT_CLOCK) {
		lcd_data('$'); //Make sure user knows clock is in set mode
	} else {
		lcd_data('&'); //Make sure user knows clock is in alarm set mode
	}
	lcd_set_position(lcdRow,lcdCol);
	lcd_cmd(0x0F); //turn cursor on
	int tempCol = lcdCol;
	char irKey = '0';
	char previousKey = '0';
	char pm[] = "PM";
	char am[] = "AM";
	int clockState = HOUR_TENS;
	uint8_t irCode = 0;
	while(irCode != PLAY) {
		delay_milli(IR_SAFETY_DELAY);
		irCode = IRDecode();
		delay_milli(IR_SAFETY_DELAY);
		irKey = IRCodeTranslate(irCode);
		switch(clockState) {
		case(HOUR_TENS):
			if(irKey > '1') {
				lcd_data('1');
				currentClock->hourTens = 1;
				irKey = '1';
			} else {
				lcd_data(irKey);
				currentClock->hourTens = IRkeyToNum(irKey);
			}
			tempCol++;
			break;
		case(HOUR_ONES):
			if(previousKey >= '1' && irKey > '2') {
				lcd_data('2');
				currentClock->hourOnes = 2;
				irKey = '2';
			} else {
				lcd_data(irKey);
				currentClock->hourOnes = IRkeyToNum(irKey);
			}
			tempCol+=2; //to pass the ':' in the clock
			break;
		case(MINUTE_TENS):
			if(irKey > '5') {
				lcd_data('5');
				currentClock->minuteTens = 5;
				irKey = '5';
			} else {
				lcd_data(irKey);
				currentClock->minuteTens = IRkeyToNum(irKey);
			}
			tempCol++;
			break;
		case(MINUTE_ONES):
			if(previousKey >= '5' && irKey > '9') {
				lcd_data('9');
				currentClock->minuteOnes = 9;
				irKey = '9';
			} else {
				lcd_data(irKey);
				currentClock->minuteOnes = IRkeyToNum(irKey);
			}
			tempCol+=2; //to separate 'AM/PM' from rest of clock
			break;
		case(TIME_OF_DAY):
			while(irCode != FORW_SKIP && irCode != BACK_SKIP && irCode != PLAY) {
				delay_milli(IR_SAFETY_DELAY);
				irCode = IRDecode();
				delay_milli(IR_SAFETY_DELAY);
			}
			if(currentClock->timeOfDay == AM && irCode != PLAY) {
				lcd_print_string(pm);
				currentClock->timeOfDay = PM;
			} else if(currentClock->timeOfDay == PM && irCode != PLAY){
				lcd_print_string(am);
				currentClock->timeOfDay = AM;
			}
			irKey = '0';
			break;
		default:
			break;
		}
		clockState++;
		previousKey = irKey;
		if(clockState == RESET) {
			clockState = HOUR_TENS;
			lcd_set_position(lcdRow,lcdCol);
			tempCol = lcdCol;
		} else {
			lcd_set_position(lcdRow,tempCol);
		}
	}
	lcd_cmd(0x0C); //turn cursor off
}

/*
 *	Function used to display a clock to the LCD depending on the
 *	position. Also displays if the alarm is active.
 *
 *	Param: row & column position of clock, clock to be displayed
 *	return: none
 */
void displayClock(int lcdRow,int lcdCol,Clock* currentClock) {

	const char AFTERNOON[] = "PM";
	const char MORNING[] = "AM";
	char activeAlarmLeft[MAX_CUSTOM_CHAR_SIZE] = {0x18,0x17,0xD,0xE,0xF,0xF,0xF,0x3};
	char activeAlarmRight[MAX_CUSTOM_CHAR_SIZE] = {0x3,0x1D,0x1E,0x1E,0x6,0x1E,0x1E,0x18};
	const int LCD_ROW_ALARM_ACTIVE_LEFT = 1;
	const int LCD_COL_ALARM_ACTIVE_LEFT = 0;
	const int LCD_ROW_ALARM_ACTIVE_RIGHT = 1;
	const int LCD_COL_ALARM_ACTIVE_RIGHT = 1;
	const int LCD_ROW_ALARM_SOUNDING = 1;
	const int LCD_COL_ALARM_SOUNDING = 15;
	lcd_clear();
	lcd_set_position(lcdRow,lcdCol);
	lcd_print_num(currentClock->hourTens);
	lcd_print_num(currentClock->hourOnes);
	lcd_data(':');
	lcd_print_num(currentClock->minuteTens);
	lcd_print_num(currentClock->minuteOnes);
	lcd_data(' ');
	if(currentClock->timeOfDay == AM) {
		lcd_print_string(MORNING);
	} else {
		lcd_print_string(AFTERNOON);
	}
	lcd_set_position(LCD_ROW_ALARM_ACTIVE_LEFT,LCD_COL_ALARM_ACTIVE_LEFT);
	if(activateAlarm == ALARM_EN) {
		customLCDChar(0,activeAlarmLeft);
		lcd_set_position(LCD_ROW_ALARM_ACTIVE_LEFT,LCD_COL_ALARM_ACTIVE_LEFT);
		lcd_data(0x00); //custom alarm active char location
		customLCDChar(4,activeAlarmRight);
		lcd_set_position(LCD_ROW_ALARM_ACTIVE_RIGHT,LCD_COL_ALARM_ACTIVE_RIGHT);
		lcd_data(0x04); //custom alarm active char location
	} else {
		lcd_data(' ');
		lcd_data(' ');
	}
	/*lcd_set_position(LCD_ROW_ALARM_SOUNDING,LCD_COL_ALARM_SOUNDING);
	if(soundAlarm == ALARM_ACTIVE) {
		lcd_data('@');
	} else {
		lcd_data(' ');
	}*/
	lcd_set_position(lcdRow,lcdCol); //reset original position
}

/*
 *	Function that updates the clock display and keeps track
 *	of what time it is to set the correct time of day and reset
 *	after midnight or afternoon is reached since it is a 12 hour clock.
 *
 *	Param: home clock, alarm clock
 *	return: none
 */
void updateRealClock(Clock* currentClock) {

	const uint8_t MAX_MINUTES_ONES = 9;
	const uint8_t MAX_MINUTES_TENS = 5;
	const uint8_t MAX_HOUR_ONES = 2;
	const uint8_t MAX_HOUR_TENS = 1;

	if(currentClock->minuteOnes >= MAX_MINUTES_ONES) { //Ex: 12:49 + 1min -> 12:50
		if(currentClock->minuteTens >= MAX_MINUTES_TENS) {
			currentClock->minuteTens = 0;
			if(currentClock->hourOnes >= MAX_HOUR_ONES && currentClock->hourTens >= MAX_HOUR_TENS) { //Ex: 12:59 + 1min -> 01:00
				currentClock->hourOnes = 1;
				currentClock->hourTens = 0;
			} else if(currentClock->hourOnes >= MAX_MINUTES_ONES){ //Ex: 09:59 + 1min -> 10:00
				currentClock->hourOnes = 0;
				(currentClock->hourTens)++;
			} else {
				(currentClock->hourOnes)++;
			}
		} else {
			(currentClock->minuteTens)++;
		}
		currentClock->minuteOnes = 0;
	} else {
		(currentClock->minuteOnes)++;
	}
	if(currentClock->hourTens == MAX_HOUR_TENS && currentClock->hourOnes == MAX_HOUR_ONES
			&& currentClock->minuteTens == 0 && currentClock->minuteOnes == 0) {
		if(currentClock->timeOfDay == AM) {
			currentClock->timeOfDay = PM;
		} else {
			currentClock->timeOfDay = AM;
		}
	}
}

/*
 *	Function used to set the home clock based on the
 *	clock that is currently being set
 *
 *	Param: current home clock
 *	return: none
 */
void setHomeClock(Clock* currentClock) {

	homeClock.hourTens = currentClock->hourTens;
	homeClock.hourOnes = currentClock->hourOnes;
	homeClock.minuteTens = currentClock->minuteTens;
	homeClock.minuteOnes = currentClock->minuteOnes;
	homeClock.timeOfDay = currentClock->timeOfDay;
}

/*
 *	Function used to compare the home clock to the alarm clock
 *	in order to determine whether or not the alarm should be set
 *	to active.
 *
 *	Param: current home clock, alarm clock
 *	return: none
 */
void clockCompare(Clock* currentClock, Clock* alarmClock) {

	int returnBoolean = 1;
	const int LCD_ROW_ALARM_SOUNDING = 1;
	const int LCD_COL_ALARM_SOUNDING = 15;
	char alarmSounding[MAX_CUSTOM_CHAR_SIZE] = {0x4,0xE,0xE,0xE,0x1F,0x00,0x4,0x0};
	returnBoolean &= (currentClock->hourTens == alarmClock->hourTens);
	returnBoolean &= (currentClock->hourOnes == alarmClock->hourOnes);
	returnBoolean &= (currentClock->minuteTens == alarmClock->minuteTens);
	returnBoolean &= (currentClock->minuteOnes == alarmClock->minuteOnes);
	returnBoolean &= (currentClock->timeOfDay == alarmClock->timeOfDay);
	if(returnBoolean == 1 && activateAlarm == ALARM_EN) {
		customLCDChar(3,alarmSounding);
		lcd_set_position(LCD_ROW_ALARM_SOUNDING,LCD_COL_ALARM_SOUNDING);
		lcd_data(0x03); //custom alarm active char location
		soundAlarm = ALARM_ACTIVE;
	}
}

/*
 *	Get function used to return clock update signal
 *
 *	Param: none
 *	return: none
 */
int getUpdateStatus() {
	return updateClock;
}

/*
 *	Clear function used to reset the update status for
 *	a clock
 *
 *	Param: none
 *	return: none
 */
void resetUpdateStatus() {
	updateClock = NO_CLOCK_UPDATE;
}

/*
 *	Get function used to return the sound alarm signal
 *
 *	Param: none
 *	return: none
 */
int getAlarmStatus() {
	return soundAlarm;
}

/*
 *	Clear function used to reset the sound alarm status for
 *	a clock
 *
 *	Param: none
 *	return: none
 */
void resetAlarmStatus() {
	soundAlarm = ALARM_INACTIVE;
}

/*
 *	Function used to toggle the alarm active signal from being on
 *	to off (vice versa)
 *
 *	Param: none
 *	return: none
 */
void toggleAlarm() {
	if(activateAlarm == ALARM_DIS) {
		activateAlarm = ALARM_EN;
	} else {
		activateAlarm = ALARM_DIS;
	}
}

/*
 *	Function used to setup the systick timer in order
 *	to trigger every 1 ms
 *
 *	Param: none
 *	return: none
 */
void Systick_init() {

	//Set CNTL and VAL to 0 to start fresh
	stk->CNTL = 0;
	stk->VAL = 0;

	//No prescaler so, 1 ms would be 1000 ticks
	stk->LOAD = ((F_CPU)/1000)-1;

	//Start Systick timer with interrupt and regular clock
	stk->CNTL = STK_ENABLE|STK_INTERRUPT_EN|STK_CLKSRC;
}

/*
 *	Function used to delay a function with a user specified time
 *	in milliseconds
 *
 *	Param: millisecond delay
 *	return: none
 */
void delay_milli(uint32_t delay) {

	uint32_t start = sysTickCount;
	uint32_t stop = (sysTickCount + delay);

	if(stop < start) {
		while(sysTickCount > stop){}
	}

	while(sysTickCount < stop){}

}

/*
 *	Systick ISR that handles precision timing for sounding
 *	the alarm, triggering the LEDs when alarm goes off, and
 *	whether or not the clock should be updated
 *
 *	Param: none
 *	return: none
 */
void SysTick_Handler() {

	sysTickCount++;
	if(activateAlarm == ALARM_DIS) {
		alarmTickCount = 0;
	} else {
		alarmTickCount++;
	}
	int alarmCount = 0;
	if(soundAlarm == ALARM_ACTIVE && alarmTickCount >= ALARM_INTERVAL) {
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
			//Pass in 1 into the corresponding ODRx bit to turn on
			//Lower 5 LEDs *bits 7-11 (0b11111<<6) -> GPIOA_ODR
			*(GPIOA_ODR) |= (0b11111<<7);
			//Upper 5 LEDs *bits 8-10 * 12-13 (0b111011<<7) -> GPIOB_ODR
			*(GPIOB_ODR) |= (0b110111<<8);
			while(alarmCount < (MILLISECOND_DELAY*MILLISECOND_FACTOR)) {
				alarmCount++;
			}
			sysTickCount += MILLISECOND_DELAY;
			alarmCount = 0;
 			//Disable counter
			*(TIM3_CR1) = 0; //CEN = 0
			//Pass in 0 into the corresponding ODRx bit to turn off
			//Lower 5 LEDs *bits 7-11 (0b11111<<6) -> GPIOA_ODR
			*(GPIOA_ODR) &= (0b00000<<7);
			//Upper 5 LEDs *bits 8-10 * 12-13 (0b111011<<7) -> GPIOB_ODR
			*(GPIOB_ODR) &= (0b000000<<8);

			while(alarmCount < (MILLISECOND_DELAY*MILLISECOND_FACTOR)) {
				alarmCount++;
			}
			alarmCount = 0;
		}
		alarmTickCount = 0;
	}
	if(sysTickCount >= MINUTE_FACTOR) {
		updateClock = CLOCK_UPDATE;
		sysTickCount = 0;
	}
}
