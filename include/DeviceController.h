#pragma once
#include <string>
#include <vector>
#include <setupapi.h>
#include <windows.h>

// Forward declarations
struct _SP_DEVINFO_DATA; // from setupapi.h
typedef struct _SP_DEVINFO_DATA SP_DEVINFO_DATA;
typedef void* HDEVINFO;

class Logger;

class DeviceController {
public:
    explicit DeviceController(Logger* logger);
    ~DeviceController();

    bool EnableDevice(const std::string& deviceId);
    bool DisableDevice(const std::string& deviceId);
    bool IsDeviceEnabled(const std::string& deviceId) const;

    bool EnableUSBStorage();
    bool DisableUSBStorage();
    bool IsUSBStorageEnabled() const;

    std::vector<std::string> GetAllUSBDevices() const;
    std::vector<std::string> GetDisabledDevices() const;

private:
    Logger* m_logger;

    // Corrected method signatures here
    HDEVINFO GetDeviceInfoSet() const;
    bool FindDevice(HDEVINFO hDevInfo, const std::string& deviceId, SP_DEVINFO_DATA& devInfoData) const;

    bool SetRegistryValue(HKEY hKey, const std::string& subKey, const std::string& valueName, DWORD value);
    bool GetRegistryValue(HKEY hKey, const std::string& subKey, const std::string& valueName, DWORD& value) const;
    bool ChangeDeviceState(const std::string& deviceId, bool enable);
};
