#pragma once

#include <string>
#include <fstream>    // For std::wofstream
#include <filesystem> // For std::filesystem::path
#include <mutex>      // For std::mutex
#include <memory>     // For std::unique_ptr (if using PIMPL or specific singleton patterns)

// Enum for log levels
enum class LogLevel {
    DEBUG_MSG, // Renamed to avoid conflict if Windows.h DEBUG is defined by any chance
    INFO,
    WARNING,
    ERROR_MSG  // Renamed to avoid conflict if Windows.h ERROR is defined
};

class Logger {
public:
    // Static method to get the singleton instance
    static Logger& getInstance();

    // Logging methods
    void log(LogLevel level, const std::wstring& message);
    void logDebug(const std::wstring& message);
    void logInfo(const std::wstring& message);
    void logWarning(const std::wstring& message);
    void logError(const std::wstring& message);

    // Disable copy and assignment for singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();  // Private constructor for singleton
    ~Logger(); // Private destructor

    std::wstring levelToString(LogLevel level) const;
    std::wstring getCurrentTimestamp() const;
    // std::filesystem::path getLogFilePath() const; // Path determined in constructor

    std::wofstream m_logFileStream;            // Log file stream object
    std::filesystem::path m_logFilePath;       // Full path to the log file
    std::mutex m_logMutex;                   // Mutex for thread-safe logging operations
};

// Optional: Global convenience macros for logging
// These are often placed in the header for easy use across the project.
// Ensure they don't have ODR issues if defined directly in header without inline/static.
// A common practice is inline functions or macros that call the singleton.
#define LOG_DEBUG(message) Logger::getInstance().logDebug(message)
#define LOG_INFO(message) Logger::getInstance().logInfo(message)
#define LOG_WARNING(message) Logger::getInstance().logWarning(message)
#define LOG_ERROR(message) Logger::getInstance().logError(message)

// Example of inline function alternatives (safer than complex macros)
// inline void LogDebug(const std::wstring& message) { Logger::getInstance().logDebug(message); }
// inline void LogInfo(const std::wstring& message) { Logger::getInstance().logInfo(message); }
// inline void LogWarning(const std::wstring& message) { Logger::getInstance().logWarning(message); }
// inline void LogError(const std::wstring& message) { Logger::getInstance().logError(message); }
