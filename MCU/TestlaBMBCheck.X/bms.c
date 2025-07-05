#include <pic18.h>
#include <math.h>
#include "base.h"

#include "uart.h"
#include "bms.h"
#include "bms-util.h"
#include "lcd.h"
#include "utils.h"

uint8_t FindBoardId() {
    uint8_t payload[3];
    uint8_t buff[8];

    payload[0] = 0;
    payload[1] = 0; // read registers starting at 0
    payload[2] = 1; // read one byte
    for (uint8_t x = 1; x <= MAX_MODULE_ADDR; x++) {
        payload[0] = (uint8_t)(x << 1);
        BMS_SendData(payload, 3, 0);
        BMS_ShortDelay();
        BMS_ShortDelay();
        if (BMS_GetReply(buff, 8) > 4) {
            if (buff[0] == (x << 1) && buff[1] == 0 && buff[2] == 1 && buff[4] > 0)
                return x;
        }
        BMS_ShortDelay();
    }
    
    return 0;
}

bool ResetBoard() {
    uint8_t payload[3];
    uint8_t buff[8];
    
    for (int i = 0; i < 8; i++) {
        buff[i] = 0;
    }

    for (int attempts = 0; attempts < MAX_ATTEMPTS; attempts++) {
        payload[0] = 0x3F << 1; // broadcast the reset command
        payload[1] = REG_RESET; // reset
        payload[2] = RESET_MAGIC_CODE; // data to cause a reset
        
        BMS_SendData(payload, 3, 1);
        BMS_LongDelay();
        BMS_GetReply(buff, 8);
        if (buff[0] == 0x7F && buff[1] == 0x3C && buff[2] == 0xA5 && buff[3] == 0x57) return 1;
       
        BMS_LongDelay();
    }
    return 0;
}

bool SetNewBoardId(uint8_t id) {
    uint8_t payload[3];
    uint8_t buff[10];
    int retLen;

    payload[0] = 0;
    payload[1] = 0;
    payload[2] = 1;

    payload[0] = 0;
    payload[1] = 0;
    payload[2] = 1;
    retLen = BMS_SendDataWithReply(payload, 3, 0, buff, 4);
    if (retLen == 4) {
        if (buff[0] == 0x80 && buff[1] == 0 && buff[2] == 1) {
            payload[0] = 0;
            payload[1] = REG_ADDR_CTRL;
            payload[2] = id | 0x80;
            BMS_SendData(payload, 3, 1);
            BMS_LongDelay();
            if (BMS_GetReply(buff, 10) > 2) {
                if (buff[0] == (0x81) && buff[1] == REG_ADDR_CTRL && buff[2] == (id + 0x80))
                    return 1;
            }
        }
    }
    return 0;
}

static uint16_t CalcTemp(uint8_t buff[], int index) {
    float tempCalc;
    float tempTemp;

    tempTemp = (1.78f / ((buff[index] * 256 + buff[index + 1] + 2) / 33046.0f) - 3.57f);
    tempTemp *= 1000.0f;
    tempCalc =  (float)(1.0f / (0.0007610373573f + (0.0002728524832 * logf(tempTemp)) + (powf(logf(tempTemp), 3) * 0.0000001022822735f)));
    tempCalc = tempCalc - 273.15f;
    if (tempCalc < 0) tempCalc = 0;
    if (tempCalc > 99) tempCalc = 99;
    return (uint16_t)(tempCalc * 10.0f);
}

struct BmsData ReadBmsData(uint8_t moduleId) {
    struct BmsData data;
    uint8_t payload[4];
    uint8_t buff[50];
    uint8_t calcCRC;
    int retLen;
    float cellVolt;
    
    data.mv = 0;
    for (int i = 0; i < 6; i++) data.v[i] = 0;
    for (int i = 0; i < 2; i++) data.t[i] = 0;
    
    payload[0] = (uint8_t)(moduleId << 1);
    
    payload[1] = REG_ADC_CTRL;
    payload[2] = 0b00111101; // ADC Auto mode, read every ADC input we can (Both Temps, Pack, 6 cells)
    BMS_SendDataWithReply(payload, 3, 1, buff, 3);
 
    payload[1] = REG_IO_CTRL;
    payload[2] = 0b00000011; // enable temperature measurement VSS pins
    BMS_SendDataWithReply(payload, 3, 1, buff, 3);
            
    payload[1] = REG_ADC_CONV; // start all ADC conversions
    payload[2] = 1;
    BMS_SendDataWithReply(payload, 3, 1, buff, 3);
                
    payload[1] = REG_GPAI; // start reading registers at the module voltage registers
    payload[2] = 0x12;     // read 18 bytes (Each value takes 2 - ModuleV, CellV1-6, Temp1, Temp2)
    retLen = BMS_SendDataWithReply(payload, 3, 0, buff, 22);
            
    calcCRC = BMS_GenCRC(buff, retLen - 1);

    // 18 data bytes, address, command, length, and CRC = 22 bytes returned
    // Also validate CRC to ensure we didn't get garbage data.
    if ((retLen == 22) && (buff[21] == calcCRC)) {
        if (buff[0] == (moduleId << 1) && buff[1] == REG_GPAI && buff[2] == 0x12) { // Also ensure this is actually the reply to our intended query
            
            // payload is 2 bytes QPAI, 2 bytes for each of 6 cell voltages, 2 bytes for each of two temperatures (18 bytes of data)
            data.mv = (uint16_t)((buff[3] * 256 + buff[4]) * 0.002034609f * 1000.0f);
            for (int i = 0; i < 6; i++) {
                cellVolt = (buff[5 + (i * 2)] * 256 + buff[6 + (i * 2)]) * 0.000381493f;
                data.v[i] = (uint16_t)(cellVolt * 1000.0f);
            }
            data.t[0] = CalcTemp(buff, 17);
            data.t[1] = CalcTemp(buff, 19);
        }        
    }
     
    return data;
}
