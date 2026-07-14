#ifndef DEVICE_OBJECT_NATIVE_HPP
#define DEVICE_OBJECT_NATIVE_HPP

#include "Interface Files/DeviceObjectBase.h"

#include <functional>
#include <mutex>
#include <string>

namespace AnkerNet {

// Concrete DeviceObjectBase populated from the query_fdm_list / get_dsk_keys HTTP
// responses (Phase 2). Only identity/display getters return real data; live-status
// getters (temperatures, layers, progress, exceptions) return safe idle defaults
// until MQTT telemetry lands in Phase 4, and the control setters (print/level/
// extrude/temperature) are no-ops until PPPP + MQTT control land in Phases 3-4.
class DeviceObjectNative : public DeviceObjectBase
{
public:
    // --- populated from the device-list API ---
    std::string sn;          // station_sn
    std::string name;        // station_name
    std::string model;       // station_model
    std::string ipAddr;      // ip_addr
    std::string mqttKey;     // secret_key (hex; MQTT AES key, used in Phase 4)
    std::string p2pDuid;     // p2p_did (PPPP device id, used in Phase 3)
    std::string p2pKey;      // dsk_key (PPPP key, from get_dsk_keys; used in Phase 3)
    int stationId = 0;       // station_id
    bool online = true;      // no online flag in the list API; refined via MQTT (Phase 4)
    anker_device_type deviceType = DEVICE_V8111_TYPE; // default M5

