

#!/usr/bin/env python3

import sys
import os
import time
import grpc
import shlex
from typing import Dict, Optional

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'generated'))

try:
    import device_management_pb2
    import device_management_pb2_grpc 
except ImportError:
    print("Error: gRPC code not found. Please run './setup.sh' or './generate_grpc_code.sh' to generate it.")
    print("See README.md for instructions.")
    sys.exit(1)


class DeviceManagementClient:
    
    def __init__(self, server_address: str = "localhost:50051"):
        # keep the target endpoint configurable for local or docker use
        self.server_address = server_address
        self.channel = None
        self.stub = None
    
    def connect(self):
        try:
            self.channel = grpc.insecure_channel(self.server_address)
            self.stub = device_management_pb2_grpc.DeviceManagementServiceStub(self.channel)
            grpc.channel_ready_future(self.channel).result(timeout=5)
            return True
        except grpc.FutureTimeoutError:
            print(f"Error: Could not connect to server at {self.server_address}")
            print("Make sure the server is running.")
            return False
        except Exception as e:
            print(f"Error connecting to server: {e}")
            return False
    
    def close(self):
        if self.channel:
            self.channel.close()
    
    def register_device(self, device_id: str, device_name: str = "", 
                       device_type: str = "", initial_status: str = "IDLE") -> bool:
        try:
            status_enum = self._parse_device_status(initial_status)
            if status_enum is None:
                print(f"Error: Invalid status '{initial_status}'")
                return False
            
            request = device_management_pb2.RegisterDeviceRequest(
                device_id=device_id,
                device_name=device_name,
                device_type=device_type,
                initial_status=status_enum
            )
            
            response = self.stub.RegisterDevice(request)
            
            if response.success:
                print(f"✓ Device '{device_id}' registered successfully")
                return True
            else:
                print(f"✗ Failed to register device: {response.message}")
                return False
                
        except grpc.RpcError as e:
            print(f"✗ gRPC error: {e.code()} - {e.details()}")
            return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def set_device_status(self, device_id: str, status: str) -> bool:
        try:
            status_enum = self._parse_device_status(status)
            if status_enum is None:
                print(f"Error: Invalid status '{status}'")
                return False
            
            request = device_management_pb2.SetDeviceStatusRequest(
                device_id=device_id,
                status=status_enum
            )
            
            response = self.stub.SetDeviceStatus(request)
            
            if response.success:
                print(f"✓ Device '{device_id}' status updated:")
                print(f"  Previous: {self._device_status_to_string(response.previous_status)}")
                print(f"  Current:  {self._device_status_to_string(response.current_status)}")
                return True
            else:
                print(f"✗ Failed to update status: {response.message}")
                return False
                
        except grpc.RpcError as e:
            print(f"✗ gRPC error: {e.code()} - {e.details()}")
            return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def get_device_info(self, device_id: str) -> bool:
        try:
            request = device_management_pb2.GetDeviceInfoRequest(device_id=device_id)
            
            response = self.stub.GetDeviceInfo(request)
            
            if response.success:
                device = response.device_info
                print(f"\nDevice Information:")
                print(f"  ID:            {device.device_id}")
                print(f"  Name:          {device.device_name}")
                print(f"  Type:          {device.device_type}")
                print(f"  Status:        {self._device_status_to_string(device.status)}")
                print(f"  Registered:    {self._format_timestamp(device.registered_at)}")
                print(f"  Last Updated:  {self._format_timestamp(device.last_updated)}")
                if device.current_action_id:
                    print(f"  Current Action: {device.current_action_id}")
                else:
                    print(f"  Current Action: None")
                return True
            else:
                print(f"✗ {response.message}")
                return False
                
        except grpc.RpcError as e:
            print(f"✗ gRPC error: {e.code()} - {e.details()}")
            return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def list_devices(self) -> bool:
        try:
            request = device_management_pb2.ListDevicesRequest()
            
            response = self.stub.ListDevices(request)
            
            if response.success:
                devices = response.devices
                if len(devices) == 0:
                    print("No devices registered.")
                    return True
                
                print(f"\nRegistered Devices ({len(devices)} total):")
                print("-" * 80)
                print(f"{'Device ID':<20} {'Name':<15} {'Type':<15} {'Status':<15} {'Action':<20}")
                print("-" * 80)
                
                for device in devices:
                    action = device.current_action_id[:17] + "..." if len(device.current_action_id) > 20 else device.current_action_id or "None"
                    print(f"{device.device_id:<20} {device.device_name:<15} {device.device_type:<15} "
                          f"{self._device_status_to_string(device.status):<15} {action:<20}")
                
                return True
            else:
                print(f"✗ {response.message}")
                return False
                
        except grpc.RpcError as e:
            print(f"✗ gRPC error: {e.code()} - {e.details()}")
            return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def initiate_action(self, device_id: str, action_type: str, 
                       action_params: Dict[str, str] = None) -> Optional[str]:
        try:
            action_enum = self._parse_action_type(action_type)
            if action_enum is None:
                print(f"Error: Invalid action type '{action_type}'")
                return None
            
            request = device_management_pb2.InitiateDeviceActionRequest(
                device_id=device_id,
                action_type=action_enum
            )
            
            if action_params:
                for key, value in action_params.items():
                    request.action_params[key] = value
            
            response = self.stub.InitiateDeviceAction(request)
            
            if response.success:
                print(f"✓ Action initiated successfully")
                print(f"  Action ID: {response.action_id}")
                print(f"  Status:    {self._action_status_to_string(response.action_status)}")
                return response.action_id
            else:
                print(f"✗ Failed to initiate action: {response.message}")
                return None
                
        except grpc.RpcError as e:
            print(f"✗ gRPC error: {e.code()} - {e.details()}")
            return None
        except Exception as e:
            print(f"✗ Error: {e}")
            return None
    
    def get_action_status(self, action_id: str) -> bool:
        try:
            request = device_management_pb2.GetDeviceActionStatusRequest(action_id=action_id)
            
            response = self.stub.GetDeviceActionStatus(request)
            
            if response.success:
                action = response.action_info
                print(f"\nAction Information:")
                print(f"  Action ID:     {action.action_id}")
                print(f"  Device ID:     {action.device_id}")
                print(f"  Action Type:   {self._action_type_to_string(action.action_type)}")
                print(f"  Status:        {self._action_status_to_string(action.status)}")
                print(f"  Initiated:     {self._format_timestamp(action.initiated_at)}")
                
                if action.completed_at > 0:
                    print(f"  Completed:    {self._format_timestamp(action.completed_at)}")
                    duration = action.completed_at - action.initiated_at
                    print(f"  Duration:     {duration} seconds")
                else:
                    print(f"  Completed:    In progress...")
                
                if action.action_params:
                    print(f"  Parameters:")
                    for key, value in action.action_params.items():
                        print(f"    {key} = {value}")
                
                if action.error_message:
                    print(f"  Error:        {action.error_message}")
                
                return True
            else:
                print(f"✗ {response.message}")
                return False
                
        except grpc.RpcError as e:
            print(f"✗ gRPC error: {e.code()} - {e.details()}")
            return False
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def poll_action_status(self, action_id: str, interval: float = 2.0) -> bool:
        print(f"Polling action '{action_id}' (interval: {interval}s)...")
        print("Press Ctrl+C to stop polling\n")
        
        try:
            while True:
                request = device_management_pb2.GetDeviceActionStatusRequest(action_id=action_id)
                response = self.stub.GetDeviceActionStatus(request)
                
                if not response.success:
                    print(f"✗ {response.message}")
                    return False
                
                action = response.action_info
                status_str = self._action_status_to_string(action.status)
                print(f"[{time.strftime('%H:%M:%S')}] Status: {status_str}", end="\r")
                
                if action.status == device_management_pb2.COMPLETED:
                    print(f"\n✓ Action completed successfully!")
                    return True
                elif action.status == device_management_pb2.FAILED:
                    print(f"\n✗ Action failed!")
                    if action.error_message:
                        print(f"  Error: {action.error_message}")
                    return False
                
                time.sleep(interval)
                
        except KeyboardInterrupt:
            print("\n\nPolling stopped by user.")
            return False
        except grpc.RpcError as e:
            print(f"\n✗ gRPC error: {e.code()} - {e.details()}")
            return False
        except Exception as e:
            print(f"\n✗ Error: {e}")
            return False
    
    def _parse_device_status(self, status_str: str) -> Optional[int]:
        status_map = {
            "IDLE": device_management_pb2.IDLE,
            "BUSY": device_management_pb2.BUSY,
            "OFFLINE": device_management_pb2.OFFLINE,
            "MAINTENANCE": device_management_pb2.MAINTENANCE,
            "UPDATING": device_management_pb2.UPDATING,
            "RECOVERING": device_management_pb2.RECOVERING,
            "ERROR": device_management_pb2.ERROR,
        }
        return status_map.get(status_str.upper())
    
    def _device_status_to_string(self, status_enum: int) -> str:
        status_map = {
            device_management_pb2.IDLE: "IDLE",
            device_management_pb2.BUSY: "BUSY",
            device_management_pb2.OFFLINE: "OFFLINE",
            device_management_pb2.MAINTENANCE: "MAINTENANCE",
            device_management_pb2.UPDATING: "UPDATING",
            device_management_pb2.RECOVERING: "RECOVERING",
            device_management_pb2.ERROR: "ERROR",
        }
        return status_map.get(status_enum, "UNKNOWN")
    
    def _parse_action_type(self, action_str: str) -> Optional[int]:
        action_map = {
            "SOFTWARE_UPDATE": device_management_pb2.SOFTWARE_UPDATE,
            "FIRMWARE_UPDATE": device_management_pb2.FIRMWARE_UPDATE,
            "SYSTEM_REBOOT": device_management_pb2.SYSTEM_REBOOT,
            "CONFIGURATION_CHANGE": device_management_pb2.CONFIGURATION_CHANGE,
        }
        return action_map.get(action_str.upper())
    
    def _action_type_to_string(self, action_enum: int) -> str:
        action_map = {
            device_management_pb2.SOFTWARE_UPDATE: "SOFTWARE_UPDATE",
            device_management_pb2.FIRMWARE_UPDATE: "FIRMWARE_UPDATE",
            device_management_pb2.SYSTEM_REBOOT: "SYSTEM_REBOOT",
            device_management_pb2.CONFIGURATION_CHANGE: "CONFIGURATION_CHANGE",
        }
        return action_map.get(action_enum, "UNKNOWN")
    
    def _action_status_to_string(self, status_enum: int) -> str:
        status_map = {
            device_management_pb2.PENDING: "PENDING",
            device_management_pb2.RUNNING: "RUNNING",
            device_management_pb2.COMPLETED: "COMPLETED",
            device_management_pb2.FAILED: "FAILED",
        }
        return status_map.get(status_enum, "UNKNOWN")
    
    def _format_timestamp(self, timestamp: int) -> str:
        if timestamp == 0:
            return "N/A"
        return time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(timestamp))


