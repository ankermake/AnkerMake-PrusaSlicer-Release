#ifndef NETWORK_DEVICE_OBJECT_H
#define NETWORK_DEVICE_OBJECT_H

#include "mqttprotocolbase.h"
#include "basetype.hpp"
#include "MsgText.hpp"
#include <boost/signals2.hpp>

DEF_PTR(DeviceObject)
class DeviceObject
{
public:
    std::string station_id = ""; // From server.
    std::string station_name = ""; // From server.
    std::string station_model = ""; // From server.
    std::string m_sn = ""; // From server.

    bool onlined = false; // Mqtt onlined from server/mqtt.
    bool is_command = false; // ctrl Permission From server.
    bool is_camera = false; 
    mqtt_print_event_e deviceStatus = MQTT_PRINT_EVENT_IDLE; // Print status from mqtt.
    mqtt_marlin_alert_event_e exceptionStatus = MQTT_MARLIN_ALERT_NOWARING;; // marlin exception status from mqtt.
    mqtt_ai_alert_event_e aiWarningStatus = MQTT_AI_ALERT_MAX; // AI warning from mqtt.

    MqttType::GeneralException2Gui generalException;

    std::string fileName = ""; // Picked file name, from mqtt.
    std::string printFile = ""; // Picked print file, from mqtt.

    int progress = 0; // progress X100 from mqtt.
    int64_t time = 0; // Print remaining time is second, from mqtt.
    int64_t totalTime = 0; // Print accumulated time cost, from mqtt.

    mqtt_device_type deviceType = DEVICE_V8111_TYPE; // Default Device is V8111, x5.0, from server.

    int speed = 0; //From mqtt.
    int nozzle[2] = { 0 }; // Nozzle current/target Temperature. From mqtt.
    int hotdBed[2] = { 0 }; // HotRed current/target Temperature. From mqtt.

    MqttType:: FileList deviceFileList; // From mqtt.


    int filamentUsed = 0; // Estimate the required material length for printing, in millimeters.  From mqtt.
    std::string filamentUnit = "mm"; // From mqtt.

    int aiFlag = 0; // 0: No AI, 1: Yes   From mqtt.
    int modelType = 1; // 0:Cura, 1:Anker, 2:Simplfy3d, 3:Prusa, 4:Supper, 5:Unknown   From mqtt.
    int startLeftTime = 0; // 0: Countdown remaining time before starting, 1:Countdown remaining time to start.      From mqtt.
    int64_t saveTime = 0; // Report Save time, seconds.     From mqtt.

    int aiValue = 90; // Value(1-100)     From mqtt.
    int aiCameraValue = 1; // AI Camera Check Result, 1: success, 0: failed     From mqtt.
    int aiAmbientLight = 4; // Ambient Light Value(0-4)     From mqtt.
    int aiMaterialColor = 60;    // From mqtt.

    int printRequestResult = 0; // 0: sucess, 1:failed, 2: need level, 3:print file not found, 4:printing, 5:no set mode     From mqtt.
    int autoLevelingValue = 0; // Auto leveling progress.    From mqtt.
    MqttType::CustomDeviceStatus customDeviceStatus;
    int autoLevelingTotalValue = 49; // Auto leveling total progress

    std::string thumbnail = "";

    int totalLayer = 0;
    int currentLayer = 0;

    int remainTime = 0;
    bool needLevel = false;
    
    
    MqttType::MqttData* pMqttData = nullptr;
    MqttType::mqtt_command_type_e currentCmdType;


    void print();

    virtual void checkGeneralException2Gui();


    virtual void printComputerLocalFile(const std::string &fullPath);

    MqttType::CustomDeviceStatus getCustomDeviceStatus();

    //Mqtt notice message
    MqttType::ThumbPtr getThumbInfo();

    // Printing notice.
    MqttType::PrintingNoticeDataPtr getPrintingNotice();

    // Mqtt Query CMD.
    virtual void querySystemFirmwareVersion();
    virtual std::string getSystemFirmwareVersion();
    virtual void        queryDeviceAllStatus();


    // Mqtt Ctrl CMD.
    virtual void             setDevicePrintBegin(const std::string &filepath);
    virtual void             setDevicePrintPause(const std::string &filepath = "");
    virtual void             setDevicePrintStop(const std::string &filepath = "");
    virtual void             setDevicePrintResume(const std::string &filepath = "");
    virtual void             setDevicePrintAgain(const std::string &filepath = "");
    MqttType::PrintCtlResult getDeviceCtrlResult();

