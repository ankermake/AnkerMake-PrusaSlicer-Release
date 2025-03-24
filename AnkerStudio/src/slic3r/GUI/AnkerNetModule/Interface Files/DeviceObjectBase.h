#ifndef DEVICE_OBJECT_BASE_H
#define DEVICE_OBJECT_BASE_H

#include "AnkerNetDefines.h"

namespace AnkerNet
{
	
DEF_PTR(DeviceObjectBase)
class DeviceObjectBase
{
public:
    virtual ~DeviceObjectBase(){};

    //Get Device Name
    virtual std::string GetStationName() = 0;
    //Get Device Serial Number
    virtual std::string GetSn() = 0;
    //Get Device Online State
    virtual bool GetOnline() = 0;

    //Get Device Function Status
    virtual aknmt_print_event_e GetDeviceStatus() = 0;
    virtual GeneralException2Gui GetGeneralException() = 0;

    virtual std::string GetPrintFile() = 0;
    virtual std::string GetFileName() = 0;

    virtual void GetMsgCenterInfo(std::string&errorCode,std::string& errorLevel) = 0;    

    virtual int GetProcess() = 0;
    virtual int64_t GetTime() = 0;

    virtual anker_device_type GetDeviceType() = 0;
    virtual anker_device_parts_type GetDevicePartsType() = 0;
    virtual std::string GetDeviceVersion() = 0;

    virtual int GetHotBedCurrentTemperature() = 0;
    virtual int GetHotBedTargetTemperature() = 0;

	virtual int GetFilamentUsed() = 0;
    virtual std::string GetFilamentUnit() = 0;

    virtual std::string GetThumbnail() = 0;

    virtual int GetCurrentLayer() = 0;
    virtual int GetTotalLayer() = 0;
   
    virtual void resetStatus() = 0;
    virtual std::list<FileInfo> getDeviceFileList() = 0;

    virtual GUI_DEVICE_STATUS_TYPE getGuiDeviceStatus() = 0;

    virtual void clearExceptionFinished() = 0;
    virtual PrintFailedInfo GetPrintFailedInfo() const = 0;

    virtual MtColorSlotDataVec GetMtSlotData() const = 0;

    // return map is empty, no cutoff or clogging
    virtual int GetMultiCutoffCloggingMapSize() const = 0;

    virtual CustomDeviceStatus getCustomDeviceStatus() = 0;


    // Printing notice.
    virtual PrintNoticeInfo GetPrintNoticeInfo() = 0;

    // get nozzle max temperature    
    virtual  MaxNozzleTemp GetNozzleMaxTemp() = 0;

    // Mqtt Ctrl CMD.
    virtual void SetLocalPrintData(const VrCardInfoMap& vrCardInfoMap, const std::string& filepath = "") = 0;
    virtual void SetRemotePrintData(const VrCardInfoMap& vrCardInfoMap, const std::string& filepath = "") = 0;
    virtual void SendErrWinResToMachine(const std::string& errorCode, const std::string& errorLevel) = 0;
    virtual void setDevicePrintPause() = 0;
    virtual void setDevicePrintStop() = 0;
    virtual void setDevicePrintResume() = 0;
    virtual void setDevicePrintAgain() = 0;

    virtual bool IsOnlined() const = 0;
    virtual bool IsBusy() const = 0;

    virtual  bool GetMultiColorDeviceUnInited() = 0;

    //virtual MqttType::PrintCtlResult getDeviceCtrlResult() = 0;
    virtual void clearDeviceCtrlResult() = 0;
    virtual void resetDeviceIdel() = 0;

    virtual void getDeviceLocalFileLists() = 0;
    virtual void getDeviceUsbFileLists() = 0;

    // temperture
    virtual void SetTemperture(int bedTemperture, int nozzleTemperture, int nozzleNum = 0) = 0;
    virtual TemperatureInfo GetNozzleTemperature(int nozzleNum = 0) = 0;

    virtual void setCalibrationStop() = 0;
    virtual  MtCalibration getCalibrationInfo() const = 0;

    virtual  bool IsMultiColorDevice() const = 0;

    // asy query before print
    virtual void AsyQueryAllInfo() = 0;

    virtual  bool GetHaveCalibrator() = 0;
    virtual  bool GetIsCalibrated() = 0;
    virtual  bool GetIsLeveled(int type = -1) = 0;

    virtual void setLevelBegin() = 0;
    virtual void setLevelStop() = 0;
    virtual LevelData GetProgressValue() const = 0;

    virtual void setZAxisCompensation(float value) = 0;
    virtual float getZAxisCompensationValue() = 0;
    virtual void setDischargeExtrusion(int stepLen, int temperature, int nozzleNum = -1) = 0;
    virtual void setMaterialReturnExtrusion(int stepLen, int temperature, int nozzleNum = -1) = 0;
    virtual void setStopExtrusion() = 0;
    virtual int GetExtrusionValue() const = 0;

    virtual void setRequestGCodeInfo(const std::string& filepath) = 0;   
    virtual GCodeInfo GetGcodeInfo() const = 0;
    virtual void SetLastFilament() = 0;
    virtual std::string GetLastFilament() const = 0;

    virtual  PliesInfo GetLayerPtr() const = 0;
    
    virtual void clearDeviceExceptionInfo() = 0;

    virtual void NozzleSwitch(int newNozzleNum = 0) = 0;

    virtual bool GetCameraLimit() = 0;

    virtual bool GetTransfering() = 0;

    virtual void SetDeviceFunctions() = 0;
    virtual bool GetPreheatFunction() const = 0;
    virtual void SendSwitchInfoToDevice(const std::string& cmd, bool isOpen) = 0;
    virtual std::tuple<bool, std::string> RecvSwitchInfoFromDevice() = 0;
};

}

#endif
