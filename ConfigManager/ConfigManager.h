#pragma once

#include <string>
#include <vector>
#include <filesystem> // For std::filesystem::path
#include "../libs/nlohmann_json/json.hpp"
#include <mutex>

using json = nlohmann::json;

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadSettings();
    bool saveSettings();

    bool isWebClassifierEnabled() const;
    void setWebClassifierEnabled(bool enabled);

    bool shouldUseRecycleBin() const;
    void setShouldUseRecycleBin(bool useRecycleBin);

    const std::vector<std::wstring>& getExcludedPaths() const;
    void addExcludedPath(const std::wstring& path);
    void removeExcludedPath(const std::wstring& path);
    void setExcludedPaths(const std::vector<std::wstring>& paths);

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

private:
    ConfigManager();
    ~ConfigManager();

    std::filesystem::path getSettingsFilePath() const;
    void applyDefaultSettings();

    bool m_webClassifierEnabled;
    bool m_useRecycleBin;
    std::vector<std::wstring> m_excludedPaths;

    std::filesystem::path m_settingsFilePath;

    static std::mutex m_mutex;
};
