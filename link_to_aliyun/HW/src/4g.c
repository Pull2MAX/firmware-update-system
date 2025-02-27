#include "gd32f10x.h"
#include "main.h"
#include "4g.h"
#include "delay.h"
#include "usart.h"
#include "w25q64.h"
#include "m24c02.h"
#include "mqtt.h"

/*
    @brief Initialize the 4G module, including GPIO configuration and reset operation of the 4G module
    @param None
    @return None
    @details Configure PB0 as input pull-down mode to detect server connection status
             Configure PB2 as push-pull output mode to control the reset of the 4G module
             Execute reset timing: PB2 outputs high for 500ms and then pulls low
*/

extern MQTT_CONTROL_BLOCK Aliyun_mqtt;
void GM_4G_Init(void)
{
    // Enable GPIO clock
    rcu_periph_clock_enable(RCU_GPIOB);
    
    // Initialize IO ports
    gpio_init(GPIOB, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, GPIO_PIN_0); // PB0 is used to detect its level to check if connected to the server, so it needs to be configured as input mode
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2); // PB2 is used to control the reset of the 4G module (high level), so it needs to be configured as output mode

    U0_MyPrintf("4G module is resetting, please wait...\r\n");
    gpio_bit_set(GPIOB, GPIO_PIN_2); // Reset 4G module
    Delay_Ms(500);
    gpio_bit_reset(GPIOB, GPIO_PIN_2); 
}



