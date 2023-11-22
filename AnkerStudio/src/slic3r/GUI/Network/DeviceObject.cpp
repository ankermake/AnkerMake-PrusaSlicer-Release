#include "DeviceObject.h"
#include "../../Utils/DataManger.hpp"
#include "libslic3r/Utils.hpp"
#include <boost/bind.hpp>

using namespace MqttType;
using namespace NetworkMsgText;

std::string topicPrefix = "";
std::string topicQuerySuffix = "";
std::string topicCtrlSuffix = "";


void DeviceObject::setDeviceFileList(const MqttType::FileList& files)
{
    std::unique_lock<std::recursive_mutex> lock(m_deviceFileListMutex);
    deviceFileList = files;
}

void DeviceObject::sortDeviceFileList()
{

}

void DeviceObject::clearDeviceFileList()
{

}

int DeviceObject::getDeviceFileListSize()
{
    return 0;
}

void DeviceObject::appendDeviceFileList(const FileList& files)
{

}

MqttType::FileList DeviceObject::getDeviceFileList()
{
    std::unique_lock<std::recursive_mutex> lock(m_deviceFileListMutex);
    return deviceFileList;
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

MqttType::MuticolorSlotChangePtr DeviceObject::getMuticolorSlotChangeNotice()  const
{
    return nullptr;
}

MqttType::MulticolorAccessoryBoxStatusPtr DeviceObject::getMulticolorAccessoryBoxStatus()  const
{

    return MqttType::MulticolorAccessoryBoxStatusPtr();
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

MqttType::NozzleModuleSwitchTypePtr DeviceObject::GetNozzleModuleSwitchInst()
{
    return MqttType::NozzleModuleSwitchTypePtr();
}

void DeviceObject::CheckExtrusionTemperature(int& temperature)
{

}

void DeviceObject::initStatus()
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

void DeviceObject::clearRequestDeviceCmdType()
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

void DeviceObject::setPreheadtBegin(int nozzleValue, int heatbedValue, int nozzelNum)
{

}


void DeviceObject::setPreheadtStop(int nozzelNum)
{

}

int DeviceObject::getPreheadtResult()
{
    int res = 0;

    return res;
}

void DeviceObject::setCalibrationBegin()
{
    setCalibration(0);
}

void DeviceObject::setCalibrationStop()
{
    setCalibration(1);
}

void DeviceObject::setCalibrationQueryStatus()
{
    setCalibration(2);
}

MqttType::MulticolorCalibrationPtr DeviceObject::getCalibrationInfo() const
{

    return MqttType::MulticolorCalibrationPtr();
}

void DeviceObject::setMultiColorConsumableEditing(const MqttType::MulticolorConsumableEditingCtrl& data)
{

}

MqttType::MulticolorConsumableEditingPtr DeviceObject::getConsumableEditing() const
{

    return MqttType::MulticolorConsumableEditingPtr();
}

void DeviceObject::queryDeviceCanPrint()
{

}

void DeviceObject::setAccessoriesIncubatorOn()
{
    setAccessoriesIncubator(1);
}

void DeviceObject::setAccessoriesIncubatorOff()
{
    setAccessoriesIncubator(0);
}

void DeviceObject::setAccessInstructionDefault()
{
    setAccessInstruction();
}

void DeviceObject::setAccessInstructionConnectedAndUninitialized()
{
    setAccessInstruction(1);
}

void DeviceObject::setAccessInstructionConnectedAndInitialized()
{
    setAccessInstruction(2);
}

void DeviceObject::setAutoLevelStatus(int value, int mode)
{

}

void DeviceObject::setPrintCtrl(const PrintCtrlRequest& data)
{

}


void DeviceObject::clearDeviceCtrlResult()
{

}

void DeviceObject::resetDeviceIdel()
{

}

void DeviceObject::getDeviceFileLists(const FileList& data)
{

}

void DeviceObject::setLevelBegin()
{
    clearDeviceCtrlResult();
    setAutoLevelStatus(0);
}
void DeviceObject::setLevelTightAlignmentBegin()
{
    clearDeviceCtrlResult();
    setAutoLevelStatus(0, 0);
}
void DeviceObject::setLevelCoarseBegin()
{
    clearDeviceCtrlResult();
    setAutoLevelStatus(0, 1);
}
void DeviceObject::setLevelStop()
{
    setAutoLevelStatus(1);
}

LevelProcessPtr DeviceObject::getProgress() const
{

    return LevelProcessPtr();
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

void DeviceObject::setDischargeExtrusion(int stepLen, int temperature, int nozzleNum)
{

}

void DeviceObject::setMaterialReturnExtrusion(int stepLen, int temperature, int nozzleNum)
{

}

void DeviceObject::setStopExtrusion()
{

}

ExtrusionInfoPtr  DeviceObject::getExtrusionInfo() const
{

    return nullptr;
}

void DeviceObject::setRequestGCodeInfo(const std::string& filepath, int type)
{

}

MqttType::GCodeInfoPtr DeviceObject::getGcodeInfo() const
{

    return nullptr;
}

void DeviceObject::setSwitchingNozzle(int value)
{

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
    setAxisMove("x", value);
}

void DeviceObject::setYAxisMove(int value)
{
    setAxisMove("y", value);
}

void DeviceObject::setZAxisMove(int value)
{
    setAxisMove("z", value);
}

void DeviceObject::setXYMoveZero()
{
    setMoveZero(0);
}

void DeviceObject::setZMoveZero()
{
    setMoveZero(1);
}

void DeviceObject::setAllMoveZero()
{
    setMoveZero(2);
}

void DeviceObject::setRestoreFactorySettings()
{
    setReset(1);
}

void DeviceObject::setResetNetwork()
{
    setReset(2);
}

void DeviceObject::setBroadcastOn()
{
    setBroadcast(1);
}

void DeviceObject::setBroadcastOff()
{
    setBroadcast(0);
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
    setModal(1);
}

void DeviceObject::setModalOff()
{
    setModal(0);
}

void DeviceObject::setMotorOn()
{
    setMotorLock(1);
}

void DeviceObject::setMotorOff()
{
    setMotorLock(0);
}

void DeviceObject::setStopTemperaturePreheating()
{

}

void DeviceObject::setStartTemperaturePreheating(int nozzle, int heatbed)
{

}

TemperaturePreheatingPtr DeviceObject::getTemperaturePreheating() const
{

    return TemperaturePreheatingPtr();
}

void DeviceObject::setPowerOutageContinuationNo()
{
    setPowerOutageContinuation(0);
}

void DeviceObject::setPowerOutageContinuationYes()
{
    setPowerOutageContinuation(1);
}

void DeviceObject::setPowerOutageContinuationHave()
{
    setPowerOutageContinuation(2);
}

void DeviceObject::setDelayedVideoOn()
{
    setDelayedVideo(0);
}

void DeviceObject::setDelayedVideoOff()
{
    setDelayedVideo(1);
}

void DeviceObject::setGcodeCmd(const std::string& cmd, int len)
{

}

void DeviceObject::setDeviceCheckSelfOn()
{
    setDeviceCheckSelf(0);
}

void DeviceObject::setDeviceCheckSelfOff()
{
    setDeviceCheckSelf(1);
}

void DeviceObject::setDeviceCheckSelfQueryRemainder()
{
    setDeviceCheckSelf(2);
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

void DeviceObject::clearDeviceExceptionInfo()
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

void DeviceObject::setCalibration(int value)
{

}

void DeviceObject::setAccessoriesIncubator(int value)
{

}

void DeviceObject::setAccessInstruction(int value)
{

}

void DeviceObject::sendMsgSig(const NetworkMsg& msg)
{
    m_msgs.push(msg);  
    if (m_msgs.size() == 1) {
        sendMsgSig();
    }
}

void DeviceObject::sendMsgSig() 
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
    return false;
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
    std::string usrid = Datamanger::GetInstance().m_userInfo.user_id;
    return usrid;
}

std::string DeviceObject::getNickName()
{
    return Datamanger::GetInstance().m_userInfo.nick_name;
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

