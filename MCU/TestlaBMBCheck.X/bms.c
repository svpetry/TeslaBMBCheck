#include <pic18.h>
#include "base.h"

#include "uart.h"
#include "bms.h"
#include "bms-util.h"
#include "lcd.h"
#include "utils.h"

#define MAX_MODULE_ADDR 63
#define MAX_ATTEMPTS 5

static const uint16_t temp_table[1024] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
    59, 60, 61, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 73, 74, 75,
    76, 77, 78, 79, 80, 81, 81, 82, 83, 84, 85, 86, 87, 88, 89, 89, 90, 91, 92,
    93, 94, 95, 95, 96, 97, 98, 99, 100, 101, 101, 102, 103, 104, 105, 106, 107,
    107, 108, 109, 110, 111, 112, 112, 113, 114, 115, 116, 117, 117, 118, 119,
    120, 121, 121, 122, 123, 124, 125, 126, 126, 127, 128, 129, 130, 130, 131,
    132, 133, 134, 134, 135, 136, 137, 138, 138, 139, 140, 141, 141, 142, 143,
    144, 145, 145, 146, 147, 148, 148, 149, 150, 151, 152, 152, 153, 154, 155,
    155, 156, 157, 158, 158, 159, 160, 161, 162, 162, 163, 164, 165, 165, 166,
    167, 168, 168, 169, 170, 171, 171, 172, 173, 173, 174, 175, 176, 176, 177,
    178, 179, 179, 180, 181, 182, 182, 183, 184, 185, 185, 186, 187, 187, 188,
    189, 190, 190, 191, 192, 192, 193, 194, 195, 195, 196, 197, 197, 198, 199,
    200, 200, 201, 202, 202, 203, 204, 205, 205, 206, 207, 207, 208, 209, 210,
    210, 211, 212, 212, 213, 214, 214, 215, 216, 216, 217, 218, 219, 219, 220,
    221, 221, 222, 223, 223, 224, 225, 225, 226, 227, 228, 228, 229, 230, 230,
    231, 232, 232, 233, 234, 234, 235, 236, 236, 237, 238, 238, 239, 240, 240,
    241, 242, 242, 243, 244, 244, 245, 246, 246, 247, 248, 248, 249, 250, 250,
    251, 252, 252, 253, 254, 254, 255, 256, 256, 257, 258, 258, 259, 260, 260,
    261, 262, 262, 263, 264, 264, 265, 266, 266, 267, 268, 268, 269, 270, 270,
    271, 271, 272, 273, 273, 274, 275, 275, 276, 277, 277, 278, 279, 279, 280,
    281, 281, 282, 282, 283, 284, 284, 285, 286, 286, 287, 288, 288, 289, 289,
    290, 291, 291, 292, 293, 293, 294, 295, 295, 296, 296, 297, 298, 298, 299,
    300, 300, 301, 302, 302, 303, 303, 304, 305, 305, 306, 307, 307, 308, 308,
    309, 310, 310, 311, 312, 312, 313, 313, 314, 315, 315, 316, 317, 317, 318,
    318, 319, 320, 320, 321, 322, 322, 323, 323, 324, 325, 325, 326, 327, 327,
    328, 328, 329, 330, 330, 331, 331, 332, 333, 333, 334, 335, 335, 336, 336,
    337, 338, 338, 339, 339, 340, 341, 341, 342, 343, 343, 344, 344, 345, 346,
    346, 347, 347, 348, 349, 349, 350, 350, 351, 352, 352, 353, 354, 354, 355,
    355, 356, 357, 357, 358, 358, 359, 360, 360, 361, 361, 362, 363, 363, 364,
    364, 365, 366, 366, 367, 367, 368, 369, 369, 370, 370, 371, 372, 372, 373,
    374, 374, 375, 375, 376, 377, 377, 378, 378, 379, 380, 380, 381, 381, 382,
    383, 383, 384, 384, 385, 386, 386, 387, 387, 388, 389, 389, 390, 390, 391,
    392, 392, 393, 393, 394, 395, 395, 396, 396, 397, 398, 398, 399, 399, 400,
    401, 401, 402, 402, 403, 404, 404, 405, 405, 406, 407, 407, 408, 408, 409,
    409, 410, 411, 411, 412, 412, 413, 414, 414, 415, 415, 416, 417, 417, 418,
    418, 419, 420, 420, 421, 421, 422, 423, 423, 424, 424, 425, 426, 426, 427,
    427, 428, 429, 429, 430, 430, 431, 432, 432, 433, 433, 434, 435, 435, 436,
    436, 437, 437, 438, 439, 439, 440, 440, 441, 442, 442, 443, 443, 444, 445,
    445, 446, 446, 447, 448, 448, 449, 449, 450, 451, 451, 452, 452, 453, 454,
    454, 455, 455, 456, 457, 457, 458, 458, 459, 459, 460, 461, 461, 462, 462,
    463, 464, 464, 465, 465, 466, 467, 467, 468, 468, 469, 470, 470, 471, 471,
    472, 473, 473, 474, 474, 475, 476, 476, 477, 477, 478, 479, 479, 480, 480,
    481, 482, 482, 483, 483, 484, 485, 485, 486, 486, 487, 488, 488, 489, 489,
    490, 491, 491, 492, 492, 493, 494, 494, 495, 495, 496, 497, 497, 498, 498,
    499, 500, 500, 501, 501, 502, 503, 503, 504, 504, 505, 506, 506, 507, 507,
    508, 509, 509, 510, 510, 511, 512, 512, 513, 513, 514, 515, 515, 516, 516,
    517, 518, 518, 519, 519, 520, 521, 521, 522, 522, 523, 524, 524, 525, 525,
    526, 527, 527, 528, 528, 529, 530, 530, 531, 532, 532, 533, 533, 534, 535,
    535, 536, 536, 537, 538, 538, 539, 539, 540, 541, 541, 542, 542, 543, 544
};

