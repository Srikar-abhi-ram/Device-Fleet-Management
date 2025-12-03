cmake_minimum_required(VERSION 3.15)
project(DeviceFleetManagement VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard (C++17 for modern features)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type (Release by default, Debug for development)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Threads REQUIRED)

# On macOS with Homebrew, there's a known issue where finding Protobuf before gRPC
# causes target conflicts. Solution: find gRPC first, which handles Protobuf internally.
# Then we access Protobuf variables that gRPC has set up.
find_package(gRPC REQUIRED)

# gRPC should have found Protobuf, but we need to ensure protoc is available
# Try to find protoc executable directly if Protobuf_PROTOC_EXECUTABLE is not set
if(NOT Protobuf_PROTOC_EXECUTABLE)
    find_program(Protobuf_PROTOC_EXECUTABLE
        NAMES protoc
        PATHS
            /opt/homebrew/bin
            /usr/local/bin
            /usr/bin
        DOC "Protocol Buffers compiler"
    )
    if(NOT Protobuf_PROTOC_EXECUTABLE)
        message(FATAL_ERROR "protoc not found. Please install Protocol Buffers.")
    endif()
endif()

# Find gRPC C++ plugin if not set by find_package
if(NOT gRPC_CPP_PLUGIN_EXECUTABLE)
    find_program(gRPC_CPP_PLUGIN_EXECUTABLE
        NAMES grpc_cpp_plugin
        PATHS
            /opt/homebrew/bin
            /usr/local/bin
            /usr/bin
        DOC "gRPC C++ plugin"
    )
    if(NOT gRPC_CPP_PLUGIN_EXECUTABLE)
        message(FATAL_ERROR "grpc_cpp_plugin not found. Please install gRPC.")
    endif()
endif()

# Create generated directory for protobuf/gRPC code
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)

# Protocol Buffers and gRPC code generation
# This generates C++ code from the .proto file
set(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/../proto/device_management.proto
)

# Generate protobuf and gRPC code
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_PATH ${PROTO_FILE} PATH)
    
    # Generated files
    set(PROTO_SRC ${CMAKE_CURRENT_BINARY_DIR}/generated/${PROTO_NAME}.pb.cc)
    set(PROTO_HDR ${CMAKE_CURRENT_BINARY_DIR}/generated/${PROTO_NAME}.pb.h)
    set(GRPC_SRC ${CMAKE_CURRENT_BINARY_DIR}/generated/${PROTO_NAME}.grpc.pb.cc)
    set(GRPC_HDR ${CMAKE_CURRENT_BINARY_DIR}/generated/${PROTO_NAME}.grpc.pb.h)
    
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    list(APPEND GRPC_SRCS ${GRPC_SRC})
    list(APPEND GRPC_HDRS ${GRPC_HDR})
    
    # Generate protobuf code
    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/generated
             --proto_path=${CMAKE_SOURCE_DIR}/../proto
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf code for ${PROTO_NAME}"
    )
    
    # Generate gRPC code
    add_custom_command(
        OUTPUT ${GRPC_SRC} ${GRPC_HDR}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}/generated
             --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/generated
             --plugin=protoc-gen-grpc=${gRPC_CPP_PLUGIN_EXECUTABLE}
             --proto_path=${CMAKE_SOURCE_DIR}/../proto
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE} ${PROTO_SRC} ${PROTO_HDR}
        COMMENT "Generating gRPC code for ${PROTO_NAME}"
    )
endforeach()

# Source files
set(SOURCES
    src/device_manager.cpp
    src/device_management_service_impl.cpp
    src/server.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

# Header files (for IDE support)
set(HEADERS
    src/device_manager.h
    src/device_management_service_impl.h
    ${PROTO_HDRS}
    ${GRPC_HDRS}
)

# Create executable
add_executable(device_management_server ${SOURCES} ${HEADERS})

# Link libraries
# Use protobuf targets from gRPC if available, otherwise use Protobuf_LIBRARIES
if(TARGET protobuf::libprotobuf)
    target_link_libraries(device_management_server PRIVATE
        gRPC::grpc++
        gRPC::grpc++_reflection
        protobuf::libprotobuf
        Threads::Threads
    )
else()
    target_link_libraries(device_management_server PRIVATE
        gRPC::grpc++
        gRPC::grpc++_reflection
        ${Protobuf_LIBRARIES}
        Threads::Threads
    )
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(device_management_server PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Print configuration summary
message(STATUS "=========================================")
message(STATUS "Device Fleet Management Service")
message(STATUS "=========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Protobuf version: ${Protobuf_VERSION}")
message(STATUS "gRPC version: ${gRPC_VERSION}")
message(STATUS "=========================================")

