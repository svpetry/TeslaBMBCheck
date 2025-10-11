/* 
 * File:   types.h
 * Author: svpet
 *
 * Created on June 14, 2025, 7:45 AM
 */

#ifndef TYPES_H
#define	TYPES_H

typedef char bool;

struct BmsData {
    uint16_t v[6]; // Cell voltage in mV
    uint16_t t[2]; // Temperature * 10 in degC
    uint16_t mv;   // Module voltage in mV
    uint16_t v_min;
    uint16_t v_max;
};

#endif	/* TYPES_H */

