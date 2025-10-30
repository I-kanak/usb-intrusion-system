#include "EmailSender.h"
#include "Config.h"
#include <iostream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

EmailSender::EmailSender()
    : m_smtpServer(SMTP_SERVER)
    , m_smtpPort(SMTP_PORT)
    , m_username(EMAIL_USERNAME)
    , m_password(EMAIL_PASSWORD)
    , m_fromAddress(EMAIL_FROM) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

EmailSender::~EmailSender() {
    WSACleanup();
}

bool EmailSender::SendEmail(const std::string& to, const std::string& subject, const std::string& body) {
    if (!IsConfigured()) {
        m_lastError = "Email sender not properly configured";
        return false;
    }
    try {
        return SendRawEmail(to, subject, body);
    } catch (const std::exception& e) {
        m_lastError = std::string("Email send failed: ") + e.what();
        return false;
    }
}

bool EmailSender::SendRawEmail(const std::string& to, const std::string& subject, const std::string& body) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        m_lastError = "Failed to create socket";
        return false;
    }
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* result = nullptr;
    if (getaddrinfo(m_smtpServer.c_str(), std::to_string(m_smtpPort).c_str(), &hints, &result) != 0) {
        closesocket(sock);
        m_lastError = "Failed to resolve SMTP server";
        return false;
    }
    if (connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(sock);
        m_lastError = "Failed to connect to SMTP server";
        return false;
    }
    freeaddrinfo(result);

    char buffer[1024];
    recv(sock, buffer, sizeof(buffer) - 1, 0);

    std::string helo = "HELO localhost\r\n";
    send(sock, helo.c_str(), static_cast<int>(helo.length()), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);

    std::string mailFrom = "MAIL FROM: <" + m_fromAddress + ">\r\n";
    send(sock, mailFrom.c_str(), static_cast<int>(mailFrom.length()), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);

    std::string rcptTo = "RCPT TO: <" + to + ">\r\n";
    send(sock, rcptTo.c_str(), static_cast<int>(rcptTo.length()), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);

    std::string data = "DATA\r\n";
    send(sock, data.c_str(), static_cast<int>(data.length()), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);

    std::ostringstream email;
    email << "From: " << m_fromAddress << "\r\n";
    email << "To: " << to << "\r\n";
    email << "Subject: " << subject << "\r\n";
    email << "Content-Type: text/plain; charset=UTF-8\r\n";
    email << "\r\n";
    email << body << "\r\n";
    email << ".\r\n";

    std::string emailContent = email.str();
    send(sock, emailContent.c_str(), static_cast<int>(emailContent.length()), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);

    std::string quit = "QUIT\r\n";
    send(sock, quit.c_str(), static_cast<int>(quit.length()), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);

    closesocket(sock);
    return true;
}

// These setters just update the values; not always needed for basic config
void EmailSender::SetSMTPServer(const std::string& server, int port) { m_smtpServer = server; m_smtpPort = port; }
void EmailSender::SetCredentials(const std::string& username, const std::string& password) { m_username = username; m_password = password; }
void EmailSender::SetFromAddress(const std::string& fromAddress) { m_fromAddress = fromAddress; }
bool EmailSender::IsConfigured() const { return !m_smtpServer.empty() && m_smtpPort > 0 && !m_fromAddress.empty(); }
std::string EmailSender::GetLastError() const { return m_lastError; }
std::string EmailSender::Base64Encode(const std::string& input) {
    // Use a standard base64 implementation here if you need SMTP AUTH
    return input;
}
