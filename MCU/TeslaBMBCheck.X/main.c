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
 * RC1: LCD backlight
 * RC2: Info LED
 * RC3: Charging relais
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

#define ENABLE_CHARGING 0

#define MODULE_ID 3
#define BMS_POWER_ON_DELAY 3000
#define BATT_SYMBOL_COUNT 4
#define VOLTAGE_CHECK_INTERVAL 5
#define CHARGE_CHECK_INTERVAL 60
#define MIN_CELL_VOLTAGE 2800
#define MAX_CELL_VOLTAGE 4175
#define CELL_CHARGE_TRESHOLD 2
#define DEFAULT_CELL_VOLTAGE 3700

// Each setting occupies three EEPROM bytes: valid marker, low byte, high byte.
#define BALANCE_VOLTAGE_EEPROM_ADDRESS 0
#define CHARGE_VOLTAGE_EEPROM_ADDRESS 3
#define VOLTAGE_SETTING_VALID 0xA5

bool connected = 0;

static uint8_t ReadEeprom(uint8_t address) {
    while (EECON1bits.WR);
    EEADR = address;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.RD = 1;
    return EEDATA;
}

static void WriteEeprom(uint8_t address, uint8_t value) {
    if (ReadEeprom(address) == value)
        return;

    EEADR = address;
    EEDATA = value;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;

    // Only the unlock sequence needs to block the UART receive interrupt.
    bool interrupts_enabled = INTCONbits.GIE;
    INTCONbits.GIE = 0;
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    INTCONbits.GIE = interrupts_enabled;

    while (EECON1bits.WR);
    EECON1bits.WREN = 0;
    PIR2bits.EEIF = 0;
}

static uint16_t LoadVoltageSetting(uint8_t address) {
    if (ReadEeprom(address) != VOLTAGE_SETTING_VALID)
        return DEFAULT_CELL_VOLTAGE;

    uint16_t voltage = ReadEeprom(address + 1);
    voltage |= (uint16_t)ReadEeprom(address + 2) << 8;
    if (voltage < MIN_CELL_VOLTAGE || voltage > MAX_CELL_VOLTAGE)
        return DEFAULT_CELL_VOLTAGE;

    return voltage;
}

static void SaveVoltageSetting(uint8_t address, uint16_t voltage) {
    if (ReadEeprom(address) == VOLTAGE_SETTING_VALID &&
            ReadEeprom(address + 1) == (uint8_t)voltage &&
            ReadEeprom(address + 2) == (uint8_t)(voltage >> 8))
        return;

    // Commit the marker last so an interrupted update is not loaded as a voltage.
    WriteEeprom(address, 0);
    WriteEeprom(address + 1, (uint8_t)voltage);
    WriteEeprom(address + 2, (uint8_t)(voltage >> 8));
    WriteEeprom(address, VOLTAGE_SETTING_VALID);
}

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
    bool abort = 0;
    while (!abort) {
        struct BmsData data = ReadBmsData(MODULE_ID);
        ShowBmsData(data, 1);
        DelayBtnCheck(1000, &abort);
    }
}

void ShowBatterySymbol(uint8_t cell, uint8_t counter, bool state) {
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

static bool BalanceCells(uint16_t voltage, bool stop_when_done) {
    LcdClear();

    bool balancers[CELL_COUNT];
    uint8_t batt_symbol = 0;
    
    char s[8];
    VoltageToStr(s, voltage, 0);
    LcdPuts(14, 3, s);
    
    struct BmsData data = ReadBmsData(MODULE_ID);
    ShowBmsData(data, 0);
    bool abort = data.v_min == 0;
    int count = 0;

    for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
        if (!abort && data.v[cell] > voltage) {
            ShowBatterySymbol(cell, batt_symbol, 1);
            balancers[cell] = 1;
            count++;
        } else
            balancers[cell] = 0;
    }
    
    EnableBalancers(balancers);
    
    int seconds = 0;
    
    while (!abort && (!stop_when_done || count > 0)) {
        if (seconds == VOLTAGE_CHECK_INTERVAL - 1) {
            EnableBmsBalancers(MODULE_ID, 0);
            DelayBtnCheck(100, &abort);
            if (abort)
                break;
            data = ReadBmsData(MODULE_ID);
            ShowBmsData(data, 0);
            if (data.v_min == 0) {
                abort = 1;
                break;
            }

            count = 0;
            for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
                if (balancers[cell]) {
                    if (data.v[cell] <= voltage) {
                        balancers[cell] = 0;
                        ShowBatterySymbol(cell, batt_symbol, 0);
                    } else
                      count++;
                }
            }
            if (count == 0)
                LcdPuts(8, 3, "Done.");
            
            EnableBalancers(balancers);
            DelayBtnCheck(400, &abort);
        } else
            DelayBtnCheck(500, &abort);
        if (abort)
            break;
       
        for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
            if (balancers[cell])
                ShowBatterySymbol(cell, batt_symbol, 1);
        }
        batt_symbol = (batt_symbol + 1) % BATT_SYMBOL_COUNT;

        DelayBtnCheck(500, &abort);
        if (abort)
            break;

        for (uint8_t cell = 0; cell < CELL_COUNT; cell++) {
            if (balancers[cell])
                ShowBatterySymbol(cell, batt_symbol, 1);
        }
        batt_symbol = (batt_symbol + 1) % BATT_SYMBOL_COUNT;

        if (seconds == VOLTAGE_CHECK_INTERVAL - 1)
            seconds = 0;
        else
            seconds++;
    }
    EnableBmsBalancers(MODULE_ID, 0);
    return !abort;
}

