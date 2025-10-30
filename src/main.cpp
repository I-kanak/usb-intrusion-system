#include <windows.h>
#include <iostream>
#include <thread>
#include <memory>

#include "../include/Config.h"
#include "../include/USBMonitor.h"
#include "../include/WebServer.h"
#include "../include/Logger.h"

#ifndef HWND_MESSAGE
#define HWND_MESSAGE ((HWND)-3)
#endif

std::unique_ptr<USBMonitor> g_usbMonitor;
std::unique_ptr<WebServer> g_webServer;
std::unique_ptr<Logger> g_logger;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DEVICECHANGE:
            std::cout << "[Debug] WM_DEVICECHANGE received wParam = " << wParam << std::endl;
            if (g_usbMonitor) {
                g_usbMonitor->HandleDeviceChange(wParam, lParam);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

HWND CreateMessageWindow() {
    const char* className = "USBMonitorWindow";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = className;
    if (!RegisterClassA(&wc)) {
        std::cerr << "Failed to register window class." << std::endl;
        return nullptr;
    }
    return CreateWindowA(className, "USB Monitor", 0, 0, 0, 0, 0,
                        HWND_MESSAGE, nullptr, GetModuleHandleA(nullptr), nullptr);
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  USB Storage Device Monitoring System  " << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;

    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    if (!isAdmin) {
        std::cerr << "ERROR: Requires Administrator privileges." << std::endl;
        std::cerr << "Please run as Administrator." << std::endl;
        std::cout << std::endl << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    try {
        g_logger = std::make_unique<Logger>(LOG_FILE);
        g_logger->Log("System", "USB Monitor starting...");

        HWND messageWindow = CreateMessageWindow();
        if (!messageWindow) {
            throw std::runtime_error("Failed to create message window");
        }

        g_usbMonitor = std::make_unique<USBMonitor>(messageWindow, g_logger.get());
        if (!g_usbMonitor->Initialize()) {
            throw std::runtime_error("Failed to initialize USB monitoring");
        }

        std::cout << "[OK] USB monitoring initialized" << std::endl;

        g_webServer = std::make_unique<WebServer>(WEB_SERVER_PORT, g_usbMonitor.get(), g_logger.get());
        std::thread webServerThread([&]() {
            g_webServer->Start();
        });

        std::cout << "[OK] Web server started" << std::endl << std::endl;
        std::cout << "Web Interface: http://localhost:" << WEB_SERVER_PORT << std::endl;
        std::cout << "Press Ctrl+C to exit..." << std::endl << std::endl;

        g_logger->Log("System", "USB Monitor operational");

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        g_webServer->Stop();
        if (webServerThread.joinable()) {
            webServerThread.join();
        }

        g_logger->Log("System", "USB Monitor shutdown");
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        if (g_logger) {
            g_logger->Log("System", std::string("Fatal error: ") + e.what());
        }
        std::cout << std::endl << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    return 0;
}
