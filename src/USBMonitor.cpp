#include "../include/USBMonitor.h"
#include "../include/Logger.h"
#include "../include/EmailSender.h"
#include "../include/DeviceController.h"
#include "../include/Config.h"
#include <initguid.h> 
#include <usbioctl.h>
#include <windows.h>
#include <setupapi.h>
#include <dbt.h>
#include <winioctl.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#pragma comment(lib, "setupapi.lib")

USBMonitor::USBMonitor(HWND messageWindow, Logger* logger)
    : m_messageWindow(messageWindow)
    , m_logger(logger)
    , m_deviceNotificationHandle(nullptr) {

    m_emailSender = std::make_unique<EmailSender>();
    m_deviceController = std::make_unique<DeviceController>(logger);
}

USBMonitor::~USBMonitor() {
    UnregisterDeviceNotifications();
}

bool USBMonitor::Initialize() {
    if (!RegisterForDeviceNotifications()) {
        m_logger->Log("USBMonitor", "Failed to register for device notifications");
        return false;
    }
    m_logger->Log("USBMonitor", "USB monitoring initialized successfully");
    return true;
}

bool USBMonitor::RegisterForDeviceNotifications() {
    DEV_BROADCAST_DEVICEINTERFACE notificationFilter = {0};
    notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notificationFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

    m_deviceNotificationHandle = RegisterDeviceNotification(
        m_messageWindow,
        &notificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE
    );
if (m_deviceNotificationHandle == nullptr) {
    m_logger->LogError("USBMonitor", "Failed to register device notification");
    return false;
    }
}
void USBMonitor::UnregisterDeviceNotifications() {
    if (m_deviceNotificationHandle) {
        UnregisterDeviceNotification(m_deviceNotificationHandle);
        m_deviceNotificationHandle = nullptr;
    }
}

void USBMonitor::HandleDeviceChange(WPARAM wParam, LPARAM lParam) {
    switch (wParam) {
        case DBT_DEVICEARRIVAL:
            OnDeviceArrival(reinterpret_cast<PDEV_BROADCAST_HDR>(lParam));
            break;

        case DBT_DEVICEREMOVECOMPLETE:
            OnDeviceRemoval(reinterpret_cast<PDEV_BROADCAST_HDR>(lParam));
            break;

        default:
            break;
    }
}

void USBMonitor::OnDeviceArrival(PDEV_BROADCAST_HDR pHdr) {
    if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
        PDEV_BROADCAST_DEVICEINTERFACE pDevInterface =
            reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(pHdr);

#ifdef UNICODE
        std::wstring devicePathW(pDevInterface->dbcc_name);
        std::string devicePath(devicePathW.begin(), devicePathW.end());
#else
        std::string devicePath(pDevInterface->dbcc_name);
#endif

        std::cout << "[Debug] devicePath: " << devicePath << std::endl;
        std::cout << "[Debug] IsStorageDevice: " << IsStorageDevice(devicePath) << std::endl;

        if (IsStorageDevice(devicePath)) {
            USBDeviceInfo deviceInfo;
            deviceInfo.devicePath = devicePath;
            deviceInfo.deviceId = ExtractDeviceId(devicePath);
            deviceInfo.friendlyName = "USB Storage Device";
            deviceInfo.isAllowed = true;

            {
                std::lock_guard<std::mutex> lock(m_devicesMutex);
                m_connectedDevices[deviceInfo.deviceId] = deviceInfo;
                std::cout << "[Debug] Map size after insert: " << m_connectedDevices.size() << std::endl;
            }

            LogDeviceEvent("Device Inserted", deviceInfo);
            SendDeviceNotification(deviceInfo, true);
        }
    }
}

void USBMonitor::OnDeviceRemoval(PDEV_BROADCAST_HDR pHdr) {
    std::cout << "[Debug] OnDeviceRemoval called" << std::endl;  // Debug log

    if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
        PDEV_BROADCAST_DEVICEINTERFACE pDevInterface =
            reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(pHdr);

#ifdef UNICODE
        std::wstring devicePathW(pDevInterface->dbcc_name);
        std::string devicePath(devicePathW.begin(), devicePathW.end());
#else
        std::string devicePath(pDevInterface->dbcc_name);
#endif

        std::cout << "[Debug] Device path: " << devicePath << std::endl;  // Debug log

        std::string deviceId = ExtractDeviceId(devicePath);

        USBDeviceInfo deviceInfo;
        {
            std::lock_guard<std::mutex> lock(m_devicesMutex);
            auto it = m_connectedDevices.find(deviceId);
            if (it != m_connectedDevices.end()) {
                deviceInfo = it->second;
                m_connectedDevices.erase(it);
            }
        }

        if (!deviceInfo.deviceId.empty()) {
            LogDeviceEvent("Device Removed", deviceInfo);
        }
    }
}

