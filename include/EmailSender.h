#pragma once
#include <string>

class EmailSender {
public:
    EmailSender();
    ~EmailSender();

    bool SendEmail(const std::string& to, const std::string& subject, const std::string& body);
    bool SendEmailWithAttachment(const std::string& to, const std::string& subject, const std::string& body, const std::string& attachmentPath);

    void SetSMTPServer(const std::string& server, int port);
    void SetCredentials(const std::string& username, const std::string& password);
    void SetFromAddress(const std::string& fromAddress);

    bool IsConfigured() const;
    std::string GetLastError() const;

private:
    std::string m_smtpServer;
    int m_smtpPort;
    std::string m_username;
    std::string m_password;
    std::string m_fromAddress;
    std::string m_lastError;

    bool ConnectToSMTP();
    bool AuthenticateToSMTP();
    bool SendSMTPCommand(const std::string& command, const std::string& expectedResponse);
    std::string Base64Encode(const std::string& input);
    bool SendRawEmail(const std::string& to, const std::string& subject, const std::string& body);
};