    // --- live status, updated from MQTT telemetry (guarded by m_liveMutex) ---
    // Temperatures are stored already scaled to whole degrees C (raw MQTT is x100).
    void setNozzleTemp(int cur, int tgt)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_nozzleCur = cur; m_nozzleTgt = tgt; m_hasNozzle = true;
    }
    void setBedTemp(int cur, int tgt)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_bedCur = cur; m_bedTgt = tgt; m_hasBed = true;
    }
    void setPrintStatus(aknmt_print_event_e ev, int progress)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_printEvent = ev; m_progress = progress;
    }
    // z-offset (mm) reported by 1021 telemetry; UI reads via getZAxisCompensationValue.
    void setZOffsetValue(float mm)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_zOffset = mm;
    }
    // Sends a JSON command to /device/maker/<sn>/command via the plugin's MQTT client.
    using CommandSender = std::function<void(const std::string& sn, const std::string& json)>;
    void setCommandSender(CommandSender fn) { m_sendCommand = std::move(fn); }

    // --- identity / display ---
    std::string GetStationName() override { return name; }
    std::string GetSn() override { return sn; }
    bool GetOnline() override { return online; }
    bool IsOnlined() const override { return online; }
    anker_device_type GetDeviceType() override { return deviceType; }
    anker_device_parts_type GetDevicePartsType() override { return DEVICE_PARTS_NO; }
    std::string GetDeviceVersion() override { return ""; }

    // --- status (live from MQTT telemetry) ---
    aknmt_print_event_e GetDeviceStatus() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_printEvent;
    }
    GUI_DEVICE_STATUS_TYPE getGuiDeviceStatus() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return static_cast<GUI_DEVICE_STATUS_TYPE>(m_printEvent);
    }
    CustomDeviceStatus getCustomDeviceStatus() override { return CustomDeviceStatus_Max; }
    GeneralException2Gui GetGeneralException() override { return GeneralException2Gui_No_Error; }
    bool IsBusy() const override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_printEvent != AKNMT_PRINT_EVENT_IDLE;
    }

    std::string GetPrintFile() override { return ""; }
    std::string GetFileName() override { return ""; }
    void GetMsgCenterInfo(std::string& errorCode, std::string& errorLevel) override { errorCode.clear(); errorLevel.clear(); }
    int GetProcess() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_progress;
    }
    int64_t GetTime() override { return 0; }
    int GetHotBedCurrentTemperature() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_bedCur;
    }
    int GetHotBedTargetTemperature() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_bedTgt;
    }
    int GetFilamentUsed() override { return 0; }
    std::string GetFilamentUnit() override { return "mm"; }
    std::string GetThumbnail() override { return ""; }
    int GetCurrentLayer() override { return 0; }
    int GetTotalLayer() override { return 0; }
    void resetStatus() override {}
    std::list<FileInfo> getDeviceFileList() override { return {}; }
    void clearExceptionFinished() override {}
    PrintFailedInfo GetPrintFailedInfo() const override { return PrintFailedInfo{}; }
    MtColorSlotDataVec GetMtSlotData() const override { return {}; }
    int GetMultiCutoffCloggingMapSize() const override { return 0; }
    PrintNoticeInfo GetPrintNoticeInfo() override { return PrintNoticeInfo{}; }
    MaxNozzleTemp GetNozzleMaxTemp() override { return MaxNozzleTemp::PTFE; }

    // --- control (no-ops until Phases 3-4) ---
    void SetLocalPrintData(const VrCardInfoMap&, const std::string& = "") override {}
    void SetRemotePrintData(const VrCardInfoMap&, const std::string& = "") override {}
    void SendErrWinResToMachine(const std::string&, const std::string&) override {}
    void setDevicePrintPause() override {}
    void setDevicePrintStop() override {}
    void setDevicePrintResume() override {}
    void setDevicePrintAgain() override {}
    bool GetMultiColorDeviceUnInited() override { return false; }
    void clearDeviceCtrlResult() override {}
    void resetDeviceIdel() override {}
    void getDeviceLocalFileLists() override {}
    void getDeviceUsbFileLists() override {}
    // bedTemp/nozzleTemp are whole degrees C; -1 means "leave this one unchanged".
    // 1003/1004 {value:1/100 C} set the stored target, but the heater only actually
    // engages when the matching Marlin g-code is also sent (M104 nozzle / M140 bed)
    // via 1043 GCODE_COMMAND -- 1003/1004 alone are acked but don't heat.
    void SetTemperture(int bedTemp, int nozzleTemp, int /*nozzleNum*/ = 0) override
    {
        if (nozzleTemp >= 0) {
            sendCommand(1003, "\"value\":" + std::to_string(nozzleTemp * 100));
            sendGcode("M104 S" + std::to_string(nozzleTemp));
        }
        if (bedTemp >= 0) {
            sendCommand(1004, "\"value\":" + std::to_string(bedTemp * 100));
            sendGcode("M140 S" + std::to_string(bedTemp));
        }
    }
    TemperatureInfo GetNozzleTemperature(int = 0) override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        TemperatureInfo t;
        t.currentTemp = m_nozzleCur;
        t.targetTemp = m_nozzleTgt;
        t.isNull = !m_hasNozzle;
        return t;
    }
    void setCalibrationStop() override {}
    MtCalibration getCalibrationInfo() const override { return MtCalibration{}; }
    bool IsMultiColorDevice() const override { return false; }
    void AsyQueryAllInfo() override {}
    bool GetHaveCalibrator() override { return false; }
    bool GetIsCalibrated() override { return true; }
    // Report the bed as already leveled. AnkerTaskPanel::updateStatus() fires the
    // NEED_LEVEL reminder whenever this is false, and it runs on every telemetry
    // tick (~2/sec) -- returning false produced a relentless auto-level nag. We
    // don't yet track real leveling state from the device, and the user never
    // wants this reminder, so assume leveled.
    bool GetIsLeveled(int = -1) override { return true; }
    void setLevelBegin() override {}
    void setLevelStop() override {}
    LevelData GetProgressValue() const override { return LevelData{}; }
    // z-offset compensation, mm. Wire value is (best guess) 1/100 mm as int. 1021.
    void setZAxisCompensation(float value) override
    {
        int v = static_cast<int>(value * 100.0f + (value >= 0 ? 0.5f : -0.5f));
        sendCommand(1021, "\"value\":" + std::to_string(v));
    }
    float getZAxisCompensationValue() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_zOffset;
    }
    // Extrude (feed) / retract (return) via the proven 1043 g-code path -- same
    // mechanism as temperature. temperature is whole deg C; if > 0 we ensure the
    // nozzle is heating first (cold-extrusion is rejected by firmware). M83 sets
    // relative extrusion so E is a delta; F180 = 3 mm/s.
    void setDischargeExtrusion(int stepLen, int temperature, int /*nozzleNum*/ = -1) override
    {
        if (temperature > 0)
            sendGcode("M104 S" + std::to_string(temperature));
        sendGcode("M83");
        sendGcode("G1 E" + std::to_string(stepLen) + " F180");
    }
    void setMaterialReturnExtrusion(int stepLen, int temperature, int /*nozzleNum*/ = -1) override
    {
        if (temperature > 0)
            sendGcode("M104 S" + std::to_string(temperature));
        sendGcode("M83");
        sendGcode("G1 E-" + std::to_string(stepLen) + " F180");
    }
    void setStopExtrusion() override
    {
        // No clean "abort current E move"; stop heating as a best-effort stop.
        sendGcode("M104 S0");
    }
    int GetExtrusionValue() const override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_extrusionValue;
    }
    void setRequestGCodeInfo(const std::string&) override {}
    GCodeInfo GetGcodeInfo() const override { return GCodeInfo{}; }
    void SetLastFilament() override {}
    std::string GetLastFilament() const override { return ""; }
    PliesInfo GetLayerPtr() const override { return PliesInfo{}; }
    void clearDeviceExceptionInfo() override {}
    void NozzleSwitch(int = 0) override {}
    // Gates the video UI: false makes AnkerVideo report "no camera permission" and
    // bail before StartP2pOperator. The M5 has a camera, so allow it.
    bool GetCameraLimit() override { return true; }
    bool GetTransfering() override { return false; }
    void SetDeviceFunctions() override {}
    bool GetPreheatFunction() const override { return false; }
    void SendSwitchInfoToDevice(const std::string&, bool) override {}
    std::tuple<bool, std::string> RecvSwitchInfoFromDevice() override { return { false, std::string() }; }

private:
    // Builds {"commandType":<cmd>,<fields>} and hands it to the MQTT command sender.
    void sendCommand(int cmd, const std::string& fields)
    {
        if (!m_sendCommand)
            return;
        std::string json = "{\"commandType\":" + std::to_string(cmd);
        if (!fields.empty())
            json += "," + fields;
        json += "}";
        m_sendCommand(sn, json);
    }

    // Runs a raw Marlin g-code line on the printer via 1043 GCODE_COMMAND.
    void sendGcode(const std::string& gcode)
    {
        sendCommand(1043, "\"cmdLen\":" + std::to_string(gcode.size()) +
            ",\"cmdData\":\"" + gcode + "\"");
    }

    CommandSender m_sendCommand;

    mutable std::mutex m_liveMutex;
    int m_nozzleCur = 0, m_nozzleTgt = 0;
    int m_bedCur = 0, m_bedTgt = 0;
    bool m_hasNozzle = false, m_hasBed = false;
    int m_progress = 0;
    aknmt_print_event_e m_printEvent = AKNMT_PRINT_EVENT_IDLE;
    float m_zOffset = 0.0f;
    int m_extrusionValue = 0;
};

} // namespace AnkerNet

#endif // DEVICE_OBJECT_NATIVE_HPP
