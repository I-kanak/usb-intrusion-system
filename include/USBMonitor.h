#pragma once

#include <windows.h>
#include <dbt.h>
#include <setupapi.h>
#include <devguid.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

class Logger;
class EmailSender;
class DeviceController;

struct USBDeviceInfo {
    std::string deviceId;
    std::string vendorId;
    std::string productId;
    std::string serialNumber;
    std::string friendlyName;
    std::string driveLetter;
    std::string devicePath;
    bool isAllowed;
    FILETIME insertTime;

    USBDeviceInfo() : isAllowed(false) {
        GetSystemTimeAsFileTime(&insertTime);
    }
};

class USBMonitor {
public:
    USBMonitor(HWND messageWindow, Logger* logger);
    ~USBMonitor();
    bool Initialize();
    void HandleDeviceChange(WPARAM wParam, LPARAM lParam);

    std::vector<USBDeviceInfo> GetConnectedDevices() const;
    bool AllowDevice(const std::string& deviceId);
    bool DenyDevice(const std::string& deviceId);
    bool IsDeviceAllowed(const std::string& deviceId) const;
    USBDeviceInfo GetDeviceInfo(const std::string& deviceId) const;
    size_t GetDeviceCount() const;

private:
    HWND m_messageWindow;
    Logger* m_logger;
    std::unique_ptr<EmailSender> m_emailSender;
    std::unique_ptr<DeviceController> m_deviceController;

    mutable std::mutex m_devicesMutex;
    std::unordered_map<std::string, USBDeviceInfo> m_connectedDevices;
    HDEVNOTIFY m_deviceNotificationHandle;

    void OnDeviceArrival(PDEV_BROADCAST_HDR pHdr);
    void OnDeviceRemoval(PDEV_BROADCAST_HDR pHdr);
    bool RegisterForDeviceNotifications();
    void UnregisterDeviceNotifications();

    std::string ExtractDeviceId(const std::string& devicePath);
    std::string GetDeviceProperty(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD property);
    std::string GetDriveLetterFromDevicePath(const std::string& devicePath);
    bool IsStorageDevice(const std::string& devicePath);
    std::string ExtractVendorId(const std::string& hardwareId);
    std::string ExtractProductId(const std::string& hardwareId);
    std::string GetCurrentTimeString() const;
    void LogDeviceEvent(const std::string& event, const USBDeviceInfo& device);
    void SendDeviceNotification(const USBDeviceInfo& device, bool inserted);
};