uint8_t FindBoardId() {
    uint8_t payload[3];
    uint8_t buff[8];

    payload[0] = 0;
    payload[1] = 0; // read registers starting at 0
    payload[2] = 1; // read one byte
    for (uint8_t x = 1; x <= MAX_MODULE_ADDR; x++) {
        payload[0] = (uint8_t)(x << 1);
        BMS_SendData(payload, 3, 0);
        BMS_ShortDelay();
        BMS_ShortDelay();
        if (BMS_GetReply(buff, 8) > 4) {
            if (buff[0] == (x << 1) && buff[1] == 0 && buff[2] == 1 && buff[4] > 0)
                return x;
        }
        BMS_ShortDelay();
    }
    
    return 0;
}

bool ResetBoard() {
    uint8_t payload[3];
    uint8_t buff[8];
    
    for (int i = 0; i < 8; i++) {
        buff[i] = 0;
    }

    for (int attempts = 0; attempts < MAX_ATTEMPTS; attempts++) {
        payload[0] = 0x3F << 1; // broadcast the reset command
        payload[1] = REG_RESET; // reset
        payload[2] = RESET_MAGIC_CODE; // data to cause a reset
        
        BMS_SendData(payload, 3, 1);
        BMS_LongDelay();
        BMS_GetReply(buff, 8);
        if (buff[0] == 0x7F && buff[1] == 0x3C && buff[2] == 0xA5 && buff[3] == 0x57) return 1;
       
        BMS_LongDelay();
    }
    return 0;
}

bool SetNewBoardId(uint8_t id) {
    uint8_t payload[3];
    uint8_t buff[10];
    int retLen;

    payload[0] = 0;
    payload[1] = 0;
    payload[2] = 1;

    retLen = BMS_SendDataWithReply(payload, 3, 0, buff, 4);
    if (retLen == 4) {
        if (buff[0] == 0x80 && buff[1] == 0 && buff[2] == 1) {
            payload[0] = 0;
            payload[1] = REG_ADDR_CTRL;
            payload[2] = id | 0x80;
            BMS_SendData(payload, 3, 1);
            BMS_LongDelay();
            if (BMS_GetReply(buff, 10) > 2) {
                if (buff[0] == (0x81) && buff[1] == REG_ADDR_CTRL && buff[2] == (id + 0x80))
                    return 1;
            }
        }
    }
    return 0;
}

/*
 * Old method with Steinhart-Hart formula.
 * 
static uint16_t CalcTemp(uint8_t buff[], int index) {
    float tempCalc;
    float tempTemp;

    tempTemp = (1.78f / ((buff[index] * 256 + buff[index + 1] + 2) / 33046.0f) - 3.57f);
    tempTemp *= 1000.0f;
    tempCalc =  (float)(1.0f / (0.0007610373573f + (0.0002728524832 * logf(tempTemp)) + (powf(logf(tempTemp), 3) * 0.0000001022822735f)));
    tempCalc = tempCalc - 273.15f;
    if (tempCalc < 0) tempCalc = 0;
    if (tempCalc > 99) tempCalc = 99;
    return (uint16_t)(tempCalc * 10.0f);
}
*/

