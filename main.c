/*
 * Raunel Albiter
 * CE2812-31 Embedded Systems II
 * Dr.Livingston
 * Project: Lab8Albiter
 * File: main.c
 * Revision 1: 2/16/17
 */

#include <stdio.h>
#include <stdlib.h>
#include "uart_driver.h"
#include "lcd_driver.h"
#include "led_driver.h"
#include "IRdecoder_driver.h"
#include "buzzer_driver.h"
#include "realTimeClock_driver.h"
#include "adc_driver.h"

#define F_CPU 16000000UL
double tempValFaren = 0;
double tempValCels = 0;
double currentTemp = 0;

int subMenuConversion(int,int);
void printTemperature(double,int,int);

/*
 *	Main function that handles the overall interface
 *	which involves setting an alarm on a clock that also
 *	is able to tell the current temperature
 *
 *	Param: none
 *	return: none
 */
int main(void){

	int homeRowPos = 0;
	int homeColPos = 4;
	int tempState = FARENHEIT;
	int units = 0;
	double voltTemp = 0;
	Clock currentClock = {1,2,0,0,AM};
	Clock alarmClock = {0,6,0,0,AM};
	uint8_t remoteEntry;
	Systick_init();
	init_lcd();
	init_led();
	IRdecoder_init();
	buzzer_init();
	init_adc();
	displayClock(homeRowPos,homeColPos,&currentClock);
	voltTemp = convertVoltage(data_conversion());
	tempValCels = convertCelsius(voltTemp,1,0);
	tempValFaren = convertFarenheit(tempValCels);
	if(tempState == FARENHEIT) {
		currentTemp = tempValFaren;
	} else {
		currentTemp = tempValCels;
	}
	printTemperature(currentTemp,0,tempState);
	while(1) {
		remoteEntry = IRDecode();
		switch(remoteEntry) {
		case TIME: //Display the current time
			displayClock(homeRowPos,homeColPos,&currentClock);
			printTemperature(currentTemp,0,tempState);
			break;
		case SHUFFLE: //Switch between displaying temperature in celsius to farenheit (vice versa)
			displayClock(homeRowPos,homeColPos,&currentClock);
			tempState = subMenuConversion(tempState,0);
			if(tempState == FARENHEIT) {
				printTemperature(tempValFaren,0,tempState);
			} else if(tempState == CELSIUS) {
				printTemperature(tempValCels,0,tempState);
			}
			//tempState = subMenuConversion(tempState,0);
			break;
		case PLAY: //Set the alarm to active/inactive & display in clock menu to let user know its active
			toggleAlarm();
			displayClock(homeRowPos,homeColPos,&currentClock);
			printTemperature(currentTemp,0,tempState);
			break;
		case MEMORY_CHECK: //Go to set alarm menu
			displayClock(homeRowPos,homeColPos,&alarmClock);
			setClock(homeRowPos,homeColPos,&alarmClock,ALARM_CLOCK);
			displayClock(homeRowPos,homeColPos,&currentClock);
			printTemperature(currentTemp,0,tempState);
			break;
		case INTRO_CHECK: //Go to set clock menu
			setClock(homeRowPos,homeColPos,&currentClock,CURRENT_CLOCK);
			displayClock(homeRowPos,homeColPos,&currentClock);
			printTemperature(currentTemp,0,tempState);
			break;
		case PAUSE: //Update the clock **Only to be software triggered and not by user
			updateRealClock(&currentClock,&alarmClock);
			displayClock(homeRowPos,homeColPos,&currentClock);
			resetUpdateStatus();
			voltTemp = convertVoltage(data_conversion());
			tempValCels = convertCelsius(voltTemp,1,0);
			tempValFaren = convertFarenheit(tempValCels);
			if(tempState == CELSIUS) {
				currentTemp = tempValCels;
			} else {
				currentTemp = tempValFaren;
			}
			printTemperature(currentTemp,0,tempState);
			clockCompare(&currentClock,&alarmClock);
			break;
		case CLEAR: //Shut off the alarm
			resetAlarmStatus();
			displayClock(homeRowPos,homeColPos,&currentClock);
			printTemperature(currentTemp,0,tempState);
			break;
		default : //For remote debugging and fun piezo will sound a null noise to let user know key is invalid
			break;
		}
	}
	return 0;
}

/*
 * Function that allows for conversion within a sub-menu
 * if the current temperature is not standard units that the
 * user prefers
 *
 * Param: temperature state, current units of the extremes
 * Return: new temperature state
 */
int subMenuConversion(int tempUnits,int extremeUnits) {

	int returnState = tempUnits;
	if(tempUnits == extremeUnits) { //default
		//currentTemp = tempValCels;
		//returnState = 2;
	} else if(tempUnits == 1){ //Farenheit --> Celsius
		currentTemp = tempValCels;
		returnState = 2;
	} else { //Celsius --> Farenheit
		currentTemp = tempValFaren;
		returnState = 1;
	}
	return returnState;
}

/*
 * Function that allows for the printing of the
 * current temperature nicely formatted onto the LCD since
 * the LCD does not support printing doubles currently
 *
 * Param: currentTemperature, decimal point accuracy
 * Return: none
 */
void printTemperature(double temperature,int accuracy,int tempState) {

	char hotTemp[MAX_CUSTOM_CHAR_SIZE] = {0x0,0x0,0x0,0x4,0x11,0xE,0x1F,0x1F};
	char coldTemp[MAX_CUSTOM_CHAR_SIZE] = {0x0,0x15,0xE,0x1F,0xE,0x15,0x0,0x0};
	const int LCD_ROW_TEMP = 1;
	const int LCD_COL_TEMP = 6;
	const int HOT_TEMP_FAREN = 66;
	const int COLD_TEMP_FAREN = 65;
	const int HOT_TEMP_CELS = 19;
	const int COLD_TEMP_CELS = 18;
	const int NO_DECIMALS = 0;
	int tempInteger = 0;
	int tempDecimal = 0;
	tempInteger = getInteger(temperature);
	tempDecimal = getDecimal(temperature,accuracy);
	if(temperature >= HOT_TEMP_FAREN || temperature >= HOT_TEMP_CELS) {
		customLCDChar(1,hotTemp);
		lcd_set_position(LCD_ROW_TEMP,LCD_COL_TEMP);
		lcd_data(0x01); //custom alarm active char location
	} else if(temperature <= COLD_TEMP_FAREN || temperature <= COLD_TEMP_CELS) {
		customLCDChar(2,coldTemp);
		lcd_set_position(LCD_ROW_TEMP,LCD_COL_TEMP);
		lcd_data(0x02); //custom alarm active char location
	}
	lcd_print_num(tempInteger);
	if(accuracy != NO_DECIMALS) { //print as int if not decimal points wanted
		lcd_data('.');
		lcd_print_num(tempDecimal);
	}
	if(tempState == CELSIUS) {
		lcd_data('C');
	} else {
		lcd_data('F');
	}
}

