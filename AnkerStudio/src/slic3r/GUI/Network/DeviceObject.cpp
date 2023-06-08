#include "DeviceObject.h"
#include "../../Utils/DataManger.hpp"
#include <boost/bind.hpp>


using namespace MqttType;
using namespace NetworkMsgText;


void DeviceObject::print()
{
}

void DeviceObject::checkGeneralException2Gui()
{
    
}

void DeviceObject::videoStopSussSlot(const std::string& sn)
{

}

void DeviceObject::printComputerLocalFile(const std::string& fullPath)
{
   
}

MqttType::CustomDeviceStatus DeviceObject::getCustomDeviceStatus()
{
    CustomDeviceStatus stat = CustomDeviceStatus_Max;
    return stat;
}

MqttType::ThumbPtr DeviceObject::getThumbInfo()
{
    return nullptr;
}

MqttType::PrintingNoticeDataPtr DeviceObject::getPrintingNotice()
{
    return nullptr;
}

void DeviceObject::querySystemFirmwareVersion()
{

}

std::string DeviceObject::getSystemFirmwareVersion()
{
    std::string version = "";

    return version;
}

void DeviceObject::queryDeviceAllStatus()
{
}

void  DeviceObject::setDevicePrintBegin(const std::string& filepath)
{
    
}

void  DeviceObject::setDevicePrintPause(const std::string& filepath)
{
}

void  DeviceObject::setDevicePrintStop(const std::string& filepath)
{
}

void DeviceObject::setDevicePrintResume(const std::string& filepath)
{
}

void DeviceObject::setDevicePrintAgain(const std::string& filepath)
{
}

void  DeviceObject::getDeviceLocalFileLists(FileList data)
{
}

void  DeviceObject::getDeviceUsbFileLists(FileList data)
{
}

void  DeviceObject::setBedTargetTemperature(int value)
{
}

TemperaturePtr DeviceObject::getBedTargetTemperature()
{   
    return nullptr;
}

void  DeviceObject::setNozzleTargetTemperature(int value)
{
}

TemperaturePtr DeviceObject::getNozzleTargetTemperature()
{
    return nullptr;
}

void DeviceObject::setFanSpeed(int value)
{

}

int DeviceObject::getFanSpeed()
{
    int res = 0;
    return res;
}

void DeviceObject::setPrintingSpeed(int value)
{

}

int DeviceObject::getPrintingSpeed()
{
    int res = 0;
    return res;
}

void DeviceObject::setPreheadtBegin(int nozzleValue, int heatbedValue)
{

}


void DeviceObject::setPreheadtStop()
{

}

int DeviceObject::getPreheadtResult()
{
    int res = 0;
    return res;
}

void DeviceObject::setAutoLevelStatus(int value)
{
}

void DeviceObject::setPrintCtrl(const PrintCtrlRequest& data)
{
}

PrintCtlResult DeviceObject::getDeviceCtrlResult()
{
    PrintCtlResult res = PrintCtlResult_Failed;
    return res;
}

void DeviceObject::getDeviceFileLists(const FileList& data)
{

}

void DeviceObject::setLevelBegin()
{

}
void DeviceObject::setLevelStop()
{
}

int DeviceObject::getLevelLocationValue()
{
    int res = 0;
    return res;
}

void DeviceObject::setGCodeDownload(const GcodeDownloadCtrl& data)
{

}

int DeviceObject::getGCodeDownloadProcess()
{
    int res = 0;

    return res;
}

void DeviceObject::setZAxisCompensation(float value)
{
}

float DeviceObject::getZAxisCompensationValue()
{
    float res = 0.0;
    return res;
}

void DeviceObject::setDischargeExtrusion(int stepLen, int temperature)
{

}

void DeviceObject::setMaterialReturnExtrusion(int stepLen, int temperature)
{

}

void DeviceObject::setStopExtrusion()
{

}

ExtrusionInfoPtr  DeviceObject::getExtrusionInfo()
{
    return nullptr;
}

