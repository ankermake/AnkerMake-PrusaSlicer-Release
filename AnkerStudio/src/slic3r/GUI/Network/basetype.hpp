#ifndef ANKER_NET_BASETYPE_HPP
#define ANKER_NET_BASETYPE_HPP

#include <memory>
#include <map>
#include <string>
#include <list>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32
#ifdef __APPLE__
#include <cstdint>
#include <dlfcn.h>
typedef uint64_t UINT64;
#endif

#define DEF_PTR(className) class className; typedef std::shared_ptr<className> className##Ptr;

// public server address
#define SERVER_ADDRESS_PUBLIC_QA ""
#define SERVER_ADDRESS_PUBLIC_US ""
#define SERVER_ADDRESS_PUBLIC_EU ""

// API address
#define API_ADDRESS_QA ""
#define API_ADDRESS_US ""
#define API_ADDRESS_EU ""

// MQTT address
#define MQTT_ADDRESS_QA ""
#define MQTT_ADDRESS_US ""
#define MQTT_ADDRESS_EU ""

// MQTT certificate
#define MQTT_CERTIFICATE_QA ""
#define MQTT_CERTIFICATE_US ""
#define MQTT_CERTIFICATE_EU ""

// HTTPS certificate
#define HTTPS_CERTIFICATE_QA ""
#define HTTPS_CERTIFICATE_PEM_US ""
#define HTTPS_CERTIFICATE_CER_EU ""

#define MQTT_CONNECT_PORT 1000


// Query DeviceList
#define QUERY_DEVICELIST_FROM_BK ""

// Query DSK
#define QUERY_DSK_FROM_BK ""

// Query User info
#define QUERY_USER_INFO_FROM_BK ""

typedef int (*AsynMQTTClientCallback) (void* context, char* topicName, int topiclen);

void PrintLog(const std::string& str);
void mySleep(UINT64 ms);


namespace P2pType {

	enum RequestP2pType {
		RequestP2pType_Print,
		RequestP2pType_Video,
		RequestP2pType_Unknown
	};
	typedef struct _ReadWriteData
	{
		char mode = '1';
		std::string didStr = std::string();
		std::string initStr = std::string();
		std::string license = std::string();
		std::string usrid = std::string();
		std::string nickName = std::string();
		std::string dsk = std::string();
		std::string sn = std::string();
		int chData = 1;
		int sessionId = -99;
		char uuid[16] = { 0 };
		std::string uuidStr = std::string();
		bool needLevel = false;

	} ReadWriteData, * pReadWriteData;


	typedef struct _SendWorkData
	{
		int sessionId = 0;
		unsigned char chData = '1';

		int timeout = 0;
		std::string sn = std::string();
		std::string didStr = std::string();
		std::string initStr = std::string();
		std::string fileName = std::string();
		std::string usrName = std::string();
		std::string usrId = std::string();

		bool threadActive = false;
		bool needLevel = false;
		char* uuid = nullptr;
	}SendWorkData, pSendWorkData;

	typedef struct _VideoCtrlData
	{
		char mode = '1';
		std::string didStr = std::string();
		std::string initStr = std::string();
		std::string license = std::string();
		std::string usrid = std::string();
		std::string dsk = std::string();
		std::string sn = std::string();
		std::string uuidStr = std::string();
		int sessionId = -99;
		char uuid[16] = { 0 };


		_VideoCtrlData& operator=(const _VideoCtrlData& other) {
			if (this != &other) {
				mode = other.mode;
				didStr = other.didStr;
				initStr = other.initStr;
				license = other.license;
				usrid = other.usrid;
				dsk = other.dsk;
				sn = other.sn;
				uuidStr = other.uuidStr;
				sessionId = other.sessionId;
				memcpy(uuid, other.uuid, sizeof(uuid));
			}
			return *this;
		}
	} VideoCtrlData, * pVideoCtrlData;

	typedef enum {
	   P2P_IDLE = 0,
	   P2P_TRANSFER_FILE = 1,
	   P2P_TRANSFER_VIDEO_STREAM = 2
	}P2POperationType;