def parse_action_params(params_list: list) -> Dict[str, str]:
    params = {}
    for param in params_list:
        if "=" not in param:
            print(f"Warning: Invalid parameter format '{param}'. Expected KEY=VALUE")
            continue
        key, value = param.split("=", 1)
        params[key] = value
    return params


def print_help():
    print("\nAvailable commands:")
    print("  list")
    print("  register <device_id> [--name NAME] [--type TYPE] [--status STATUS]")
    print("  set-status <device_id> <status>")
    print("  get-info <device_id>")
    print("  initiate-action <device_id> <action_type> [--params KEY=VALUE ...]")
    print("  action-status <action_id>")
    print("  poll-action <action_id> [--interval SECONDS]")
    print("  help")
    print("  exit")
    print()


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Device Fleet Management CLI Client")
    parser.add_argument("--server", default="localhost:50051", help="gRPC server address")
    args = parser.parse_args()
    
    client = DeviceManagementClient(args.server)
    if not client.connect():
        return 1
    
    print("Device Fleet Management CLI")
    print("Type 'help' for available commands, 'exit' to quit")
    print()
    
    try:
        while True:
            try:
                command = input("> ").strip()
                
                if not command:
                    continue
                
                if command == "exit" or command == "quit":
                    break
                
                if command == "help":
                    print_help()
                    continue
                
                parts = shlex.split(command)
                
                if not parts:
                    continue
                
                cmd = parts[0].lower()
                
                if cmd == "list":
                    client.list_devices()
                
                elif cmd == "register":
                    if len(parts) < 2:
                        print("Error: Device ID required")
                        print("Usage: register <device_id> [--name NAME] [--type TYPE] [--status STATUS]")
                        continue
                    
                    device_id = parts[1]
                    name = ""
                    device_type = ""
                    status = "IDLE"
                    
                    i = 2
                    while i < len(parts):
                        if parts[i] == "--name" and i + 1 < len(parts):
                            name = parts[i + 1]
                            i += 2
                        elif parts[i] == "--type" and i + 1 < len(parts):
                            device_type = parts[i + 1]
                            i += 2
                        elif parts[i] == "--status" and i + 1 < len(parts):
                            status = parts[i + 1]
                            i += 2
                        else:
                            i += 1
                    
                    client.register_device(device_id, name, device_type, status)
                
                elif cmd == "set-status":
                    if len(parts) < 3:
                        print("Error: Device ID and status required")
                        print("Usage: set-status <device_id> <status>")
                        continue
                    client.set_device_status(parts[1], parts[2])
                
                elif cmd == "get-info":
                    if len(parts) < 2:
                        print("Error: Device ID required")
                        print("Usage: get-info <device_id>")
                        continue
                    client.get_device_info(parts[1])
                
                elif cmd == "initiate-action":
                    if len(parts) < 3:
                        print("Error: Device ID and action type required")
                        print("Usage: initiate-action <device_id> <action_type> [--params KEY=VALUE ...]")
                        continue
                    
                    device_id = parts[1]
                    action_type = parts[2]
                    params = {}
                    
                    if "--params" in parts:
                        idx = parts.index("--params")
                        if idx + 1 < len(parts):
                            param_list = parts[idx + 1:]
                            params = parse_action_params(param_list)
                    
                    client.initiate_action(device_id, action_type, params if params else None)
                
                elif cmd == "action-status":
                    if len(parts) < 2:
                        print("Error: Action ID required")
                        print("Usage: action-status <action_id>")
                        continue
                    client.get_action_status(parts[1])
                
                elif cmd == "poll-action":
                    if len(parts) < 2:
                        print("Error: Action ID required")
                        print("Usage: poll-action <action_id> [--interval SECONDS]")
                        continue
                    
                    action_id = parts[1]
                    interval = 2.0
                    
                    if "--interval" in parts:
                        idx = parts.index("--interval")
                        if idx + 1 < len(parts):
                            try:
                                interval = float(parts[idx + 1])
                            except ValueError:
                                print("Error: Invalid interval value")
                                continue
                    
                    client.poll_action_status(action_id, interval)
                
                else:
                    print(f"Unknown command: {cmd}")
                    print("Type 'help' for available commands")
            
            except KeyboardInterrupt:
                print("\nUse 'exit' to quit")
            except EOFError:
                break
            except Exception as e:
                print(f"Error: {e}")
    
    finally:
        client.close()
        print("\nGoodbye!")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