void U2_Event(uint8_t *data, uint16_t datalen)
{
    if((datalen == 6)&&(memcmp(data, "xxxxxx", 6) == 0)) // 
    {
        U0_MyPrintf("\r\n4G module reset successful! Waiting for server connection (PB0=1)...\r\n");
        
        // Enable external interrupt for PB0
        rcu_periph_clock_enable(RCU_AF); // To use external interrupt functionality, the clock needs to be enabled
        exti_deinit();
        exti_init(EXTI_0,EXTI_INTERRUPT,EXTI_TRIG_BOTH);
        gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_0);
        exti_interrupt_enable(EXTI_0);
        nvic_irq_enable(EXTI0_IRQn,0,0);
    }

    if((datalen == 4)&&(data[0] == 0x20))
    {
        U0_MyPrintf("Received server connect_ack message\r\n");
        if(data[3] == 0x00)
        {
            U0_MyPrintf("After analyzing connect_ack message, server connection successful\r\n");
            BootStatusFlag |= MQTT_CONNECT_OK_FLAG;
            MQTT_SubscribePack("/sys/hh6zPbXNLbr/D001/thing/service/property/set");
            MQTT_SubscribePack("/sys/hh6zPbXNLbr/D001/thing/file/download_reply"); // After subscribing, the server will actively push new bin files to us if available
            Report_OTA_Version();

        }else{
            U0_MyPrintf("After analyzing connect_ack message, preparing to restart and reconnect!\r\n");
            NVIC_SystemReset();
        }
    }

    if((datalen == 5)&&(data[0] == 0x90))
    {
        U0_MyPrintf("Received server subscribe_ack message\r\n");
        if((data[datalen - 1] == 0x00) || (data[datalen - 1] == 0x01))
        {
            U0_MyPrintf("After analyzing subscribe_ack message, subscription successful\r\n");
        }else{
            U0_MyPrintf("After analyzing subscribe_ack message, subscription failed!\r\n");
            NVIC_SystemReset();
        }
    }

    if((BootStatusFlag & MQTT_CONNECT_OK_FLAG)&&(data[0] == 0x30)) // If the connection is successful and the first byte is 0x30
    {
        U0_MyPrintf("Received server level 0 push message\r\n");
        MQTT_DealPack(data,datalen); // This function has already placed useful commands into memory
        if(strstr((char *)Aliyun_mqtt.CMD_Buff,"{\"Switch1\":0}"))
        {
            U0_MyPrintf("Turning off the switch\r\n");
            MQTT_ResponseQs0Pack("/sys/hh6zBbXNLbr/D001/thing/event/property/post","{\"params\":{\"Switch1\":0}}");
        }
        if(strstr((char *)Aliyun_mqtt.CMD_Buff,"{\"Switch1\":1}"))
        {
            U0_MyPrintf("Turning on the switch\r\n");
            MQTT_ResponseQs0Pack("/sys/hh6zBbXNLbr/D001/thing/event/property/post","{\"params\":{\"Switch1\":1}}");
        }
        if(strstr((char *)Aliyun_mqtt.CMD_Buff,"/ota/device/upgrade/hh6zPbXNLbr/D001"))
        {
            if(sscanf((char *)Aliyun_mqtt.CMD_Buff,"/ota/device/upgrade/hh6zPbXNLbr/D001{\"code\":\"1000\",\"data\":{\"size\":%d,\"streamId\":%d,\"sign\":\"%*32s\",\"dProtocol\":\"mqtt\",\"version\":\"%26s\",\"signMethod\":\"Md5\",\"streamFileId\":1,\"md5\":\"%*32s\"},\"id\":%*d,\"message\":\"success\"}",&Aliyun_mqtt.size,&Aliyun_mqtt.streamId,Aliyun_mqtt.OTA_tempver)==3) // Use sscanf to extract the content of the message returned by the server
            {
                U0_MyPrintf("Extracting firmware information failed\r\n");
            }
            else
            { 
                U0_MyPrintf("Extracting firmware information failed\r\n");
            }
        }

        if(strstr((char *)Aliyun_mqtt.CMD_Buff,"/ota/device/upgrade/hh6zPbXNLbr/D001/download_reply"))
        {
            W25Q64_PageWrite(&data[datalen - Aliyun_mqtt.now_download_size - 2], Aliyun_mqtt.now_download_counter - 1);
            Aliyun_mqtt.now_download_counter++;
            if(Aliyun_mqtt.now_download_counter < Aliyun_mqtt.bin_total_download_counter)
            {
                Aliyun_mqtt.now_download_size = 256;
                Aliyun_mqtt.now_download_size = Aliyun_mqtt.size % 256;
            }
            else if(Aliyun_mqtt.now_download_counter == Aliyun_mqtt.bin_total_download_counter)
            {
                if(Aliyun_mqtt.size % 256 == 0)
                {
                    Aliyun_mqtt.now_download_size = 256;
                    Aliyun_mqtt.now_download_size = Aliyun_mqtt.size % 256;
                }
                else
                {
                    Aliyun_mqtt.now_download_size = Aliyun_mqtt.size % 256;

                    OTA_Download_by_Piece(Aliyun_mqtt.now_download_size, (Aliyun_mqtt.now_download_counter - 1) * 256);
                }
                
            }
            else
            {
                U0_MyPrintf("OTA download completed\r\n");
                memset(ota_info.OTA_VERSION, 0, 32);
                memcpy(ota_info.OTA_VERSION, Aliyun_mqtt.OTA_tempver, 26);
                ota_info.FireLen[0] = Aliyun_mqtt.size;
                ota_info.ota_flag = OTA_UPDATE_FLAG;
                M24C02_WriteOTAInfo();
                NVIC_SystemReset(); 
            }
        }

    }
}


void Report_OTA_Version(void)
{
    char temp[128];
    memset(temp,0,128);
    sprintf(temp, "{\"id\": \"1\",\"params\": {\"version\": \"%s\"}}", ota_info.OTA_VERSION);
    MQTT_ResponseQs1Pack("/ota/device/inform/hh6zPbXNLbr/D001", temp); // This is to report the version number to Alibaba Cloud, the first parameter is the topic to report
}


void OTA_Download_by_Piece(int size,int offset) // This function continuously downloads from the server in pieces
{
    char temp[256];
    memset(temp,0,256);
    sprintf(temp,"{\"id\": \"1\",\"params\": {\"fileInfo\":{\"streamId\":%d,\"fileId\":1},\"fileBlock\":{\"size\":%d,\"offset\":%d}}}",Aliyun_mqtt.streamId,size,offset);
    U0_MyPrintf("Download progress: %d/%d\r\n",Aliyun_mqtt.now_download_counter,Aliyun_mqtt.bin_total_download_counter);
    MQTT_ResponseQs0Pack("/sys/hh6zPbXNLbr/D001/thing/file/download", temp); 

    Delay_Ms(300); // This is a delay made to cope with the upload speed limit of Alibaba Cloud