	enum P2P_File_Transfer_Code{
		P2P_File_Transfer_Code_Succeed = 0,
		P2P_File_Transfer_Code_Device_Busy = -1,
		P2P_File_Transfer_Code_ECC_Error = -2,
		P2P_File_Transfer_Code_Frame_Error = -3,
		P2P_File_Transfer_Code_Unknown_Error = -4,
		P2P_File_Transfer_Code_File_Error =  -5,
		P2P_File_Transfer_Code_Header_Error = -111,
		P2P_File_Transfer_Code_File_Open_Error = -112,
		P2P_File_Transfer_Code_Transfer_Finished = 101,
	};
};

typedef enum
{
	VIDEO_CLOSE_NONE = -1,
	VIDEO_CLOSE_BY_USER = 0,
	VIDEO_CLOSE_BY_APP_QUIT = 1,
	VIDEO_CLOSE_BY_TRANSFER_FILE = 2,
	VIDEO_CLOSE_BY_ABNORMAL_EVENT = 3,
	VIDEO_CLOSE_BY_LOGOUT = 4
} VideoCloseReasonType;

namespace HttpsType {
	enum REQUEST_STYLE
	{
		REQUEST_STYLE_UNKNOWN = -1,
		REQUEST_STYLE_POST,
		REQUEST_STYLE_PUT,
		REQUEST_STYLE_DELETE,
		REQUEST_STYLE_GET
	};

	enum LoginErrorCode
	{
		CodeSystemException = -1,
		CodeInputParamInvalid = 10000,
		CodeThirdPartyLoginFailed = 26005,
		CodeLoginFailed = 26006,
		CodeAccountNotActivated = 26015,
		CodeModelTypeLoginLimit = 26049,
		CodeNeedCaptcha = 100032,
		CodeCaptchaError = 100033,
		CodeLoginLimit = 100023,
		CodeHeadErr = 100024,
		CodeBlackListLoginLimit = 100027,
		CodeMaxLoginLimit = 100028,
	};


enum EnvironmentType {
	EnvironmentType_Unknown = -1,
};

enum AKeyPrintType {
	AKeyPrintType_Unknown = -1,
	AKeyPrintType_AnkerSlicer,
	AKeyPrintType_Network
};

typedef struct _header_info
{
	std::string x_auth_token = "";
	std::string gtoken = "";
	std::string user_country = "";
	std::string app_name = "";
	std::string modelType = "PC";
	std::string app_version = "";
	std::string country = "";
	std::string language = "";
	std::string timezone = "";
	std::string phone_model = "";
	std::string open_uuid = "";
	std::string os_type = "";
	std::string os_version = "";
	std::string user_agent = "";
	std::string x_request_id = "";
	std::string time_offset = "";
	std::string x_real_ip = "";
	std::string x_forwarded_for = "";
}HeaderInfo;

/*********************************************************************************************************/

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
	}
}LOGIN_DATA, * pLOGIN_DATA;

typedef struct _PRINT_MACHINE_PARAMS
{
	int param_type = 0;
	int  param_value = 0;
	int param_id = 0;
	std::string station_sn = std::string();
	unsigned long create_time = 0;
	unsigned long update_time = 0;
	int status = 0;

}PRINT_MACHINE_PARAMS, * pPRINT_MACHINE_PARAMS;


