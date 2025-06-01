#include "ConfigManager.h"
#include <fstream>   // Still needed for basic file checks or future use
#include <iostream>  // For std::wcerr
#include <algorithm> // For std::find
#include <mutex>
#include <cstring>   // For strlen (used in logging e.what())

#include "../Logging/Logging.h" // For LOG_ macros

#ifdef _WIN32
#include <windows.h>
#endif

// Define static mutex member
std::mutex ConfigManager::m_mutex;

// Helper to convert wstring to UTF-8 string for JSON keys/values (Commented out as JSON is stubbed)
/*
static std::string wstringToUtf8_cm(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.length(), NULL, 0, NULL, NULL);
    if (size_needed == 0 && wstr.length() > 0) { return std::string(); }
    if (size_needed == 0) return std::string();
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#else
    // Basic stub...
    std::string str; str.reserve(wstr.length());
    for(char32_t c : wstr) {
        if (c < 0x80) { str += static_cast<char>(c); }
        // ... (rest of basic UTF-8 conversion stub)
        else { str += '?';} // Simplified for brevity
    }
    return str;
#endif
}

// Helper to convert UTF-8 string from JSON back to wstring (Commented out)
static std::wstring utf8ToWstring_cm(const std::string& str) {
    if (str.empty()) return std::wstring();
#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.length(), NULL, 0);
    if (size_needed == 0 && str.length() > 0) { return std::wstring(); }
    if (size_needed == 0) return std::wstring();
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.length(), &wstrTo[0], size_needed);
    return wstrTo;
#else
    // Basic stub...
    std::wstring wstr; wstr.reserve(str.length());
    for(char c : str) { wstr += static_cast<wchar_t>(static_cast<unsigned char>(c)); }
    return wstr;
#endif
}
*/

ConfigManager& ConfigManager::getInstance() {
    std::lock_guard<std::mutex> lock(m_mutex);
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    LOG_DEBUG(L"ConfigManager: Constructor called.");
#ifdef _WIN32
    WCHAR path_buf[MAX_PATH];
    if (GetModuleFileNameW(NULL, path_buf, MAX_PATH) == 0) {
        LOG_ERROR(L"ConfigManager: Failed to get module file name. Error: " + std::to_wstring(GetLastError())
                   + L". Using current directory for settings.json.");
        try {
            m_settingsFilePath = std::filesystem::current_path() / "settings.json";
        } catch (const std::filesystem::filesystem_error& e) {
            LOG_ERROR(L"ConfigManager: Filesystem error getting current_path: " + std::wstring(e.what(), e.what() + strlen(e.what())) + L". Using relative settings.json.");
            m_settingsFilePath = "settings.json";
        }
    } else {
        try {
            std::filesystem::path executablePath = path_buf;
            m_settingsFilePath = executablePath.parent_path() / "settings.json";
        } catch (const std::exception& e) {
             LOG_ERROR(L"ConfigManager: Filesystem error creating settings path: " + std::wstring(e.what(), e.what() + strlen(e.what()))
                        + L". Using current directory for settings.json.");
             m_settingsFilePath = std::filesystem::current_path() / "settings.json";
        }
    }
#else
    try {
        m_settingsFilePath = std::filesystem::current_path() / "settings.json";
    } catch (const std::exception& e) {
        LOG_ERROR(L"ConfigManager (non-Windows): Filesystem error creating settings path: " + std::wstring(e.what(), e.what() + strlen(e.what()))
                   + L". Using relative 'settings.json'.");
        m_settingsFilePath = "settings.json";
    }
#endif
    LOG_INFO(L"ConfigManager: Settings file path determined as: " + m_settingsFilePath.wstring());

    applyDefaultSettings();
    loadSettings();
}

ConfigManager::~ConfigManager() {
    LOG_DEBUG(L"ConfigManager: Destructor called.");
    // saveSettings(); // Optionally save on exit
}

std::filesystem::path ConfigManager::getSettingsFilePath() const {
    return m_settingsFilePath;
}

void ConfigManager::applyDefaultSettings() {
    LOG_INFO(L"ConfigManager: Applying default settings.");
    m_webClassifierEnabled = true;
    m_useRecycleBin = true;
    m_excludedPaths.clear();

    // m_settingsJson = json::object(); // JSON part commented out
    // m_settingsJson["webClassifierEnabled"] = m_webClassifierEnabled;
    // m_settingsJson["useRecycleBin"] = m_useRecycleBin;
    // m_settingsJson["excludedPaths"] = json::array();
}

bool ConfigManager::loadSettings() {
    LOG_INFO(L"ConfigManager::loadSettings() called (JSON I/O is STUBBED OUT). Defaults are applied.");
    return false;
}

bool ConfigManager::saveSettings() {
    LOG_INFO(L"ConfigManager::saveSettings() called (JSON I/O is STUBBED OUT).");
    return true;
}

bool ConfigManager::isWebClassifierEnabled() const { return m_webClassifierEnabled; }
void ConfigManager::setWebClassifierEnabled(bool enabled) {
    if (m_webClassifierEnabled != enabled) {
        m_webClassifierEnabled = enabled;
        LOG_INFO(L"ConfigManager: Web Classifier Enabled set to " + std::wstring(enabled ? L"true" : L"false") + L". (Saving stubbed)");
        saveSettings();
    }
}

bool ConfigManager::shouldUseRecycleBin() const { return m_useRecycleBin; }
void ConfigManager::setShouldUseRecycleBin(bool useRecycleBin) {
    if (m_useRecycleBin != useRecycleBin) {
        m_useRecycleBin = useRecycleBin;
        LOG_INFO(L"ConfigManager: Use Recycle Bin set to " + std::wstring(useRecycleBin ? L"true" : L"false") + L". (Saving stubbed)");
        saveSettings();
    }
}

const std::vector<std::wstring>& ConfigManager::getExcludedPaths() const { return m_excludedPaths; }

void ConfigManager::addExcludedPath(const std::wstring& path) {
    if (path.empty()) {
        LOG_WARNING(L"ConfigManager: Attempted to add empty path to exclusions.");
        return;
    }
    if (std::find(m_excludedPaths.begin(), m_excludedPaths.end(), path) == m_excludedPaths.end()) {
        m_excludedPaths.push_back(path);
        LOG_INFO(L"ConfigManager: Added exclusion: " + path + L". (Saving stubbed)");
        saveSettings();
    } else {
        LOG_DEBUG(L"ConfigManager: Path already in exclusions: " + path);
    }
}

void ConfigManager::removeExcludedPath(const std::wstring& path) {
    auto it = std::find(m_excludedPaths.begin(), m_excludedPaths.end(), path);
    if (it != m_excludedPaths.end()) {
        m_excludedPaths.erase(it);
        LOG_INFO(L"ConfigManager: Removed exclusion: " + path + L". (Saving stubbed)");
        saveSettings();
    } else {
        LOG_WARNING(L"ConfigManager: Attempted to remove path not in exclusions: " + path);
    }
}

void ConfigManager::setExcludedPaths(const std::vector<std::wstring>& paths) {
    m_excludedPaths = paths;
    LOG_INFO(L"ConfigManager: Excluded paths set (count: " + std::to_wstring(paths.size()) + L"). (Saving stubbed)");
    saveSettings();
}
