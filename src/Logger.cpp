
#include "../include/Logger.h"
#include "../include/Config.h"
#include <deque>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <windows.h>

Logger::Logger(const std::string& logFileName)
    : m_logFileName(logFileName) {
    OpenLogFile();
}

Logger::~Logger() {
    CloseLogFile();
}

void Logger::Log(const std::string& category, const std::string& message) {
    WriteLogEntry("INFO", category, message);
}

void Logger::LogError(const std::string& category, const std::string& error) {
    WriteLogEntry("ERROR", category, error);
}

void Logger::LogWarning(const std::string& category, const std::string& warning) {
    WriteLogEntry("WARN", category, warning);
}

void Logger::LogInfo(const std::string& category, const std::string& info) {
    WriteLogEntry("INFO", category, info);
}

void Logger::WriteLogEntry(const std::string& level, const std::string& category, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_logMutex);

    if (!m_logFile || !m_logFile->is_open()) {
        if (!OpenLogFile()) {
            std::cerr << "Failed to open log file: " << m_logFileName << std::endl;
            return;
        }
    }

    std::string timestamp = GetCurrentTimestamp();
    *m_logFile << timestamp << " [" << level << "] [" << category << "] " << message << std::endl;
    m_logFile->flush();

    std::cout << timestamp << " [" << level << "] [" << category << "] " << message << std::endl;

    RotateLogIfNeeded();
}

std::string Logger::GetCurrentTimestamp() const {
    SYSTEMTIME st;
    GetLocalTime(&st);

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << st.wYear << "-"
        << std::setw(2) << st.wMonth << "-"
        << std::setw(2) << st.wDay << " "
        << std::setw(2) << st.wHour << ":"
        << std::setw(2) << st.wMinute << ":"
        << std::setw(2) << st.wSecond << "."
        << std::setw(3) << st.wMilliseconds;

    return oss.str();
}

bool Logger::OpenLogFile() {
    try {
        m_logFile = std::make_unique<std::ofstream>(m_logFileName, std::ios::app);
        return m_logFile->is_open();
    } catch (const std::exception& e) {
        std::cerr << "Failed to open log file: " << e.what() << std::endl;
        return false;
    }
}

void Logger::CloseLogFile() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (m_logFile && m_logFile->is_open()) {
        m_logFile->close();
    }
}

size_t Logger::GetLogSize() const {
    try {
        return std::filesystem::file_size(m_logFileName);
    } catch (const std::exception&) {
        return 0;
    }
}

bool Logger::RotateLogIfNeeded() {
    if (GetLogSize() > MAX_LOG_SIZE) {
        CloseLogFile();

        for (int i = MAX_LOG_FILES - 1; i > 0; --i) {
            std::string oldFile = GetRotatedFileName(i - 1);
            std::string newFile = GetRotatedFileName(i);

            if (std::filesystem::exists(oldFile)) {
                std::filesystem::rename(oldFile, newFile);
            }
        }

        std::filesystem::rename(m_logFileName, GetRotatedFileName(1));

        return OpenLogFile();
    }

    return true;
}

std::string Logger::GetRotatedFileName(int index) const {
    return m_logFileName + "." + std::to_string(index);
}

std::vector<std::string> Logger::GetRecentEntries(size_t count) const {
    std::vector<std::string> entries;
    std::ifstream file(m_logFileName);

    if (file.is_open()) {
        std::string line;
        std::deque<std::string> recentLines;

        while (std::getline(file, line)) {
            recentLines.push_back(line);
            if (recentLines.size() > count) {
                recentLines.pop_front();
            }
        }

        entries.assign(recentLines.begin(), recentLines.end());
    }

    return entries;
}

void Logger::FlushLog() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (m_logFile) {
        m_logFile->flush();
    }
}
