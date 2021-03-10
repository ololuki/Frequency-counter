/*
Copyright (C) 2018 Ololuki
https://ololuki.pl

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "LcdHd44780.h"
#include <avr/io.h>
#include <util/delay.h>


#define RS_PORT B
#define RS 5
#define EN_PORT B
#define EN 4
#define D7_PORT B
#define D7 3
#define D6_PORT B
#define D6 2
#define D5_PORT B
#define D5 1
#define D4_PORT B
#define D4 0

// delay times - must be increased for slower displays

// clearDisplay and returnHome execution time
// datasheet says 1.52 ms
const int clearReturnDelay_ms = 3;

// another instructions execution time
// datasheet says 37 us
const int instructionDelay_us = 50;


#define PORT(x) XPORT(x)
#define XPORT(x) (PORT##x)
#define PIN(x) XPIN(x)
#define XPIN(x) (PIN##x)
#define DDR(x) XDDR(x)
#define XDDR(x) (DDR##x)

const uint8_t clearDisplay = 0x01;
const uint8_t returnHome = 0x02;

// Set cursor or display move direction during data write and read
const uint8_t entryModeSet = 0x04;
const uint8_t entryModeIncrement = 0x02;
const uint8_t entryModeDecrement = 0x00;
const uint8_t entryModeDisplayShift = 0x01;

const uint8_t display = 0x08;
const uint8_t displayOn = 0x04;
const uint8_t displayOff = 0x00;
const uint8_t displayCursorOn = 0x02;
const uint8_t displayCursorOff = 0x00;
const uint8_t displayBlinkingOn = 0x01;
const uint8_t displayBlinkingOff = 0x00;

// Move cursor or shift disply without changing DDRAM contents
const uint8_t shift = 0x10;
const uint8_t shiftDisplay = 0x08;
const uint8_t shiftCursor = 0x00;
const uint8_t shiftRight = 0x04;
const uint8_t shiftLeft = 0x00;

const uint8_t functionSet = 0x20;
const uint8_t functionSet8Bit = 0x10;
const uint8_t functionSet4Bit = 0x00;
const uint8_t functionSet2Lines = 0x08;
const uint8_t functionSet1Line = 0x00;
const uint8_t functionSet5x10Dots = 0x04;
const uint8_t functionSet5x8Dots = 0x00;

const uint8_t setCGRamAddress = 0x40;

const uint8_t setDDRamAddress = 0x80;


void LcdHd44780_powerOnWait(void)
{
	// wait 15 ms for 5V or 40 ms for 2,7V
	_delay_ms(15);
}

void LcdHd44780_init(void)
{
	DDR(RS_PORT) |= (1<<RS);
	DDR(EN_PORT) |= (1<<EN);
	
	DDR(D7_PORT) |= (1<<D7);
	DDR(D7_PORT) |= (1<<D6);
	DDR(D7_PORT) |= (1<<D5);
	DDR(D7_PORT) |= (1<<D4);
	
	const uint8_t byte = (functionSet | functionSet8Bit);
	
	LcdHd44780_writeNibble(byte >> 4);
	_delay_ms(5); // datasheet says 5 ms
	LcdHd44780_writeNibble(byte >> 4);
	_delay_us(300); // datasheet says 100 us
	LcdHd44780_writeNibble(byte >> 4);
	_delay_us(instructionDelay_us);
	
	LcdHd44780_writeNibble((functionSet | functionSet4Bit) >> 4);
	_delay_us(instructionDelay_us);
	
	LcdHd44780_writeCommand(functionSet | functionSet4Bit | functionSet2Lines | functionSet5x8Dots);
	LcdHd44780_writeCommand(display | displayOff);
	LcdHd44780_writeCommand(clearDisplay);
	_delay_ms(clearReturnDelay_ms);
	
	LcdHd44780_writeCommand(entryModeSet | entryModeIncrement);
	LcdHd44780_writeCommand(display | displayOn);
}

void LcdHd44780_clear(void)
{
	LcdHd44780_writeCommand(clearDisplay);
	_delay_ms(2);
}

/// Move cursor (DDRAM address).
void LcdHd44780_cursor(uint8_t line, uint8_t pos)
{
	LcdHd44780_writeCommand(setDDRamAddress | ((0x40 * line) + pos));
}

/// Send c-string. Put chars until '\0'. Function waits until whole data is sent.
void LcdHd44780_print(const char* str)
{
	while(*str)
	{
		LcdHd44780_writeData(*str++);
	}
}

/// Put lower 4 bits to data bus
void LcdHd44780_outNibble(uint8_t nibble)
{
	if (nibble & (1 << 3))
		PORT(D7_PORT) |= (1<<D7);
	else
		PORT(D7_PORT) &= ~(1<<D7);
	if (nibble & (1 << 2))
		PORT(D6_PORT) |= (1<<D6);
	else
		PORT(D6_PORT) &= ~(1<<D6);
	if (nibble & (1 << 1))
		PORT(D5_PORT) |= (1<<D5);
	else
		PORT(D5_PORT) &= ~(1<<D5);
	if (nibble & (1 << 0))
		PORT(D4_PORT) |= (1<<D4);
	else
		PORT(D4_PORT) &= ~(1<<D4);
}

/// Write 4 bits to LCD
void LcdHd44780_writeNibble(uint8_t nibble)
{
	PORT(EN_PORT) |= (1<<EN);
	LcdHd44780_outNibble(nibble);
	PORT(EN_PORT) &= ~(1<<EN);
}

/// Write whole byte to LCD and wait for execution on LCD
void LcdHd44780_writeByte(uint8_t byte)
{
	LcdHd44780_writeNibble(byte >> 4);
	LcdHd44780_writeNibble(byte & 0x0F);
	_delay_us(instructionDelay_us);
}

/// Write byte as command
void LcdHd44780_writeCommand(uint8_t cmd)
{
	PORT(RS_PORT) &= ~(1<<RS);
	LcdHd44780_writeByte(cmd);
}

/// Write byte as data
void LcdHd44780_writeData(uint8_t data)
{
	PORT(RS_PORT) |= (1<<RS);
	LcdHd44780_writeByte(data);
}