typedef struct _PRINT_MACHINE
{
	int  station_id = 0;
	std::string station_sn = std::string();
	std::string station_name = std::string();
	std::string station_model = std::string();
	std::string time_zone = std::string();
	std::string wifi_ssid = std::string();
	std::string ip_addr = std::string();
	std::string wifi_mac = std::string();
	std::string main_sw_version = std::string();
	std::string main_hw_version = std::string();
	std::string sec_sw_version = std::string();
	std::string sec_hw_version = std::string();
	int device_type = 0;
	std::string machine_name = std::string();
	std::string secret_key = std::string();
	int mqtt_status = 0;
	int p2p_status = 0;
	unsigned long create_time = 0;
	unsigned long update_time = 0;

	std::string p2p_did = std::string();
	std::string push_did = std::string();
	std::string p2p_license = std::string();
	std::string push_license = std::string();
	std::string ndt_did = std::string();
	std::string ndt_license = std::string();
	int wakeup_flag = 0;
	std::string p2p_conn = std::string();
	std::string app_conn = std::string();
	std::string wipn_enc_dec_key = std::string();
	std::string wipn_ndt_aes128key = std::string();
	std::string query_server_did = std::string();
	std::string prefix = std::string();
	std::string wakeup_key = std::string();
	std::vector<PRINT_MACHINE_PARAMS> machineList = {};

	int member_type = 0;
	bool is_query = false;
	bool is_command = false;
	bool is_camera = false;

	void clearData()
	{
		station_sn = std::string();
		station_name = std::string();
		station_model = std::string();
		time_zone = std::string();
		wifi_ssid = std::string();
		ip_addr = std::string();
		wifi_mac = std::string();
		main_sw_version = std::string();
		main_hw_version = std::string();
		sec_sw_version = std::string();
		sec_hw_version = std::string();
		device_type = 0;
		machine_name = std::string();
		secret_key = std::string();
		mqtt_status = 0;
		p2p_status = 0;
		create_time = 0;
		update_time = 0;

		p2p_did = std::string();
		push_did = std::string();
		p2p_license = std::string();
		push_license = std::string();
		ndt_did = std::string();
		ndt_license = std::string();
		wakeup_flag = 0;
		p2p_conn = std::string();
		app_conn = std::string();
		wipn_enc_dec_key = std::string();
		wipn_ndt_aes128key = std::string();
		query_server_did = std::string();
		prefix = std::string();
		wakeup_key = std::string();
		machineList.clear();

		member_type = 0;
		is_query = false;
		is_command = false;
		is_camera = false;

	}

}PRINT_MACHINE, * pPRINT_MACHINE;
typedef std::vector<PRINT_MACHINE> PRINT_MACHINE_LIST;
typedef std::vector<PRINT_MACHINE*> pPRINT_MACHINE_LIST;

typedef struct _DSK_CFG
{
	std::string station_sn = std::string();
	std::string dsk_key = std::string();
	unsigned long expiration = 0;
	bool about_to_be_replaced = false;
}DSK_CFG, * pDSK_CFG;

typedef struct _DSK_INFO
{
	bool enabled = false;
	std::vector<DSK_CFG> dskCfgList = {};

	void clearData()
	{
		enabled = false;
		dskCfgList.clear();
	}

}DSK_INFO, * pDSK_INFO;

typedef struct _PRINT_LOG
{
	unsigned int id = 0;
	unsigned int  operate_type = 0;
	unsigned long device_time = 0;
	unsigned int client_type = 0;
	std::string operate_user_id = std::string();
	std::string model_name = std::string();
	unsigned int param_type = 0;
	unsigned int param_src = 0;
	unsigned int param_dst = 0;
	unsigned int error_message = 0;
	unsigned long create_time = 0;
	std::string station_sn = std::string();
	std::string nick_name = std::string();
	std::string operate_user_email = std::string();

	void clearData()
	{
		int id = 0;
		int  operate_type = 0;
		long device_time = 0;
		int client_type = 0;
		operate_user_id = std::string();
		model_name = std::string();
		param_type = 0;
		param_src = 0;
		param_dst = 0;
		error_message = 0;
		create_time = 0;
		station_sn = std::string();
		nick_name = std::string();
		operate_user_email = std::string();
	}

}PRINT_LOG, * pPRINT_LOG;


typedef struct _OTA_DEVICE_VERSION_DATA
{
	std::string device_type = std::string();
	unsigned int rom_version = 0;
	std::string rom_version_name = std::string();
	std::string introduction = std::string();
	std::string note = std::string();

	unsigned int upgrade_flag = 0;
	unsigned int upgrade_type = 0;

	bool force_upgrade = false;
	bool up_forced = false;

	//full_package
	std::string file_path = std::string();
	unsigned long file_size = 0;
	std::string file_name = std::string();
	std::string file_md5 = std::string();
}OTA_DEVICE_VERSION_DATA, * pOTA_DEVICE_VERSION_DATA;

typedef struct  _OTA_DEVICE_VERSION_INFO
{
	OTA_DEVICE_VERSION_DATA otaVersionData;
	std::vector<OTA_DEVICE_VERSION_DATA> childrenPack = {};//only one layer

}OTA_DEVICE_VERSION_INFO, * pOTA_DEVICE_VERSION_INFO;