    virtual void             getDeviceLocalFileLists(MqttType::FileList data = MqttType::FileList());
    virtual void             getDeviceUsbFileLists(MqttType::FileList data = MqttType::FileList());
    virtual void             getDeviceFileLists(const MqttType::FileList &data);
    virtual void             setBedTargetTemperature(int value);
    MqttType::TemperaturePtr getBedTargetTemperature();

    virtual void             setNozzleTargetTemperature(int value);
    MqttType::TemperaturePtr getNozzleTargetTemperature();
    virtual void             setFanSpeed(int value); // eg: 25%
    virtual int              getFanSpeed();
    virtual void             setPrintingSpeed(int value);  
    virtual int              getPrintingSpeed();

    virtual void setPreheadtBegin(int nozzleValue, int heatbedValue);
    virtual void setPreheadtStop();
    virtual int  getPreheadtResult();

    virtual void setLevelBegin();
    virtual void setLevelStop();
    virtual int  getLevelLocationValue();

    virtual void               setGCodeDownload(const MqttType::GcodeDownloadCtrl &data);
    virtual int                getGCodeDownloadProcess();
    virtual void               setZAxisCompensation(float value);
    virtual float              getZAxisCompensationValue();
    virtual void               setDischargeExtrusion(int stepLen, int temperature);
    virtual void               setMaterialReturnExtrusion(int stepLen, int temperature);
    virtual void               setStopExtrusion();
    MqttType::ExtrusionInfoPtr getExtrusionInfo();

    virtual void           setRequestGCodeInfo(const std::string &filepath, int type = 0); //  type: 0(Select file.) , 1(Not select file.)
    MqttType::GCodeInfoPtr getGcodeInfo();

    virtual void setMoveStep(int value);
    virtual int  getMoveStepResult();

    virtual void setXAxisMove(int value);
    virtual void setYAxisMove(int value);
    virtual void setZAxisMove(int value);

    virtual void setXYMoveZero();
    virtual void setZMoveZero();
    virtual void setAllMoveZero();

    virtual void setRestoreFactorySettings();
    virtual void setResetNetwork();

    virtual void setBroadcastOn();
    virtual void setBroadcastOff();

    virtual void setDeleteDeviceFile(const std::string &filePath);
    virtual void setResetGcodeParams();

    virtual void setDeviceName(const std::string &name);

    virtual void setUploadlog();

    virtual void setModalOn();
    virtual void setModalOff();

    virtual void setMotorOn();
    virtual void setMotorOff();

    virtual void setStopTemperaturePreheating();
    virtual void setStartTemperaturePreheating(int nozzle, int heatbed);

    virtual void setPowerOutageContinuationNo();
    virtual void setPowerOutageContinuationYes();
    virtual void setPowerOutageContinuationHave();

    virtual void setDelayedVideoOn();
    virtual void setDelayedVideoOff();

    virtual void setGcodeCmd(const std::string &cmd, int len);

    virtual void setDeviceCheckSelfOn();
    virtual void setDeviceCheckSelfOff();
    virtual void setDeviceCheckSelfQueryRemainder();

    virtual void setAIThreshold(const MqttType::ThresholdValueCtrl &value);
    virtual void setAIWaringQuery(int id);
    virtual void setLayerQuery();
    virtual void setFileMaxSpeedQuery();

	// P2P video
    virtual void setVideoPlay();
    virtual void setVideoStop(int reason = 0);
    virtual void setVideoMode(int mode); // 1: HD; 2:Smooth
    virtual void setCameraLight(bool onOff);
    virtual void videoStopSussSlot(const std::string &sn);

    DeviceObject& operator=(const DeviceObject& info);
    bool operator==(const DeviceObject& info);
    bool operator!=(const DeviceObject& info);

private:
    void publishMessage(const std::string& topicStr, const std::string& str);
    std::string getUserId();
    std::string getNickName();
    std::string getUserName();
    


    void setAutoLevelStatus(int value); // 0: begin, 1: stop
    void setPrintCtrl(const MqttType::PrintCtrlRequest& data);
    
    void setExtrusion(const MqttType::ExtrusionCtrl& data);
    void setAxisMove(const std::string& axis, int value);
    void setMoveZero(int axis); // 0: XY axis to zero, 1: Z axis to zero, 2: all axis to zero
    void setReset(int value); // 1: Restore factory settings, 2: Network Reset
    void setBroadcast(int value);
    void setModal(int value);
    void setMotorLock(int value);
    void setTemperaturePreheating(const MqttType::TemperaturePreheatingCtrl& value);
    void setPowerOutageContinuation(int value);
    void setDelayedVideo(int value);
    void setDeviceCheckSelf(int value);

    boost::signals2::connection m_videoStopSussConnect;
};


#endif // !NETWORK_DEVICE_OBJECT_H
