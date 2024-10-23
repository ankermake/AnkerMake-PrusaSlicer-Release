#ifndef ANKER_NET_DEFINES_H
#define ANKER_NET_DEFINES_H

#ifdef __APPLE__
#define DLL_EXPORT  __attribute__((visibility("default")))
#else
#define  DLL_EXPORT _declspec(dllexport)
#endif

#include <functional>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <list>
#include <map>

#define MappingVersion 1052402

#define DEF_PTR(className) class className; typedef std::shared_ptr<className> className##Ptr;

#define ProgressCallback std::function<void (double dltotal, double dlnow, double ultotal, double ulnow)>

#define SnCallBackFunc std::function<void (const std::string& sn)>
#define SnStateCallBackFunc std::function<void (const std::string& sn, const int state)>
#define SnImgCallBackFunc std::function<void (std::string& sn, const unsigned char* imgDate, short frameWith, short frameHeight)>

#define LogOutputCallBackFunc std::function<void (unsigned int logLevel, const std::string strLogMsg,  const std::string strFileName,\
            const std::string strFuncName, const unsigned int lineNumber)>

#define NormalCallBack std::function<void ()>
#define CommentFlagsCallBack std::function<void (std::vector<std::string> dataList)>

//webview
#define PrivayChoiceCb std::function<void(const std::string&)>
#define LoginFinishCb std::function<void()>


#define CallBack_OtaInfoRecv std::function<void (OtaInfo info)>
#define CallBack_MsgCenterCfg std::function<void (std::map<std::string, MsgCenterConfig> dataMap)>
#define CallBack_MsgCenterErrCodeInfo std::function<void (std::vector<MsgErrCodeInfo> dataList)>
#define CallBack_MsgCenterStatus std::function<void (int officicalNews, int printNews)>
#define CallBack_MsgCenterRecords std::function<void (std::vector<MsgCenterItem>)>


typedef size_t(*CallBackFunction)(char* dest, size_t size, size_t nmemb, void* userp);

#define CURL_CODE_OK 0

#define HTTP_CODE_OK 200

namespace AnkerNet
{

	
enum P2P_Video_Mode_t
{
    P2P_VIDEO_MODE_HD = 1,
    P2P_VIDEO_MODE_SMOOTH = 2
};

// max nozzle temperature
// all-metal hotend: 300, ptfe hotend: 260
// http://codingcorp.coding.anker-in.com/p/zz_3d_pc/requirements/issues/1200/detail
enum class MaxNozzleTemp {
    PTFE = 260,
    ALLMETAL = 300
};

typedef enum
{
    VIDEO_CLOSE_NONE = -1,
    VIDEO_CLOSE_BY_USER = 0,
    VIDEO_CLOSE_BY_APP_QUIT = 1,
    VIDEO_CLOSE_BY_TRANSFER_FILE = 2,
    VIDEO_CLOSE_BY_ABNORMAL_EVENT = 3,
    VIDEO_CLOSE_BY_LOGOUT = 4,
    VIDEO_CLOSE_BY_UI_TIMEOUT = 5,
    VIDEO_CLOSE_BY_OFF_LINE = 6
} VideoCloseReasonType;


enum class AKNMT_LOG_LEVEL {
    MAX = 1,
    MID,
    MIN,
    PROTOCOL,
    ERRORO,
    SEVERE,
    FATAL
};

struct SysInfo
{
    //std::string m_os_type = "";
    std::string m_os_version = "";
    std::string m_cpu_model = "";
    std::string m_gpu_model = "";
    std::string m_ram = "";
    std::string m_video_ram = "";
    std::string m_primary_screen_resolution = "";
    std::string m_extended_screen_resolution = "";
    std::string m_opengl_version = "";
    std::string m_machineid = "";
};

// Reference AnkerWebView.cpp and DataManger.cpp
struct AnkerNetInitPara
{
    // http header
    std::string App_name;
    std::string Model_type;
    std::string App_version_V;//
    std::string App_version;//
    int			Version_code;//
    std::string Country;
    std::string Language;
    std::string Openudid;
    std::string Os_version;
    std::string Os_type;
    std::string Content_Type;

