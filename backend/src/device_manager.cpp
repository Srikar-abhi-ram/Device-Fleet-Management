#include "device_manager.h"
#include <sstream>
#include <random>
#include <chrono> 
#include <iomanip>
#include <map>
#include <vector>

namespace device_management {

DeviceManager::DeviceManager() = default;

DeviceManager::~DeviceManager() = default;

bool DeviceManager::RegisterDevice(const std::string& device_id,
                                   const std::string& device_name,
                                   const std::string& device_type,
                                   DeviceStatus initial_status) {
    // keep device map modifications behind a lock
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    if (devices_.find(device_id) != devices_.end()) {
        return false;
    }
    
    DeviceData device;
    device.device_id = device_id;
    device.device_name = device_name;
    device.device_type = device_type;
    device.status = initial_status;
    device.registered_at = std::chrono::system_clock::now();
    device.last_updated = std::chrono::system_clock::now();
    device.current_action_id = "";
    
    devices_[device_id] = device;
    
    return true;
}

bool DeviceManager::SetDeviceStatus(const std::string& device_id, 
                                    DeviceStatus status,
                                    DeviceStatus* previous_status) {
    // device status updates also go through the same lock
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        return false;
    }
    
    if (previous_status != nullptr) {
        *previous_status = it->second.status;
    }
    
    it->second.status = status;
    it->second.last_updated = std::chrono::system_clock::now();
    
    return true;
}

std::unique_ptr<DeviceInfo> DeviceManager::GetDeviceInfo(const std::string& device_id) {
    // read access still uses the mutex since devices_ can be written elsewhere
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        return nullptr;
    }
    
    auto device_info = std::make_unique<DeviceInfo>();
    const DeviceData& device = it->second;
    
    device_info->set_device_id(device.device_id);
    device_info->set_status(device.status);
    device_info->set_device_name(device.device_name);
    device_info->set_device_type(device.device_type);
    
    auto registered_time = std::chrono::duration_cast<std::chrono::seconds>(
        device.registered_at.time_since_epoch()).count();
    auto updated_time = std::chrono::duration_cast<std::chrono::seconds>(
        device.last_updated.time_since_epoch()).count();
    
    device_info->set_registered_at(registered_time);
    device_info->set_last_updated(updated_time);
    device_info->set_current_action_id(device.current_action_id);
    
    return device_info;
}

std::vector<DeviceInfo> DeviceManager::ListAllDevices() {
    // take a snapshot of current devices under the lock and transform to protobuf objects
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    std::vector<DeviceInfo> device_list;
    device_list.reserve(devices_.size());
    
    for (const auto& pair : devices_) {
        const DeviceData& device = pair.second;
        
        DeviceInfo device_info;
        device_info.set_device_id(device.device_id);
        device_info.set_status(device.status);
        device_info.set_device_name(device.device_name);
        device_info.set_device_type(device.device_type);
        
        auto registered_time = std::chrono::duration_cast<std::chrono::seconds>(
            device.registered_at.time_since_epoch()).count();
        auto updated_time = std::chrono::duration_cast<std::chrono::seconds>(
            device.last_updated.time_since_epoch()).count();
        
        device_info.set_registered_at(registered_time);
        device_info.set_last_updated(updated_time);
        device_info.set_current_action_id(device.current_action_id);
        
        device_list.push_back(device_info);
    }
    
    return device_list;
}

bool DeviceManager::SetDeviceAction(const std::string& device_id, const std::string& action_id) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        return false;
    }
    
    it->second.current_action_id = action_id;
    it->second.last_updated = std::chrono::system_clock::now();
    
    return true;
}

bool DeviceManager::ClearDeviceAction(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = devices_.find(device_id);
    if (it == devices_.end()) {
        return false;
    }
    
    it->second.current_action_id = "";
    it->second.last_updated = std::chrono::system_clock::now();
    
    return true;
}

bool DeviceManager::DeviceExists(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    return devices_.find(device_id) != devices_.end();
}

ActionSimulator::ActionSimulator(DeviceManager* device_manager)
    : device_manager_(device_manager), action_id_counter_(0), shutdown_requested_(false) {
}

ActionSimulator::~ActionSimulator() {
    Shutdown();
}