void DeviceObject::setRequestGCodeInfo(const std::string& filepath, int type)
{
}

MqttType::GCodeInfoPtr DeviceObject::getGcodeInfo()
{
    return nullptr;
}

void DeviceObject::setMoveStep(int value)
{
}

int DeviceObject::getMoveStepResult()
{
    return 0;
}

void DeviceObject::setXAxisMove(int value)
{
}

void DeviceObject::setYAxisMove(int value)
{
}

void DeviceObject::setZAxisMove(int value)
{
}

void DeviceObject::setXYMoveZero()
{
}

void DeviceObject::setZMoveZero()
{
}

void DeviceObject::setAllMoveZero()
{
}

void DeviceObject::setRestoreFactorySettings()
{
}

void DeviceObject::setResetNetwork()
{
}

void DeviceObject::setBroadcastOn()
{
}

void DeviceObject::setBroadcastOff()
{
}

void DeviceObject::setDeleteDeviceFile(const std::string& filePath)
{
}

void DeviceObject::setResetGcodeParams()
{
}

void DeviceObject::setDeviceName(const std::string& name)
{
}

void DeviceObject::setUploadlog()
{
}

void DeviceObject::setModalOn()
{
}

void DeviceObject::setModalOff()
{
}

void DeviceObject::setMotorOn()
{
}

void DeviceObject::setMotorOff()
{
}

void DeviceObject::setStopTemperaturePreheating()
{
}

void DeviceObject::setStartTemperaturePreheating(int nozzle, int heatbed)
{
}

void DeviceObject::setPowerOutageContinuationNo()
{
}

void DeviceObject::setPowerOutageContinuationYes()
{
}

void DeviceObject::setPowerOutageContinuationHave()
{
}

void DeviceObject::setDelayedVideoOn()
{
}

void DeviceObject::setDelayedVideoOff()
{
}

void DeviceObject::setGcodeCmd(const std::string& cmd, int len)
{
}

void DeviceObject::setDeviceCheckSelfOn()
{
 
}

void DeviceObject::setDeviceCheckSelfOff()
{

}

void DeviceObject::setDeviceCheckSelfQueryRemainder()
{
}

void DeviceObject::setAIThreshold(const MqttType::ThresholdValueCtrl& value)
{
}

void DeviceObject::setAIWaringQuery(int id)
{
}

void DeviceObject::setLayerQuery()
{
}

void DeviceObject::setFileMaxSpeedQuery()
{
}


void DeviceObject::setMoveZero(int axis)
{
}

void DeviceObject::setReset(int value)
{
}

void DeviceObject::setBroadcast(int value)
{
}

void DeviceObject::setModal(int value)
{
}

void DeviceObject::setMotorLock(int value)
{
}

void DeviceObject::setTemperaturePreheating(const MqttType::TemperaturePreheatingCtrl& data)
{
}

void DeviceObject::setPowerOutageContinuation(int value)
{
}

void DeviceObject::setDelayedVideo(int value)
{
}

void DeviceObject::setDeviceCheckSelf(int value)
{
}

void DeviceObject::setExtrusion(const MqttType::ExtrusionCtrl& data)
{
}

void DeviceObject::setAxisMove(const std::string& axis, int value)
{

}

DeviceObject& DeviceObject::operator=(const DeviceObject& info)
{
    return *this;
}

bool DeviceObject::operator==(const DeviceObject& info)
{
    return (true);
}

bool DeviceObject::operator!=(const DeviceObject& info)
{
    return !(*this == info);
}

void DeviceObject::publishMessage(const std::string& topicStr, const std::string& str)
{
    
}

std::string DeviceObject::getUserId()
{
    return "";
}

std::string DeviceObject::getNickName()
{
    return "";
}

std::string DeviceObject::getUserName()
{
    return std::string();
}



void DeviceObject::setVideoPlay()
{

}

void DeviceObject::setVideoStop(int reason)
{
}

void DeviceObject::setVideoMode(int mode)
{

}

void DeviceObject::setCameraLight(bool onOff) 
{

}

