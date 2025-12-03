#include <iostream>
#include <string>
#include <signal.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "device_management_service_impl.h"

using grpc::Server;
using grpc::ServerBuilder;

using device_management::DeviceManagementServiceImpl; //create an instance of the service implementation

Server* g_server = nullptr; 

void SignalHandler(int signal) { //Used to handle the signal and shutdown the server gracefully
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully...\n";
    if (g_server) {
        g_server->Shutdown();
    }
}

int ParsePort(int argc, char** argv) {  //Used to extract the port from the command line arguments
    int port = 50051;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[i + 1]);
            break;
        }
    }
    
    return port;
}


void RunServer(int port) {
    std::string server_address = "0.0.0.0:" + std::to_string(port);
    
    DeviceManagementServiceImpl service;
    
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    
    ServerBuilder builder;
    
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    
    builder.RegisterService(&service);
    
    std::unique_ptr<Server> server(builder.BuildAndStart());
    g_server = server.get();
    
    std::cout << "========================================\n";
    std::cout << "Device Fleet Management Service\n";
    std::cout << "========================================\n";
    std::cout << "Server listening on " << server_address << "\n";
    std::cout << "Press Ctrl+C to shutdown\n";
    std::cout << "========================================\n";
    
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    server->Wait();
    
    std::cout << "Server shutdown complete.\n";
}

int main(int argc, char** argv) {
    try {
        int port = ParsePort(argc, argv);
        RunServer(port);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
