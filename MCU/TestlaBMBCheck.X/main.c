/*
 * File:   main.c
 * Author: svpet
 *
 * 
 * pin layout:
 * RA0: SW1 (0 = pressed)
 * RA1: SW2 (0 = pressed)
 * RA2: BMS_FAULT
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
#include <string.h>
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
#define BATT_SYMBOL_COUNT 4
#define BALANCE_CHECK_INTERVAL 5

bool connected = 0;

void Initialize(void) {
    TRISA = 0b11000111;
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
    LcdPuts(4, 1, "Connecting...");
    
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

    __delay_ms(100);
    ClearBmsFaults(MODULE_ID);

    connected = 1;
}

void DisconnectBms() {
    SetBmsPower(0);
    connected = 0;
    LcdClear();
    LcdPuts(4, 1, "Disconnected.");
    __delay_ms(1000);
}

void ShowBmsData(struct BmsData bms_data, bool show_temp) {
    char s[8];
    
    for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
        VoltageToStr(s, bms_data.v[cell], 0);
        LcdPuts((cell % 3) * 7, cell / 3, s);
    }
    
    if (show_temp) {
        for (uint8_t i = 0; i < 2; i++) {
            TempToStr(s, bms_data.t[i]);
            LcdPuts(3 + i * 10, 3, s);
        }
    } else {
        VoltageToStr(s, bms_data.mv, 1);
        LcdPuts(1, 3, s);
    }

    if (BmsFaultActive())
        LcdPuts(0, 2, "\x05\x06 Fault detected.");
    else
        LcdPuts(0, 2, "                    ");
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

void ShowBalanceMarker(uint8_t cell, uint8_t counter, bool state) {
    uint8_t col = (cell % 3) * 7 + 5;
    uint8_t row = cell / 3;
    
    // Battery symbols are characters 1 (full), 2, 3 and 4 (empty)
    char battChar = 1 + counter;
    
    LcdPutChar(col, row, state ? battChar : ' ');
}

void EnableBalancers(bool balancers[CELL_COUNT]) {
    uint8_t bits = 0;
    for (int i = 0; i < CELL_COUNT; i++) {
        if (balancers[i])
            bits |= 1 << i;
    }
    EnableBmsBalancers(MODULE_ID, bits);
}

void Balance(uint16_t voltage) {
    LcdClear();

    bool balancers[CELL_COUNT];
    uint8_t batt_symbol = 0;
    
    char s[8];
    VoltageToStr(s, voltage, 0);
    LcdPuts(14, 3, s);
    
    struct BmsData data = ReadBmsData(MODULE_ID);
    ShowBmsData(data, 0);

    for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
        if (data.v[cell] > voltage) {
            ShowBalanceMarker(cell, batt_symbol, 1);
            balancers[cell] = 1;
        } else
            balancers[cell] = 0;
    }
    
    WaitButtonsReleased();
    EnableBalancers(balancers);
    
    int seconds = 0;
    
    while (!GetBtnState(0) && !GetBtnState(1)) {
        if (seconds == BALANCE_CHECK_INTERVAL - 1) {
            EnableBmsBalancers(MODULE_ID, 0);
            __delay_ms(100);
            data = ReadBmsData(MODULE_ID);
            ShowBmsData(data, 0);
            EnableBalancers(balancers);
            __delay_ms(400);

            int count = 0;
            for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
                if (balancers[cell]) {
                    if (data.v[cell] <= voltage) {
                        balancers[cell] = 0;
                        ShowBalanceMarker(cell, batt_symbol, 0);
                        seconds = 0;
                    } else
                      count++;
                }
            }
            if (count == 0)
                LcdPuts(8, 3, "Done.");
            
            EnableBalancers(balancers);
        } else
            __delay_ms(500);
       
        for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
            if (balancers[cell])
                ShowBalanceMarker(cell, batt_symbol, 1);
        }
        batt_symbol = (batt_symbol + 1) % BATT_SYMBOL_COUNT;

        __delay_ms(500);

        for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
            if (balancers[cell])
                ShowBalanceMarker(cell, batt_symbol, 1);
        }
        batt_symbol = (batt_symbol + 1) % BATT_SYMBOL_COUNT;

        if (seconds == BALANCE_CHECK_INTERVAL - 1)
            seconds = 0;
        else
            seconds++;
    }
    EnableBmsBalancers(MODULE_ID, 0);

    LcdClear();
    WaitForButtonPressed();
}

uint16_t InputVoltage() {
    LcdClear();
    LcdPuts(2, 1, "Balance voltage:");
    LcdPuts(3, 2, "3.700 V  Start");
    
    bool start = 0;
    int voltage = 0;
    char s[8];
    uint8_t pos = 0;
    char digits[4];
    
    digits[0] = 3;
    digits[1] = 7;
    digits[2] = 0;
    digits[3] = 0;
    
    do {
        WaitButtonsReleased();
        strcpy(s, "     ");
        if (pos < 5) {
            s[pos] = '^';
            LcdPuts(12, 3, "     ");
        } else 
            LcdPuts(12, 3, "^^^^^");
        LcdPuts(3, 3, s);
        
        uint8_t dpos = pos;
        if (dpos > 0) dpos--;

        while (!GetBtnState(0) && !start) {
            WaitForButtonPressed();
            if (GetBtnState(1)) {
                if (pos < 5) {
                    digits[dpos] = (digits[dpos] + 1) % 10;
                    s[0] = '0' + digits[dpos];
                    s[1] = 0;
                    LcdPuts(3 + pos, 2, s);
                } else {
                    start = 1;
                }
                WaitButtonsReleased();
            }
        }
        
        pos++;
        if (pos == 1)
            pos++;
        if (pos == 6)
            pos = 0;
    } while (!start);
    
    int factor = 1000;
    for (int i = 0; i < 4; i++) {
        voltage += digits[i] * factor;
        factor /= 10;
    }

    if (voltage > 4175)
        voltage = 4175;
    if (voltage < 3200)
        voltage = 3200;
    
    return (uint16_t)voltage;
}

void ClearFaults() {
    LcdClear();
    WaitButtonsReleased();
    LcdPuts(1, 1, "Clearing faults...");
    __delay_ms(200);
    ClearBmsFaults(MODULE_ID);
    LcdPuts(7, 2, "Done.");
    WaitForButtonPressed();
}

void MainMenu() {
    while (1) {
        LcdClear();
        WaitButtonsReleased();
        
        if (!connected) {
            LcdPuts(0, 1, "Press SW2 to connect");
            LcdPuts(0, 3, "TeslaBMBChecker V1.0");
            while (!GetBtnState(1)) {
                SetLcdBacklight(0);
                __delay_ms(4);
                SetLcdBacklight(1);
                __delay_ms(1);
            }
            ConnectToBms(MODULE_ID);
            if (!connected) {
                LcdPuts(0, 1, "Connection failed.  ");
                WaitForButtonPressed();
            }
        } else {
            
            LcdPuts(0, 0, "> Show sensor values");
            LcdPuts(0, 1, "  Start balancing   ");
            LcdPuts(0, 2, "  Clear faults      ");
            LcdPuts(0, 3, "  Disconnect        ");
            
            int menu_idx = 0;
            while (!GetBtnState(1)) {
                
                WaitButtonsReleased();

                bool fault = 0;
                while (!GetBtnState(0) && !GetBtnState(1)) {
                    if (BmsFaultActive() != fault) {
                        fault = BmsFaultActive();
                        if (fault)
                            LcdPuts(18, 3, "\x05\x06");
                        else
                            LcdPuts(18, 3, "  ");
                    }
                }

                WaitForButtonPressed();
                
                if (GetBtnState(0)) {
                    menu_idx = (menu_idx + 1) % 4;

                    for (uint8_t i = 0; i < 4; i++) {
                        LcdPuts(0, i, menu_idx == i ? ">" : " ");
                    }
                }
            }
            
            LcdClear();
            WaitButtonsReleased();
            
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
                    ClearFaults();
                    break;
                }
                case 3: {
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
