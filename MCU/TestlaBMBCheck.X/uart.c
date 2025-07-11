#include <xc.h>
#include <pic18.h>
#include <pic18f2480.h>
#include "base.h"

#include "uart.h"
#include "utils.h"

#define UART_BUFFER_SIZE 32

volatile uint8_t rx_buffer[UART_BUFFER_SIZE];
volatile uint8_t rx_head = 0, rx_tail = 0;

static inline uint8_t rx_count(void) {
    return (rx_head + UART_BUFFER_SIZE - rx_tail) % UART_BUFFER_SIZE;
}

void __interrupt() ISR(void) {
    // RX interrupt
    if (PIR1bits.RCIF && PIE1bits.RCIE) {

        // read received byte
        uint8_t c = RCREG;
        // clear overrun if needed
        if (RCSTAbits.OERR) {
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
        }
        // store in buffer if space remains
        uint8_t next = (rx_head + 1) % UART_BUFFER_SIZE;
        if (next != rx_tail) {
            rx_buffer[rx_head] = c;
            rx_head = next;
        }
        // else: buffer full, drop byte
        PIR1bits.RCIF = 0;
    }
}

void UART_Init(uint32_t baudrate) {
    // disable analog on all pins
    ADCON1 = 0x0F;
    
    // 16-bit BRG, high-speed mode
    BAUDCONbits.BRG16 = 1;
    TXSTAbits.BRGH   = 1;
    {
        unsigned long brg = (_XTAL_FREQ / (4UL * baudrate)) - 1;
        SPBRGH = (brg >> 8) & 0xFF;
        SPBRG  = brg        & 0xFF;
    }

    // enable UART pins
    RCSTAbits.SPEN = 1;
    // enable TX and RX
    TXSTAbits.TXEN = 1;
    RCSTAbits.CREN = 1;

    // clear flags
    PIR1bits.RCIF = 0;
    PIR1bits.TXIF = 0;

    // enable USART interrupts
    PIE1bits.RCIE = 1;  // receive interrupt
    PIE1bits.TXIE = 0;  // we'll turn TXIE on when data is waiting

    // global & peripheral interrupts
    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;
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
    return rx_count() > 0;
}

uint8_t UART_Read(void) {
    // wait for data
    while (rx_count() == 0);

    // disable interrupts while modifying tail
    INTCONbits.GIE = 0;
    uint8_t data = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % UART_BUFFER_SIZE;
    INTCONbits.GIE = 1;
    return data;
}
