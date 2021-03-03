/*
 * adc_driver.c
 *
 *  Created on: Feb 13, 2017
 *      Author: trapPollo
 */


/*
 * Raunel Albiter

 * CE2812-31 Embedded Systems II
 * Dr.Livingston
 * Project: Lab3Albiter
 * File: adc_driver.c
 * Revision 1: 1/12/17
 */

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "adc_driver.h"

/*
 * Initial analog to digital converter that will
 * allow for the temperature sensor data to be
 * read in properly and be converted to a temperature.
 *
 * Param: none
 * Return: none
 */
void init_adc() {

	//Enable clock to ADC
	*(RCC_APB2ENR) |= (1<<8); //ADC1 is bit 8

	*(RCC_AHB1ENR) |= (1<<GPIOAEN); //GPIOA is bit 0

	//Enable analog to PA6
	*(GPIOA_MODER) &= ~(0b11<<12); //clear bits 12-13
	*(GPIOA_MODER) |= (0b11<<12); //analog mode

	//Turn ADC on
	*(ADC1_CR2) = (1<<ADONEN);

	//Select a channel. Dev board temp sensor is ADC channel 6
	*(ADC1_SQR3) = 6;
	//this assumes ADC_SQR1 is defaulted to 0, which would mean
	//one channel in the scan
}

/*
 * Function that allows for the raw data to be
 * acquired from the temp sensor when the conversion from the
 * ADC is done
 *
 * Param: none
 * Return: Raw Data
 */
int data_conversion() {

	int statusReg = 0;
	//Start conversion - set SWSTART bit in CR2 (ADON too)
	*(ADC1_CR2) |= (1<<30); // bit 30 = SWSTART
	//Poll status register to EOC
	statusReg = (*(ADC1_SR)>>1) & 1;
	while(statusReg == 0) {
		statusReg = (*(ADC1_SR)>>1) & 1;
	}

	return *(ADC1_DR); //return raw data
}

/*
 * Function that converts the raw data from the
 * temp sensor to a readable voltage data in mV
 *
 * Param: rawData
 * Return: voltageData
 */
double convertVoltage(int rawData) {
	const double REF_VOLTAGE = 3.3;
	const int DEFAULT_RESOLUTION = 4095;
	const int MILLIVOLT_CONV_FACTOR = 1000;
	double tempVoltage = 0;
	//Convert to volts
	tempVoltage = rawData*REF_VOLTAGE;
	tempVoltage /= DEFAULT_RESOLUTION;
	tempVoltage *= MILLIVOLT_CONV_FACTOR;
	return tempVoltage;
}

/*
 * Function that allows for the raw data to be
 * acquired from the temp sensor when the conversion from the
 * ADC is done
 *
 * Param: voltageData, tempState = current temperature units,
 * 			tempFarenheit = farenheit temperature
 * Return: temperature data in celsius
 */
double convertCelsius(double voltageData,int tempState,double tempFarenheit) {

	double tempTotal = voltageData;
	if(tempState == 1) {
		const int TEST_COND_TEMP_CELSIUS = 25;
		const int TYP_OUT_VOLTAGE_MV = 750;
		const int TYP_SCALE_VOLTAGE_MV = 10;

		//Convert to Celsius
		tempTotal = (TEST_COND_TEMP_CELSIUS+((tempTotal-TYP_OUT_VOLTAGE_MV)/TYP_SCALE_VOLTAGE_MV));
	} else {
		const int CELSIUS_CONV_FACTOR = 32;
		tempTotal = tempFarenheit - CELSIUS_CONV_FACTOR;
		tempTotal = (tempTotal*5)/9;
	}

	return tempTotal;
}

/*
 * Function that converts the celsius temperature
 * data into farenheit
 *
 * Param: temperature in Celsius
 * Return: temperature in Farenheit
 */
double convertFarenheit(double tempCelsius) {

	const int FARENHEIT_CONV_FACTOR = 32;
	double returnTemp = 0;
	returnTemp = (tempCelsius*9)/5;
	return returnTemp + FARENHEIT_CONV_FACTOR;
}

/*
 * Function that allows for only the integer
 * portion of the temperature to be returned
 * in order to print it on the LCD
 *
 * Param: current temperature
 * Return: integer portion of temperature
 */
int getInteger(double temperature) {

	double tempData[2];
	tempData[1] = modf(temperature,&tempData[0]);
	return tempData[0];
}

/*
 * Function that allows for only the decimal
 * portion of the temperature to be returned
 * with the amount of decimal places specified
 * in order to print it on the LCD
 *
 * Param: current temperature
 * Return: decimal portion of temperature with specified decimal places
 */
int getDecimal(double temperature,int accuracy) {

	double tempData[2];
	int tempAccuracy = 10;

	if(accuracy > 0) {
		tempAccuracy = pow(tempAccuracy,accuracy);
	}
	tempData[1] = modf(temperature,&tempData[0]);
	return tempData[1]*tempAccuracy;
}
