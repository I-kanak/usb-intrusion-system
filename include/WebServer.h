#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

class USBMonitor;
class Logger;

struct Session {
    std::string sessionId;
    std::string username;
    time_t createTime;
    time_t lastAccess;
    bool isAuthenticated;
    Session() : createTime(0), lastAccess(0), isAuthenticated(false) {}
};

class WebServer {
public:
    WebServer(int port, USBMonitor* usbMonitor, Logger* logger);
    ~WebServer();

    bool Start();
    void Stop();
    bool IsRunning() const;

private:
    int m_port;
    bool m_running;
    USBMonitor* m_usbMonitor;
    Logger* m_logger;

    mutable std::mutex m_sessionsMutex;
    std::unordered_map<std::string, Session> m_sessions;

    void SetupRoutes();
    std::string HandleRequest(const std::string& method, const std::string& path, const std::string& body, const std::unordered_map<std::string, std::string>& headers);

    std::string HandleLogin(const std::string& body);
    std::string HandleLogout(const std::string& sessionId);
    bool IsValidSession(const std::string& sessionId);
    std::string CreateSession(const std::string& username);
    void CleanupExpiredSessions();

    std::string HandleAdminPage(const std::string& sessionId);
    std::string HandleDashboard(const std::string& sessionId);
    std::string HandleDevicesAPI(const std::string& sessionId);
    std::string HandleLogsAPI(const std::string& sessionId);
    std::string HandleDeviceControlAPI(const std::string& method, const std::string& path, const std::string& body);

    std::string GenerateSessionId();
    std::string ExtractSessionFromCookie(const std::string& cookie);

    std::string CreateLoginPage(const std::string& error = "");
    std::string CreateDashboardPage();
    std::string CreateDevicesPage();
    std::string CreateLogsPage();

    std::string GenerateHTMLResponse(const std::string& title, const std::string& content, bool includeNav = true);
    std::string GenerateNavigation();
    std::string GenerateDeviceTable();
    std::string GenerateLogTable();

    std::string GenerateDevicesJSON();
    std::string GenerateLogsJSON();
    std::string GenerateStatusJSON();

    std::unordered_map<std::string, std::string> ParseFormData(const std::string& body);
    std::string URLDecode(const std::string& str);
    std::string HTMLEscape(const std::string& str);
};
