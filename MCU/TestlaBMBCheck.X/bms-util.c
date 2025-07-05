#include <pic18.h>
#include "base.h"
#include "types.h"
#include "uart.h"

static void BMS_Delay(int time) {
    int d = time / 10;
    for (int i = 0; i < d; i++) __delay_ms(10);
    d = time % 10;
    for (int i = 0; i < d; i++) __delay_ms(1);
}

uint8_t BMS_GenCRC(uint8_t *input, int lenInput) {
    uint8_t generator = 0x07;
    uint8_t crc = 0;

    for (int x = 0; x < lenInput; x++) {
        crc ^= input[x]; /* XOR-in the next input byte */

        for (int i = 0; i < 8; i++) {
            if ((crc & 0x80) != 0) {
                crc = (uint8_t)((crc << 1) ^ generator);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void BMS_ShortDelay(void) {
    __delay_ms(10);
}

void BMS_LongDelay(void) {
    __delay_ms(100);
}

void BMS_SendData(uint8_t *data, uint8_t dataLen, bool isWrite) {
    uint8_t orig = data[0];
    uint8_t addrByte = data[0];
    if (isWrite) addrByte |= 1;
    UART_Write(addrByte);
    UART_WriteArray(&data[1], dataLen - 1);  //assumes that there are at least 2 bytes sent every time. There should be, addr and cmd at the least.
    data[0] = addrByte;
    if (isWrite) UART_Write(BMS_GenCRC(data, dataLen));        
    data[0] = orig;
}

int BMS_GetReply(uint8_t *data, int maxLen) { 
    int numBytes = 0; 
    while (UART_DataAvailable() && numBytes < maxLen) {
        data[numBytes] = UART_Read();
        numBytes++;
    }
    if (maxLen == numBytes) {
        while (UART_DataAvailable()) UART_Read();
    }
    return numBytes;
}

// Uses above functions to send data then get the response. Will auto retry if response not 
// the expected return length. This helps to alleviate any comm issues. The Due cannot exactly
// match the correct comm speed so sometimes there are data glitches.
int BMS_SendDataWithReply(uint8_t *data, uint8_t dataLen, bool isWrite, uint8_t *retData, int retLen) {
    int attempts = 1;
    int returnedLength;
    while (attempts < 4) {
        BMS_SendData(data, dataLen, isWrite);
        BMS_Delay(2 * ((retLen / 8) + 1));
        returnedLength = BMS_GetReply(retData, retLen);
        if (returnedLength == retLen) return returnedLength;
        attempts++;
    }
    return returnedLength; //failed to get a proper response.
}