typedef struct _CLIENT_OTA_INFO
{
	std::string id = std::string();
	std::string device_type = std::string();
	std::string version_name = std::string();
	std::string release_note = std::string();
	std::string download_path = std::string();
	std::string md5 = std::string();
	bool is_forced = false;
	bool enabled = false;
	unsigned int version_code = 0;
	unsigned int size = 0;
	unsigned int upgrade_scheme = 0;
	unsigned long update_time = 0;
	unsigned long create_time = 0;
	bool up_forced = false;

	void clearData()
	{
		id = std::string();
		device_type = std::string();
		version_name = std::string();
		release_note = std::string();
		download_path = std::string();
		md5 = std::string();
		is_forced = false;
		enabled = false;
		version_code = 0;
		size = 0;
		upgrade_scheme = 0;
		update_time = 0;
		create_time = 0;
		up_forced = false;
	}

}CLIENT_OTA_INFO, * pCLIENT_OTA_INFO;

typedef struct _QUEUE_SYS_MSG
{
	unsigned long news_id = 0;
	std::string  user_id = std::string();
	unsigned long  news_type = 0;
	std::string  content = std::string();
	std::string  url = std::string();
	std::string  url_note = std::string();
	std::string  title = std::string();
	std::string  title_note = std::string();
	std::string  content_note = std::string();
	std::string  device_type = std::string();

	unsigned long  create_time = 0;
	unsigned long  update_time = 0;
	unsigned long  status = 0;



	void clearData()
	{
		news_id = 0;
		user_id = std::string();
		news_type = 0;
		content = std::string();
		url = std::string();
		url_note = std::string();
		title = std::string();
		title_note = std::string();
		content_note = std::string();
		device_type = std::string();
		create_time = 0;
		update_time = 0;
		status = 0;
	}
}QUEUE_SYS_MSG, * pQUEUE_SYS_MSG;


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

	std::vector<COUNTRY_INFO>  countryInfo = {};
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

		countryInfo.clear();
		bool is_subscribe = false;
		mac_addr = std::string();
	}

}USER_INFO, * pUSER_INFO;

//
typedef struct _UPDATE_USER_INFO
{
	std::string user_id = std::string();
	std::string email = std::string();
	std::string nick_name = std::string();
	std::string avatar = std::string();
	void clearData()
	{
		user_id = std::string();
		email = std::string();
		nick_name = std::string();
		avatar = std::string();
	}

}UPDATE_USER_INFO, * pUPDATE_USER_INFO;

typedef struct _VEDIO_LIST_INFO
{
	unsigned long id = 0;
	std::string station_sn = std::string();
	std::string create_time = std::string();
	unsigned long duration = 0;

	std::string thumb = std::string();
	std::string video = std::string();
	std::string task_id = std::string();
	std::string model_name = std::string();
	std::string file_name = std::string();
	bool is_share = false;

	void clearData()
	{
		long id = 0;
		station_sn = std::string();
		create_time = std::string();
		duration = 0;

		thumb = std::string();
		video = std::string();
		task_id = std::string();
		model_name = std::string();
		file_name = std::string();
		bool is_share = false;
	}
}VEDIO_LIST_INFO, * pVEDIO_LIST_INFO;

typedef struct _VEDIO_CFG
{
	std::string station_sn = std::string();
	std::string station_name = std::string();
	std::string station_model = std::string();
	bool is_open = false;

	void clearData()
	{
		station_sn = std::string();
		station_name = std::string();
		station_model = std::string();
		is_open = false;
	}

}VEDIO_CFG, * pVEDIO_CFG;

///family/invite/add
//std::string invite_number = std::string();
//bool  email_sent = false;

///family/invite/pending
typedef struct _QUEUE_WAIT_INVITE
{
	unsigned long invite_id = 0;
	std::string action_nick = std::string();
	std::string station_sn = std::string();
	std::string email = std::string();
	std::string action_user_id = std::string();
	std::string action_email = std::string();
	std::string member_nick = std::string();

	unsigned long expires_time = 0;
	unsigned long create_time = 0;
	unsigned long update_time = 0;
	unsigned long permissions = 0;
	bool is_query = false;
	bool is_command = false;
	bool is_camera = false;

	void clearData()
	{
		invite_id = 0;
		action_nick = std::string();
		station_sn = std::string();
		email = std::string();
		action_user_id = std::string();
		action_email = std::string();
		member_nick = std::string();

		expires_time = 0;
		create_time = 0;
		update_time = 0;
		permissions = 0;
		is_query = false;
		is_command = false;
		is_camera = false;
	}
}QUEUE_WAIT_INVITE, * pQUEUE_WAIT_INVITE;

