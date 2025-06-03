#include "ConfigManager.h"
#include <fstream>   // Still needed for basic file checks or future use
#include <iostream>  // For std::wcerr
#include <algorithm> // For std::find
#include <mutex>
#include <cstring>   // For strlen (used in logging e.what())
#include <iomanip>   // For std::setw in saveSettings

#include "../Logging/Logging.h" // For LOG_ macros

#ifdef _WIN32
#include <windows.h>
#endif

// Define static mutex member
std::mutex ConfigManager::m_mutex;

// Helper to convert wstring to UTF-8 string for JSON keys/values
static std::string wstringToUtf8_cm(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.length(), NULL, 0, NULL, NULL);
    if (size_needed == 0) { // Check if conversion failed or empty string
        if (wstr.length() > 0) {
             LOG_WARNING(L"wstringToUtf8_cm: WideCharToMultiByte failed for non-empty string. Error: " + std::to_wstring(GetLastError()));
        }
        return std::string();
    }
    std::string strTo(size_needed, 0);
    int result = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
    if (result == 0) {
        LOG_WARNING(L"wstringToUtf8_cm: WideCharToMultiByte failed during conversion. Error: " + std::to_wstring(GetLastError()));
        return std::string();
    }
    return strTo;
#else
    // Basic stub for non-Windows - consider a robust library like Boost.Locale or ICU for production
    std::string str; str.reserve(wstr.length());
    for(wchar_t wc : wstr) { // Use wchar_t for wider compatibility on Linux
        if (wc >= 0 && wc < 0x80) { // Basic ASCII
            str += static_cast<char>(wc);
        } else { // Non-ASCII characters are replaced or handled with more complex logic
            str += '?'; // Placeholder for non-ASCII
            LOG_DEBUG(L"wstringToUtf8_cm (stub): Non-ASCII character encountered, replaced with '?'.");
        }
    }
    return str;
#endif
}

// Helper to convert UTF-8 string from JSON back to wstring
static std::wstring utf8ToWstring_cm(const std::string& str) {
    if (str.empty()) return std::wstring();
#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.length(), NULL, 0);
    if (size_needed == 0) { // Check if conversion failed or empty string
        if (str.length() > 0) {
            LOG_WARNING(L"utf8ToWstring_cm: MultiByteToWideChar failed for non-empty string. Error: " + std::to_wstring(GetLastError()));
        }
        return std::wstring();
    }
    std::wstring wstrTo(size_needed, 0);
    int result = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.length(), &wstrTo[0], size_needed);
    if (result == 0) {
        LOG_WARNING(L"utf8ToWstring_cm: MultiByteToWideChar failed during conversion. Error: " + std::to_wstring(GetLastError()));
        return std::wstring();
    }
    return wstrTo;
#else
    // Basic stub for non-Windows
    std::wstring wstr; wstr.reserve(str.length());
    for(char c : str) { wstr += static_cast<wchar_t>(static_cast<unsigned char>(c)); } // Assumes simple char to wchar_t mapping
    return wstr;
#endif
}

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

    // Apply defaults first, then try to load settings which might override them.
    applyDefaultSettings();
    if (!loadSettings()) {
        LOG_WARNING(L"ConfigManager: Failed to load settings. Using default settings. Attempting to save defaults.");
        saveSettings(); // Try to save the default settings if loading failed
    }
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
    // No longer need to interact with m_settingsJson here
}

