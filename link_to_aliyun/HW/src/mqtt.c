#include "mqtt.h"
#include "gd32f10x.h"
#include "main.h"
#include "4g.h"
#include "delay.h"
#include "usart.h"
#include "w25q64.h"
#include "m24c02.h"

MQTT_CONTROL_BLOCK Aliyun_mqtt;

/*
    The packet for connecting to Aliyun, the whole process is just filling the pack_buff
*/
void MQTT_ConnectPack(void)
{
    /*----- Describe the byte count of each part ------*/
    Aliyun_mqtt.MessgeID = 1; // This is the message sequence number sent to the server
    Aliyun_mqtt.Fix_len = 1; // From index 1, the remaining length occupies how many bytes, at most 4 bytes
    Aliyun_mqtt.Variable_len = 10; // This is fixed, according to the data manual
    Aliyun_mqtt.Payload_len = 2 + strlen(CLINET_ID) + 2 + strlen(USERNAME) + 2 + strlen(PASSWORD); // This is the requirement of the connect packet, it must include the client id, username, and password, each preceded by a 2-byte length indicator 
    Aliyun_mqtt.Remaining_len = Aliyun_mqtt.Variable_len + Aliyun_mqtt.Payload_len;
    /*------ Fixed part ------*/
    Aliyun_mqtt.Pack_buff[0] = 0x10;

    // This is the key calculation, calculating how many bytes are needed to represent the remaining length
    do {
        if(Aliyun_mqtt.Remaining_len / 128 == 0) // Within 128 bytes, it can be represented with 1 byte
        {
            Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len] = Aliyun_mqtt.Remaining_len;
        }
        else // Beyond 128 bytes, it needs 2-4 bytes to represent the remaining length
        {
            Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len] = (Aliyun_mqtt.Remaining_len % 128) | 0x80; // Write the remaining length and carry to index 1
        }
        Aliyun_mqtt.Fix_len++;
        Aliyun_mqtt.Remaining_len = Aliyun_mqtt.Remaining_len / 128;
    } while(Aliyun_mqtt.Remaining_len);

    /*------ Variable part (strictly follow the MQTT manual) ------*/
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 0] = 0x00; // Protocol length msb
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 1] = 0x04; // Protocol length lsb
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 2] = 0x4d; // 'M'
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 3] = 0x51; // 'Q'
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 4] = 0x54; // 'T'
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 5] = 0x54; // 'T'
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 6] = 0x04; // Protocol level
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 7] = 0xC2; // Connection flags
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 8] = 0x00; // Reserved bit
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 9] = 0x64; // Keep alive time

    /*----- Payload part -----*/
    /* Client id length and storage of the actual client id */
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 10] = strlen(CLINET_ID) / 256; // Client id length msb
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 11] = strlen(CLINET_ID) % 256; // Client id length lsb
    memcpy(&Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 12], CLINET_ID, strlen(CLINET_ID)); // The actual CLINET_ID needs to be copied into pack_buff

    /* Username length and storage of the actual username */
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 12 + strlen(CLINET_ID)] = strlen(USERNAME) / 256; // Username id length msb
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 13 + strlen(CLINET_ID)] = strlen(USERNAME) % 256; // Username id length lsb
    memcpy(&Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 14 + strlen(CLINET_ID)], USERNAME, strlen(USERNAME)); // The actual USERNAME needs to be copied into pack_buff

    /* Password length and storage of the actual password */
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 14 + strlen(CLINET_ID) + strlen(USERNAME)] = strlen(PASSWORD) / 256; // Password id length msb
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 15 + strlen(CLINET_ID) + strlen(USERNAME)] = strlen(PASSWORD) % 256; // Password id length lsb
    memcpy(&Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 16 + strlen(CLINET_ID) + strlen(USERNAME)], PASSWORD, strlen(PASSWORD)); // The actual PASSWORD needs to be copied into pack_buff

    U2_SendData(Aliyun_mqtt.Pack_buff, Aliyun_mqtt.Remaining_len + Aliyun_mqtt.Fix_len); // Store the connection packet into the send buffer, then send it in the main function's while loop
}

