/* 
 * File:   utils.h
 * Author: svpet
 *
 * Created on June 12, 2025, 6:24 PM
 */

#include <pic18.h>
#include "types.h"

#ifndef UTILS_H
#define	UTILS_H

void VoltageToStr(char *s, uint16_t voltage, bool decades);
void TempToStr(char *s, uint16_t temp);
void HexStr(uint8_t value, char *str);

#endif	/* UTILS_H */