typedef struct _DEVICE_SHARED_MEMBER_LIST
{
	unsigned long record_id = 0;
	unsigned long record_type = 0;

	std::string station_sn = std::string();
	std::string email = std::string();
	std::string member_user_id = std::string();
	std::string member_nick = std::string();
	std::string member_avatar = std::string();

	unsigned long expires_time = 0;
	unsigned long create_time = 0;
	unsigned long update_time = 0;
	unsigned long permissions = 0;

	bool is_query = false;
	bool is_command = false;
	bool is_camera = false;

	void clearData()
	{
		record_id = 0;
		record_type = 0;

		station_sn = std::string();
		email = std::string();
		member_user_id = std::string();
		member_nick = std::string();
		member_avatar = std::string();

		expires_time = 0;
		create_time = 0;
		update_time = 0;
		permissions = 0;

		is_query = false;
		is_command = false;
		is_camera = false;
	}

}DEVICE_SHARED_MEMBER_LIST, * pDEVICE_SHARED_MEMBER_LIST;
};

namespace MqttType{

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
		GeneralException2Gui_System_Error,	 // Data Transmission Error. The system error, please try again.
		GeneralException2Gui_Advance_Pause, // A Gcode pause command has been executed. Check your print.
		GeneralException2Gui_Bed_Adhesion_Failure, // Bed Adhesion Failure
		GeneralException2Gui_Spaghetti_Mess, // Spaghetti Mess.
		GeneralException2Gui_HomingFailed, // Homing failed. Try again later.
		GeneralException2Gui_Break_Point, // No notice.
		GeneralException2Gui_Level_100_Times, // Before printing, we recommend using auto-level for better results. The process will take about 10 minutes.
		GeneralException2Gui_LowTemperature, // Low Temperature. For best results, we recommend printing in a location with an ambient temperature between 15°C - 35°C.
	};


typedef struct _ASYN_MQTT_DATA
{

}ASYN_MQTT_DATA, * pASYN_MQTT_DATA;

// Field definition of the 3D printing device reported to the server
enum Device_Info_List_Code
{
	
};


// http cmd
enum SELECT_CMD_TYPE
{
};
typedef std::map<SELECT_CMD_TYPE, void*> HttpsDataMap;
typedef std::shared_ptr< std::map<SELECT_CMD_TYPE, void*>> HttpsDataMapPtr;

enum CustomDeviceStatus {
	CustomDeviceStatus_Level_Finished,
	CustomDeviceStatus_Exception_Finished,
	CustomDeviceStatus_File_Transfer,
	CustomDeviceStatus_Max
};

enum mqtt_command_type_e
{
	MQTT_CMD_EVENT_NOTIFY,
	MQTT_CMD_PRINT_SCHEDULE,
	MQTT_CMD_FIRMWARE_VERSION,
	MQTT_CMD_NOZZLE_TEMP,
	MQTT_CMD_HOTBED_TEMP,
	MQTT_CMD_FAN_SPEED,
	MQTT_CMD_PRINT_SPEED,
	MQTT_CMD_AUTO_LEVELIN,
	MQTT_CMD_PRINT_CONTROL,
	MQTT_CMD_FILE_LIST_REQUEST,
	MQTT_CMD_GCODE_FILE_REQUEST,
	MQTT_CMD_ALLOW_FIRMWARE_UPDATE,
	MQTT_CMD_GCODE_FILE_DOWNLOAD,
	MQTT_CMD_Z_AXIS_RECOUP,
	MQTT_CMD_EXTRUSION_STEP,
	MQTT_CMD_ENTER_OR_QUIT_MATERIEL,
	MQTT_CMD_MOVE_STEP,
	MQTT_CMD_MOVE_DIRECTION,
	MQTT_CMD_MOVE_ZERO,
	MQTT_CMD_APP_QUERY_STATUS,
	MQTT_CMD_ONLINE_NOTIFY,
	MQTT_CMD_RECOVER_FACTORY,
	MQTT_CMD_SET_PRINT_MODE,
	MQTT_CMD_BLUETOOTH_BROADCAST_SWITCH,
	MQTT_CMD_DELETE_FILE,
	MQTT_CMD_GCODE_PARAMS_RESET,
	MQTT_CMD_SET_DEVICE_NAME,
	MQTT_CMD_REPORT_DEVICE_LOG,
	MQTT_CMD_IN_OUT_MODE,
	MQTT_CMD_SET_MOTOR_MUTEX,
	MQTT_CMD_NOZZLE_HOTRED_TEMP_PREHEADT,
	MQTT_CMD_BREAK_POINT_PRINT,
	MQTT_CMD_AI_CALIBRATION_EVENT,
	MQTT_CMD_TIME_RLY_VIDEO_ON_OFF,
	MQTT_CMD_HIGH_PARAMS_SET,
	MQTT_CMD_GCODE_API,
	MQTT_CMD_THUMBNAIL_UPLOAD_NOTICE,
	MQTT_CMD_DEVICE_SELF_TEST = 1049,
	MQTT_CMD_AI_THRESHOLD_ON_OFF_SET,
	MQTT_CMD_AI_INFOR_QUERY,
	MQTT_CMD_QUERY_LAYER,
	MQTT_CMD_QUERY_FILE_MAX_SPEED,
	MQTT_CMD_MAX,
};


