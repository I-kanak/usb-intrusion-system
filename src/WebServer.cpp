#include "../include/WebServer.h"
#include "../include/USBMonitor.h"
#include "../include/Logger.h"
#include "../include/Config.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Helper function to escape backslashes and quotes in JSON strings
std::string escape_json(const std::string& input) {
    std::string out;
    for (char c : input) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else out += c;
    }
    return out;
}

WebServer::WebServer(int port, USBMonitor* usbMonitor, Logger* logger)
    : m_port(port), m_running(false), m_usbMonitor(usbMonitor), m_logger(logger)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

WebServer::~WebServer() {
    Stop();
    WSACleanup();
}

bool WebServer::Start() {
    if (m_running) return false;
    m_logger->Log("WebServer", "Starting web server on port " + std::to_string(m_port));
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        m_logger->LogError("WebServer", "Failed to create listen socket");
        return false;
    }
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_short>(m_port));
    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        m_logger->LogError("WebServer", "Failed to bind socket to port " + std::to_string(m_port));
        closesocket(listenSocket);
        return false;
    }
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        m_logger->LogError("WebServer", "Failed to listen on socket");
        closesocket(listenSocket);
        return false;
    }
    m_running = true;
    m_logger->Log("WebServer", "Web server started successfully");

    while (m_running) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            if (m_running) m_logger->LogError("WebServer", "Failed to accept client connection");
            continue;
        }
        char buffer[4096] = {};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        buffer[bytesReceived] = '\0'; // Null-terminate!
        std::string request(buffer, bytesReceived);

        std::string response;
        // GET /api/devices endpoint
        if (request.find("GET /api/devices") == 0) {
            std::vector<USBDeviceInfo> devices = m_usbMonitor ? m_usbMonitor->GetConnectedDevices() : std::vector<USBDeviceInfo>();
            std::ostringstream json;
            json << "[";
            for (size_t i = 0; i < devices.size(); ++i) {
                if (i > 0) json << ",";
                json << "{";
                json << "\"deviceId\":\"" << escape_json(devices[i].deviceId) << "\",";
                json << "\"friendlyName\":\"" << escape_json(devices[i].friendlyName) << "\",";
                json << "\"isAllowed\":" << (devices[i].isAllowed ? "true" : "false") << ",";
                json << "\"devicePath\":\"" << escape_json(devices[i].devicePath) << "\"";
                json << "}";
            }
            json << "]";
            std::string jsonStr = json.str();
            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: application/json\r\n"
                << "Access-Control-Allow-Origin: *\r\n"
                << "Content-Length: " << jsonStr.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << jsonStr;
            response = oss.str();
        }
        // POST /api/device/allow endpoint
        else if (request.find("POST /api/device/allow") == 0) {
            std::string body = request.substr(request.find("\r\n\r\n") + 4);
            bool ok = m_usbMonitor && m_usbMonitor->AllowDevice(body);
            std::string jsonStr = std::string("{\"success\":") + (ok ? "true" : "false") + "}";
            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: application/json\r\n"
                << "Access-Control-Allow-Origin: *\r\n"
                << "Content-Length: " << jsonStr.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << jsonStr;
            response = oss.str();
        }
        // POST /api/device/deny endpoint
        else if (request.find("POST /api/device/deny") == 0) {
            std::string body = request.substr(request.find("\r\n\r\n") + 4);
            bool ok = m_usbMonitor && m_usbMonitor->DenyDevice(body);
            std::string jsonStr = std::string("{\"success\":") + (ok ? "true" : "false") + "}";
            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: application/json\r\n"
                << "Access-Control-Allow-Origin: *\r\n"
                << "Content-Length: " << jsonStr.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << jsonStr;
            response = oss.str();
        }
        // Fallback 404
        else {
            std::string notFound = "404 Not Found";
            std::ostringstream oss;
            oss << "HTTP/1.1 404 Not Found\r\n"
                << "Content-Type: text/plain\r\n"
                << "Content-Length: " << notFound.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << notFound;
            response = oss.str();
        }
        send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        closesocket(clientSocket);
    }
    closesocket(listenSocket);
    return true;
}

void WebServer::Stop() {
    m_running = false;
    m_logger->Log("WebServer", "Web server stopped");
}

bool WebServer::IsRunning() const {
    return m_running;
}