std::vector<USBDeviceInfo> USBMonitor::GetConnectedDevices() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    std::vector<USBDeviceInfo> devices;
    for (const auto& pair : m_connectedDevices) {
        devices.push_back(pair.second);
    }
    return devices;
}

bool USBMonitor::AllowDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_connectedDevices.find(deviceId);
    if (it != m_connectedDevices.end()) {
        it->second.isAllowed = true;
        m_deviceController->EnableDevice(deviceId);
        m_logger->Log("USBMonitor", "Device allowed: " + deviceId);
        return true;
    }
    return false;
}

bool USBMonitor::DenyDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_connectedDevices.find(deviceId);
    if (it != m_connectedDevices.end()) {
        it->second.isAllowed = false;
        m_deviceController->DisableDevice(deviceId);
        m_logger->Log("USBMonitor", "Device denied: " + deviceId);
        return true;
    }
    return false;
}

bool USBMonitor::IsDeviceAllowed(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_connectedDevices.find(deviceId);
    return it != m_connectedDevices.end() && it->second.isAllowed;
}

USBDeviceInfo USBMonitor::GetDeviceInfo(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_connectedDevices.find(deviceId);
    if (it != m_connectedDevices.end()) {
        return it->second;
    }
    return USBDeviceInfo();
}

size_t USBMonitor::GetDeviceCount() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_connectedDevices.size();
}

std::string USBMonitor::ExtractDeviceId(const std::string& devicePath) {
    size_t vidPos = devicePath.find("VID_");
    size_t pidPos = devicePath.find("PID_");

    if (vidPos != std::string::npos && pidPos != std::string::npos) {
        std::string vid = devicePath.substr(vidPos + 4, 4);
        std::string pid = devicePath.substr(pidPos + 4, 4);
        return "VID_" + vid + "&PID_" + pid;
    }
    return devicePath;
}

std::string USBMonitor::ExtractVendorId(const std::string& hardwareId) {
    size_t pos = hardwareId.find("VID_");
    if (pos != std::string::npos) {
        return hardwareId.substr(pos + 4, 4);
    }
    return "Unknown";
}

std::string USBMonitor::ExtractProductId(const std::string& hardwareId) {
    size_t pos = hardwareId.find("PID_");
    if (pos != std::string::npos) {
        return hardwareId.substr(pos + 4, 4);
    }
    return "Unknown";
}

std::string USBMonitor::GetDeviceProperty(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD property) {
    DWORD dataType, requiredSize;

    if (!SetupDiGetDeviceRegistryProperty(hDevInfo, pDevInfoData, property,
                                         &dataType, nullptr, 0, &requiredSize)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::vector<BYTE> buffer(requiredSize);

            if (SetupDiGetDeviceRegistryProperty(hDevInfo, pDevInfoData, property,
                                               &dataType, buffer.data(), requiredSize, nullptr)) {
                return std::string(reinterpret_cast<char*>(buffer.data()));
            }
        }
    }
    return "";
}

std::string USBMonitor::GetDriveLetterFromDevicePath(const std::string& devicePath) {
    return ""; // You may implement logic for this if needed
}

bool USBMonitor::IsStorageDevice(const std::string& devicePath) {
    std::cout << "[Debug] Checking devicePath: " << devicePath << std::endl;
    return true; // or your actual filter logic
}


void USBMonitor::LogDeviceEvent(const std::string& event, const USBDeviceInfo& device) {
    std::ostringstream oss;
    oss << event << " - Device: " << device.deviceId
        << ", Name: " << device.friendlyName;
    m_logger->Log("USBEvent", oss.str());
}

void USBMonitor::SendDeviceNotification(const USBDeviceInfo& device, bool inserted) {
    if (inserted && EMAIL_NOTIFICATIONS_ENABLED) {
        std::string subject = "USB Device Inserted: " + device.friendlyName;
        std::ostringstream body;
        body << "A USB storage device has been inserted:\n\n"
             << "Device ID: " << device.deviceId << "\n"
             << "Friendly Name: " << device.friendlyName << "\n"
             << "Time: " << GetCurrentTimeString() << "\n\n"
             << "Please review this device in the admin panel.";
        m_emailSender->SendEmail(EMAIL_TO, subject, body.str());
    }
}

std::string USBMonitor::GetCurrentTimeString() const {
    SYSTEMTIME st;
    GetLocalTime(&st);

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << st.wYear << "-"
        << std::setw(2) << st.wMonth << "-"
        << std::setw(2) << st.wDay << " "
        << std::setw(2) << st.wHour << ":"
        << std::setw(2) << st.wMinute << ":"
        << std::setw(2) << st.wSecond;

    return oss.str();
}
