/*
 * Raunel Albiter
 * CE2812-31 Embedded Systems II
 * Dr.Livingston
 * Project: Lab8Albiter
 * File: lcd_driver.c
 * Revision 1: 2/16/17
 */

#include <stdio.h>
#include <stdbool.h>
#include "lcd_driver.h"

void shift_position(int);

/*
 * Initializes all necessary GPIO pins, resets lcd values, and initializes
 * the display to 4 bit mode
 *
 * Param: none
 * Return: none
 */
void init_lcd() {

	const int POWER_ON_DELAY = 5000;
	lcd_delay(POWER_ON_DELAY);

	//Enable clocks for GPIOC & B
	*(RCC_AHB1ENR) |= (1<<GPIOCEN) | (1<<GPIOBEN);

	//Set PB0-2 to output
	*(GPIOB_MODER) &= ~(0b111111<<0); //clear low 6 bits
	*(GPIOB_MODER) |= (0b010101);

	//Set PC8-11 to output
	*(GPIOC_MODER) &= ~(0b11111111<<16); //clear bits 16-23
	*(GPIOC_MODER) |= (0x550000);

	//Init display
	lcd_cmd(0x30); //8-bit mode
	lcd_cmd(0x28); //4-bit mode

	lcd_clear();
	lcd_home();
	lcd_cmd(0x06); //entry mode, move right, no shift
	lcd_cmd(0x0C); //display on, no cursor
}

/*
 * Helper function used to make sure LCD rw & rs are low
 *
 * Param: commandData = lcd data command
 * Return: none
 */
void lcd_cmd(uint8_t commandData) {

	//make sure rw and rs are low
	*(GPIOB_BSRR) |= (LCD_RW_CLR|LCD_RS_CLR);
	lcd_exec(commandData);
}

/*
 * Helper function used to make sure LCD rw is low and
 * rs is high as well as bringing E high
 *
 * Param: printData = data that is input in in order to write to LCD
 * Return: none
 */
void lcd_data(uint8_t printData) {

	//make sure rw is low, rs is high, and bring E high
	*(GPIOB_BSRR) |= (LCD_RW_CLR|LCD_RS_SET);
	lcd_exec(printData);
}

/*
 * Helper function used to execute data to be written to LCD
 *
 * Param: commandData = data/command byte
 * Return: none
 */
void lcd_exec(uint8_t commandData) {

	const int EXEC_DELAY = 40;

	lcd_set_upper_nibble(commandData);
	lcd_latch();
	lcd_set_lower_nibble(commandData);
	lcd_latch();
	lcd_delay(EXEC_DELAY);
}

/*
 * Helper function used to set the upper nibble of the
 * input command/data byte
 *
 * Param: commandData = data/command byte
 * Return: none
 */
void lcd_set_upper_nibble(uint32_t commandData) {

	//Mask off lower nibble
	commandData &= (0xF0);
	//Shift left in order to be in PC8-11 position
	commandData = (commandData<<4);
	//Set upper halfword bits ensure bits that were on will not be on
	*(GPIOC_BSRR) |= (commandData|0xF000000);
}

/*
 * Helper function used to set the lower nibble of the
 * input command/data byte
 *
 * Param: commandData = data/command byte
 * Return: none
 */
void lcd_set_lower_nibble(uint32_t commandData) {

	//Mask off lower nibble
	commandData &= (0x0F);
	//Shift left in order to be in PC8-11 position
	commandData = (commandData<<8);
	//Set upper halfword bits ensure bits that were on will not be on
	*(GPIOC_BSRR) |= (commandData|0xF000000);
}

/*
 * Helper function used as a latch for writing out data
 * to LCD
 *
 * Param: none
 * Return: none
 */
void lcd_latch() {

	//Bring E high
	*(GPIOB_BSRR) = (LCD_E_SET);

	//Necessary E set delay
	lcd_delay(1);

	//Bring E low to latch
	*(GPIOB_BSRR) = (LCD_E_CLR);

	//Necessary E clear delay
	lcd_delay(1);
}

/*
 * Function used in order to send clear command to
 * the LCD and delete everything displayed
 *
 * Param: none
 * Return: none
 */
void lcd_clear() {

	const uint8_t CLEAR_CMD = 0x01;
	const int CLEAR_DELAY = 1500;
	lcd_cmd(CLEAR_CMD); //clear
	lcd_delay(CLEAR_DELAY);
}

/*
 * Function used in order to send home command to
 * the LCD and go back to position row = 0 & column = 0 on LCD
 *
 * Param: none
 * Return: none
 */
void lcd_home() {

	const uint8_t HOME_CMD = 0x02;
	const int HOME_DELAY = 1500;
	lcd_cmd(HOME_CMD); //home
	lcd_delay(HOME_DELAY);
}

/*
 * Function used in order to go to a user specified address on the
 * display where by passing in the row and column position.
 *
 * Param: row & col
 * Return: none
 */