    // data dir
    std::string exeDir;
    std::string dataDir;
    std::string resourcesDir;
    std::string certDir;
    std::string logDir;

    AKNMT_LOG_LEVEL aknmtLogLevel = AKNMT_LOG_LEVEL::FATAL;

    SysInfo sysInfo;
};

struct CardInfo {
public:
    int index = 0;
    long long materialId = 0;
    long long colorId = 0;
};

// <virtural for gcode, real for the device>
using VrCardInfoMap = std::vector<std::pair<CardInfo/*gcode*/, CardInfo/*device*/>>;

enum OtaCheckType {
    OtaCheckType_Unknown,
    OtaCheckType_Manual,
    OtaCheckType_AppBegin,
    OtaCheckType_LoginSuccess,
    OtaCheckType_24Hours,
};

struct OtaInfo
{
    std::string device_type = "";
    std::string version_name = "";
    std::string release_note = "";
    std::string download_path = "";
    std::string md5 = "";
    bool is_forced = false;
    bool enabled = false;
    int version_code = -1;
    int size = -1;
    int update_time = -1;
    int create_time = -1;
    bool up_forced = false;

    bool noUpdate = false;//add by tab,means data is null
};

//msg center config

struct MsgCenterConfig 
{
    int id = 0;
    std::string error_code = "";
    std::string error_level = "";
    int code_source = 0;
    std::string originMsg = "";
    struct ArticleInfo {
        std::string language = "";
        std::string article_url = "";
        std::string article_title = "";
    };

    std::vector<ArticleInfo> article_info;
};

struct MsgErrCodeInfo
{
    std::string language = "";
    std::string version = "";
    std::string release_version = "";
    std::string originMsg = "";
    std::map<std::string, std::string> errorCodeUrlMap;
};


struct MsgCenterItem
{
    int msgID = 0;
    std::string msgTitle = "";
    std::string msgContent = "";
    std::string msgUrl = "";
    std::string msgCreateTime = "";
    std::string msgErrorCode = "";    
    std::string msgLevel = "";
    int alarm_type = 0;
    int ai_alarm_type = 0;
    bool msgIsNew = true;    
};

struct StarCommentData {
    std::string       reviewNameID = "";
    std::string       reviewName = "";
    std::string       appVersion = "";
    std::string       country = "";

    std::string       sliceCount = "";

    int               action = 2;
    int               rating = 0;
    std::string       reviewData = "";
    std::string       clientId = "";
};

struct PrintStopReasonInfo
{
    std::string reason_title = "";
    std::string reason_value = "-1";
    bool support_japan = true;
    std::string language = "en";
    int weight = 0;
    std::string help_desc = "";
    std::string help_desc_light = "";
    std::string help_link = "";

    void reset() {
        reason_title = "";
        reason_value = "-1";
        support_japan = true;
        language = "en";
        weight = 0;
        help_desc = "";
        help_desc_light = "";
        help_link = "";
    }
    bool isValid() { 
        return !reason_title.empty() && reason_value != "-1"; 
    }
};
using PrintStopReasons = std::vector<PrintStopReasonInfo>;

struct SliceTip
{
    std::string tip_title = "";
    std::string tip_value = "-1";
    bool support_japan = true;
    std::string language = "en";
    int weight = 0;
    std::string image_url = "";
    std::string help_desc = "";
    std::string help_desc_light = "";
    std::string help_link = "";

    void reset() {
        tip_title = "";
        tip_value = "-1";
        support_japan = true;
        language = "en";
        weight = 0;
        image_url = "";
        help_desc = "";
        help_desc_light = "";
        help_link = "";
    }
    bool isValid() {
        return !tip_title.empty() && tip_value != "-1";
    }
};
using SliceTips = std::vector<SliceTip>;


// http request result.
// Mainly to solve the problem of http status code return and the problem of http content being returned multiple times.
// add by tab wang
struct HttpRequestResult
{
    // curl perform result.
    // 0 means success
    int curlCode = 0;

    // Http Status Code
    // Relevant details can be found on Baidu HTTP status code
    // 200 means OK
    int httpCode = 0;

