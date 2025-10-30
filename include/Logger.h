#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <memory>

class Logger {
public:
    explicit Logger(const std::string& logFileName);
    ~Logger();

    void Log(const std::string& category, const std::string& message);
    void LogError(const std::string& category, const std::string& error);
    void LogWarning(const std::string& category, const std::string& warning);
    void LogInfo(const std::string& category, const std::string& info);

    void FlushLog();
    bool RotateLogIfNeeded();
    size_t GetLogSize() const;
    std::vector<std::string> GetRecentEntries(size_t count = 100) const;

private:
    std::string m_logFileName;
    mutable std::mutex m_logMutex;
    std::unique_ptr<std::ofstream> m_logFile;

    void WriteLogEntry(const std::string& level, const std::string& category, const std::string& message);
    std::string GetCurrentTimestamp() const;
    bool OpenLogFile();
    void CloseLogFile();
    std::string GetRotatedFileName(int index) const;
};
