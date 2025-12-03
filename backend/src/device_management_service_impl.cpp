#include "device_management_service_impl.h"
#include "device_management.pb.h"
#include <grpcpp/grpcpp.h>

namespace device_management {

DeviceManagementServiceImpl::DeviceManagementServiceImpl() { //Constructor for the service implementation
    device_manager_ = std::make_unique<DeviceManager>();
    action_simulator_ = std::make_unique<ActionSimulator>(device_manager_.get());
}

DeviceManagementServiceImpl::~DeviceManagementServiceImpl() { //Destructor for the service implementation
    if (action_simulator_) {
        action_simulator_->Shutdown();
    }
}

grpc::Status DeviceManagementServiceImpl::RegisterDevice(
    grpc::ServerContext* /* context */,
    const RegisterDeviceRequest* request,
    RegisterDeviceResponse* response) {
    
    const std::string& device_id = request->device_id();
    const std::string& device_name = request->device_name();
    const std::string& device_type = request->device_type();
    
    DeviceStatus initial_status = request->initial_status();
    if (initial_status == DeviceStatus::DEVICE_STATUS_UNKNOWN) {
        initial_status = DeviceStatus::IDLE;
    }
    
    if (device_id.empty()) {
        response->set_success(false);
        response->set_message("Device ID cannot be empty");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Device ID cannot be empty");
    }
    
    bool success = device_manager_->RegisterDevice(device_id, device_name, device_type, initial_status);
    
    if (success) {
        response->set_success(true);
        response->set_message("Device registered successfully");
        response->set_device_id(device_id);
        return grpc::Status::OK;
    } else {
        response->set_success(false);
        response->set_message("Device with ID '" + device_id + "' already exists");
        return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Device already exists");
    }
}

grpc::Status DeviceManagementServiceImpl::SetDeviceStatus(
    grpc::ServerContext* /* context */,
    const SetDeviceStatusRequest* request,
    SetDeviceStatusResponse* response) {
    
    const std::string& device_id = request->device_id();
    DeviceStatus new_status = request->status();
    
    if (device_id.empty()) {
        response->set_success(false);
        response->set_message("Device ID cannot be empty");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Device ID cannot be empty");
    }
    
    if (new_status == DeviceStatus::DEVICE_STATUS_UNKNOWN) {
        response->set_success(false);
        response->set_message("Invalid device status");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid device status");
    }
    
    DeviceStatus previous_status;
    bool success = device_manager_->SetDeviceStatus(device_id, new_status, &previous_status);
    
    if (success) {
        response->set_success(true);
        response->set_message("Device status updated successfully");
        response->set_previous_status(previous_status);
        response->set_current_status(new_status);
        return grpc::Status::OK;
    } else {
        response->set_success(false);
        response->set_message("Device with ID '" + device_id + "' not found");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
    }
}

grpc::Status DeviceManagementServiceImpl::GetDeviceInfo(
    grpc::ServerContext* /* context */,
    const GetDeviceInfoRequest* request,
    GetDeviceInfoResponse* response) {
    
    const std::string& device_id = request->device_id();
    
    if (device_id.empty()) {
        response->set_success(false);
        response->set_message("Device ID cannot be empty");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Device ID cannot be empty");
    }
    
    auto device_info = device_manager_->GetDeviceInfo(device_id);
    
    if (device_info) {
        response->set_success(true);
        response->set_message("Device information retrieved successfully");
        response->mutable_device_info()->CopyFrom(*device_info);
        return grpc::Status::OK;
    } else {
        response->set_success(false);
        response->set_message("Device with ID '" + device_id + "' not found");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
    }
}

grpc::Status DeviceManagementServiceImpl::InitiateDeviceAction(
    grpc::ServerContext* /* context */,
    const InitiateDeviceActionRequest* request,
    InitiateDeviceActionResponse* response) {
    
    const std::string& device_id = request->device_id();
    ActionType action_type = request->action_type();
    
    if (device_id.empty()) {
        response->set_success(false);
        response->set_message("Device ID cannot be empty");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Device ID cannot be empty");
    }
    
    if (action_type == ActionType::ACTION_TYPE_UNKNOWN) {
        response->set_success(false);
        response->set_message("Invalid action type");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid action type");
    }
    
    if (!device_manager_->DeviceExists(device_id)) {
        response->set_success(false);
        response->set_message("Device with ID '" + device_id + "' not found");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
    }
    
    auto device_info = device_manager_->GetDeviceInfo(device_id);
    if (device_info && !device_info->current_action_id().empty()) {
        response->set_success(false);
        response->set_message("Device is already busy with action: " + device_info->current_action_id());
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Device is already busy");
    }
    
    std::map<std::string, std::string> action_params;
    for (const auto& param : request->action_params()) {
        action_params[param.first] = param.second;
    }
    
    std::string action_id = action_simulator_->InitiateAction(
        device_id,
        action_type,
        action_params,
        nullptr
    );
    
    auto action_info = action_simulator_->GetActionStatus(action_id);
    
    if (action_info) {
        response->set_success(true);
        response->set_message("Action initiated successfully");
        response->set_action_id(action_id);
        response->set_action_status(action_info->status());
        return grpc::Status::OK;
    } else {
        response->set_success(false);
        response->set_message("Failed to initiate action");
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to initiate action");
    }
}

grpc::Status DeviceManagementServiceImpl::GetDeviceActionStatus(
    grpc::ServerContext* /* context */,
    const GetDeviceActionStatusRequest* request,
    GetDeviceActionStatusResponse* response) {
    
    const std::string& action_id = request->action_id();
    
    if (action_id.empty()) {
        response->set_success(false);
        response->set_message("Action ID cannot be empty");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Action ID cannot be empty");
    }
    
    auto action_info = action_simulator_->GetActionStatus(action_id);
    
    if (action_info) {
        response->set_success(true);
        response->set_message("Action status retrieved successfully");
        response->mutable_action_info()->CopyFrom(*action_info);
        return grpc::Status::OK;
    } else {
        response->set_success(false);
        response->set_message("Action with ID '" + action_id + "' not found");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Action not found");
    }
}

grpc::Status DeviceManagementServiceImpl::ListDevices(
    grpc::ServerContext* /* context */,
    const ListDevicesRequest* /* request */,
    ListDevicesResponse* response) {
    
    std::vector<DeviceInfo> devices = device_manager_->ListAllDevices();
    
    response->set_success(true);
    response->set_message("Retrieved " + std::to_string(devices.size()) + " device(s)");
    
    for (const auto& device : devices) {
        DeviceInfo* device_info = response->add_devices();
        device_info->CopyFrom(device);
    }
    
    return grpc::Status::OK;
}

}
