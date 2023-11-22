#ifndef MQTTPROTOCOLBASE_H
#define MQTTPROTOCOLBASE_H

#include <iostream>
#include <stdlib.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;


struct mqtt_gcode_file_info_report_t
{
    int normal;        
    int panel;         
    int nozzel;        
    int speed;         
    int fans;          
    int leftTime;      
    int displacement;   
    int filamentUsed;   
    char filename[128];
    char url[128];      
};

struct mqtt_frame_t
{
    uint8_t version;    
    uint8_t productType;
    uint8_t deviceType; 
    uint8_t dataSource;
    uint8_t cmd;        
    uint8_t packageType; 
    uint16_t packageNum; 
    uint8_t uuid[40]; 
    uint8_t reserved[8];   
    uint32_t time;      
};

#define mqtt_error(X) printf(X)

// protocol header
#define MQTT_FRAME_A 0
#define MQTT_FRAME_M 0

//max Subcontract length
#define MQTT_PACKAGE_BUF_MAX_LEN 0
#define MQTT_REF_TIME 0

// protocal version
#define PROTOCOL_VERSION 0

// cmd
#define MQTT_CMD_SEND_CODE 0
#define MQTT_CMD_RESPOND_CODE 0


// product type
enum mqtt_product_type
{
    PRODUCT_FDM_TYPE = 0,
};

// device type
enum mqtt_device_type
{
    DEVICE_UNKNOWN_TYPE = 0
};

// device parts
enum mqtt_device_parts_type
{
    DEVICE_PARTS_NO,
    DEVICE_PARTS_MULTI_COLOR,
};

// data source
enum mqtt_data_source
{
    DATA_SOURCE_IOS = 0,
    DATA_SOURCE_ANDROID = 0,
    DATA_SOURCE_PC_MAC = 0,
    DATA_SOURCE_PC_WIN = 0,
    DATA_SOURCE_MACHINE_DEVICE = 0,
    DATA_SOURCE_SMART_ALEXA = 0,
    DATA_SOURCE_SMART_GOOGLE_HOME = 0,
};

#endif // MQTTPROTOCOLBASE_H
