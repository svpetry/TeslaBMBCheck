#include "utils.h"
#include <pic18.h>

void VoltageToStr(char *s, uint16_t voltage) {
    *s++ = '0' + (char)(voltage / 1000);
    voltage %= 1000;
    *s++ = '.';
    *s++ = '0' + (char)(voltage / 100);
    voltage %= 100;
    *s++ = '0' + (char)(voltage / 10);
    voltage %= 10;
    *s++ = '0' + (char)voltage;
    *s = 0;
}

void TempToStr(char *s, uint16_t temp) {
    *s++ = '0' + (char)(temp / 100);
    temp %= 100;
    *s++ = '0' + (char)(temp / 10);
    temp %= 10;
    *s++ = '.';
    *s++ = '0' + (char)temp;
    *s = 0;
}

/*
char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void HexStr(uint8_t value, char *str) {
    *(str++) = hex[value >> 4];
    *(str++) = hex[value & 0x0F];
    *str = 0;
}
*/

void Halt(void) {
    while (1) ;
}
