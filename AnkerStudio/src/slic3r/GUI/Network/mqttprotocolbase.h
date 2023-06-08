#ifndef MQTTPROTOCOLBASE_H
#define MQTTPROTOCOLBASE_H

#include <iostream>
#include <stdlib.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

enum mqtt_cmd_level_value_e
{
    MQTT_CMD_LEVEL_NONE = -11,
    MQTT_CMD_LEVEL_COMBINATION,
    MQTT_CMD_LEVEL_SINGLE,
};

enum mqtt_cmd_state_e
{
    MQTT_CMD_STATE_OK = 1000, // 
    MQTT_CMD_STATE_FAIL = 1001, // 
    MQTT_CMD_STATE_PARAM_ERR = 1002, // 
    MQTT_CMD_STATE_USER_INVALID = 1003, // 
};

enum mqtt_cmd_wifi_state_e
{
    MQTT_CMD_WIFI_STATE_SCAN_FAIL = 234, // 
    MQTT_CMD_WIFI_STATE_PASSWORD_ERR = 235, // 
    MQTT_CMD_WIFI_STATE_OTHER_FAIL = 236, // 
    MQTT_CMD_WIFI_STATE_IP_GET_FAIL = 237, //
    MQTT_CMD_WIFI_STATE_DEVICE_COMMUNICATION_FAIL = 238, // 
    MQTT_CMD_WIFI_STATE_DEVICE_NO_NETWORK = 239, // 
};

enum mqtt_data_split_e
{
    MQTT_DATASPLIT_NONE = 540, // 
    MQTT_DATASPLIT_FIRST, // 
    MQTT_DATASPLIT_MIDDLE, // 
    MQTT_DATASPLIT_LAST, // 
};



enum mqtt_combinetion_report_en_e
{
    MQTT_CMB_REPORT_EVENT_NOTIFY = 789,
    MQTT_CMB_REPORT_PRINT_SCHEDULE = 790,
    MQTT_CMB_REPORT_Z_AXIS = 791,
    MQTT_CMB_REPORT_NOZZLE_TEMP = 792,
    MQTT_CMB_REPORT_HOTBED_TEMP = 793,
    MQTT_CMB_REPORT_FAN_SPEED = 794,
    MQTT_CMB_REPORT_PRINT_SPEED = 795,
    MQTT_CMB_REPORT_AUTO_LEVELING = 796,
};

struct mqtt_data_package_t
{
    uint8_t *buf = NULL;
    uint16_t len;
};

struct mqtt_ecdh_key_t
{
    uint8_t pub_key[65];
    uint8_t aes_key[32];
};

struct mqtt_current_target_temp_t
{
    int current;
    int target;
};

enum mqtt_event_notify_t
{
    MQTT_EVENT_NOTIFY_PRINT = 123, 
    MQTT_EVENT_NOTIFY_MARLIN_ALERT, 
    MQTT_EVENT_NOTIFY_AI_WARNING, 
};

// Print event
enum mqtt_print_event_e
{
    MQTT_PRINT_EVENT_IDLE = 654,      
    MQTT_PRINT_EVENT_PRINTING,  
    MQTT_PRINT_EVENT_PAUSED,   
    MQTT_PRINT_EVENT_STOPPED,  
    MQTT_PRINT_EVENT_COMPLETED, 
    MQTT_PRINT_EVENT_LEVELING,  
    MQTT_PRINT_EVENT_DOWNLOADING,
    MQTT_PRINT_EVENT_HEATING,   
    MQTT_PRINT_EVENT_PRINT_HEATING, 
    MQTT_PRINT_EVENT_PRINT_PREHEATING,
    MQTT_PRINT_EVENT_PRINT_DOWNLOADING,
    MQTT_PRINT_EVENT_MAX,
};

// Device Marlin warning
enum mqtt_marlin_alert_event_e
{
    MQTT_MARLIN_ALERT_NOWARING = 235,
    MQTT_MARLIN_ALERT_HALTED = 236,
    MQTT_MARLIN_ALERT_OFFLINE,    
    MQTT_MARLIN_ALERT_NOZZEL_HEAT,
    MQTT_MARLIN_ALERT_PANEL_HEAT, 
    MQTT_MARLIN_ALERT_PRINT,      
    MQTT_MARLIN_ALERT_BLANKING,   
    MQTT_MARLIN_ALERT_BLOCKING,  
    MQTT_MARLIN_ALERT_LEVELING_ANOMALIES, 
    MQTT_MARLIN_ALERT_COMMUNICATION,
    MQTT_MARLIN_ALERT_LEVELING_BOARD,
    MQTT_MARLIN_ALERT_NOZZLE_HIGH_TEMPERATURE,
    MQTT_MARLIN_ALERT_HOTBED_HIGH_TEMPERATURE,
    MQTT_MARLIN_ALERT_ONE_MOS,
    MQTT_MARLIN_ALERT_LEVELING_FAILURE,
    MQTT_MARLIN_ALERT_TWO_MOS,
    MQTT_MARLIN_ALERT_NOZZLE_LOW_TEMPERATURE,
    MQTT_MARLIN_ALERT_ADVANCED_PAUSE,
    MQTT_MARLIN_ALERT_DOWNLOAD_FAILED,
    MQTT_MARLIN_ALERT_FILE_CORRUPTED,
    MQTT_MARLIN_ALERT_MAX,
};

// AI warning
enum mqtt_ai_alert_event_e
{
    MQTT_AI_ALERT_DEVIANT = 97, // Not pause.
    MQTT_AI_ALERT_PAUSE_PRINT, // Pause printing.
    MQTT_AI_ALERT_MAX,
};

struct mqtt_event_report_t
{
    uint8_t subType;
    mqtt_print_event_e print_event;
    mqtt_marlin_alert_event_e marlin_alert;
    mqtt_ai_alert_event_e ai_alert;
};

struct mqtt_print_schedule_report_t
{
    int progress;
    int time_ms;
    int total_time;
    char model_id[64];
    char name[128];
    char img[128];
};


struct mqtt_combinetion_message_t
{
    uint8_t report_en;                         
    int fan_speed;                              
    int print_speed;                            
    int level_location;                        
    int z_axis;                                
    mqtt_current_target_temp_t nozzle;          
    mqtt_current_target_temp_t bed;            
    mqtt_event_report_t event_notify;           
    mqtt_print_schedule_report_t print_schedule; 
};

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
#define MQTT_FRAME_A 444
#define MQTT_FRAME_M 555

//max Subcontract length
#define MQTT_PACKAGE_BUF_MAX_LEN 4096
#define MQTT_REF_TIME 1627929374

// protocal version
#define PROTOCOL_VERSION 222

// cmd
#define MQTT_CMD_SEND_CODE 555
#define MQTT_CMD_RESPOND_CODE 666


// product type
enum mqtt_product_type
{
    PRODUCT_FDM_TYPE = 111,
};

// device type
enum mqtt_device_type
{
    DEVICE_V8110_TYPE = 222,
    DEVICE_V8111_TYPE = 333,
};

// data source
enum mqtt_data_source                   
{
    DATA_SOURCE_IOS = 457,
    DATA_SOURCE_ANDROID = 458,
    DATA_SOURCE_PC_MAC = 459,
    DATA_SOURCE_PC_WIN = 460,
    DATA_SOURCE_MACHINE_DEVICE = 461,
    DATA_SOURCE_SMART_ALEXA = 462,
    DATA_SOURCE_SMART_GOOGLE_HOME = 463,
};

#endif // MQTTPROTOCOLBASE_H