std::string ActionSimulator::InitiateAction(const std::string& device_id,
                                            ActionType action_type,
                                            const std::map<std::string, std::string>& action_params,
                                            std::function<void(const std::string&, ActionStatus)> status_callback) {
    std::string action_id = GenerateActionId();
    
    auto action_data = std::make_unique<ActionData>();
    action_data->action_id = action_id;
    action_data->device_id = device_id;
    action_data->action_type = action_type;
    action_data->status = ActionStatus::PENDING;
    action_data->action_params = action_params;
    action_data->initiated_at = std::chrono::system_clock::now();
    action_data->completed_at = std::chrono::system_clock::time_point::min();
    action_data->error_message = "";
    action_data->should_stop = false;
    
    {
        std::lock_guard<std::mutex> lock(actions_mutex_);
        ActionData* raw_ptr = action_data.get();
        actions_[action_id] = std::move(action_data);
        
        raw_ptr->simulation_thread = std::thread([this, action_id, device_id, action_type, status_callback, raw_ptr]() {
            this->SimulateAction(action_id, device_id, action_type, status_callback);
        });
    }
    
    device_manager_->SetDeviceAction(device_id, action_id);
    
    {
        std::lock_guard<std::mutex> lock(actions_mutex_);
        auto it = actions_.find(action_id);
        if (it != actions_.end()) {
            it->second->status = ActionStatus::RUNNING;
        }
    }
    
    DeviceStatus device_status = DeviceStatus::BUSY;
    if (action_type == ActionType::SOFTWARE_UPDATE || action_type == ActionType::FIRMWARE_UPDATE) {
        device_status = DeviceStatus::UPDATING;
    }
    device_manager_->SetDeviceStatus(device_id, device_status);
    
    if (status_callback) {
        status_callback(action_id, ActionStatus::RUNNING);
    }
    
    return action_id;
}

std::unique_ptr<ActionInfo> ActionSimulator::GetActionStatus(const std::string& action_id) {
    std::lock_guard<std::mutex> lock(actions_mutex_);
    
    auto it = actions_.find(action_id);
    if (it == actions_.end()) {
        return nullptr;
    }
    
    const ActionData& action = *(it->second);
    auto action_info = std::make_unique<ActionInfo>();
    
    action_info->set_action_id(action.action_id);
    action_info->set_device_id(action.device_id);
    action_info->set_action_type(action.action_type);
    action_info->set_status(action.status);
    
    for (const auto& param : action.action_params) {
        (*action_info->mutable_action_params())[param.first] = param.second;
    }
    
    auto initiated_time = std::chrono::duration_cast<std::chrono::seconds>(
        action.initiated_at.time_since_epoch()).count();
    action_info->set_initiated_at(initiated_time);
    
    if (action.completed_at != std::chrono::system_clock::time_point::min()) {
        auto completed_time = std::chrono::duration_cast<std::chrono::seconds>(
            action.completed_at.time_since_epoch()).count();
        action_info->set_completed_at(completed_time);
    } else {
        action_info->set_completed_at(0);
    }
    
    action_info->set_error_message(action.error_message);
    
    return action_info;
}

void ActionSimulator::Shutdown() {
    shutdown_requested_ = true;
    
    {
        std::lock_guard<std::mutex> lock(actions_mutex_);
        for (auto& pair : actions_) {
            pair.second->should_stop = true;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(actions_mutex_);
        for (auto& pair : actions_) {
            if (pair.second->simulation_thread.joinable()) {
                pair.second->simulation_thread.join();
            }
        }
    }
}

void ActionSimulator::SimulateAction(const std::string& action_id,
                                    const std::string& device_id,
                                    ActionType action_type,
                                    std::function<void(const std::string&, ActionStatus)> status_callback) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10, 30);
    
    int duration_seconds = dis(gen);
    
    bool should_stop = false;
    for (int i = 0; i < duration_seconds && !should_stop; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        {
            std::lock_guard<std::mutex> lock(actions_mutex_);
            auto it = actions_.find(action_id);
            if (it != actions_.end() && it->second->should_stop) {
                should_stop = true;
            }
        }
    }
    
    std::uniform_int_distribution<> success_dis(1, 100);
    bool success = (success_dis(gen) <= 90);
    
    {
        std::lock_guard<std::mutex> lock(actions_mutex_);
        auto it = actions_.find(action_id);
        if (it != actions_.end()) {
            if (should_stop || shutdown_requested_) {
                it->second->status = ActionStatus::FAILED;
                it->second->error_message = "Action was cancelled";
            } else if (success) {
                it->second->status = ActionStatus::COMPLETED;
            } else {
                it->second->status = ActionStatus::FAILED;
                it->second->error_message = "Action simulation failed (random failure)";
            }
            it->second->completed_at = std::chrono::system_clock::now();
        }
    }
    
    if (!should_stop && !shutdown_requested_) {
        DeviceStatus new_device_status = success ? DeviceStatus::IDLE : DeviceStatus::ERROR;
        device_manager_->SetDeviceStatus(device_id, new_device_status);
        
        device_manager_->ClearDeviceAction(device_id);
    }
    
    if (status_callback) {
        ActionStatus final_status = should_stop ? ActionStatus::FAILED : 
                                   (success ? ActionStatus::COMPLETED : ActionStatus::FAILED);
        status_callback(action_id, final_status);
    }
}

std::string ActionSimulator::GenerateActionId() {
    uint64_t counter = action_id_counter_.fetch_add(1);
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::ostringstream oss;
    oss << "action_" << timestamp << "_" << counter;
    return oss.str();
}

}
