#include <xc.h>
#include <pic18.h>
#include <pic18f2525.h>
#include "base.h"

#include "uart.h"
#include "utils.h"

void UART_Init(uint32_t baudrate) {
    // 1) Disable analog functionality on UART pins
    ADCON1 = 0x0F;          // All ports digital (PCFG<3:0>=1111)
    
    // 2) Baud-rate generator setup
    BAUDCONbits.BRG16 = 1;  // 16-bit Baud Rate Generator
    TXSTAbits.BRGH   = 1;   // High-speed mode
    
    // calculate and load SPBRG16:SPBRG = (_XTAL_FREQ/(4*BAUD)) - 1
    {
        uint32_t brg = (_XTAL_FREQ / (4UL * baudrate)) - 1;
        SPBRGH = (brg >> 8) & 0xFF;
        SPBRG  = brg        & 0xFF;
    }
    
    // 3) Enable the UART module
    RCSTAbits.SPEN = 1;     // Enable serial port (configures TX/RX pins)
    
    // 4) Enable transmitter
    TXSTAbits.TXEN = 1;     // Enable TX
    
    // 5) Enable continuous receiver
    RCSTAbits.CREN = 1;     // Enable RX
    
    // clear interrupt flags (if using interrupts)
    PIR1bits.TXIF = 0;
    PIR1bits.RCIF = 0;
}

void UART_Write(uint8_t data) {
    while (!PIR1bits.TXIF) ;  // Wait until TXREG is empty
    TXREG = data;             // Send byte
}

void UART_WriteArray(uint8_t *data, uint16_t length) {
    while (length > 0) {
        UART_Write(*data);
        data++;
        length--;
    }
}

bool UART_DataAvailable() {
    return PIR1bits.RCIF;
}

uint8_t UART_Read() {
    while (!PIR1bits.RCIF) ;  // Wait for data
/*
    // Handle overrun
    if (RCSTAbits.OERR) {
        RCSTAbits.CREN = 0;   // Reset receiver
        RCSTAbits.CREN = 1;
    }
 */
    uint8_t data = RCREG; // Read received byte

    return data;
}
