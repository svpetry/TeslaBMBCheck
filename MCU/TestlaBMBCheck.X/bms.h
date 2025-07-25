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
#define MAX_ATTEMPTS 5

uint8_t FindBoardId(void);
bool ResetBoard(void);
bool SetNewBoardId(uint8_t id);
struct BmsData ReadBmsData(uint8_t module_id);
void EnableBalancer(uint8_t module_id, uint8_t cell_id, bool state);
void EnableBmsBalancers(uint8_t module_id);

#endif	/* BMS_H */

