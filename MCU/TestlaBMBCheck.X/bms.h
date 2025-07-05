/* 
 * File:   bms.h
 * Author: svpet
 *
 * Created on June 14, 2025, 7:50 AM
 */

#ifndef BMS_H
#define	BMS_H

#include "types.h"

#define MAX_MODULE_ADDR 63
#define MAX_ATTEMPTS 1000

uint8_t FindBoardId(void);
bool ResetBoard(void);
bool SetNewBoardId(uint8_t id);
struct BmsData ReadBmsData(uint8_t moduleId);

#endif	/* BMS_H */