bool ConfigManager::loadSettings() {
    LOG_INFO(L"ConfigManager: Attempting to load settings from " + m_settingsFilePath.wstring());
    std::ifstream ifs(m_settingsFilePath);

    if (!ifs.is_open()) {
        LOG_WARNING(L"ConfigManager: Settings file not found or could not be opened. Using default settings.");
        return false;
    }

    // Check if file is empty
    if (ifs.peek() == std::ifstream::traits_type::eof()) {
        LOG_WARNING(L"ConfigManager: Settings file is empty. Using default settings.");
        ifs.close();
        return false;
    }

    json j;
    try {
        ifs >> j;
        ifs.close(); // Close file after successful parsing
    } catch (json::parse_error& e) {
        LOG_ERROR(L"ConfigManager: Failed to parse settings.json. Error: " + std::string(e.what()));
        ifs.close(); // Ensure file is closed
        // Optionally, attempt to rename or delete the corrupt file
        std::filesystem::path corruptFilePath = m_settingsFilePath;
        corruptFilePath += L".corrupt";
        try {
            std::filesystem::rename(m_settingsFilePath, corruptFilePath);
            LOG_WARNING(L"ConfigManager: Corrupt settings.json has been renamed to settings.json.corrupt");
        } catch (const std::filesystem::filesystem_error& fs_err) {
            LOG_ERROR(L"ConfigManager: Failed to rename corrupt settings.json. Error: " + std::string(fs_err.what()));
            // If rename fails, consider deleting
            // std::filesystem::remove(m_settingsFilePath);
        }
        return false;
    }

    LOG_INFO(L"ConfigManager: Successfully parsed settings.json. Applying loaded settings.");

    if (j.contains("webClassifierEnabled") && j["webClassifierEnabled"].is_boolean()) {
        m_webClassifierEnabled = j["webClassifierEnabled"].get<bool>();
        LOG_DEBUG(L"ConfigManager: Loaded webClassifierEnabled: " + std::wstring(m_webClassifierEnabled ? L"true" : L"false"));
    } else {
        LOG_WARNING(L"ConfigManager: 'webClassifierEnabled' not found or invalid type in settings.json. Using default.");
    }

    if (j.contains("useRecycleBin") && j["useRecycleBin"].is_boolean()) {
        m_useRecycleBin = j["useRecycleBin"].get<bool>();
        LOG_DEBUG(L"ConfigManager: Loaded useRecycleBin: " + std::wstring(m_useRecycleBin ? L"true" : L"false"));
    } else {
        LOG_WARNING(L"ConfigManager: 'useRecycleBin' not found or invalid type in settings.json. Using default.");
    }

    if (j.contains("excludedPaths") && j["excludedPaths"].is_array()) {
        m_excludedPaths.clear();
        for (const auto& item : j["excludedPaths"]) {
            if (item.is_string()) {
                std::string path_s = item.get<std::string>();
                std::wstring path_ws = utf8ToWstring_cm(path_s);
                if (!path_ws.empty()) {
                    m_excludedPaths.push_back(path_ws);
                    LOG_DEBUG(L"ConfigManager: Loaded excludedPath: " + path_ws);
                } else if (!path_s.empty()) {
                    LOG_WARNING(L"ConfigManager: Failed to convert excludedPath to wstring: " + std::wstring(path_s.begin(), path_s.end()));
                }
            } else {
                LOG_WARNING(L"ConfigManager: Non-string item found in 'excludedPaths' array. Skipping.");
            }
        }
        LOG_DEBUG(L"ConfigManager: Loaded " + std::to_wstring(m_excludedPaths.size()) + L" excluded paths.");
    } else {
        LOG_WARNING(L"ConfigManager: 'excludedPaths' not found or not an array in settings.json. Using default (empty list).");
        m_excludedPaths.clear(); // Ensure it's empty if not found or invalid
    }

    LOG_INFO(L"ConfigManager: Settings loaded successfully.");
    return true;
}

bool ConfigManager::saveSettings() {
    LOG_INFO(L"ConfigManager: Attempting to save settings to " + m_settingsFilePath.wstring());
    json j;

    j["webClassifierEnabled"] = m_webClassifierEnabled;
    j["useRecycleBin"] = m_useRecycleBin;

    json excludedPathsArray = json::array();
    for (const auto& path : m_excludedPaths) {
        std::string path_s = wstringToUtf8_cm(path);
        if (!path_s.empty()) {
            excludedPathsArray.push_back(path_s);
        } else if (!path.empty()) {
            LOG_WARNING(L"ConfigManager: Failed to convert excludedPath to UTF-8 string for saving: " + path);
        }
    }
    j["excludedPaths"] = excludedPathsArray;

    std::ofstream ofs(m_settingsFilePath);
    if (!ofs.is_open()) {
        LOG_ERROR(L"ConfigManager: Failed to open settings.json for writing. Error: " + std::to_wstring(errno)); // errno might be relevant
        return false;
    }

    try {
        ofs << std::setw(4) << j << std::endl;
        ofs.close(); // Ensure file is closed after writing
    } catch (const std::exception& e) { // Catch potential exceptions during write/serialization
        LOG_ERROR(L"ConfigManager: Exception while writing JSON to file: " + std::string(e.what()));
        ofs.close(); // Ensure file is closed
        return false;
    }

    LOG_INFO(L"ConfigManager: Settings saved successfully to " + m_settingsFilePath.wstring());
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
