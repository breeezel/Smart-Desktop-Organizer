#include "WebClassifier.h"
#include "DesktopManager/ItemTypes.h"    // Changed from DesktopSorter.h
// DesktopInfo.h is included via WebClassifier.h
// ConfigManager.h and Logging.h are included via WebClassifier.h

// Ensure these are accessible if WebClassifier.h doesn't bring them transitively for .cpp
// #include "ConfigManager/ConfigManager.h" // No longer needed directly if WebClassifier.h includes it
// #include "Logging/Logging.h"             // No longer needed directly if WebClassifier.h includes it


// Static helper toLower_internal and wstringToUtf8 remain the same

static std::string toLower_internal(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

#ifdef _WIN32
static std::string wstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.length(), NULL, 0, NULL, NULL);
    if (size_needed == 0) {
        LOG_ERROR(L"WebClassifier::wstringToUtf8: WideCharToMultiByte failed to get size. Error: " + std::to_wstring(GetLastError()));
        return std::string();
    }
    std::string strTo(size_needed, 0);
    int chars_converted = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
    if (chars_converted == 0) {
        LOG_ERROR(L"WebClassifier::wstringToUtf8: WideCharToMultiByte failed to convert. Error: " + std::to_wstring(GetLastError()));
        return std::string();
    }
    return strTo;
}
#endif


WebClassifier::WebClassifier() {
#ifdef _WIN32
    cacheFilePath = getCacheFilePath();
    if (!cacheFilePath.empty()) {
        loadCache();
    } else {
        LOG_ERROR(L"WebClassifier: Failed to determine cache file path. Cache will not be used.");
        classificationCache = json::object();
    }
#else
    // LOG_DEBUG(L"WebClassifier: Non-Windows stub initialized."); // Example
#endif
}

WebClassifier::~WebClassifier() {
    // Destructor logic (if any specific needed beyond member cleanup)
}