    // Http Request Content
    std::vector<char> httpContent;
};

enum HttpRequestType
{
    HttpRequest_Get = 0,
    HttpRequest_Post = 1,
    HttpRequest_Put = 2,
    HttpRequest_Delete = 3,
};

typedef enum {
    P2P_IDLE = 0,
    P2P_TRANSFER_LOCAL_FILE = 1,
    P2P_TRANSFER_VIDEO_STREAM = 2,
    P2P_TRANSFER_PREVIEW_FILE = 3,
}P2POperationType;

enum HttpError
{
    UnknownError = -1,
    NoError = 0,
    UserNotIdentified = -2,//HTTP_USER_AUTHENTICATION_FAILED
    ResolveHostError = -3,//curl could not resolve host
    ContentInvalid = -4,//content is not json    
};
typedef struct _FeedBackInfo {
    std::string content = "";
    std::string email = "";
    bool sendLogs = false;
    std::string logZipPath = "";
}FeedBackInfo;
/*********************************************************************************************************/

//https://make-app-us-qa.eufylife.com/v2/passport/login
typedef struct _LOGIN_DATA
{
    std::string captcha_id = std::string();
    std::string answer = std::string();

    std::string web_callback = std::string();
    std::string web_json_data = std::string();
    std::string user_id = std::string();
    std::string email = std::string();
    std::string nick_name = std::string();
    std::string auth_token = std::string();
    int token_expires_at = 0;
    std::string avatar = std::string();
    std::string invitation_code = std::string();
    std::string inviter_code = std::string();
    std::string verify_code_url = std::string();
    std::string mac_addr = std::string();
    std::string domain = std::string();
    std::string ab_code = std::string();
    std::string geo_key = std::string();
    int privilege = 0;
    std::string phone = std::string();
    std::string phone_code = std::string();
    int test_flag = 1;
    void clearData()
    {
        captcha_id = std::string();
        answer = std::string();
        user_id = std::string();
        email = std::string();
        nick_name = std::string();
        auth_token = std::string();
        token_expires_at = 0;
        avatar = std::string();
        invitation_code = std::string();
        inviter_code = std::string();
        verify_code_url = std::string();
        mac_addr = std::string();
        domain = std::string();
        ab_code = std::string();
        geo_key = std::string();
        privilege = 0;
        phone = std::string();
        phone_code = std::string();
        test_flag = 1;
    }
}LOGIN_DATA, * pLOGIN_DATA;

typedef struct _COUNTRY_INFO
{
    int id = 0;
    std::string name = std::string();
    std::string code = std::string();

}COUNTRY_INFO, * pCOUNTRY_INFO;



typedef struct _USER_INFO
{
    std::string user_id = std::string();
    std::string email = std::string();
    std::string nick_name = std::string();
    std::string avatar = std::string();
    std::string invitation_code = std::string();
    std::string inviter_code = std::string();
    std::string verify_code_url = std::string();

    COUNTRY_INFO countryInfo;
    bool is_subscribe = false;
    std::string  mac_addr = std::string();

    void clear()
    {
        user_id = std::string();
        email = std::string();
        nick_name = std::string();
        avatar = std::string();
        invitation_code = std::string();
        inviter_code = std::string();
        verify_code_url = std::string();

        bool is_subscribe = false;
        mac_addr = std::string();
    }

}USER_INFO, * pUSER_INFO;

enum GeneralException2Gui {
    GeneralException2Gui_No_Error,
    GeneralException2Gui_One_Mos, // Warning. Printer may overheat. Power off %s, remove all USB-C cables, and then plug them back in. You can also contact support@ankermake.com for help.
    GeneralException2Gui_Two_Mos,  // Warning.Power off %s. An error could cause abnormally high temperatures and damage the printer. Contact the support@ankermake.com for help.
    GeneralException2Gui_Nozzle_Temp_Too_High,  // Temp Too High.The current print job has stopped. Please go to support.ankermake.com to troubleshoot or contact support@ankermake.com for help.
    GeneralException2Gui_HotBed_Temp_Too_High,
    GeneralException2Gui_Nozzle_Heating_Error, // Heating Error.The current print job has stopped. Please go to support.ankermake.com to troubleshoot or contact support@ankermake.com for help.
    GeneralException2Gui_HotBed_Heating_Error,
    GeneralException2Gui_Filament_Broken,	// Filament Broken.Filament transfer interrupted. Printing has paused. Please resume printing after checking.
    GeneralException2Gui_Type_C_Transmission_Error,	// Data Transmission Error.Check if the USB-C cable is connected properly.
    GeneralException2Gui_Auto_Level_Error, //Auto-Level Error. Leveling failed, please try again. If the problem persists, please email support@ankermake.com to contact customer service for assistance.
    GeneralException2Gui_Auto_Level_Anomalies, //The heatbed could not be leveled. Try cleaning the nozzle and heatbed, then attempt again. If the problem persists, please email support@ankermake.com to contact customer service for assistance.
    GeneralException2Gui_System_Error,	 // Data Transmission Error. The system error, please try again.
    GeneralException2Gui_Advance_Pause, // A Gcode pause command has been executed. Check your print.
    GeneralException2Gui_Bed_Adhesion_Failure, // Bed Adhesion Failure
    GeneralException2Gui_Spaghetti_Mess, // Spaghetti Mess.
    GeneralException2Gui_HomingFailed, // Homing failed. Try again later.
    GeneralException2Gui_Break_Point, // No notice.
    GeneralException2Gui_Level_100_Times, // Before printing, we recommend using auto-level for better results. The process will take about 10 minutes.
    GeneralException2Gui_LowTemperature, // Low Temperature. For best results, we recommend printing in a location with an ambient temperature between 15 - 35.
    GeneralException2Gui_Calibration_Abnomal,
    GeneralException2Gui_Calibration_Failed,
};

class ExceptionInfo
{
public:
    GeneralException2Gui type = GeneralException2Gui_No_Error;
    std::string stationName;
    std::string sn;
    std::string external;
};

enum CustomDeviceStatus {
    CustomDeviceStatus_Level_Finished = 20000,
    CustomDeviceStatus_Exception_Finished = 20001,
    CustomDeviceStatus_File_Transfer = 20002,
    CustomDeviceStatus_File_Transfer_GCodeDecompressing = 20003,
    CustomDeviceStatus_Max
};

enum aknmt_command_type_e
{
    AKNMT_CMD_EVENT_ERROR = -99,//1
    AKNMT_CMD_EVENT_NONE = -1,//1
    AKNMT_CMD_NOZZLE_TEMP = 1003,//1
    AKNMT_CMD_HOTBED_TEMP = 1004,//1
    AKNMT_CMD_FILE_LIST_REQUEST = 1009,//1
    AKNMT_CMD_GCODE_FILE_REQUEST = 1010,//1
    AKNMT_CMD_Z_AXIS_RECOUP = 1021,//1
    AKNMT_CMD_THUMBNAIL_UPLOAD_NOTICE = 1044,//1
};

enum MatchGCodeFileStatusType
{
    MatchGCodeFileStatusType_Normal = 0,
    MatchGCodeFileStatusType_NotBelongToCurrentPrinter,
    MatchGCodeFileStatusType_NotBelongToAnker,
    MatchGCodeFileStatusType_BothPhenomena
};

enum NozzleStatus
{
    Normal = 0,
    CutOff = 1,
    Clogging = 2
};

class Base
{
public:
    bool isNull = true;
};

struct FileInfo {
    std::string name = "";
    std::string path = "";
    unsigned long long timestamp = 0;
};

class MtColorSlotData : public Base
{
public:
    int cardIndex = 0;  // 1
    int64_t materialId = 0;//1
    int64_t colorId = 0;//1
    int64_t materialColor = 0;//1
    int edit_status = 0;    //1
    NozzleStatus nozzle_status = NozzleStatus::Normal;//1
    int rfid = 0;//1
};
using MtColorSlotDataVec = std::vector<MtColorSlotData>;

class PrintFailedInfo : public Base
{
public:
    std::string name = "";	//1
    int64_t totalTime = 0;//1
    int filamentUsed = 0;//1
    std::string filamentUnit = "";//1
};

class PrintNoticeInfo : public Base
{
public:
    std::string name = "";  //1
    int totalTime = -1;//1
    int filamentUsed = -1;//1
    std::string filamentUnit = "";//1
    int realSpeed = 0;//1
};

class LevelData : public Base
{
public:
    int value = 0;
    int total_point = 49;
    int nozzle_index = 0;
    int nozzle_total_num = 1;
    int progress = 0;
};

class TemperatureInfo : public Base
{
public:
    int currentTemp = 0;// 1
    int targetTemp = 0;// 1
};

class MtCalibration : public Base
{
public:
    int progress = 0;
    int nozzelNum = 0;
};

class PliesInfo : public Base
{
public:
    int total_layer = 0;
    int real_print_layer = 0;
};

struct GFileNozzle
{
    int vindex = 0;			// virtural nozzle num in gcode
    int rindex = 0;			// real nozzle num on device(start form 0)
    int64_t materialId = 0;	// material id on device
    int64_t vcolorId = 0;	// virtural color for the nozzle in gcode
    int64_t rcolorId = 0;	// real color on device
};

class GCodeInfo : public Base
{
public:
    MatchGCodeFileStatusType file_status = MatchGCodeFileStatusType_Normal;
    std::string fileName = "";
    int leftTime = 0;
    int filamentUsed = 0;
    std::string filamentUnit = "mm";
    std::string filamentType;
    std::vector<GFileNozzle> m_gFileNozzles;
};

enum class FileTransferResult {
    Succeed = 100,
    Failed = -4,

