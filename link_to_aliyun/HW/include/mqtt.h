#ifndef _MQTT_H
#define _MQTT_H

#include "stdint.h"

#define CLINET_ID "hh6zPbXNLbr.D001|securemode=2,signmethod=hmacsha256,timestamp=16819955619281"
#define USERNAME  "D001&hh6zPbXNLbr"
#define PASSWORD  "01c2e6596e60432c4c7b1eb87393393aafbd91b74951b7981c593f4a6710830a"

typedef struct
{
    uint8_t Pack_buff[512]; // This stores a complete message
    // The following are the components of the message
    uint16_t MessgeID;
    uint16_t Fix_len; // Fixed length of the MQTT message
    uint16_t Variable_len; // Variable part of the MQTT message
    uint16_t Payload_len; // Payload length
    uint16_t Remaining_len; // Remaining length = variable + payload
    uint8_t CMD_Buff[512]; // Temporarily stores the content sent by the server for later parsing

    int size; // Counts how many bytes the firmware on the server has
    int streamId;
    uint8_t OTA_tempver[26];

    int bin_total_download_counter; // Counts how many times this file needs to be downloaded
    int now_download_counter; // Counts how many times it has been downloaded so far
    int now_download_size; // Counts how many bytes need to be downloaded currently

}MQTT_CONTROL_BLOCK;

void MQTT_ConnectPack(void);
void MQTT_SubscribePack(char *topic);
void MQTT_DealPack(uint8_t *data, uint16_t len);
void MQTT_ResponseQs0Pack(char *topic, char *data);
void MQTT_ResponseQs1Pack(char *topic, char *data);
#endif
