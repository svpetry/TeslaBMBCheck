/*
 * File:   main.c
 * Author: svpet
 *
 * 
 * pin layout:
 * RA0: SW1
 * RA1: SW2
 * RA2: -
 * RA3: -
 * RA4: -
 * RA5: -
 * RA6: OSC2
 * RA7: OSC1
 * RC0: BMS_POWER_ON (0 = active)
 * RC1: LCD BACKLIGHT
 * RC2: INFO LED
 * RC3: -
 * RC4: -
 * RC5: -
 * RC6: TX
 * RC7: RX
 * RE3: VPP (ICSP)
 * RB0: LCD_D4
 * RB1: LCD_D5
 * RB2: LCD_D6
 * RB3: LCD_D7
 * RB4: LCD_RS
 * RB5: LCD_E
 * RB6: PGC (ICSP)
 * RB7: PGD (ICSP)
 */

#include <xc.h>
#include <pic18f2480.h>
#include "config.h"
#include "base.h"
#include "lcd.h"
#include "uart.h"
#include "utils.h"
#include "bms-util.h"
#include "bms.h"
#include "types.h"

#define MODULE_ID 3

void Initialize(void) {
    TRISA = 0b11000011;
    LATA = 0;
    TRISB = 0b00000000;
    LATB = 0;
    TRISC = 0b10000000;
    LATC = 0;
    
    UART_Init(612500UL);

    __delay_ms(200);
    LcdInit();
}

void ShowBmsData(struct BmsData bms_data) {
    char s[10];
    
    for (uint8_t i = 0; i < 6; i++) {
        VoltageToStr(s, bms_data.v[i]);
        LcdPuts((i % 3) * 7, i / 3, s);
    }
    
    LcdPuts(0, 3, "T1");
    LcdPuts(10, 3, "T2");
    for (uint8_t i = 0; i < 2; i++) {
        TempToStr(s, bms_data.t[i]);
        LcdPuts(3 + i * 10, 3, s);
    }
}

void ConnectToBms(uint8_t module_id) {
    if (!ResetBoard()) {
        LcdPuts(0, 0, "Reset failed.");
        Halt();
    }
    
    if (!SetNewBoardId(module_id)) {
        LcdPuts(0, 0, "Set ID failed.");
        Halt();
    }

    uint8_t id = FindBoardId();
    if (id != module_id) {
        LcdPuts(0, 0, "Wrong ID.");
        Halt();
    }
}

void main(void) {
    Initialize();
    
    __delay_ms(3000);
    
    LcdPuts(0, 1, "    Connecting...   ");
    
    ConnectToBms(MODULE_ID);
    
    LcdClear();
    
    while (1) {
        struct BmsData data = ReadBmsData(MODULE_ID);
        ShowBmsData(data);
        __delay_ms(1000);
    }
}
