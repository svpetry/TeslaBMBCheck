/*
 * File:   main.c
 * Author: svpet
 *
 * 
 * pin layout:
 * RA0: SW1 (0 = pressed)
 * RA1: SW2 (0 = pressed)
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
#include "board.h"
#include "types.h"

#define MODULE_ID 3
#define BMS_POWER_ON_DELAY 3000

bool connected = 0;

void Initialize(void) {
    TRISA = 0b11000011;
    LATA = 0;
    TRISB = 0b00000000;
    LATB = 0;
    TRISC = 0b10000000;
    LATC = 0b00000001;
    
    UART_Init(612500UL);

    __delay_ms(200);
    LcdInit();
}

void ConnectToBms(uint8_t module_id) {
    LcdClear();
    LcdPuts(0, 1, "    Connecting...   ");
    
    connected = 0;
    
    SetBmsPower(1);
    __delay_ms(BMS_POWER_ON_DELAY);
    if (!ResetBoard()) {
        SetBmsPower(0);
        return;
    }
    if (!SetNewBoardId(module_id)) {
        SetBmsPower(0);
        return;
    }
    if (FindBoardId() != module_id) {
        SetBmsPower(0);
        return;
    }
    
    connected = 1;
}

void DisconnectBms() {
    SetBmsPower(0);
    connected = 0;
}

void ShowBmsData(struct BmsData bms_data, bool show_temp) {
    char s[10];
    
    for (uint8_t i = 0; i < 6; i++) {
        VoltageToStr(s, bms_data.v[i], 0);
        LcdPuts((i % 3) * 7, i / 3, s);
    }
    
    if (show_temp) {
        for (uint8_t i = 0; i < 2; i++) {
            TempToStr(s, bms_data.t[i]);
            LcdPuts(3 + i * 10, 3, s);
        }
    } else {
        VoltageToStr(s, bms_data.mv, 1);
        LcdPuts(0, 3, s);
    }
}

void ShowStatus() {
    LcdClear();
    LcdPuts(0, 3, "T1");
    LcdPuts(10, 3, "T2");
    while (!GetBtnState(0) && !GetBtnState(1)) {
        struct BmsData data = ReadBmsData(MODULE_ID);
        ShowBmsData(data, 1);
        __delay_ms(1000);
    }
}

void ShowBalanceMarker(uint8_t cell, bool state) {
    if (state)
        LcdPuts((cell % 3) * 7 + 5, cell / 3, "\0x09"); // arrow down
    else
        LcdPuts((cell % 3) * 7 + 5, cell / 3, " ");
}

void Balance(uint16_t voltage) {
    LcdClear();

    bool shunts[6];
    
    struct BmsData data = ReadBmsData(MODULE_ID);
    ShowBmsData(data, 0);
    for (uint8_t i = 0; i < 6; i++) {
        if (data.v[i] > voltage) {
            shunts[i] = 1;
            EnableShunt(MODULE_ID, i, 1);
        } else {
            shunts[i] = 0;
        }
    }
    
    LcdPuts(10, 3, "Balancing");
    
    while (!GetBtnState(0) && !GetBtnState(1)) {
        data = ReadBmsData(MODULE_ID);
        ShowBmsData(data, 0);
        
        int count = 0;
        for (uint8_t i = 0; i < 6; i++) {
            if (shunts[i]) {
                count++;
                ShowBalanceMarker(i, 1);
                if (data.v[i] <= voltage) {
                    shunts[i] = 0;
                    EnableShunt(MODULE_ID, i, 0);
                }
            }
        }
        
        if (count == 0)
            LcdPuts(10, 3, "Finished ");
        
        __delay_ms(500);
        
        for (uint8_t i = 0; i < 6; i++)
            ShowBalanceMarker(i, 0);

        __delay_ms(500);
    }

    for (uint8_t i = 0; i < 6; i++) {
        if (shunts[i])
            EnableShunt(MODULE_ID, i, 0);
    }
    
    WaitForButtonPressed();
}

uint16_t InputVoltage() {
    LcdClear();
    LcdPuts(2, 1, "Balance voltage:");
    LcdPuts(3, 2, "0.000 V");
    
    int voltage = 0;
    int factor = 1000;
    char s[2] = {0, 0};
    uint8_t pos = 0;
    do {
        WaitButtonsReleased();
        LcdPuts(3, 3, "     ");
        LcdPuts(3 + pos, 3, "^");
        
        char digit = 0;
        while (!GetBtnState(0)) {
            WaitForButtonPressed();
            if (GetBtnState(1)) {
                digit = (digit + 1) % 10;
                s[0] = '0' + digit;
                LcdPuts(3 + pos, 2, s);
                WaitButtonsReleased();
            }
        }
        
        voltage += digit * factor;
        factor /= 10;
        
        if (pos == 0)
            pos = 2;
        else
            pos++;
    } while (pos < 4);
    
    return (uint16_t)voltage;
}

void MainMenu() {
    while (1) {
        LcdClear();
        WaitButtonsReleased();
        
        if (!connected) {
            LcdPuts(0, 1, "Press SW2 to connect");
            while (!GetBtnState(1)) ;
            ConnectToBms(MODULE_ID);
            if (!connected) {
                LcdPuts(0, 1, "Connection failed.  ");
                while (!GetBtnState(1)) ;
            }
        } else {
            
            LcdPuts(0, 0, "> Show BMS values   ");
            LcdPuts(0, 1, "  Start balancing   ");
            LcdPuts(0, 2, "  Disconnect        ");
            
            int menu_idx = 0;
            while (!GetBtnState(1)) {
                WaitButtonsReleased();
                WaitForButtonPressed();
                
                if (GetBtnState(0)) {
                    menu_idx = (menu_idx + 1) % 3;

                    for (uint8_t i = 0; i < 3; i++) {
                        LcdPuts(0, i, menu_idx == i ? ">" : " ");
                    }
                }
            }
            
            WaitButtonsReleased();
            LcdClear();
            
            switch (menu_idx) {
                case 0: {
                    ShowStatus();
                    break;
                }
                case 1: {
                    uint16_t voltage = InputVoltage();
                    Balance(voltage);
                    break;
                }
                case 2: {
                    DisconnectBms();
                    break;
                }
            }
        }
    }
}

void main(void) {
    Initialize();
    SetLcdBacklight(1);
    MainMenu();
}
