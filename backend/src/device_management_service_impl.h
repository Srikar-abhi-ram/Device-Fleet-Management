#ifndef DEVICE_MANAGEMENT_SERVICE_IMPL_H
#define DEVICE_MANAGEMENT_SERVICE_IMPL_H

#include "device_management.grpc.pb.h"
#include "device_manager.h"
#include <memory>

namespace device_management{

    class DeviceManagementServiceImpl final : public DeviceManagementService::Service{
        public:
        DeviceManagementServiceImpl();
        ~DeviceManagementServiceImpl();

        grpc::Status RegisterDevice(grpc::ServerContext* context,
            const RegisterDeviceRequest* request,
            RegisterDeviceResponse* response) override;

        grpc::Status SetDeviceStatus(grpc::ServerContext* context,
                    const SetDeviceStatusRequest* request,
                    SetDeviceStatusResponse* response) override;

        grpc::Status GetDeviceInfo(grpc::ServerContext* context,
                const GetDeviceInfoRequest* request,
                GetDeviceInfoResponse* response) override;

        grpc::Status InitiateDeviceAction(grpc::ServerContext* context,
                        const InitiateDeviceActionRequest* request,
                        InitiateDeviceActionResponse* response) override;

        grpc::Status GetDeviceActionStatus(grpc::ServerContext* context,
                        const GetDeviceActionStatusRequest* request,
                        GetDeviceActionStatusResponse* response) override;

        grpc::Status ListDevices(grpc::ServerContext* context,
                const ListDevicesRequest* request,
                ListDevicesResponse* response) override;

    private:
    std::unique_ptr<DeviceManager> device_manager_;
    std::unique_ptr<ActionSimulator> action_simulator_;
    };
}

#endif