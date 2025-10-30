#include <windows.h>
#include "../include/DeviceController.h"
#include "../include/Logger.h"
#include "../include/Config.h"
#include <setupapi.h>
#include <devguid.h>
#include <usbioctl.h>
#include <winioctl.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "setupapi.lib")

DeviceController::DeviceController(Logger* logger) : m_logger(logger) {}
DeviceController::~DeviceController() {}

HDEVINFO DeviceController::GetDeviceInfoSet() const {
    return SetupDiGetClassDevs(&GUID_DEVCLASS_USB, nullptr, nullptr,
                              DIGCF_PRESENT | DIGCF_ALLCLASSES);
}

bool DeviceController::FindDevice(HDEVINFO hDevInfo, const std::string& deviceId, SP_DEVINFO_DATA& devInfoData) const {
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
        DWORD dataType, requiredSize;
        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID,
                                           &dataType, nullptr, 0, &requiredSize)) {
            continue;
        }
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::vector<char> buffer(requiredSize);
            if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID,
                                               &dataType, reinterpret_cast<PBYTE>(buffer.data()),
                                               requiredSize, nullptr)) {
                std::string hardwareId(buffer.data());
                if (hardwareId.find(deviceId) != std::string::npos) return true;
            }
        }
    }
    return false;
}

bool DeviceController::EnableDevice(const std::string& deviceId) {
    m_logger->Log("DeviceController", "Attempting to enable device: " + deviceId);
    bool result = ChangeDeviceState(deviceId, true);
    if (result) m_logger->Log("DeviceController", "Successfully enabled device: " + deviceId);
    else m_logger->LogError("DeviceController", "Failed to enable device: " + deviceId);
    return result;
}

bool DeviceController::DisableDevice(const std::string& deviceId) {
    m_logger->Log("DeviceController", "Attempting to disable device: " + deviceId);
    bool result = ChangeDeviceState(deviceId, false);
    if (result) m_logger->Log("DeviceController", "Successfully disabled device: " + deviceId);
    else m_logger->LogError("DeviceController", "Failed to disable device: " + deviceId);
    return result;
}

bool DeviceController::ChangeDeviceState(const std::string& deviceId, bool enable) {
    HDEVINFO hDevInfo = GetDeviceInfoSet();
    if (hDevInfo == INVALID_HANDLE_VALUE) return false;

    SP_DEVINFO_DATA devInfoData;
    if (!FindDevice(hDevInfo, deviceId, devInfoData)) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return false;
    }

    SP_PROPCHANGE_PARAMS propChangeParams = {};
    propChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    propChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    propChangeParams.StateChange = enable ? DICS_ENABLE : DICS_DISABLE;
    propChangeParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
    propChangeParams.HwProfile = 0;

    bool success = SetupDiSetClassInstallParams(hDevInfo, &devInfoData,
        &propChangeParams.ClassInstallHeader, sizeof(propChangeParams));

    if (success) {
        success = SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, &devInfoData);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return success;
}

bool DeviceController::EnableUSBStorage() {
    m_logger->Log("DeviceController", "Enabling USB storage");
    bool result = SetRegistryValue(HKEY_LOCAL_MACHINE, USBSTOR_REG_KEY, "Start", 3);
    if (result) m_logger->Log("DeviceController", "USB storage enabled successfully");
    else m_logger->LogError("DeviceController", "Failed to enable USB storage");
    return result;
}

bool DeviceController::DisableUSBStorage() {
    m_logger->Log("DeviceController", "Disabling USB storage");
    bool result = SetRegistryValue(HKEY_LOCAL_MACHINE, USBSTOR_REG_KEY, "Start", 4);
    if (result) m_logger->Log("DeviceController", "USB storage disabled successfully");
    else m_logger->LogError("DeviceController", "Failed to disable USB storage");
    return result;
}

bool DeviceController::IsUSBStorageEnabled() const {
    DWORD startType;
    if (GetRegistryValue(HKEY_LOCAL_MACHINE, USBSTOR_REG_KEY, "Start", startType)) {
        return startType == 3;
    }
    return false;
}

bool DeviceController::SetRegistryValue(HKEY hKey, const std::string& subKey, const std::string& valueName, DWORD value) {
    HKEY hSubKey;
    LONG result = RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_SET_VALUE, &hSubKey);
    if (result != ERROR_SUCCESS) return false;
    result = RegSetValueExA(hSubKey, valueName.c_str(), 0, REG_DWORD,
                          reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(hSubKey);
    return result == ERROR_SUCCESS;
}

bool DeviceController::GetRegistryValue(HKEY hKey, const std::string& subKey, const std::string& valueName, DWORD& value) const {
    HKEY hSubKey;
    LONG result = RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_QUERY_VALUE, &hSubKey);
    if (result != ERROR_SUCCESS) return false;
    DWORD dataSize = sizeof(value);
    result = RegQueryValueExA(hSubKey, valueName.c_str(), nullptr, nullptr,
                           reinterpret_cast<BYTE*>(&value), &dataSize);
    RegCloseKey(hSubKey);
    return result == ERROR_SUCCESS;
}

std::vector<std::string> DeviceController::GetAllUSBDevices() const {
    std::vector<std::string> devices;
    HDEVINFO hDevInfo = GetDeviceInfoSet();
    if (hDevInfo == INVALID_HANDLE_VALUE) return devices;
    SP_DEVINFO_DATA devInfoData = {};
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
        DWORD dataType, requiredSize;
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_DEVICEDESC,
                                            &dataType, nullptr, 0, &requiredSize) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::vector<char> buffer(requiredSize);
            if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_DEVICEDESC,
                                               &dataType, reinterpret_cast<PBYTE>(buffer.data()),
                                               requiredSize, nullptr)) {
                devices.push_back(std::string(buffer.data()));
            }
        }
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return devices;
}

bool DeviceController::IsDeviceEnabled(const std::string& deviceId) const {
    HDEVINFO hDevInfo = GetDeviceInfoSet();
    if (hDevInfo == INVALID_HANDLE_VALUE) return false;
    SP_DEVINFO_DATA devInfoData;
    if (!FindDevice(hDevInfo, deviceId, devInfoData)) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return false;
    }
    // Additional status check logic can be added here
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return true;
}