void MQTT_SubscribePack(char *topic) // Subscribe to topic
{
    Aliyun_mqtt.Fix_len = 1;
    Aliyun_mqtt.Variable_len = 2;
    Aliyun_mqtt.Payload_len = 2 + strlen(topic) + 1;
    Aliyun_mqtt.Remaining_len = Aliyun_mqtt.Variable_len + Aliyun_mqtt.Payload_len;

    Aliyun_mqtt.Pack_buff[0] = 0b10000010;

    do {
        if(Aliyun_mqtt.Remaining_len / 128 == 0) // Within 128 bytes, it can be represented with 1 byte
        {
            Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len] = Aliyun_mqtt.Remaining_len;
        }
        else // Beyond 128 bytes, it needs 2-4 bytes to represent the remaining length
        {
            Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len] = (Aliyun_mqtt.Remaining_len % 128) | 0x80; // Write the remaining length and carry to index 1
        }
        Aliyun_mqtt.Fix_len++;
        Aliyun_mqtt.Remaining_len = Aliyun_mqtt.Remaining_len / 128;
    } while(Aliyun_mqtt.Remaining_len);

    /*------ Variable part (strictly follow the MQTT manual) ------*/
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 0] = Aliyun_mqtt.MessgeID / 256;
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 1] = Aliyun_mqtt.MessgeID % 256;
    Aliyun_mqtt.MessgeID++;

    /*------ Payload part ------*/
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 2] = strlen(topic) / 256;
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 3] = strlen(topic) % 256;
    memcpy(&Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 4], topic, strlen(topic));
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 4 + strlen(topic)] = 0;

    U2_SendData(Aliyun_mqtt.Pack_buff, Aliyun_mqtt.Remaining_len + Aliyun_mqtt.Fix_len);
}

void MQTT_DealPack(uint8_t *data, uint16_t data_len)
{
    uint8_t i;
    for(i = 1; i < 5; ++i) // The purpose of this for loop is to find the number of bytes occupied by the remaining length
    {
        if((data[i] & 0x80) == 0) break; // The position where i finally stops is the number of bytes occupied by the remaining length
    }
    memset(Aliyun_mqtt.CMD_Buff, 0, 512); // Clear CMD_Buff
    memcpy(Aliyun_mqtt.CMD_Buff, &data[1 + i + 2], data_len - 1 - i - 2); // Copy the truly useful part received into CMD_Buff
}

void MQTT_ResponseQs0Pack(char *topic, char *data)
{
    Aliyun_mqtt.Fix_len = 1;
    Aliyun_mqtt.Variable_len = 2 + strlen(topic);
    Aliyun_mqtt.Payload_len = strlen(data);
    Aliyun_mqtt.Remaining_len = Aliyun_mqtt.Variable_len + Aliyun_mqtt.Payload_len;

    Aliyun_mqtt.Pack_buff[0] = 0x30;
    do {
        if(Aliyun_mqtt.Remaining_len / 128 == 0)
        {
            Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len] = Aliyun_mqtt.Remaining_len;
        }
        else
        {
            Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len] = (Aliyun_mqtt.Remaining_len % 128) | 0x80;
        }
        Aliyun_mqtt.Fix_len++;
        Aliyun_mqtt.Remaining_len = Aliyun_mqtt.Remaining_len / 128;
    } while(Aliyun_mqtt.Remaining_len);

    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 0] = strlen(topic) / 256;
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 1] = strlen(topic) % 256;
    memcpy(&Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 2], topic, strlen(topic));
    memcpy(&Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 2 + strlen(topic)], data, strlen(data));

    U2_SendData(Aliyun_mqtt.Pack_buff, Aliyun_mqtt.Remaining_len + Aliyun_mqtt.Fix_len);
}

void MQTT_ResponseQs1Pack(char *topic, char *data)
{
    Aliyun_mqtt.Fix_len = 1;
    Aliyun_mqtt.Variable_len = 2 + 2 + strlen(topic);
    Aliyun_mqtt.Payload_len = strlen(data);
    Aliyun_mqtt.Remaining_len = Aliyun_mqtt.Variable_len + Aliyun_mqtt.Payload_len;

    Aliyun_mqtt.Pack_buff[0] = 0x32;
    do {
        if(Aliyun_mqtt.Remaining_len / 128 == 0)
        {
            Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len] = Aliyun_mqtt.Remaining_len;
        }
        else
        {
            Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len] = (Aliyun_mqtt.Remaining_len % 128) | 0x80;
        }
        Aliyun_mqtt.Fix_len++;
        Aliyun_mqtt.Remaining_len = Aliyun_mqtt.Remaining_len / 128;
    } while(Aliyun_mqtt.Remaining_len);

    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 0] = strlen(topic) / 256;
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 1] = strlen(topic) % 256;
    memcpy(&Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 2], topic, strlen(topic));

    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 2 + strlen(topic)] = Aliyun_mqtt.MessgeID / 256;
    Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 3 + strlen(topic)] = Aliyun_mqtt.MessgeID % 256;
    Aliyun_mqtt.MessgeID++;
  
    memcpy(&Aliyun_mqtt.Pack_buff[Aliyun_mqtt.Fix_len + 4 + strlen(topic)], data, strlen(data));

    U2_SendData(Aliyun_mqtt.Pack_buff, Aliyun_mqtt.Remaining_len + Aliyun_mqtt.Fix_len);
}

