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

#ifndef LCDHD44780_H
#define LCDHD44780_H

#include <stdint.h>


void LcdHd44780_powerOnWait(void);
void LcdHd44780_init(void);
void LcdHd44780_clear(void);
void LcdHd44780_cursor(uint8_t line, uint8_t pos);
void LcdHd44780_print(const char* str);

void LcdHd44780_outNibble(uint8_t nibble);
void LcdHd44780_writeNibble(uint8_t nibble);
void LcdHd44780_writeByte(uint8_t byte);
void LcdHd44780_writeCommand(uint8_t cmd);
void LcdHd44780_writeData(uint8_t data);

#endif // LCDHD44780_H
