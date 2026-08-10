#ifndef DEVICE_OBJECT_NATIVE_HPP
#define DEVICE_OBJECT_NATIVE_HPP

#include "Interface Files/DeviceObjectBase.h"

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

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
    std::string p2pConn;     // p2p_conn init-string -> relay server hosts (remote/WAN)
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
    // Print event (idle/heating/printing/paused/completed/...), from 1000 subType=1.
    void setPrintEvent(aknmt_print_event_e ev)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_printEvent = ev;
        // A new print starting clears any pending "cancelled" state from a prior job.
        if (ev == AKNMT_PRINT_EVENT_PRINTING || ev == AKNMT_PRINT_EVENT_PRINT_HEATING ||
            ev == AKNMT_PRINT_EVENT_PREHEATING)
            m_jobEndedFailed = false;
    }
    // Progress in basis points (0-10000 = 0-100.00%), from 1001's "progress" field --
    // matches AnkerTaskPanel's setProgressRange(10000) directly, no rescaling needed.
    void setPrintProgress(int progress)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_progress = progress;
    }
    // Print job info, from 1001: file name, total filament for the job (the UI
    // multiplies this by progress/10000 to estimate filament used so far), remaining
    // seconds (1001's "time" field, counts down while printing), elapsed seconds
    // (1001's "totalTime" field, counts up -- used as the finish dialog's duration),
    // and current print speed.
    void setPrintJobInfo(const std::string& fileName, int filamentTotal,
        const std::string& filamentUnit, int64_t timeLeftSec, int64_t elapsedSec, int realSpeed)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_printFile = fileName;
        m_filamentTotal = filamentTotal;
        m_filamentUnit = filamentUnit.empty() ? "mm" : filamentUnit;
        m_timeLeftSec = timeLeftSec;
        m_elapsedSec = elapsedSec;
        m_realSpeed = realSpeed;
    }
    // Layer progress, from 1052.
    void setLayerInfo(int totalLayer, int realPrintLayer)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_totalLayer = totalLayer;
        m_currentLayer = realPrintLayer;
    }
    // Current print job's task_id (from 1001 telemetry); used to build the cancel command.
    void setTaskId(const std::string& taskId)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_taskId = taskId;
    }
    // Job-ended summary from 1068. A job that ended WITHOUT reaching COMPLETED was
    // cancelled or failed -> report PRINT_FAILED so AnkerTaskPanel shows the cancelled
    // dialog (on a cancel the print-event never reports "stopped", it just returns to
    // idle, so 1068 is the only signal). We deliberately do NOT key off 1068's "trigger"
    // field -- it varies (3 and 4 both seen for user cancels); "did it reach COMPLETED"
    // is the reliable discriminator. Natural completion is handled by the 1000 COMPLETED
    // event, so this leaves m_jobEndedFailed false. Sticky until a new print starts or the
    // dialog is dismissed.
    void setJobEnded(const std::string& name, int64_t totalTime,
        int filamentUsed, const std::string& filamentUnit)
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_printFile = name;
        m_elapsedSec = totalTime;
        m_filamentTotal = filamentUsed;
        m_filamentUnit = filamentUnit.empty() ? "mm" : filamentUnit;
        m_jobEndedFailed = (m_printEvent != AKNMT_PRINT_EVENT_COMPLETED);
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
    // Account identity, required in the 1008 print-control command payload.
    void setUserInfo(const std::string& userId, const std::string& userName)
    {
        m_userId = userId;
        m_userName = userName;
    }

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
        // A cancelled job (1068 trigger:4) reports PRINT_FAILED. Sticky so the trailing
        // idle print-event doesn't wipe it before AnkerTaskPanel fires the dialog; cleared
        // when a new print starts (setPrintEvent) or the dialog is dismissed
        // (clearExceptionFinished/resetDeviceIdel/clearDeviceCtrlResult).
        if (m_jobEndedFailed)
            return GUI_DEVICE_STATUS_TYPE_PRINT_FAILED;
        // GUI_DEVICE_STATUS_TYPE_PRINT_FINISHED/_FAILED are NOT numerically equal to
        // aknmt_print_event_e's COMPLETED/STOPPED (they're separate auto-incremented
        // constants) -- the direct cast below only covers the states that do share
        // numbering (idle/printing/paused/leveling/etc). Completed and
        // stopped need an explicit translation so the finish dialog (AnkerTaskPanel)
        // actually fires; without this it never sees a status it recognizes as "done".
        if (m_printEvent == AKNMT_PRINT_EVENT_COMPLETED)
            return GUI_DEVICE_STATUS_TYPE_PRINT_FINISHED;
        if (m_printEvent == AKNMT_PRINT_EVENT_STOPPED)
            return GUI_DEVICE_STATUS_TYPE_PRINT_FAILED;
        return static_cast<GUI_DEVICE_STATUS_TYPE>(m_printEvent);
    }
    CustomDeviceStatus getCustomDeviceStatus() override { return CustomDeviceStatus_Max; }
    GeneralException2Gui GetGeneralException() override { return GeneralException2Gui_No_Error; }
    bool IsBusy() const override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_printEvent != AKNMT_PRINT_EVENT_IDLE;
    }

    std::string GetPrintFile() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_printFile;
    }
    std::string GetFileName() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_printFile;
    }
    void GetMsgCenterInfo(std::string& errorCode, std::string& errorLevel) override { errorCode.clear(); errorLevel.clear(); }
    int GetProcess() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_progress;
    }
    int64_t GetTime() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_timeLeftSec;
    }
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
    int GetFilamentUsed() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_filamentTotal;
    }
    std::string GetFilamentUnit() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_filamentUnit;
    }
    std::string GetThumbnail() override { return ""; }
    int GetCurrentLayer() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_currentLayer;
    }
    int GetTotalLayer() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        return m_totalLayer;
    }
    void resetStatus() override {}
    std::list<FileInfo> getDeviceFileList() override { return {}; }
    void clearExceptionFinished() override { clearJobEnded(); }
    PrintFailedInfo GetPrintFailedInfo() const override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        PrintFailedInfo info;
        info.isNull = false;
        info.name = m_printFile;
        info.totalTime = m_elapsedSec;
        info.filamentUsed = m_filamentTotal;
        info.filamentUnit = m_filamentUnit;
        return info;
    }
    MtColorSlotDataVec GetMtSlotData() const override { return {}; }
    int GetMultiCutoffCloggingMapSize() const override { return 0; }
    PrintNoticeInfo GetPrintNoticeInfo() override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        PrintNoticeInfo info;
        info.isNull = false;
        info.name = m_printFile;
        info.totalTime = static_cast<int>(m_elapsedSec);
        info.filamentUsed = m_filamentTotal;
        info.filamentUnit = m_filamentUnit;
        info.realSpeed = m_realSpeed;
        return info;
    }
    MaxNozzleTemp GetNozzleMaxTemp() override { return MaxNozzleTemp::PTFE; }

    // --- control (no-ops until Phases 3-4) ---
    void SetLocalPrintData(const VrCardInfoMap&, const std::string& = "") override {}
    void SetRemotePrintData(const VrCardInfoMap&, const std::string& = "") override {}
    void SendErrWinResToMachine(const std::string&, const std::string&) override {}
    // Print control -- commandType 1008 (PRINT_CONTROL) over MQTT. Value selects the
    // action: 2 = pause, 3 = resume, 4 = stop/cancel. Discovered 2026-07-15 by decrypting
    // the production app's own MQTT (TLS-intercept capture): the command REQUIRES the extra
    // fields userId/printMode/userName/filePath -- sending 1008 with only "value" is why
    // every earlier attempt was silently ignored by the printer. Sent reliably (QoS-0
    // resend). Payload mirrors the official app exactly:
    //   {"commandType":1008,"value":N,"printMode":1,"userName":..,"filePath":"","userId":..}
    void setDevicePrintPause()  override { sendPrintControl(2); }
    void setDevicePrintResume() override { sendPrintControl(3); }
    void setDevicePrintStop()   override { sendPrintControl(4); }
    void setDevicePrintAgain() override {}
    bool GetMultiColorDeviceUnInited() override { return false; }
    // These are called by AnkerTaskPanel after the finish/cancel dialog is dismissed;
    // clear the sticky "cancelled" state so the panel returns to its idle view.
    void clearDeviceCtrlResult() override { clearJobEnded(); }
    void resetDeviceIdel() override { clearJobEnded(); }
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
    PliesInfo GetLayerPtr() const override
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        PliesInfo info;
        info.isNull = false;
        info.total_layer = m_totalLayer;
        info.real_print_layer = m_currentLayer;
        return info;
    }
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

    // Our MQTT client publishes at QoS 0 (fire-and-forget: no ack, no retransmit), so
    // any single command can be silently dropped -- observed live: a Stop click's M524
    // needed 3 manual retries (over ~90s) before one actually reached the printer.
    // Resend a few times over ~1s as cheap, pragmatic redundancy for the print-control
    // commands (stop/pause/resume) where a dropped command is most disruptive, rather
    // than rewriting the MQTT client for QoS 1. AnkerMqttClient::publish is already
    // mutex-guarded, so firing from a background thread is safe.
    void sendGcodeReliably(const std::string& gcode)
    {
        sendGcode(gcode);
        std::thread([this, gcode]() {
            for (int i = 0; i < 2; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                sendGcode(gcode);
            }
        }).detach();
    }
    // Same redundancy, for a raw vendor commandType instead of a gcode wrapper.
    void sendCommandReliably(int cmd, const std::string& fields)
    {
        sendCommand(cmd, fields);
        std::thread([this, cmd, fields]() {
            for (int i = 0; i < 2; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                sendCommand(cmd, fields);
            }
        }).detach();
    }

    // Minimal JSON string escape (for userName/userId going into the command payload).
    static std::string jsonEscape(const std::string& s)
    {
        std::string o;
        for (char c : s) {
            if (c == '"' || c == '\\') o += '\\';
            o += c;
        }
        return o;
    }

    void clearJobEnded()
    {
        std::lock_guard<std::mutex> l(m_liveMutex);
        m_jobEndedFailed = false;
    }

    // Build + send the 1008 PRINT_CONTROL command (value: 2=pause, 3=resume, 4=stop) with
    // the exact field set the production app uses. userId is required by the firmware.
    void sendPrintControl(int value)
    {
        const std::string fields =
            "\"value\":" + std::to_string(value) +
            ",\"printMode\":1" +
            ",\"userName\":\"" + jsonEscape(m_userName) + "\"" +
            ",\"filePath\":\"\"" +
            ",\"userId\":\"" + jsonEscape(m_userId) + "\"";
        sendCommandReliably(1008, fields);
    }

    CommandSender m_sendCommand;
    std::string m_userId, m_userName;

    mutable std::mutex m_liveMutex;
    int m_nozzleCur = 0, m_nozzleTgt = 0;
    int m_bedCur = 0, m_bedTgt = 0;
    bool m_hasNozzle = false, m_hasBed = false;
    int m_progress = 0; // basis points, 0-10000
    aknmt_print_event_e m_printEvent = AKNMT_PRINT_EVENT_IDLE;
    float m_zOffset = 0.0f;
    int m_extrusionValue = 0;
    std::string m_printFile;
    int m_filamentTotal = 0;
    std::string m_filamentUnit = "mm";
    int64_t m_timeLeftSec = 0;
    int64_t m_elapsedSec = 0;
    int m_realSpeed = 0;
    int m_currentLayer = 0, m_totalLayer = 0;
    std::string m_taskId;
    bool m_jobEndedFailed = false; // sticky: set by 1068 trigger:4 (cancel), drives PRINT_FAILED
};

} // namespace AnkerNet

#endif // DEVICE_OBJECT_NATIVE_HPP