static uint16_t CalcTemp(uint8_t buff[], int index) {
    uint16_t value = buff[index] * 256 + buff[index + 1];
    uint16_t table_idx = value >> 3;
    if (table_idx > 1022)
        return 999;
    
    // interpolation (not really necessary...)
    uint16_t v1 = temp_table[table_idx];
    uint16_t v2 = temp_table[table_idx + 1];
    return v1 + (((v2 - v1) * (value & 0x07)) >> 3);
}
 
struct BmsData ReadBmsData(uint8_t module_id) {
    struct BmsData data = {0};
    uint8_t payload[4];
    uint8_t buff[50];
    uint8_t calcCRC;
    int retLen;
    uint16_t cellVolt;
    
    payload[0] = (uint8_t)(module_id << 1);
    
    payload[1] = REG_ADC_CTRL;
    payload[2] = 0b00111101; // ADC Auto mode, read every ADC input we can (Both Temps, Pack, 6 cells)
    BMS_SendDataWithReply(payload, 3, 1, buff, 3);
 
    payload[1] = REG_IO_CTRL;
    payload[2] = 0b00000011; // enable temperature measurement VSS pins
    BMS_SendDataWithReply(payload, 3, 1, buff, 3);
            
    payload[1] = REG_ADC_CONV; // start all ADC conversions
    payload[2] = 1;
    BMS_SendDataWithReply(payload, 3, 1, buff, 3);
                
    payload[1] = REG_GPAI; // start reading registers at the module voltage registers
    payload[2] = 0x12;     // read 18 bytes (Each value takes 2 - ModuleV, CellV1-6, Temp1, Temp2)
    retLen = BMS_SendDataWithReply(payload, 3, 0, buff, 22);
            
    calcCRC = BMS_GenCRC(buff, retLen - 1);

    // 18 data bytes, address, command, length, and CRC = 22 bytes returned
    // Also validate CRC to ensure we didn't get garbage data.
    if ((retLen == 22) && (buff[21] == calcCRC)) {
        if (buff[0] == (module_id << 1) && buff[1] == REG_GPAI && buff[2] == 0x12) { // Also ensure this is actually the reply to our intended query
            
            // payload is 2 bytes QPAI, 2 bytes for each of 6 cell voltages, 2 bytes for each of two temperatures (18 bytes of data)
            cellVolt = (uint16_t)buff[3] * 256 + (uint16_t)buff[4];
            data.mv = (uint16_t)(((uint32_t)cellVolt * 133340U) >> 16); // * 2.034609f
            for (int i = 0; i < 6; i++) {
                cellVolt = (uint16_t)buff[5 + (i * 2)] * 256 + (uint16_t)buff[6 + (i * 2)];
                data.v[i] = (uint16_t)(((uint32_t)cellVolt * 50003U) >> 17); // * 0.381493f
            }
            data.t[0] = CalcTemp(buff, 17);
            data.t[1] = CalcTemp(buff, 19);
        }        
    }
  
    return data;
}

void EnableBmsBalancers(uint8_t module_id, uint8_t balancer_bits) {
    uint8_t payload[4];
    uint8_t buff[30];

    payload[0] = (uint8_t)(module_id << 1);
    payload[1] = REG_BAL_TIME;
    payload[2] = 60; // 60 second balance limit, if not triggered to balance it will stop after 5 seconds
    BMS_SendData(payload, 3, 1);
    BMS_ShortDelay();
    BMS_GetReply(buff, 30);

    payload[0] = (uint8_t)(module_id << 1);
    payload[1] = REG_BAL_CTRL;
    payload[2] = balancer_bits; // write balance state to register
    BMS_SendData(payload, 3, 1);
    BMS_ShortDelay();
    BMS_GetReply(buff, 30);
}

void ClearBmsFaults(uint8_t module_id) {
    uint8_t payload[3];
    uint8_t buff[8];
    
    payload[0] = (uint8_t)(module_id << 1);
    payload[1] = REG_ALERT_STATUS; // Alert Status
    payload[2] = 0xFF; // data to cause a reset
    BMS_SendDataWithReply(payload, 3, 1, buff, 4);
    
    payload[0] = (uint8_t)(module_id << 1);
    payload[2] = 0x00; // data to clear
    BMS_SendDataWithReply(payload, 3, 1, buff, 4);
  
    payload[0] = (uint8_t)(module_id << 1);
    payload[1] = REG_FAULT_STATUS; // Fault Status
    payload[2] = 0xFF; // data to cause a reset
    BMS_SendDataWithReply(payload, 3, 1, buff, 4);
    
    payload[0] = (uint8_t)(module_id << 1);
    payload[2] = 0x00; // data to clear
    BMS_SendDataWithReply(payload, 3, 1, buff, 4);
}
