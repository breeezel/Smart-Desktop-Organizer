#include "Logging.h"
#include <chrono>    // For std::chrono::system_clock, time_point
#include <iomanip>   // For std::put_time
#include <sstream>   // For std::wostringstream
#include <iostream>  // For std::wcerr if log file fails

#ifdef _WIN32
#include <windows.h> // For GetModuleFileNameW
// For _localtime_s alternative (localtime_s is C11 standard, but _localtime_s for MSVC)
#include <time.h>
#else
#include <time.h>    // For localtime_r (POSIX) or localtime
#endif

// --- Logger Singleton Implementation ---
Logger& Logger::getInstance() {
    // Meyers' Singleton: thread-safe initialization in C++11 and later.
    static Logger instance;
    return instance;
}

// --- Logger Constructor & Destructor ---
Logger::Logger() {
    // Determine log file path (e.g., next to executable or in user app data folder)
#ifdef _WIN32
    WCHAR path_buf[MAX_PATH];
    if (GetModuleFileNameW(NULL, path_buf, MAX_PATH) == 0) {
        // Fallback if GetModuleFileNameW fails
        std::wcerr << L"Logger: Failed to get module file name. Error: " << GetLastError()
                   << L". Using current directory for log file." << std::endl;
        try {
             m_logFilePath = std::filesystem::current_path() / "SmartDesktopOrganizer.log";
        } catch (const std::filesystem::filesystem_error& e) {
            std::wcerr << L"Logger: Filesystem error getting current_path: " << e.what() << L". Using relative log path." << std::endl;
            m_logFilePath = "SmartDesktopOrganizer.log"; // Last resort
        }
    } else {
        try {
            std::filesystem::path executablePath = path_buf;
            m_logFilePath = executablePath.parent_path() / "SmartDesktopOrganizer.log";
        } catch (const std::filesystem::filesystem_error& e) {
            std::wcerr << L"Logger: Filesystem error creating log path: " << e.what() << L". Using relative log path." << std::endl;
            m_logFilePath = "SmartDesktopOrganizer.log"; // Last resort
        }
    }
#else
    // For non-Windows, place it in current directory or use a platform-specific path
    // (e.g., ~/.cache/SmartDesktopOrganizer/app.log or similar based on XDG)
    try {
        m_logFilePath = std::filesystem::current_path() / "SmartDesktopOrganizer.log"; // Simple default
    } catch (const std::filesystem::filesystem_error& e) {
         std::wcerr << L"Logger: Filesystem error getting current_path: " << e.what() << L". Using relative log path." << std::endl;
         m_logFilePath = "SmartDesktopOrganizer.log"; // Last resort
    }
#endif

    // std::wcout << L"Logger: Log file path: " << m_logFilePath.wstring() << std::endl; // For debugging path

    // Open the log file in append mode.
    // Using std::ios_base::binary can be good practice even for text files on Windows
    // to ensure consistent newline handling (\n vs \r\n), though wofstream might handle it.
    m_logFileStream.open(m_logFilePath, std::ios_base::app | std::ios_base::binary);
    if (!m_logFileStream.is_open()) {
        std::wcerr << L"Logger: CRITICAL - Failed to open log file for writing: " << m_logFilePath.wstring() << std::endl;
    } else {
        // Optional: Write an initial session start message
        // This call to logInfo will acquire the lock.
        // logInfo(L"Logging session started."); // This might be too early if other static initializations depend on logging
    }
}

Logger::~Logger() {
    if (m_logFileStream.is_open()) {
        // This call to logInfo will acquire the lock.
        logInfo(L"Logging session ended.");
        m_logFileStream.flush(); // Ensure all buffered logs are written
        m_logFileStream.close();
    }
}

// --- Private Helper Methods ---

std::wstring Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG_MSG: return L"DEBUG";
        case LogLevel::INFO:    return L"INFO";
        case LogLevel::WARNING: return L"WARNING";
        case LogLevel::ERROR_MSG:  return L"ERROR";
        default: {
            // Should not happen with a well-defined enum
            wchar_t buffer[20];
            swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"LVL_%d", static_cast<int>(level));
            return buffer;
        }
    }
}

std::wstring Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::wostringstream ss;
    std::tm timeinfo_tm;

#ifdef _WIN32
    // Use localtime_s on Windows (thread-safe)
    if (localtime_s(&timeinfo_tm, &in_time_t) != 0) {
        // Handle error, e.g., return an error string or throw
        ss << L"TimestampError";
        return ss.str();
    }
#else
    // Use localtime_r on POSIX systems (thread-safe)
    // For other C++ standard compliant systems, std::localtime might be used
    // but its thread-safety is not guaranteed by the standard itself.
    // Most modern systems provide a thread-safe localtime_r or similar.
    if (localtime_r(&in_time_t, &timeinfo_tm) == nullptr) {
        // Handle error
        ss << L"TimestampError";
        return ss.str();
    }
#endif
    ss << std::put_time(&timeinfo_tm, L"%Y-%m-%d %H:%M:%S"); // ISO 8601 like format
    return ss.str();
}

// --- Public Logging Methods ---

void Logger::log(LogLevel level, const std::wstring& message) {
    // Acquire lock to ensure thread-safe access to the file stream
    std::lock_guard<std::mutex> guard(m_logMutex);

    // If file wasn't opened in constructor, or closed due to an error, try to (re)open.
    if (!m_logFileStream.is_open()) {
        // Attempt to reopen. This is a fallback.
        m_logFileStream.open(m_logFilePath, std::ios_base::app | std::ios_base::binary);
        if (!m_logFileStream.is_open()) {
             // If still not open, log to standard error as a last resort.
             // Avoid using std::wcerr directly if it might not be initialized or thread-safe without precautions.
             // For this example, assuming std::wcerr is available for critical failures.
             std::wcerr << L"Logger: Log file not open. Original message: [" << levelToString(level)
                        << L"] " << getCurrentTimestamp() << L" - " << message << std::endl;
             return;
        }
    }

    // Write the formatted log message to the file stream
    m_logFileStream << getCurrentTimestamp() << L" [" << levelToString(level) << L"] " << message << std::endl;

    // Optionally flush for critical messages, but be mindful of performance.
    // Errors are flushed immediately in logError.
    // if (level == LogLevel::ERROR_MSG || level == LogLevel::WARNING) {
    //     m_logFileStream.flush();
    // }
}

void Logger::logDebug(const std::wstring& message) {
    log(LogLevel::DEBUG_MSG, message);
}

void Logger::logInfo(const std::wstring& message) {
    log(LogLevel::INFO, message);
}

void Logger::logWarning(const std::wstring& message) {
    log(LogLevel::WARNING, message);
}

void Logger::logError(const std::wstring& message) {
    log(LogLevel::ERROR_MSG, message);
    // Ensure critical errors are written to disk immediately.
    if (m_logFileStream.is_open()) {
        std::lock_guard<std::mutex> guard(m_logMutex); // Re-acquire lock if flush isn't inherently safe with stream state
        m_logFileStream.flush();
    }
}
