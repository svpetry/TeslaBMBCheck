/* 
 * File:   board.h
 * Author: svpet
 *
 * Created on July 12, 2025, 8:44 AM
 */

#ifndef BOARD_H
#define	BOARD_H

void WaitButtonsReleased(void);
void WaitForButtonPressed(void);
bool GetBtnState(uint8_t id);
void SetInfoLed(bool state);
void SetLcdBacklight(bool state);
void SetBmsPower(bool state);
bool BmsFaultActive(void);
void SetChargeRelais(bool state);

#endif	/* BOARD_H */
