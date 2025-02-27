#ifndef _MAIN_H
#define _MAIN_H

#include "stdint.h" // To allow this file to recognize uint8_t


#define GD32_FLASH_START_ADDR           0x08000000              // Starting address of flash
#define GD32_ONE_PAGE_BYTESIZE          1024                    // The size of one page in flash is 1KB
#define GD32_PAGE_B_AREA_NUM            20                      // Number of pages allocated for area B
#define GD32_PAGE_TOTAL_NUM             64                      // Total number of pages
#define GD32_PAGE_A_AREA_NUM            GD32_PAGE_TOTAL_NUM - GD32_PAGE_B_AREA_NUM  // Number of pages allocated for area A
#define GD32_A_AREA_START_PAGE          GD32_PAGE_B_AREA_NUM    // Starting page number for area A == number of pages in area B
#define GD32_A_AREA_START_ADDR          GD32_FLASH_START_ADDR + GD32_PAGE_B_AREA_NUM * GD32_ONE_PAGE_BYTESIZE // Starting address for area A

#define OTA_UPDATE_FLAG                    0x66666666

#define UPDATA_A_FLAG                       0x00000001 // This flag indicates that the Boot status is preparing to update the app area
#define XMODEM_SEND_C_FLAG                  0x00000002 // This flag indicates that xmodem has sent the character 'C'====> for service of function 2
#define XMODEM_DATA_FLAG                    0x00000004 // This flag indicates that xmodem has sent data====> for service of function 2
#define SET_VERSION_FLAG                    0x00000008 // This flag indicates that the version number needs to be set====> for service of function 3
#define STORE_APP_TO_W25Q64_FLAG            0x00000010 // This flag indicates that the app firmware needs to be stored to w25q64====> implements function 5
#define XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG 0x00000020 // This flag indicates that xmodem sending data is for storing app firmware to w25q64====> implements function 5
#define USE_APP_IN_W25Q64_FLAG              0x00000040 // This flag indicates that the app firmware in w25q64 needs to be used====> implements function 6
#define SET_SERVER_INFO_FLAG                0x00000080 // This flag indicates that server connection information needs to be set====> implements function 8

typedef struct 
{
    uint32_t ota_flag;
    uint32_t FireLen[11];       // There may be many app files stored in w25q64, the length of this array is chosen for convenience of reading and writing 16 bytes in 24c02
    uint8_t OTA_VERSION[32];   // Store version detail information, 32 bytes mainly to ensure it is a multiple of 16
}OTA_CONTROL_INFO;             // The content of this structure is stored in 24c02

#define OTA_CONTROL_INFO_SIZE sizeof(OTA_CONTROL_INFO)

typedef struct
{
    uint8_t  updata_buffer[GD32_ONE_PAGE_BYTESIZE];
    uint32_t W25Q64_BLOCK_NUMBER; // One block of w25q64 is 64KB, this member indicates which external flash block to download
    uint32_t Xmodem_Timer;
    uint32_t Xmodem_Number; // One xmodem actual data part is 128 bytes, this member records how many xmodem packets have been sent
    uint32_t Xmodem_CRC_Result; // This member records the CRC check result calculated by xmodem
}UPDATA_A_CONTROL_INFO;

extern OTA_CONTROL_INFO ota_info;
extern UPDATA_A_CONTROL_INFO updataA;
extern uint32_t BootStatusFlag;

#endif
