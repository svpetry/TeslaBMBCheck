#include <pic18.h>
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
        BMS_WaitForData(100);
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
        BMS_WaitForData(100);
        BMS_GetReply(buff, 8);
        if (buff[0] == 0x7F && buff[1] == 0x3C && buff[2] == 0xA5 && buff[3] == 0x57) return 1;

        LcdClear();
        char s[8];
        HexStr(buff[0], s);
        LcdPuts(0, 2, s);
        HexStr(buff[1], s);
        LcdPuts(4, 2, s);
        HexStr(buff[2], s);
        LcdPuts(8, 2, s);
        HexStr(buff[3], s);
        LcdPuts(12, 2, s);
        
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
            BMS_WaitForData(100);
            if (BMS_GetReply(buff, 10) > 2) {
                if (buff[0] == (0x81) && buff[1] == REG_ADDR_CTRL && buff[2] == (id + 0x80))
                    return 1;
            }
        }
    }
    return 0;
}