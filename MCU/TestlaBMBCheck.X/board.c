#include <pic18.h>
#include "base.h"
#include "types.h"

#include "board.h"


void WaitButtonsReleased() {
    while (!PORTAbits.RA0 || !PORTAbits.RA1) ;
}

void WaitForButtonPressed() {
    while (PORTAbits.RA0 && PORTAbits.RA1) ;
}

bool GetBtnState(uint8_t id) {
    if (id == 0)
        return !PORTAbits.RA0;
    else
        return !PORTAbits.RA1;
}

void SetInfoLed(bool state) {
    LATCbits.LATC2 = state;
}

void SetLcdBacklight(bool state) {
    LATCbits.LATC1 = state;
}

void SetBmsPower(bool state) {
    LATCbits.LATC0 = state ? 0 : 1;
}

bool BmsFaultActive() {
    return !PORTAbits.RA2;
}

void SetChargeRelais(bool state) {
    LATCbits.LATC3 = state;
}
