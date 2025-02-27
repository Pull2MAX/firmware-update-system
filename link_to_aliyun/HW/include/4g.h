#ifndef _4G_H
#define _4G_H

#include "stdint.h" // To allow this file to recognize uint8_t

void GM_4G_Init(void);
void U2_Event(uint8_t *data, uint16_t datalen);
void Report_OTA_Version(void);
void OTA_Download_by_Piece(int size, int offset);
#endif
