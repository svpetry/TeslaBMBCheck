#include <pic18.h>
#include "base.h"

#include "lcd.h"

#define LCD_PORT LATB
#define LCD_RS 0b00010000
#define LCD_E  0b00100000

static const char battChar[4][8] = {
    {
        0b01110,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111
    },
    {
        0b01110,
        0b11011,
        0b10001,
        0b11111,
        0b11111,
        0b11111,
        0b11111,
        0b11111
    },
    {
        0b01110,
        0b11011,
        0b10001,
        0b10001,
        0b10001,
        0b11111,
        0b11111,
        0b11111
    },
    {
        0b01110,
        0b11011,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b11111
    }
};

static const char warnChar[2][8] = {
    {
        0b00001,
        0b00010,
        0b00101,
        0b00101,
        0b01000,
        0b01001,
        0b10000,
        0b11111
    },
    {
        0b00000,
        0b10000,
        0b01000,
        0b01000,
        0b00100,
        0b00100,
        0b00010,
        0b11110
    }
};

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

static void LcdData(uint8_t data) {
    uint8_t cmd;

    cmd = data >> 4;
    cmd |= LCD_RS | LCD_E;
    LCD_PORT = cmd;
    cmd &= ~LCD_E;
    LCD_PORT = cmd;

    cmd = data & 0x0F;
    cmd |= LCD_RS | LCD_E;
    LCD_PORT = cmd;
    cmd &= ~LCD_E;
    LCD_PORT = cmd;

    LcdDelayShort();    
}

static void LcdGoto(uint8_t col, uint8_t row) {
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
}

static void DefineChar(uint8_t charnum, const char values[]) {
    int i;
    
    // set LCD address to CG RAM
    LcdCmd(0x40 + charnum * 8);

    // enter character bytes into CG RAM
    for (i = 0; i < 8; i++)
        LcdData(values[i]);

    // set LCD address back to display RAM
    LcdGoto(0, 0);
}

static void InitCustomChars() {
    for (uint8_t i = 0; i < 4; i++)
        DefineChar(i + 1, battChar[i]);
    for (uint8_t i = 0; i < 2; i++)
        DefineChar(i + 5, warnChar[i]);
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
    
    InitCustomChars();
}

void LcdPuts(uint8_t col, uint8_t row, const char *str) {
    LcdGoto(col, row);
            
    // write characters
    LCD_PORT = LCD_RS;
    while (*str) {
        LcdData(*str);
        str++;
    }
}

void LcdPutChar(uint8_t col, uint8_t row, char ch) {
    LcdGoto(col, row);
    LCD_PORT = LCD_RS;
    LcdData(ch);
}