void Balance(uint16_t voltage) {
    BalanceCells(voltage, 0);
    LcdClear();
    WaitButtonsReleased();
}

static bool ChargeCells(uint16_t voltage) {
    SetChargeRelais(0);
    EnableBmsBalancers(MODULE_ID, 0);
    LcdClear();
    
    uint8_t batt_symbol = BATT_SYMBOL_COUNT - 1;
    bool balancers[CELL_COUNT] = {0};

    char s[8];
    VoltageToStr(s, voltage, 0);
    LcdPuts(14, 3, s);

    struct BmsData data = ReadBmsData(MODULE_ID);
    ShowBmsData(data, 0);
    
    int seconds = 0;
    bool abort = data.v_min == 0;

    if (!abort && data.v_min < voltage) {
        while (!abort) {
            bool check_voltage = seconds % VOLTAGE_CHECK_INTERVAL == 0;
            if (check_voltage) {
                if (seconds == 0)
                    SetChargeRelais(0);

                // Let cell voltages settle without the balancing load.
                EnableBmsBalancers(MODULE_ID, 0);
                DelayBtnCheck(seconds == 0 ? 500 : 100, &abort);
                if (abort)
                    break;

                data = ReadBmsData(MODULE_ID);
                ShowBmsData(data, 0);
                if (data.v_min == 0) {
                    abort = 1;
                    break;
                }
                if (seconds == 0 && data.v_min >= voltage + CELL_CHARGE_TRESHOLD)
                    break;

                for (uint8_t cell = 0; cell < CELL_COUNT; cell++)
                    balancers[cell] = data.v[cell] > voltage;

                if (seconds == 0)
                    SetChargeRelais(1);
            }

            // Refresh every second so balancing does not time out between readings.
            EnableBalancers(balancers);
            if (seconds != 0) {
                DelayBtnCheck(check_voltage ? 400 : 500, &abort);
                if (abort)
                    break;
            }
            
            for (uint8_t cell = 0; cell < CELL_COUNT; cell++)
                ShowBatterySymbol(cell, batt_symbol, 1);
            batt_symbol = (batt_symbol - 1) % BATT_SYMBOL_COUNT;
            
            DelayBtnCheck(500, &abort);
            if (abort)
                break;

            for (uint8_t cell = 0; cell < CELL_COUNT; cell++)
                ShowBatterySymbol(cell, batt_symbol, 1);
            batt_symbol = (batt_symbol - 1) % BATT_SYMBOL_COUNT;

            if (seconds == CHARGE_CHECK_INTERVAL - 1)
                seconds = 0;
            else
                seconds++;
        }
    }
    SetChargeRelais(0);
    EnableBmsBalancers(MODULE_ID, 0);
    return !abort;
}

void Charge(uint16_t voltage) {
    bool abort = 0;
    while (!abort) {
        if (!ChargeCells(voltage) || !BalanceCells(voltage, 1))
            break;

        LcdPuts(8, 3, "Done.");
        // Keep both loads off while the cells settle and monitor for a voltage drop.
        while (!abort) {
            DelayBtnCheck(VOLTAGE_CHECK_INTERVAL * 1000, &abort);
            if (abort)
                break;

            struct BmsData data = ReadBmsData(MODULE_ID);
            ShowBmsData(data, 0);
            if (data.v_min == 0) {
                abort = 1;
                break;
            }
            if (data.v_min < voltage)
                break;
        }
    }

    SetChargeRelais(0);
    EnableBmsBalancers(MODULE_ID, 0);
    LcdClear();
    WaitButtonsReleased();
}

uint16_t InputVoltage(uint8_t settings_address) {
    uint16_t initial_voltage = LoadVoltageSetting(settings_address);
    LcdClear();
    LcdPuts(2, 1, "Target voltage:");
    LcdPuts(8, 2, " V  Start");
    
    bool start = 0;
    int voltage = 0;
    char s[8];
    uint8_t pos = 0;
    char digits[4];
    
    VoltageToStr(s, initial_voltage, 0);
    LcdPuts(3, 2, s);
    digits[0] = initial_voltage / 1000;
    digits[1] = (initial_voltage / 100) % 10;
    digits[2] = (initial_voltage / 10) % 10;
    digits[3] = initial_voltage % 10;
    
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

    if (voltage > MAX_CELL_VOLTAGE)
        voltage = MAX_CELL_VOLTAGE;
    if (voltage < MIN_CELL_VOLTAGE)
        voltage = MIN_CELL_VOLTAGE;
    
    SaveVoltageSetting(settings_address, (uint16_t)voltage);
    WaitButtonsReleased();
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
            LcdPuts(0, 3, "TeslaBMBChecker V1.1");
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
#if ENABLE_CHARGING
            LcdPuts(0, 1, "  Balance charge    ");
#else
            LcdPuts(0, 1, "  Balance           ");
#endif
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
                            LcdPuts(18, 3, "\x05\x06"); // Warning sign
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
#if ENABLE_CHARGING
                    uint16_t voltage = InputVoltage(CHARGE_VOLTAGE_EEPROM_ADDRESS);
                    Charge(voltage);
#else
                    uint16_t voltage = InputVoltage(BALANCE_VOLTAGE_EEPROM_ADDRESS);
                    Balance(voltage);
#endif
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
