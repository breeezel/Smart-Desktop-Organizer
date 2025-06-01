#pragma once
#include <string>
#include <vector>
#include "DesktopManager/DesktopSorter.h"
#include "DesktopManager/DesktopInfo.h"
#include "ConfigManager/ConfigManager.h" // Include ConfigManager
#include "Logging/Logging.h"             // For LOG_ macros

#ifdef _WIN32
#include <windows.h>
#include "libs/nlohmann_json/json.hpp"
using json = nlohmann::json;
#endif

class WebClassifier {
public:
    WebClassifier();
    ~WebClassifier();

    bool executeHttpGet(const std::string& url, std::string& response);
    ItemCategory performSearch(const DesktopItem& item, std::string& outHtmlResponse);
    ItemCategory classifyItemFromHtml(const std::string& htmlResponse, const std::wstring& itemName);

#ifdef _WIN32
    void clearCache();
#endif

private:
#ifdef _WIN32
    json classificationCache;
    std::wstring cacheFilePath;

    std::wstring getCacheFilePath();
    void loadCache();
    void saveCache();
    ItemCategory getCachedCategory(const std::wstring& itemNameOrKey);
    void updateCache(const std::wstring& itemNameOrKey, ItemCategory category);

    std::string categoryToString(ItemCategory category);
    ItemCategory stringToCategory(const std::string& categoryStr);

    std::wstring getParentFolderPath(const std::wstring& itemPath);
    std::wstring getFolderName(const std::wstring& folderPath);
#endif

    std::string urlEncode(const std::wstring& data);
};