#ifdef _WIN32
std::wstring WebClassifier::getCacheFilePath() {
    // (Implementation remains the same, add logging for errors)
    wchar_t path_buf[MAX_PATH];
    if (GetModuleFileNameW(NULL, path_buf, MAX_PATH) == 0) {
        LOG_ERROR(L"WebClassifier::getCacheFilePath: GetModuleFileNameW failed. Error: " + std::to_wstring(GetLastError()));
        try {
            return std::filesystem::current_path() / L"classification_cache.json"; // Fallback with logging
        } catch (const std::filesystem::filesystem_error& e) {
            LOG_ERROR(L"WebClassifier::getCacheFilePath: Filesystem error getting current_path for fallback: " + std::wstring(e.what(), e.what() + strlen(e.what())));
            return L"classification_cache.json"; // Absolute last resort
        }
    }
    try {
        std::filesystem::path executablePath = path_buf;
        return executablePath.parent_path() / L"classification_cache.json";
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR(L"WebClassifier::getCacheFilePath: Filesystem error constructing path: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        return L"classification_cache.json"; // Fallback
    }
}

void WebClassifier::loadCache() {
    if (cacheFilePath.empty()) {
        LOG_WARNING(L"WebClassifier::loadCache: Cache file path is empty, cannot load.");
        classificationCache = json::object();
        return;
    }
    std::ifstream ifs(cacheFilePath);
    if (!ifs.is_open()) {
        LOG_INFO(L"WebClassifier::loadCache: Cache file not found: " + cacheFilePath.wstring() + L". A new one will be created if updates occur.");
        classificationCache = json::object();
        return;
    }
    try {
        ifs >> classificationCache;
        if (!classificationCache.is_object()) {
            LOG_WARNING(L"WebClassifier::loadCache: Cache data in '" + cacheFilePath.wstring() + L"' is not a valid JSON object. Initializing empty cache.");
            classificationCache = json::object();
        } else {
            LOG_INFO(L"WebClassifier::loadCache: Cache loaded successfully from " + cacheFilePath.wstring());
        }
    } catch (json::parse_error& e) {
        LOG_ERROR(L"WebClassifier::loadCache: Error parsing cache file '" + cacheFilePath.wstring()
                   + L"'. Error: " + std::wstring(e.what(), e.what() + strlen(e.what())) + L". Starting with an empty cache.");
        classificationCache = json::object();
    } catch (const std::exception& e) {
        LOG_ERROR(L"WebClassifier::loadCache: Generic error reading cache file '" + cacheFilePath.wstring()
                   + L"'. Error: " + std::wstring(e.what(), e.what() + strlen(e.what())) + L". Starting with an empty cache.");
        classificationCache = json::object();
    }
     if (ifs.bad()) {
        LOG_ERROR(L"WebClassifier::loadCache: Stream error after reading cache file '" + cacheFilePath.wstring() + L"'.");
        classificationCache = json::object();
    }
    ifs.close();
}

void WebClassifier::saveCache() {
    if (cacheFilePath.empty()) {
        LOG_ERROR(L"WebClassifier::saveCache: Cache file path is empty, cannot save.");
        return;
    }
    std::ofstream ofs(cacheFilePath);
    if (!ofs.is_open()) {
        LOG_ERROR(L"WebClassifier::saveCache: Failed to open cache file for saving: " + cacheFilePath.wstring());
        return;
    }
    try {
        if (classificationCache.is_null()) classificationCache = json::object();
        ofs << classificationCache.dump(4);
        if (ofs.fail()) {
            LOG_ERROR(L"WebClassifier::saveCache: Failed to write to cache file: " + cacheFilePath.wstring());
        } else {
            // LOG_DEBUG(L"WebClassifier::saveCache: Cache saved to " + cacheFilePath.wstring());
        }
    } catch (const std::exception& e) {
        LOG_ERROR(L"WebClassifier::saveCache: Error dumping JSON to string for saving: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    ofs.close();
}

void WebClassifier::clearCache() {
    LOG_INFO(L"WebClassifier::clearCache - Cache clearing requested.");
    classificationCache = json::object();
    saveCache();
    if (!cacheFilePath.empty()) {
        std::error_code ec;
        std::filesystem::remove(cacheFilePath, ec);
        if (ec) {
            LOG_ERROR(L"WebClassifier::clearCache: Could not delete cache file: " + cacheFilePath.wstring()
                       + L". Error: " + std::wstring(ec.message().begin(), ec.message().end()));
        } else {
            LOG_INFO(L"WebClassifier::clearCache: Cache file deleted: " + cacheFilePath.wstring());
        }
    }
    classificationCache = json::object();
}

ItemCategory WebClassifier::getCachedCategory(const std::wstring& itemNameOrKey) {
    // (Implementation remains similar, add logging for type error)
    std::string key = wstringToUtf8(itemNameOrKey);
    if (key.empty() && !itemNameOrKey.empty()) {
        LOG_WARNING(L"WebClassifier::getCachedCategory: Failed to convert key '" + itemNameOrKey + L"' to UTF-8 for cache lookup.");
        return ItemCategory::UNCLASSIFIED;
    }
    if (classificationCache.is_null()) classificationCache = json::object();
    if (classificationCache.contains(key)) {
        try {
            std::string categoryStr = classificationCache[key].get<std::string>();
            return stringToCategory(categoryStr);
        } catch (json::type_error& e) {
             LOG_ERROR(L"WebClassifier::getCachedCategory: JSON type error for key '" + itemNameOrKey
                        + L"'. Expected string. Error: " + std::wstring(e.what(), e.what() + strlen(e.what())));
            return ItemCategory::UNCLASSIFIED;
        }
    }
    return ItemCategory::UNCLASSIFIED;
}

void WebClassifier::updateCache(const std::wstring& itemNameOrKey, ItemCategory category) {
    // (Implementation remains similar, add logging for key conversion error)
    if (classificationCache.is_null()) classificationCache = json::object();
    std::string key = wstringToUtf8(itemNameOrKey);
    if (key.empty() && !itemNameOrKey.empty()) {
        LOG_ERROR(L"WebClassifier::updateCache: Failed to convert item name '" + itemNameOrKey + L"' to UTF-8 for cache key. Not updating cache.");
        return;
    }
    std::string categoryStr = categoryToString(category);
    classificationCache[key] = categoryStr;
    // LOG_DEBUG(L"WebClassifier::updateCache: Updated cache for key '" + itemNameOrKey + L"' with category " + std::wstring(categoryStr.begin(), categoryStr.end()));
    saveCache();
}

std::string WebClassifier::categoryToString(ItemCategory category) { /* remains same */
    switch (category) {
        case ItemCategory::FOLDER: return "FOLDER";
        case ItemCategory::GAME: return "GAME";
        case ItemCategory::PROGRAM: return "PROGRAM";
        case ItemCategory::TEXT_FILE: return "TEXT_FILE";
        case ItemCategory::IMAGE_FILE: return "IMAGE_FILE";
        case ItemCategory::VIDEO_FILE: return "VIDEO_FILE";
        case ItemCategory::ARCHIVE_FILE: return "ARCHIVE_FILE";
        case ItemCategory::SPECIAL_SYSTEM: return "SPECIAL_SYSTEM";
        case ItemCategory::UNCLASSIFIED: return "UNCLASSIFIED";
        default: return "UNKNOWN";
    }
}
ItemCategory WebClassifier::stringToCategory(const std::string& categoryStr) { /* remains same */
    if (categoryStr == "FOLDER") return ItemCategory::FOLDER;
    if (categoryStr == "GAME") return ItemCategory::GAME;
    if (categoryStr == "PROGRAM") return ItemCategory::PROGRAM;
    if (categoryStr == "TEXT_FILE") return ItemCategory::TEXT_FILE;
    if (categoryStr == "IMAGE_FILE") return ItemCategory::IMAGE_FILE;
    if (categoryStr == "VIDEO_FILE") return ItemCategory::VIDEO_FILE;
    if (categoryStr == "ARCHIVE_FILE") return ItemCategory::ARCHIVE_FILE;
    if (categoryStr == "SPECIAL_SYSTEM") return ItemCategory::SPECIAL_SYSTEM;
    return ItemCategory::UNCLASSIFIED;
}
std::wstring WebClassifier::getParentFolderPath(const std::wstring& itemPath) { /* remains same, add logging */
    if (itemPath.empty()) return L"";
    try {
        std::filesystem::path p(itemPath);
        if (p.has_parent_path()) {
            return p.parent_path().wstring();
        }
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR(L"WebClassifier::getParentFolderPath: Filesystem error for path '" + itemPath
                   + L"'. Error: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    } catch (const std::exception& e) {
        LOG_ERROR(L"WebClassifier::getParentFolderPath: Generic error for path '" + itemPath
                   + L"'. Error: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    return L"";
}
std::wstring WebClassifier::getFolderName(const std::wstring& folderPath) { /* remains same, add logging */
    if (folderPath.empty()) return L"";
     try {
        std::filesystem::path p(folderPath);
        if (p.has_filename()) {
            return p.filename().wstring();
        }
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR(L"WebClassifier::getFolderName: Filesystem error for path '" + folderPath
                   + L"'. Error: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    } catch (const std::exception& e) {
        LOG_ERROR(L"WebClassifier::getFolderName: Generic error for path '" + folderPath
                   + L"'. Error: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    return L"";
}
#endif

std::string WebClassifier::urlEncode(const std::wstring& wide_data) { /* remains same, add logging for error */
#ifdef _WIN32
    std::string data_utf8 = wstringToUtf8(wide_data); // wstringToUtf8 now has logging
    if (data_utf8.empty() && !wide_data.empty()) {
        return "";
    }
    if (data_utf8.empty()) return "";

    std::ostringstream encoded_stream;
    encoded_stream << std::hex << std::setfill('0');
    for (char c : data_utf8) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded_stream << c;
        } else {
            encoded_stream << '%' << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(c));
        }
    }
    return encoded_stream.str();
#else
    // LOG_DEBUG(L"WebClassifier::urlEncode: Using non-Windows stub.");
    std::string s;
    s.reserve(wide_data.length());
    for(wchar_t wc : wide_data) {
        if (wc < 128 && (isalnum(static_cast<char>(wc)) || wc == '-' || wc == '_' || wc == '.' || wc == '~') ) {
            s += static_cast<char>(wc);
        } else if (wc < 128) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(wc));
            s += buf;
        } else {
            s += "%3F";
        }
    }
    return s;
#endif
}

bool WebClassifier::executeHttpGet(const std::string& url, std::string& response) { /* remains same, add logging */
#ifdef _WIN32
    response.clear();
    HINTERNET hInternetSession = NULL;
    HINTERNET hHttpOpen = NULL;
    const DWORD BUFFER_SIZE = 8192;
    std::vector<char> buffer_vec(BUFFER_SIZE);
    DWORD bytesRead = 0;

    hInternetSession = InternetOpenA("SmartDesktopOrganizer/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternetSession) {
        LOG_ERROR(L"WebClassifier::executeHttpGet: InternetOpenA failed. Error: " + std::to_wstring(GetLastError()));
        return false;
    }
    hHttpOpen = InternetOpenUrlA(hInternetSession, url.c_str(), NULL, 0,
                               INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE | INTERNET_FLAG_KEEP_CONNECTION, 0);
    if (!hHttpOpen) {
        LOG_ERROR(L"WebClassifier::executeHttpGet: InternetOpenUrlA failed for URL '" + std::wstring(url.begin(), url.end())
                   + L"'. Error: " + std::to_wstring(GetLastError()));
        InternetCloseHandle(hInternetSession);
        return false;
    }
    while (InternetReadFile(hHttpOpen, buffer_vec.data(), (DWORD)buffer_vec.size() - 1, &bytesRead) && bytesRead > 0) {
        buffer_vec[bytesRead] = '\0';
        response.append(buffer_vec.data());
    }
    // Check for errors after loop, if InternetReadFile returned FALSE but bytesRead was 0.
    DWORD lastError = GetLastError();
    if (bytesRead == 0 && response.empty() && lastError != ERROR_SUCCESS && lastError != ERROR_INTERNET_NAME_NOT_RESOLVED /* etc. */ ) {
        // Potentially log if it seems like an error rather than just empty response
        // LOG_WARNING(L"WebClassifier::executeHttpGet: InternetReadFile completed but response is empty. LastError: " + std::to_wstring(lastError));
    }

    InternetCloseHandle(hHttpOpen);
    InternetCloseHandle(hInternetSession);
    return true;
#else
    // LOG_DEBUG(L"WebClassifier::executeHttpGet: Using non-Windows stub for URL: " + std::wstring(url.begin(), url.end()));
    response = "<html><body>Mock HTTP GET response for " + url + ". Keywords: game, software, application. Stub.</body></html>";
    return true;
#endif
}

ItemCategory WebClassifier::classifyItemFromHtml(const std::string& htmlResponse, const std::wstring& itemName) { /* remains same */
#ifdef _WIN32
    std::string lowerHtml = toLower_internal(htmlResponse);
    std::wstring lowerItemNameW = itemName;
    std::transform(lowerItemNameW.begin(), lowerItemNameW.end(), lowerItemNameW.begin(), ::towlower);
    std::vector<std::string> gameKeywords = {"video game", "computer game", "pc game", "official game site", "download game", "buy game", "play game", "игра", "компьютерная игра", "скачать игру", "купить игру", "играть в игру", "официальный сайт игры", "steam store", "epic games store", "gog.com", "origin store", "ubisoft connect", "game review", "gameplay", "walkthrough", "прохождение игры", "обзор игры"};
    std::vector<std::string> programKeywords = {"software", "application", "program", "utility", "tool", "driver", "sdk", "ide", "программа", "приложение", "утилита", "инструмент", "драйвер", "download software", "official website", "скачать программу", "официальный сайт"};
    int gameScore = 0;
    int programScore = 0;
    for (const auto& keyword : gameKeywords) if (lowerHtml.find(keyword) != std::string::npos) gameScore++;
    for (const auto& keyword : programKeywords) if (lowerHtml.find(keyword) != std::string::npos) programScore++;
    std::string lowerItemNameS_utf8 = wstringToUtf8(lowerItemNameW);
    if (toLower_internal(lowerItemNameS_utf8).find("game") != std::string::npos || toLower_internal(lowerItemNameS_utf8).find("игра") != std::string::npos) gameScore += 2;
    if (toLower_internal(lowerItemNameS_utf8).find("software") != std::string::npos || toLower_internal(lowerItemNameS_utf8).find("app") != std::string::npos || toLower_internal(lowerItemNameS_utf8).find("program") != std::string::npos) programScore += 2;
    if (gameScore > 0 && programScore == 0) return ItemCategory::GAME;
    if (programScore > 0 && gameScore == 0) return ItemCategory::PROGRAM;
    if (gameScore > programScore) {
        if (lowerItemNameW.find(L"sdk") != std::wstring::npos || lowerItemNameW.find(L"engine") != std::wstring::npos || lowerItemNameW.find(L"editor") != std::wstring::npos || lowerItemNameW.find(L"development kit") != std::wstring::npos || lowerItemNameW.find(L"dev kit") != std::wstring::npos) {
            if (lowerHtml.find("game development") != std::string::npos || lowerHtml.find("разработка игр") != std::string::npos || lowerHtml.find("game engine") != std::string::npos) {
                return ItemCategory::PROGRAM;
            }
        }
        return ItemCategory::GAME;
    }
    if (programScore > gameScore) return ItemCategory::PROGRAM;
    if (gameScore > 0 && gameScore == programScore) {
        if (lowerItemNameW.find(L"studio") != std::wstring::npos || lowerItemNameW.find(L"maker") != std::wstring::npos || lowerItemNameW.find(L"develop") != std::wstring::npos || lowerItemNameW.find(L"sdk") != std::wstring::npos) {
            return ItemCategory::PROGRAM;
        }
        return ItemCategory::UNCLASSIFIED;
    }
    return ItemCategory::UNCLASSIFIED;
#else
    // LOG_DEBUG(L"WebClassifier::classifyItemFromHtml: Using non-Windows stub for item: " + itemName);
    (void)htmlResponse;
    std::wstring lowerItemNameW = itemName;
    std::transform(lowerItemNameW.begin(), lowerItemNameW.end(), lowerItemNameW.begin(), ::towlower);
    if (lowerItemNameW.find(L"game") != std::wstring::npos) return ItemCategory::GAME;
    if (lowerItemNameW.find(L"editor") != std::wstring::npos || lowerItemNameW.find(L"browser") != std::wstring::npos) return ItemCategory::PROGRAM;
    std::string lowerMockResponse = toLower_internal(htmlResponse);
    if(lowerMockResponse.find("game")!= std::string::npos) return ItemCategory::GAME;
    if(lowerMockResponse.find("software")!= std::string::npos) return ItemCategory::PROGRAM;
    if(lowerMockResponse.find("application")!= std::string::npos) return ItemCategory::PROGRAM;
    return ItemCategory::UNCLASSIFIED;
#endif
}

ItemCategory WebClassifier::performSearch(const DesktopItem& item, std::string& outHtmlResponse) {
    outHtmlResponse.clear();
#ifdef _WIN32
    // Check ConfigManager if web classifier is enabled
    if (!ConfigManager::getInstance().isWebClassifierEnabled()) {
        LOG_INFO(L"WebClassifier::performSearch: Web Classifier is disabled in settings. Skipping for item: " + item.name);
        return ItemCategory::UNCLASSIFIED;
    }

    ItemCategory category = ItemCategory::UNCLASSIFIED;
    std::string keyItemName = wstringToUtf8(item.name);

    if (keyItemName.empty() && !item.name.empty()) {
        LOG_ERROR(L"WebClassifier::performSearch: Failed to convert item name '" + item.name + L"' to UTF-8 for cache key.");
        return ItemCategory::UNCLASSIFIED;
    }

    if (classificationCache.contains(keyItemName)) {
        category = getCachedCategory(item.name);
        LOG_INFO(L"  [Cache HIT for item: '" + item.name + L"'] -> " + std::wstring(categoryToString(category).begin(), categoryToString(category).end()));
    } else {
        LOG_INFO(L"  [Web SEARCH for item: '" + item.name + L"']");
        std::wstring searchQueryItem = item.name + L" software application game file type category";
        std::string encodedQueryItem = urlEncode(searchQueryItem);

        if (!encodedQueryItem.empty() || searchQueryItem.empty()) {
            std::string urlItem = "https://www.google.com/search?q=" + encodedQueryItem + "&hl=en";
            static auto lastRequestTime = std::chrono::steady_clock::now() - std::chrono::seconds(5);
            auto now = std::chrono::steady_clock::now();
            auto timeSinceLastRequest = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRequestTime);
            const long long minIntervalMs = 2000;
            if (timeSinceLastRequest.count() < minIntervalMs) {
                LOG_DEBUG(L"WebClassifier::performSearch: Throttling item search. Sleeping for " + std::to_wstring(minIntervalMs - timeSinceLastRequest.count()) + L"ms.");
                std::this_thread::sleep_for(std::chrono::milliseconds(minIntervalMs - timeSinceLastRequest.count()));
            }
            lastRequestTime = std::chrono::steady_clock::now();

            std::string itemHtmlResponse;
            bool success = executeHttpGet(urlItem, itemHtmlResponse);
            if (success && !itemHtmlResponse.empty()) {
                category = classifyItemFromHtml(itemHtmlResponse, item.name);
                LOG_INFO(L"WebClassifier::performSearch: Item '" + item.name + L"' classified as " + std::wstring(categoryToString(category).begin(), categoryToString(category).end()) + L" from web.");
                if (!outHtmlResponse.empty()) outHtmlResponse += "\n==== Item Search Response ("+wstringToUtf8(item.name)+") ====\n";
                outHtmlResponse += itemHtmlResponse;
            } else {
                LOG_ERROR(L"WebClassifier::performSearch: executeHttpGet failed or got empty response for item '" + item.name + L"'. URL: " + std::wstring(urlItem.begin(), urlItem.end()));
                category = ItemCategory::UNCLASSIFIED;
            }
            updateCache(item.name, category);
        } else {
             LOG_ERROR(L"WebClassifier::performSearch: URL encoding failed for item query '" + searchQueryItem + L"'.");
             category = ItemCategory::UNCLASSIFIED;
             updateCache(item.name, category);
        }
    }

    if (category == ItemCategory::UNCLASSIFIED &&
        (item.type == DesktopItem::ItemType::FILE || item.type == DesktopItem::ItemType::SHORTCUT)) {

        std::wstring parentPath = getParentFolderPath(item.path);
        if (!parentPath.empty()) {
            std::wstring folderName = getFolderName(parentPath);
            if (!folderName.empty() && folderName != L"." && folderName != L"..") {
                ItemCategory folderCategory = ItemCategory::UNCLASSIFIED;
                std::string keyFolderName = wstringToUtf8(folderName);

                if (keyFolderName.empty() && !folderName.empty()){
                     LOG_ERROR(L"WebClassifier::performSearch: Failed to convert folder name '" + folderName + L"' to UTF-8 for cache key.");
                } else if (classificationCache.contains(keyFolderName)) {
                    folderCategory = getCachedCategory(folderName);
                     LOG_INFO(L"  Item '" + item.name + L"' unclassified. [Cache HIT for parent folder: '" + folderName + L"'] -> " + std::wstring(categoryToString(folderCategory).begin(), categoryToString(folderCategory).end()));
                } else {
                    LOG_INFO(L"  Item '" + item.name + L"' unclassified. [Web SEARCH for parent folder: '" + folderName + L"']");
                    std::wstring searchQueryFolder = folderName + L" game software application program category";
                    std::string encodedQueryFolder = urlEncode(searchQueryFolder);

                    if (!encodedQueryFolder.empty() || searchQueryFolder.empty()) {
                        std::string urlFolder = "https://www.google.com/search?q=" + encodedQueryFolder + "&hl=en";
                        static auto lastRequestTimeFolder = std::chrono::steady_clock::now() - std::chrono::seconds(5);
                        auto nowFolder = std::chrono::steady_clock::now();
                        auto timeSinceLastRequestFolder = std::chrono::duration_cast<std::chrono::milliseconds>(nowFolder - lastRequestTimeFolder);
                        const long long minIntervalMs = 2000;
                        if (timeSinceLastRequestFolder.count() < minIntervalMs) {
                            LOG_DEBUG(L"WebClassifier::performSearch: Throttling folder search. Sleeping for " + std::to_wstring(minIntervalMs - timeSinceLastRequestFolder.count()) + L"ms.");
                             std::this_thread::sleep_for(std::chrono::milliseconds(minIntervalMs - timeSinceLastRequestFolder.count()));
                        }
                        lastRequestTimeFolder = std::chrono::steady_clock::now();

                        std::string folderHtmlResponse;
                        bool folderSuccess = executeHttpGet(urlFolder, folderHtmlResponse);
                        if (folderSuccess && !folderHtmlResponse.empty()) {
                            folderCategory = classifyItemFromHtml(folderHtmlResponse, folderName);
                            LOG_INFO(L"WebClassifier::performSearch: Folder '" + folderName + L"' classified as " + std::wstring(categoryToString(folderCategory).begin(), categoryToString(folderCategory).end()) + L" from web.");
                            if (!outHtmlResponse.empty()) outHtmlResponse += "\n==== Folder Search Response (" + wstringToUtf8(folderName) + ") ====\n";
                            outHtmlResponse += folderHtmlResponse;
                        } else {
                            LOG_ERROR(L"WebClassifier::performSearch: executeHttpGet failed or got empty response for folder '" + folderName + L"'. URL: " + std::wstring(urlFolder.begin(), urlFolder.end()));
                            folderCategory = ItemCategory::UNCLASSIFIED;
                        }
                        updateCache(folderName, folderCategory);
                    } else {
                        LOG_ERROR(L"WebClassifier::performSearch: URL encoding failed for folder query '" + searchQueryFolder + L"'.");
                        folderCategory = ItemCategory::UNCLASSIFIED;
                        updateCache(folderName, folderCategory);
                    }
                }

                if (folderCategory == ItemCategory::GAME || folderCategory == ItemCategory::PROGRAM) {
                    LOG_INFO(L"  Using category '" + std::wstring(categoryToString(folderCategory).begin(), categoryToString(folderCategory).end()) + L"' from parent folder '" + folderName + L"' for item '" + item.name + L"'.");
                    return folderCategory;
                }
            }
        }
    }
    return category;
#else
    // LOG_DEBUG(L"WebClassifier::performSearch: Using non-Windows stub for item: " + item.name);
    std::string tempItemName;
    for(wchar_t wc : item.name) tempItemName += static_cast<char>(wc);
    outHtmlResponse = "<html><body>Mock search results for " + tempItemName + ". Keywords: game, application, software. This is a stub for an item.</body></html>";
    ItemCategory itemCat = classifyItemFromHtml(outHtmlResponse, item.name);
    if (itemCat == ItemCategory::UNCLASSIFIED && (item.type == DesktopItem::ItemType::FILE || item.type == DesktopItem::ItemType::SHORTCUT)) {
        std::wstring parentPath = getParentFolderPath(item.path);
        if(!parentPath.empty()){
            std::wstring folderName = getFolderName(parentPath);
            if(!folderName.empty()){
                 std::string tempFolderName; for(wchar_t wc : folderName) tempFolderName += static_cast<char>(wc);
                 outHtmlResponse += "\n<html><body>Mock parent folder search for " + tempFolderName + ". Keywords: game.</body></html>";
                 return classifyItemFromHtml(outHtmlResponse, folderName);
            }
        }
    }
    return itemCat;
#endif
}

#ifndef _WIN32
// Stubs for path helpers for non-Windows to allow compilation
std::wstring WebClassifier::getParentFolderPath(const std::wstring& itemPath) {
    if (itemPath.empty()) return L"";
    try { std::filesystem::path p(itemPath); if (p.has_parent_path()) return p.parent_path().wstring(); } catch(const std::exception& e) {
        LOG_ERROR(L"WebClassifier::getParentFolderPath (stub): Filesystem error for path '" + itemPath + L"': " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    size_t last_slash_idx = itemPath.rfind(L'/');
    if (std::wstring::npos == last_slash_idx) last_slash_idx = itemPath.rfind(L'\\');
    if (std::wstring::npos != last_slash_idx) return itemPath.substr(0, last_slash_idx);
    return L"";
}
std::wstring WebClassifier::getFolderName(const std::wstring& folderPath) {
    if (folderPath.empty()) return L"";
    try { std::filesystem::path p(folderPath); if (p.has_filename()) return p.filename().wstring(); } catch(const std::exception& e) {
         LOG_ERROR(L"WebClassifier::getFolderName (stub): Filesystem error for path '" + folderPath + L"': " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    size_t last_slash_idx = folderPath.rfind(L'/');
    if (std::wstring::npos == last_slash_idx) last_slash_idx = folderPath.rfind(L'\\');
    if (std::wstring::npos != last_slash_idx) return folderPath.substr(last_slash_idx + 1);
    return folderPath;
}
#endif
