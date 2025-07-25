#include <pic18.h>
#include "base.h"

#include "lcd.h"

#define LCD_PORT LATB
#define LCD_RS 0b00010000
#define LCD_E  0b00100000

static void LcdDelayLong() {
    __delay_ms(80);
}

static void LcdDelayShort() {
    __delay_us(200);
}

static void LcdCmd(uint8_t cmd) {
    uint8_t a;

    LCD_PORT = 0x00;

    // higher nibble
    a = cmd >> 4;
    a |= LCD_E;
    LCD_PORT = a;
    a &= ~LCD_E;
    LCD_PORT = a;

    // lower nibble
    a = cmd & 0x0F;
    a |= LCD_E;
    LCD_PORT =  a;
    a &= ~LCD_E;
    LCD_PORT = a;

    LcdDelayShort();
}

void LcdClear() {
    LcdCmd(0b00000001);
    LcdDelayLong();
}

void LcdInit() {
    uint8_t a;
    
    LCD_PORT = 0x00;

    // init
    a = 0b00000011;

    a |= LCD_E;
    LCD_PORT = a;
    a &= ~LCD_E;
    LCD_PORT = a;
    LcdDelayLong();

    a |= LCD_E;
    LCD_PORT = a;
    a &= ~LCD_E;
    LCD_PORT = a;
    LcdDelayLong();

    a |= LCD_E;
    LCD_PORT = a;
    a &= ~LCD_E;
    LCD_PORT = a;
    LcdDelayLong();
    
    // init 4 bit mode
    a = 0b00000010;
    a |= LCD_E;
    LCD_PORT = a;
    a &= ~LCD_E;
    LCD_PORT = a;

    // clear screen, cursor to pos 0
    LcdClear();

    // cursor direction right, shift off
    LcdCmd(0b00000110);
    LcdDelayLong();

    // display on, cursor off
    LcdCmd(0b00001100);

    // 4 bit interface, two/four rows 
    LcdCmd(0b00101000);
}

void LcdPuts(uint8_t col, uint8_t row, const char *str) {
    uint8_t cmd;
    
    // set cursor position
    switch (row) {
        case 0:
            cmd = 0x00;
            break;
        case 1:
            cmd = 0x40;
            break;
        case 2:
            cmd = 0x14;
            break;
        case 3:
            cmd = 0x54;
            break;
    }
    cmd = (cmd + col) | 0b10000000;
    LcdCmd(cmd);

    // write characters
    LCD_PORT = LCD_RS;
    while (*str) {
        cmd = *str >> 4;
        cmd |= LCD_RS | LCD_E;
        LCD_PORT = cmd;
        cmd &= ~LCD_E;
        LCD_PORT = cmd;

        cmd = *str & 0x0F;
        cmd |= LCD_RS | LCD_E;
        LCD_PORT = cmd;
        cmd &= ~LCD_E;
        LCD_PORT = cmd;

        LcdDelayShort();

        str++;
    }
}