DEF_PTR(CmdType)
class CmdType
{
public:
	mqtt_command_type_e type;
};

typedef std::map<mqtt_command_type_e, CmdTypePtr> MqttData;
typedef std::shared_ptr <std::map<std::string/*sn*/, std::map<mqtt_command_type_e, CmdTypePtr>>>  MqttDataMap;
typedef std::shared_ptr<std::map<std::string/*sn*/, std::map<mqtt_command_type_e/*cmd*/, CmdTypePtr>>> MqttDataMapPtr;
#define CREATE_MQTT_DATAMAP_PTR (std::make_shared<std::map<std::string, std::map<mqtt_command_type_e, CmdTypePtr>>>())


struct AiContext {
	std::string url = "";
	std::string aiId = "";
	int type = 1;
	unsigned long time;
	int aiImproving = 0;
};

DEF_PTR(NoticeData)
class NoticeData : public CmdType
{

};


DEF_PTR(PrintingNoticeData)
class PrintingNoticeData : public CmdType
{
public:
	int time = -1;
	int proccess = -1;
	std::string name = "";
	std::string img = "";
	int modelId = -1;
	int totalTime = -1;
	int filamentUsed = -1;
	std::string filamentUnit = "";
	int aiFlag = -1;
	int modelType = 1;
	int startLeftTime = 0;
	int saveTime = -1;

	int AIValue = 0;
	int AIAmbientLight = 0;
	int AIMaterialColor = 0;
	int AISwitch = 0;
	int AISensitivity = 0;
	int AIPausePrint = 0;
	int AIJoinImproving = 0;
	int AICameraValue = 0;
};

DEF_PTR(Temperature)
class Temperature : public CmdType
{

};

DEF_PTR(NozzleTemperature)
class NozzleTemperature : public Temperature
{
public:
};

DEF_PTR(HotRedTemperature)
class HotRedTemperature : public Temperature
{
public:
};
DEF_PTR(Cmd2Value)
class Cmd2Value : public CmdType
{

};

//"commandType" : 1028
DEF_PTR(Online)
class Online  : public Cmd2Value
{
	
};

DEF_PTR(AiCalibration)
class AiCalibration  : public Cmd2Value
{
public:
};

DEF_PTR(Thumb)
class Thumb: public CmdType
{
};

DEF_PTR(BreakPointNotice)
class BreakPointNotice : public Cmd2Value
{
public:
};


DEF_PTR(SystemFirewareVerison)
class SystemFirewareVerison : public CmdType
{

};


DEF_PTR(FanSpeed)
class FanSpeed : public Cmd2Value
{

};

DEF_PTR(PrintSpeed)
class PrintSpeed : public Cmd2Value
{

};


DEF_PTR(LevelProcess)
class LevelProcess : public Cmd2Value
{

};

enum PrintCtlResult {
	PrintCtlResult_Succeed,
	PrintCtlResult_Failed,
	PrintCtlResult_Reminder_leveling,
	PrintCtlResult_Print_no_file,
	PrintCtlResult_Printing,
	PrintCtlResult_No_set_mode
};

DEF_PTR(PrintCtl)
class PrintCtl : public Cmd2Value
{

};

DEF_PTR(PrintCtrlRequest)
class PrintCtrlRequest : public Cmd2Value
{

};


