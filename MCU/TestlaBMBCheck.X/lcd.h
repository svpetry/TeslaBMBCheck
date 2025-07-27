/* 
 * File:   lcd.h
 * Author: svpet
 *
 * Created on June 12, 2025, 6:32 AM
 */

#ifndef LCD_H
#define	LCD_H

extern void LcdClear(void);
extern void LcdInit(void);
extern void LcdPuts(uint8_t col, uint8_t row, const char *str);
extern void LcdPutChar(uint8_t col, uint8_t row, char ch);

#endif	/* LCD_H */

