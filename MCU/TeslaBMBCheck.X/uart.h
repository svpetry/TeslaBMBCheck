/* 
 * File:   uart.h
 * Author: svpet
 *
 * Created on June 12, 2025, 7:23 PM
 */

#ifndef UART_H
#define	UART_H

#include "types.h"

void UART_Init(uint32_t baudrate);
void UART_Write(uint8_t data);
void UART_WriteArray(uint8_t *data, uint16_t length);
bool UART_DataAvailable(void);
uint8_t UART_Read(void);

#endif	/* UART_H */

