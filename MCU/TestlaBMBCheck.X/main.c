/*
 * File:   main.c
 * Author: svpet
 *
 * Created on 30. November 2024, 10:10
 * 
 * pin layout:
 * RA0: 
 * RA1: 
 * RA2: 
 * RA3: 
 * RA4: 
 * RA5: 
 * RA6: 
 * RA7: 
 * RC0: 
 * RC1: 
 * RC2: 
 * RC3: 
 * RC4: 
 * RC5: 
 * RC6: 
 * RC7: 
 * RE3: 
 * RB0: 
 * RB1: 
 * RB2: 
 * RB3: 
 * RB4: 
 * RB5: 
 * RB6: PGC
 * RB7: PGD
 */

#include <xc.h>
#include <pic18f2525.h>
#include "config.h"
#include "base.h"
#include "lcd.h"
#include "uart.h"
#include "utils.h"
#include "bms.h"

struct BmsData {
    uint16_t v[6];
    uint16_t t[2];
};

void Initialize(void) {
    TRISC = 0b10000000;
    LATC = 0;
    TRISB = 0;
    LATB = 0;
    
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

void ConnectToBms() {
    
}

struct BmsData ReadBmsData() {
    
}

void main(void) {
    Initialize();
    
    __delay_ms(3000);

    if (!ResetBoard()) {
        LcdPuts(0, 0, "Reset failed.");
        Halt();
    }

    if (!SetNewBoardId(3)) {
        LcdPuts(0, 0, "Set ID failed.");
        Halt();
    }

    uint8_t id = FindBoardId();
    
    char s[10];
    HexStr(id, s);
    LcdPuts(0, 0, "Module ID:");
    LcdPuts(11, 0, s);
    
    while (1) ;
    
    LcdPuts(0, 1, "    Connecting...   ");
    
    ConnectToBms();
    
    LcdClear();
    
    while (1) {
        struct BmsData data = ReadBmsData();
        ShowBmsData(data);
        __delay_ms(1000);
    }
}