    Transfering = 1,

    GcodePathEmpty = -11,
    GcodeNoAGcode = -12,
    GcodePathError = -13,
    ZipFailed = -14,
    InitFailed = -15,

    SendCmdFailed = -31,
    SendFileInfoFailed = -32,
    SendFileDataFailed = -33,
    OpenFileFailed = -34,

    DevicBusy = -55,
    NeedLevel = -56,

    // inner error code
    ClientP2pBusy = -71,
};

using sendSigHttpError_T = std::function <void(HttpError error)>;
using sendSigToSwitchPrintPage_T = std::function <void(const std::string& sn)>;
using sendSigToUpdateDevice_T = std::function <void()>;
using sendSigToUpdateDeviceStatus_T = std::function <void(const std::string& sn, aknmt_command_type_e type)>;
using sendSigToTransferFileProgressValue_T = std::function <void(const std::string& sn, int progess, FileTransferResult result)>;
using sendShowDeviceListDialog_T = std::function <void()>;
using GeneralExceptionMsgBox_T = std::function<void(const ExceptionInfo&)>;
using SendSigAccountLogout_T = std::function <void()>;

// Print event
enum aknmt_print_event_e
{
    AKNMT_PRINT_EVENT_IDLE = 0,
    AKNMT_PRINT_EVENT_PRINTING = 1,
    AKNMT_PRINT_EVENT_PAUSED = 2,
    AKNMT_PRINT_EVENT_STOPPED = 3,               // no use
    AKNMT_PRINT_EVENT_COMPLETED = 4,
    AKNMT_PRINT_EVENT_LEVELING = 5,
    AKNMT_PRINT_EVENT_DOWNLOADING = 6,           // no use
    AKNMT_PRINT_EVENT_LEVEL_HEATING = 7,
    AKNMT_PRINT_EVENT_PRINT_HEATING = 8,
    AKNMT_PRINT_EVENT_PREHEATING = 9,
    AKNMT_PRINT_EVENT_PRINT_DOWNLOADING = 10,    // no use
    AKNMT_PRINT_EVENT_CALIBRATION = 11,
    AKNMT_PRINT_EVENT_CALIBRATION_HEATING = 12,
    AKNMT_PRINT_EVENT_EXTRUSION_PREHEATING = 13,             // no use
    AKNMT_PRINT_EVENT_FEED_AND_RETURN_TO_THE_MATERIAL = 14,  // no use
    AKNMT_PRINT_EVENT_SLICING = 15,              // no use
    AKNMT_PRINT_EVENT_LOAD_MATERIAL = 16,
    AKNMT_PRINT_EVENT_UNLOAD_MATERIAL = 17,
    AKNMT_PRINT_EVENT_MAX,
};


// device type
enum anker_device_type
{
    DEVICE_V8110_TYPE = 0x01,
    DEVICE_V8111_TYPE = 0x02,   // M5
    DEVICE_V7111_TYPE = 0x03,   // 

