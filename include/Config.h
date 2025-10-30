#pragma once

constexpr int WEB_SERVER_PORT = 8080;
constexpr const char* ADMIN_USERNAME = "admin";
constexpr const char* ADMIN_PASSWORD = "secure123";
constexpr int SESSION_TIMEOUT = 3600;

constexpr const char* SMTP_SERVER = "smtp.gmail.com";
constexpr int SMTP_PORT = 587;
constexpr const char* EMAIL_FROM = "your_email@gmail.com";
constexpr const char* EMAIL_TO = "admin@company.com";
constexpr const char* EMAIL_USERNAME = "your_email@gmail.com";
constexpr const char* EMAIL_PASSWORD = "your_app_password";
constexpr bool EMAIL_NOTIFICATIONS_ENABLED = false;

constexpr const char* LOG_FILE = "usb_events.log";
constexpr const char* DEVICE_LOG_FILE = "device_status.log";
constexpr size_t MAX_LOG_SIZE = 10 * 1024 * 1024;
constexpr int MAX_LOG_FILES = 5;

const char USB_STORAGE_GUID[] = "{53f56307-b6bf-11d0-94f2-00a0c91efb8b}";
const char USB_DEVICE_GUID[] = "{a5dcbf10-6530-11d2-901f-00c04fb951ed}";

constexpr const char* USBSTOR_REG_KEY = "SYSTEM\\CurrentControlSet\\Services\\USBSTOR";
constexpr bool AUTO_DENY_UNKNOWN_DEVICES = false;
