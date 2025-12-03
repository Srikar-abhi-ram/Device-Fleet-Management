#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include <functional>

#include "device_management.pb.h"
#include "device_management.grpc.pb.h"

namespace device_management {



class DeviceManager {
public:
    // owns in‑memory state for all devices, one instance per server
    DeviceManager();
    ~DeviceManager();

    bool RegisterDevice(const std::string& device_id, 
                       const std::string& device_name,
                       const std::string& device_type,
                       DeviceStatus initial_status);

    bool SetDeviceStatus(const std::string& device_id, DeviceStatus status, DeviceStatus* previous_status = nullptr);

    std::unique_ptr<DeviceInfo> GetDeviceInfo(const std::string& device_id);

    std::vector<DeviceInfo> ListAllDevices();

    bool SetDeviceAction(const std::string& device_id, const std::string& action_id);

    bool ClearDeviceAction(const std::string& device_id);

    bool DeviceExists(const std::string& device_id);

private:
    struct DeviceData {
        std::string device_id;
        std::string device_name;
        std::string device_type;
        DeviceStatus status;
        std::chrono::system_clock::time_point registered_at;
        std::chrono::system_clock::time_point last_updated;
        std::string current_action_id;
    };

    std::unordered_map<std::string, DeviceData> devices_;
    std::mutex devices_mutex_;
};

class ActionSimulator {
public:
    // runs long‑lived actions on a background thread and reports progress
    ActionSimulator(DeviceManager* device_manager);
    ~ActionSimulator();

    std::string InitiateAction(const std::string& device_id,
                               ActionType action_type,
                               const std::map<std::string, std::string>& action_params,
                               std::function<void(const std::string&, ActionStatus)> status_callback);

    std::unique_ptr<ActionInfo> GetActionStatus(const std::string& action_id);

    void Shutdown();

private:
    struct ActionData {
        std::string action_id;
        std::string device_id;
        ActionType action_type;
        ActionStatus status;
        std::map<std::string, std::string> action_params;
        std::chrono::system_clock::time_point initiated_at;
        std::chrono::system_clock::time_point completed_at;
        std::string error_message;
        std::thread simulation_thread;
        std::atomic<bool> should_stop;
    };

    void SimulateAction(const std::string& action_id, 
                       const std::string& device_id,
                       ActionType action_type,
                       std::function<void(const std::string&, ActionStatus)> status_callback);

    std::string GenerateActionId();

    std::unordered_map<std::string, std::unique_ptr<ActionData>> actions_;
    std::mutex actions_mutex_;
    DeviceManager* device_manager_;
    std::atomic<uint64_t> action_id_counter_;
    std::atomic<bool> shutdown_requested_;
};

}

#endif