void lcd_set_position(int row,int col) {

	lcd_home();
	const int NEXT_ROW_CONVERSION_FACTOR = 39;
	const int MAX_COL_POSITION = 39;
	//See if value doesn't surpass actual row count
	if(row > 0) {
		row += NEXT_ROW_CONVERSION_FACTOR;
	} else {
		row = 0;
	}
	shift_position(row);

	//See if value doesn't surpass actual column count
	if(col > MAX_COL_POSITION) {
		col = MAX_COL_POSITION;
	} else if (col < 0) {
		col = 0;
	}
	shift_position(col);
}

/*
 * Function used in order to print out a number as an
 * array of ASCII characters
 *
 * Param: numberToPrint
 * Return: none
 */
void lcd_print_num(int numberToPrint) {

	int tempByteShift = 8;
	int bitwiseANDMask = 0;
	int decimalShiftAmount = 24;
	int tempNibbleMask = 4;
	const uint32_t ASCII_ERR = 0x39393939;
	uint32_t asciiMask = 0xff000000;
	uint32_t lcdAsciiData = 0;

	int asciiNumber = num_to_ascii(numberToPrint);
	if(asciiNumber > ASCII_ERR) { //Error message if ASCII is bigger than amount possible (9999)
		lcd_data('E');
		lcd_data('r');
		lcd_data('r');
	}
	int numberOfAsciiChars = ascii_count(asciiNumber);

	tempNibbleMask -= numberOfAsciiChars;
	bitwiseANDMask = tempNibbleMask * tempByteShift;
	decimalShiftAmount -= bitwiseANDMask;

	while(numberOfAsciiChars != 0) {

		//Shift mask to right in order to get each ASCII byte
		asciiMask = (asciiMask >> bitwiseANDMask);
		//Mask ASCII code bytes 1->length of ASCII converted decimal
		lcdAsciiData = (asciiMask & asciiNumber);
		//Get single ASCII code byte and send to lcd
		lcdAsciiData = (lcdAsciiData >> decimalShiftAmount);
		lcd_data(lcdAsciiData);
		decimalShiftAmount -= tempByteShift; //decimalShiftAmount -= 8
		bitwiseANDMask = 8; //shift amount for consecutive ASCII bytes
		numberOfAsciiChars--;
	}
}

/*
 * Function used in order to print out a string as an
 * array of characters 1 by 1 onto the LCD
 *
 * Param: inputString = character array
 * Return: none
 */
void lcd_print_string(char inputString[]) {

	size_t numChars = strlen(inputString);
	for(int i = 0; i < numChars; i++) {

		lcd_data(inputString[i]);
	}
}

/*
 * Helper function used in order shift the position of the LCD cursor by incrementing
 * its address by a specific factor.
 *
 * Param: cursorPosition = determines whether it should be in row 1 or 2
 * Return: none
 */
void shift_position(int cursorPosition) {

	const uint8_t POSITION_CMD = 0x14;
	const uint32_t POSITION_DELAY = 30;

	while(cursorPosition != 0) {
		lcd_cmd(POSITION_CMD);
		lcd_delay(POSITION_DELAY);
		cursorPosition--;
	}
}

/*
 * Helper function used in order have a busy wait that amounts to a millisecond of delay
 * specified by the user
 *
 * Param: microDelay = amount of milliseconds to delay
 * Return: none
 */
void lcd_delay(uint32_t microDelay) {

	const uint8_t MICRO_SECOND_CONV_FACTOR = 8;
	microDelay *= MICRO_SECOND_CONV_FACTOR;

	while(microDelay) {
		microDelay--;
	}
}

/*
 * Helper function used in order count the amount of ASCII characters
 * in a specific array of ascii characters
 *
 * Param: asciiValue
 * Return: none
 */
int ascii_count(int asciiValue) {

	int tempCounter = 0;
	int tempByteShift = 8;

	while(asciiValue != 0) {
		asciiValue = (asciiValue >> tempByteShift);
		tempCounter++;
	}

	return tempCounter;
}

/*
 * Helper function used in to convert from a number to ASCII
 *
 * Param: numberToConvert
 * Return: none
 */
int num_to_ascii(int numberToConvert) {

	if(numberToConvert == 0) {
		return '0';
	}
	uint32_t tempAsciiCode = 0;
	uint8_t initialShiftAmount = 0;
	while(numberToConvert != 0) {

		int remainder = (numberToConvert%10);
		numberToConvert = (numberToConvert/10);
		remainder += '0';
		remainder = (remainder << initialShiftAmount);
		tempAsciiCode |= remainder;
		initialShiftAmount += 8;
	}

	return tempAsciiCode;
}

/*
 *	Function used to create a new LCD character based on some
 *	char array data that is easily translated from a 5x7 bitmap
 *	in order to store into the CCGRAM which can then be accessed by
 *	the same location that it is stored in through printing out from
 *	DDRAM mode
 *
 *	Param: 0-8 range memory location to store custom char, char array that corresponds
 *	to the customer character being created
 *	return: none
 */
void customLCDChar(char location,char* customCodes) {

	const int MAX_LOC_SIZE = 8;
	if(location < MAX_LOC_SIZE) {
		lcd_cmd(0x40+(location*MAX_LOC_SIZE)); //set CCGRAM address to insert custom char
		for(int i = 0; i < MAX_LOC_SIZE; i++) {
			lcd_data(customCodes[i]);
		}
	}
	lcd_cmd(0x80); //return to default DDRAM entry mode
}