struct FileInfo {
	std::string name = ""; // "short.gcode"
	std::string path = ""; // "/tmp/udisk1/9/short.gcode"
	unsigned long long timestamp = 0; //  1645272982
};

#define QUEST_FILELIST_NUM 47
DEF_PTR(FileList)
class FileList : public  Cmd2Value
{
public:
	//std::map<unsigned long long, FileInfo> files;
	std::list<FileInfo> files;
	int isFirst = 1;
	int index = 1;
	int num = QUEST_FILELIST_NUM;

	inline void append(const FileList& fileList) {
		//files.insert(fileList.files.begin(), fileList.files.end());
		files.insert(files.end(), fileList.files.begin(), fileList.files.end());
		isFirst = fileList.isFirst;
		index = fileList.index;
		num = fileList.num;
		type = fileList.type;
	}

	static bool compare_timestamp(const FileInfo& a, const FileInfo& b) {
		return a.timestamp > b.timestamp;
	}

	inline void sortList() {
		files.sort(&FileList::compare_timestamp);
	}

	inline void clear() {
		files.clear();
		int isFirst = 1;
		int index = 1;
		int num = QUEST_FILELIST_NUM;
		int count = 0;
	}

	inline FileList& operator=(const FileList& info) {
		type = info.type;
		num = info.num;
		isFirst = info.isFirst;
		index = info.index;
		files = info.files;
		return *this;
	}

	inline bool operator==(const FileList& info) {
		return true;
	}

	inline bool operator!=(const FileList& info) {
		return !(*this == info);
	}
};


DEF_PTR(GCodeInfo)
class GCodeInfo : public CmdType
{
public:
	int normal = 0;
	std::string fileName = "";
	int panel = 0;
	int nozzel = 0;
	int speed = 0;
	int speedType = 0;
	int fans = 0;
	std::string completeUrl = "";
	int leftTime = 0;
	int displacement = 0;
	int filamentUsed = 0;
	std::string filamentUnit = "mm";
	int modelType = 1;
	int aiFlag = 0;
	int reply = 0;

	//20230426 add
	int printmode = 0; // 	PRINT_WORK_NORMAL(0), PRINT_WORK_FAST(1),  PRINT_WORK_FINE_K(2), PRINT_WORK_FINE_S(3), PRINT_WORK_MAX(4)
	int exceedDeviceSize = 0; //Whether the model size exceeds the equipment size.
};



DEF_PTR(GcodeDownloadProcess)
class GcodeDownloadProcess : public Cmd2Value
{
	
};


DEF_PTR(GcodeDownloadCtrl)
class GcodeDownloadCtrl : public Cmd2Value
{
};


DEF_PTR(ZAxisCompensation)
class ZAxisCompensation : public Cmd2Value
{

};


DEF_PTR(ExtrusionInfo)
class ExtrusionInfo : public Cmd2Value
{
};

DEF_PTR(ExtrusionCtrl)
class ExtrusionCtrl : public CmdType
{
};

DEF_PTR(MoveStep)
class MoveStep : public Cmd2Value
{

};

DEF_PTR(AxisMove)
class AxisMove : public Cmd2Value
{

};

DEF_PTR(Move2Zero)
class Move2Zero : public Cmd2Value
{

};

DEF_PTR(GCodeResetParam)
class GCodeResetParam : public CmdType
{
};

DEF_PTR(MotorMutex)
class MotorMutex : public Cmd2Value
{

};


DEF_PTR(TemperaturePreheating)
class TemperaturePreheating : public  Cmd2Value
{

};

DEF_PTR(TemperaturePreheatingCtrl)
class TemperaturePreheatingCtrl : public  Cmd2Value
{
};


DEF_PTR(GcodeCmdApi)
class GcodeCmdApi : public CmdType
{
};

DEF_PTR(DeviceSelfCheck)
class DeviceSelfCheck : public CmdType
{
};


DEF_PTR(ThresholdValue)
class ThresholdValue : public CmdType
{
};

DEF_PTR(ThresholdValueCtrl)
class ThresholdValueCtrl : public ThresholdValue
{
};


DEF_PTR(AiWarningInfo)
class AiWarningInfo : public CmdType
{	  	
};


DEF_PTR(PliesValue)
class PliesValue : public CmdType
{
};


DEF_PTR(FileMaxSpeed)
class FileMaxSpeed : public CmdType
{
};

};

#endif // !ANKER_NET_BASETYPE_HPP

