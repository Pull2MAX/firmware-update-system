#ifndef _MAIN_H
#define _MAIN_H

#include "stdint.h" // To allow this file to recognize uint8_t

#define GD32_FLASH_START_ADDR           0x08000000              // Starting address of flash
#define GD32_ONE_PAGE_BYTESIZE          1024                    // The size of one page in flash is 1KB
#define GD32_PAGE_B_AREA_NUM            20                      // Number of pages that should be allocated for area B
#define GD32_PAGE_TOTAL_NUM             64                      // Total number of pages
#define GD32_PAGE_A_AREA_NUM            GD32_PAGE_TOTAL_NUM - GD32_PAGE_B_AREA_NUM  // Number of pages that should be allocated for area A
#define GD32_A_AREA_START_PAGE          GD32_PAGE_B_AREA_NUM    // Starting page number for area A == number of pages in area B
#define GD32_A_AREA_START_ADDR          GD32_FLASH_START_ADDR + GD32_PAGE_B_AREA_NUM * GD32_ONE_PAGE_BYTESIZE // Starting address for area A

#define OTA_UPDATE_FLAG                    0x66666666

#define MQTT_CONNECT_OK_FLAG                0x00000001      // Flag indicating successful connection to MQTT
#define DOWNLOAD_BIN_FILE_FLAG              0x00000002      // Flag indicating the download of the bin file

typedef struct 
{
    uint32_t ota_flag;
    uint32_t FireLen[11];    // There may be many app files stored in W25Q64, the length of this array is chosen for convenience as the page of 24C02 is 16 bytes for read/write
    uint8_t OTA_VERSION[32];  // Store version detail information, 32 bytes mainly to ensure it is a multiple of 16
} OTA_CONTROL_INFO; // This structure contains content stored in 24C02

#define OTA_CONTROL_INFO_SIZE sizeof(OTA_CONTROL_INFO)

typedef struct
{
    uint8_t updata_buffer[GD32_ONE_PAGE_BYTESIZE];
    uint32_t W25Q64_BLOCK_NUMBER; // One block of W25Q64 is 64KB, this member indicates which external flash block to download
    uint32_t Xmodem_Timer;
    uint32_t Xmodem_Number; // One Xmodem actual data part is 128 bytes, this member records how many Xmodem packets have been sent
    uint32_t Xmodem_CRC_Result; // This member records the CRC check result calculated by Xmodem itself
} UPDATA_A_CONTROL_INFO;

extern OTA_CONTROL_INFO ota_info;
extern UPDATA_A_CONTROL_INFO updataA;
extern uint32_t BootStatusFlag;

#endif
