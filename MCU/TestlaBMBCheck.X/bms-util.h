/* 
 * File:   bms-util.h
 * Author: svpet
 *
 * Created on June 14, 2025, 7:38 AM
 */

#ifndef BMS_UTIL_H
#define	BMS_UTIL_H

#include "types.h"

#define REG_DEV_STATUS      0
#define REG_GPAI            1
#define REG_VCELL1          3
#define REG_VCELL2          5
#define REG_VCELL3          7
#define REG_VCELL4          9
#define REG_VCELL5          0xB
#define REG_VCELL6          0xD
#define REG_TEMPERATURE1    0xF
#define REG_TEMPERATURE2    0x11
#define REG_ALERT_STATUS    0x20
#define REG_FAULT_STATUS    0x21
#define REG_COV_FAULT       0x22
#define REG_CUV_FAULT       0x23
#define REG_ADC_CTRL        0x30
#define REG_IO_CTRL         0x31
#define REG_BAL_CTRL        0x32
#define REG_BAL_TIME        0x33
#define REG_ADC_CONV        0x34
#define REG_ADDR_CTRL       0x3B
#define REG_RESET           0x3C

#define RESET_MAGIC_CODE    0xA5

uint8_t BMS_GenCRC(uint8_t *input, int lenInput);
void BMS_ShortDelay(void);
void BMS_LongDelay(void);
void BMS_SendData(uint8_t *data, uint8_t dataLen, bool isWrite);
int BMS_GetReply(uint8_t *data, int maxLen);
int BMS_SendDataWithReply(uint8_t *data, uint8_t dataLen, bool isWrite, uint8_t *retData, int retLen);

#endif	/* BMS_UTIL_H */

