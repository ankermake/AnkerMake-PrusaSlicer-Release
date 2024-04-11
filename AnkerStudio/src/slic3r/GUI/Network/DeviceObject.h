#ifndef NETWORK_DEVICE_OBJECT_H
#define NETWORK_DEVICE_OBJECT_H

#include <jansson.h>
#include "anker_net/business/mqttBusiness/mqttprotocolbase.h"
#include "basetype.hpp"
#include "MsgText.hpp"
#include <boost/signals2.hpp>
#include <mutex>



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

    MqttType::GeneralException2Gui generalException = MqttType::GeneralException2Gui_No_Error;

    std::string fileName = ""; // Picked file name, from mqtt.
    std::string printFile = ""; // Picked print file, from mqtt.

    int progress = 0; // progress X100 from mqtt.
    int64_t time = 0; // Print remaining time is second, from mqtt.
    int64_t totalTime = 0; // Print accumulated time cost, from mqtt.

    mqtt_device_type deviceType = DEVICE_UNKNOWN_TYPE; 
    mqtt_device_parts_type devicePartsType = DEVICE_PARTS_NO; // Default no device parts. 

    int speed = 0; //From mqtt.
    int nozzle[2] = { 0 }; // Nozzle current/target Temperature. From mqtt.
    int hotdBed[2] = { 0 }; // HotRed current/target Temperature. From mqtt.

    MqttType:: FileList deviceFileList; // From mqtt.
    std::recursive_mutex m_deviceFileListMutex = {};

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
    MqttType::CustomDeviceStatus customDeviceStatus = MqttType::CustomDeviceStatus_Max;
    int autoLevelingTotalValue = 49; // Auto leveling total progress

    std::string thumbnail = "";

    int totalLayer = 0;
    int currentLayer = 0;

    int remainTime = 0;
    bool needLevel = false;
   
    MqttType::MqttData* pMqttData = nullptr;
    MqttType::mqtt_command_type_e currentCmdType = MqttType::MQTT_CMD_MAX;

    void initStatus();
    void setDeviceFileList(const MqttType::FileList& files);
    void sortDeviceFileList();
    void clearDeviceFileList();
    void appendDeviceFileList(const MqttType::FileList& files);
    int getDeviceFileListSize();
    MqttType::FileList getDeviceFileList();

    void checkGeneralException2Gui();



    void printComputerLocalFile(const std::string& fullPath);

    MqttType::MuticolorSlotChangePtr getMuticolorSlotChangeNotice() const;
    MqttType::MulticolorAccessoryBoxStatusPtr getMulticolorAccessoryBoxStatus()  const;

    MqttType::CustomDeviceStatus getCustomDeviceStatus();

    //Mqtt notice message
    MqttType::ThumbPtr getThumbInfo();

    // Printing notice.
    MqttType::PrintingNoticeDataPtr getPrintingNotice();

    // Mqtt Query CMD.
    void querySystemFirmwareVersion();
    std::string getSystemFirmwareVersion();
    void queryDeviceAllStatus();   

    // Mqtt Ctrl CMD.
    void setDevicePrintBegin(const std::string &filepath);
    void setDevicePrintPause(const std::string& filepath = "");
    void setDevicePrintStop(const std::string& filepath = "");
    void setDevicePrintResume(const std::string& filepath = "");
    void setDevicePrintAgain(const std::string& filepath = "");


    void clearDeviceCtrlResult();
    void resetDeviceIdel();

    void clearRequestDeviceCmdType();
    void getDeviceLocalFileLists(MqttType::FileList data = MqttType::FileList());
    void getDeviceUsbFileLists(MqttType::FileList data = MqttType::FileList());
    void getDeviceFileLists(const MqttType::FileList& data);
    void setBedTargetTemperature(int value);
    MqttType::TemperaturePtr getBedTargetTemperature();

    void setNozzleTargetTemperature(int value);
    MqttType::TemperaturePtr getNozzleTargetTemperature();
    void setFanSpeed(int value); // eg: 25%
    int getFanSpeed();
    void setPrintingSpeed(int value);  
    int getPrintingSpeed();

    void setPreheadtBegin(int nozzleValue, int heatbedValue, int nozzelNum = -1);
    void setPreheadtStop(int nozzelNum = -1);
    int getPreheadtResult();

    void setCalibrationBegin();
    void setCalibrationStop();
    void setCalibrationQueryStatus();
    MqttType::MulticolorCalibrationPtr getCalibrationInfo() const;

    void setMultiColorConsumableEditing(const MqttType::MulticolorConsumableEditingCtrl& data);
    MqttType::MulticolorConsumableEditingPtr getConsumableEditing() const;

    void queryDeviceCanPrint();

    void setAccessoriesIncubatorOn();
    void setAccessoriesIncubatorOff();

    void setAccessInstructionDefault();
    void setAccessInstructionConnectedAndUninitialized();
    void setAccessInstructionConnectedAndInitialized();

    void setLevelBegin();
    void setLevelTightAlignmentBegin();
    void setLevelCoarseBegin();
    void setLevelStop();
    MqttType::LevelProcessPtr getProgress() const;
    int getLevelLocationValue();

    void setGCodeDownload(const MqttType::GcodeDownloadCtrl& data);
    int getGCodeDownloadProcess();
    void setZAxisCompensation(float value);
    float getZAxisCompensationValue();
    void setDischargeExtrusion(int stepLen, int temperature, int nozzleNum = -1);
    void setMaterialReturnExtrusion(int stepLen, int temperature, int nozzleNum = -1);
    void setStopExtrusion();
    MqttType::ExtrusionInfoPtr getExtrusionInfo() const;

    void setRequestGCodeInfo(const std::string& filepath, int type = 0);   //  type: 0(Select file.) , 1(Not select file.)
    MqttType::GCodeInfoPtr getGcodeInfo() const;

    void setMoveStep(int value);
    int getMoveStepResult();

    void setXAxisMove(int value);
    void setYAxisMove(int value);
    void setZAxisMove(int value);

    void setXYMoveZero();
    void setZMoveZero();
    void setAllMoveZero();

    void setRestoreFactorySettings();
    void setResetNetwork();

    void setBroadcastOn();
    void setBroadcastOff();

    void setDeleteDeviceFile(const std::string& filePath);
    void setResetGcodeParams();

    void setDeviceName(const std::string& name);

    void setUploadlog();

    void setModalOn();
    void setModalOff();

    void setMotorOn();
    void setMotorOff();

    void setStopTemperaturePreheating();
    void setStartTemperaturePreheating(int nozzle, int heatbed);
    MqttType::TemperaturePreheatingPtr getTemperaturePreheating() const;

    void setPowerOutageContinuationNo();
    void setPowerOutageContinuationYes();
    void setPowerOutageContinuationHave();

    void setDelayedVideoOn();
    void setDelayedVideoOff();

    void setSwitchingNozzle(int value = 0);

    void setGcodeCmd(const std::string& cmd, int len);

    void setDeviceCheckSelfOn();
    void setDeviceCheckSelfOff();
    void setDeviceCheckSelfQueryRemainder();

    void setAIThreshold(const MqttType::ThresholdValueCtrl& value);
    void setAIWaringQuery(int id);
    void setLayerQuery();
    void setFileMaxSpeedQuery();

    void clearDeviceExceptionInfo(); // M5C

	// P2P video
	void setVideoPlay();
	void setVideoStop(int reason = 0);
	void setVideoMode(int mode);  // 1: HD; 2:Smooth
	void setCameraLight(bool onOff);
    void videoStopSussSlot(const std::string& sn);

    void sendMsgSig();

    DeviceObject& operator=(const DeviceObject& info);
    bool operator==(const DeviceObject& info);
    bool operator!=(const DeviceObject& info);

private:
    MqttType::NozzleModuleSwitchTypePtr GetNozzleModuleSwitchInst();
    void CheckExtrusionTemperature(int &temperature);

    void publishMessage(const std::string& topicStr, const std::string& str);
    std::string getUserId();
    std::string getNickName();
    std::string getUserName();

    void setAutoLevelStatus(int value, int mode = -1); // 0: begin, 1: stop
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

    void setCalibration(int value = 0);
    void setAccessoriesIncubator(int value = 0);
    void setAccessInstruction(int value = 0);

    void sendMsgSig(const NetworkMsgText::NetworkMsg& msg);
    
    std::queue<NetworkMsgText::NetworkMsg> m_msgs;

    boost::signals2::connection m_videoStopSussConnect;
};


#endif // !NETWORK_DEVICE_OBJECT_H