    DEVICE_UNKNOWN_TYPE = 0x99
};

// device parts
enum anker_device_parts_type
{
    DEVICE_PARTS_NO,
    DEVICE_PARTS_MULTI_COLOR,
};


//webview
enum class WEB_ACTION {
    EM_UNKNOW,
    EM_LOGIN,
    EM_LOGINOUT,
    EM_LOGINBACK,
    EM_OPEN_BROWSER,
    EM_GET_HEADLIST,
};

enum class LOGIN_STATUS
{
    EM_LOGIN_UNKNOW,
    EM_NO_DATA,
    EM_USER_ID_NULL,
};

struct WebJsProcessRet
{
    std::string content= "";
    LOGIN_STATUS status = LOGIN_STATUS::EM_LOGIN_UNKNOW;
    WEB_ACTION action = WEB_ACTION::EM_UNKNOW;
    std::string callBackName;
};


enum GUI_DEVICE_STATUS_TYPE {

    GUI_DEVICE_STATUS_TYPE_IDLE = AKNMT_PRINT_EVENT_IDLE,
    GUI_DEVICE_STATUS_TYPE_PRINTING = AKNMT_PRINT_EVENT_PRINTING,
    GUI_DEVICE_STATUS_TYPE_PAUSED = AKNMT_PRINT_EVENT_PAUSED,
    GUI_DEVICE_STATUS_TYPE_STOPPED = AKNMT_PRINT_EVENT_STOPPED,
    GUI_DEVICE_STATUS_TYPE_LEVELING = AKNMT_PRINT_EVENT_LEVELING,
    GUI_DEVICE_STATUS_TYPE_DOWNLOADING = AKNMT_PRINT_EVENT_DOWNLOADING,
    GUI_DEVICE_STATUS_TYPE_HEATING = AKNMT_PRINT_EVENT_LEVEL_HEATING,
    GUI_DEVICE_STATUS_TYPE_PRINT_HEATING = AKNMT_PRINT_EVENT_PRINT_HEATING,
    GUI_DEVICE_STATUS_TYPE_PRINT_PREHEATING = AKNMT_PRINT_EVENT_PREHEATING,
    GUI_DEVICE_STATUS_TYPE_PRINT_DOWNLOADING = AKNMT_PRINT_EVENT_PRINT_DOWNLOADING,
    GUI_DEVICE_STATUS_TYPECALIBRATION = AKNMT_PRINT_EVENT_CALIBRATION,
    GUI_DEVICE_STATUS_TYPE_CALIBRATION_PREHEATING = AKNMT_PRINT_EVENT_CALIBRATION_HEATING,
    GUI_DEVICE_STATUS_TYPE_EXTRUSION_PREHEATING = AKNMT_PRINT_EVENT_EXTRUSION_PREHEATING,
    GUI_DEVICE_STATUS_TYPE_FEED_AND_RETURN_TO_THE_MATERIAL = AKNMT_PRINT_EVENT_FEED_AND_RETURN_TO_THE_MATERIAL,

    GUI_DEVICE_STATUS_TYPE_REPLYING = 10002,

    GUI_DEVICE_STATUS_TYPE_LEVEL_FINISHED = CustomDeviceStatus_Level_Finished,
    GUI_DEVICE_STATUS_TYPE_EXCEPTION_FINISHED = CustomDeviceStatus_Exception_Finished,
    GUI_DEVICE_STATUS_TYPE_FILE_TRANSFER = CustomDeviceStatus_File_Transfer,
    GUI_DEVICE_STATUS_TYPE_FILE_TRANSFER_DECOMPRESSING = CustomDeviceStatus_File_Transfer_GCodeDecompressing,

    GUI_DEVICE_STATUS_TYPE_PRINT_FINISHED,
    GUI_DEVICE_STATUS_TYPE_PRINT_FAILED,

};
}

#endif // !ANKER_NET_DEFINES_H
