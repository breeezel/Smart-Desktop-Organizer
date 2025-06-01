#include "WebClassifier.h"
#include "DesktopManager/DesktopSorter.h"
#include "DesktopManager/DesktopInfo.h"

#ifdef _WIN32
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <ShlObj.h>
#include <libloaderapi.h>
#include <filesystem>
#else
#include <vector>
#include <algorithm>
#include <filesystem>
#endif

#include <iostream> // For std::wcerr, std::wcout
#include <thread>
#include <chrono>
#include <algorithm>
#include <string>

// --- Helper Functions ---

/**
 * @brief Converts a std::string to lowercase using ASCII/default locale rules.
 * @param s The string to convert.
 * @return The lowercase version of the string.
 */
static std::string toLower_internal(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

#ifdef _WIN32
/**
 * @brief Converts a std::wstring to a UTF-8 encoded std::string.
 * @param wstr The wide string to convert.
 * @return The UTF-8 encoded string, or an empty string on failure.
 */
static std::string wstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.length(), NULL, 0, NULL, NULL);
    if (size_needed == 0) { // Failure
        std::wcerr << L"WebClassifier::wstringToUtf8: WideCharToMultiByte failed to get size. Error: " << GetLastError() << std::endl;
        return std::string();
    }
    std::string strTo(size_needed, 0);
    int chars_converted = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
    if (chars_converted == 0) { // Failure
        std::wcerr << L"WebClassifier::wstringToUtf8: WideCharToMultiByte failed to convert. Error: " << GetLastError() << std::endl;
        return std::string();
    }
    return strTo;
}
#endif

// --- WebClassifier Class Implementation ---

WebClassifier::WebClassifier() {
#ifdef _WIN32
    cacheFilePath = getCacheFilePath();
    if (!cacheFilePath.empty()) {
        loadCache();
    } else {
        std::wcerr << L"WebClassifier: Failed to determine cache file path. Cache will not be used." << std::endl;
        // classificationCache remains in its default (null or empty object) state.
        // Ensure it's a valid JSON object if other methods assume so.
        classificationCache = json::object();
    }
#else
    // Non-Windows: No cache initialization needed for stub.
#endif
}

WebClassifier::~WebClassifier() {
#ifdef _WIN32
    // Cache is saved on each update, so not strictly necessary here unless there are
    // operations that modify classificationCache without calling saveCache immediately.
    // if (!cacheFilePath.empty()) {
    //     saveCache();
    // }
#else
    // Non-Windows: No cache cleanup needed for stub.
#endif
}

#ifdef _WIN32
/**
 * @brief Determines the full path for the classification cache file.
 * The cache file is typically located next to the executable.
 * @return std::wstring The full path to the cache file, or an empty string on failure.
 */
std::wstring WebClassifier::getCacheFilePath() {
    wchar_t path_buf[MAX_PATH];
    if (GetModuleFileNameW(NULL, path_buf, MAX_PATH) == 0) {
        std::wcerr << L"WebClassifier::getCacheFilePath: GetModuleFileNameW failed. Error: " << GetLastError() << std::endl;
        // Fallback to a relative path in current directory if absolute path fails.
        // This is less ideal as "current directory" can vary.
        return L"classification_cache.json";
    }
    try {
        std::filesystem::path executablePath = path_buf;
        return executablePath.parent_path() / L"classification_cache.json";
    } catch (const std::filesystem::filesystem_error& e) {
        std::wcerr << L"WebClassifier::getCacheFilePath: Filesystem error: " << e.what() << std::endl;
        return L"classification_cache.json"; // Fallback
    }
}

/**
 * @brief Loads the classification cache from the JSON file into memory.
 * If the file doesn't exist or is invalid, an empty cache is initialized.
 */
void WebClassifier::loadCache() {
    if (cacheFilePath.empty()) {
        std::wcerr << L"WebClassifier::loadCache: Cache file path is empty, cannot load." << std::endl;
        classificationCache = json::object(); // Ensure it's a valid object
        return;
    }
    std::ifstream ifs(cacheFilePath);
    if (!ifs.is_open()) {
        // This is not an error if the cache file simply doesn't exist yet.
        // std::wcout << L"WebClassifier::loadCache: Cache file not found: " << cacheFilePath << L". A new one will be created." << std::endl;
        classificationCache = json::object();
        return;
    }
    try {
        ifs >> classificationCache;
        if (!classificationCache.is_object()) {
            std::wcerr << L"WebClassifier::loadCache: Cache data is not a valid JSON object. Initializing empty cache." << std::endl;
            classificationCache = json::object();
        }
    } catch (json::parse_error& e) {
        std::wcerr << L"WebClassifier::loadCache: Error parsing cache file '" << cacheFilePath
                   << L"'. Error: " << e.what() << L". Starting with an empty cache." << std::endl;
        classificationCache = json::object();
    } catch (const std::exception& e) { // Catch other potential exceptions from stream operations
        std::wcerr << L"WebClassifier::loadCache: Generic error reading cache file '" << cacheFilePath
                   << L"'. Error: " << e.what() << L". Starting with an empty cache." << std::endl;
        classificationCache = json::object();
    }
    if (ifs.bad()) { // Check for stream errors not covered by exceptions
        std::wcerr << L"WebClassifier::loadCache: Stream error after reading cache file '" << cacheFilePath << L"'." << std::endl;
        // Potentially re-initialize cache if state is uncertain
        classificationCache = json::object();
    }
    ifs.close();
}

/**
 * @brief Saves the current in-memory classification cache to the JSON file.
 */
void WebClassifier::saveCache() {
    if (cacheFilePath.empty()) {
        std::wcerr << L"WebClassifier::saveCache: Cache file path is empty, cannot save." << std::endl;
        return;
    }
    std::ofstream ofs(cacheFilePath);
    if (!ofs.is_open()) {
        std::wcerr << L"WebClassifier::saveCache: Failed to open cache file for saving: " << cacheFilePath << std::endl;
        return;
    }
    try {
        // Ensure classificationCache is an object before dumping, especially if it could be null after a failed load.
        if (classificationCache.is_null()) classificationCache = json::object();
        ofs << classificationCache.dump(4); // Pretty print with 4 spaces
        if (ofs.fail()) { // Check for write errors
            std::wcerr << L"WebClassifier::saveCache: Failed to write to cache file: " << cacheFilePath << std::endl;
        }
    } catch (const std::exception& e) {
        std::wcerr << L"WebClassifier::saveCache: Error dumping JSON to string for saving: " << e.what() << std::endl;
    }
    ofs.close();
}

/**
 * @brief Clears the in-memory cache and deletes the cache file from disk.
 */
void WebClassifier::clearCache() {
    classificationCache = json::object(); // Clear the nlohmann::json object first
    saveCache(); // Save the now empty cache (effectively clearing the file content or creating an empty one)

    if (!cacheFilePath.empty()) {
        std::error_code ec;
        std::filesystem::remove(cacheFilePath, ec);
        if (ec) {
            std::wcerr << L"WebClassifier::clearCache: Could not delete cache file: " << cacheFilePath
                       << L". Error: " << ec.message() << std::endl;
        }
    }
    // Ensure cache is a valid empty object after clearing
    classificationCache = json::object();
}

/**
 * @brief Retrieves a category for an item name from the cache.
 * @param itemNameOrKey The name or key (UTF-8 converted) of the item.
 * @return ItemCategory The cached category, or UNCLASSIFIED if not found or on error.
 */
ItemCategory WebClassifier::getCachedCategory(const std::wstring& itemNameOrKey) {
    std::string key = wstringToUtf8(itemNameOrKey);
    if (key.empty() && !itemNameOrKey.empty()) { // Handle wstringToUtf8 failure
        return ItemCategory::UNCLASSIFIED;
    }
    if (classificationCache.is_null()) classificationCache = json::object();
    if (classificationCache.contains(key)) {
        try {
            std::string categoryStr = classificationCache[key].get<std::string>();
            return stringToCategory(categoryStr);
        } catch (json::type_error& e) {
             std::wcerr << L"WebClassifier::getCachedCategory: JSON type error for key '" << itemNameOrKey
                        << L"'. Expected string. Error: " << e.what() << std::endl;
            return ItemCategory::UNCLASSIFIED;
        }
    }
    return ItemCategory::UNCLASSIFIED;
}

/**
 * @brief Updates the cache with a classification for an item and saves the cache.
 * @param itemNameOrKey The name or key (UTF-8 converted) of the item.
 * @param category The category to cache for the item.
 */
void WebClassifier::updateCache(const std::wstring& itemNameOrKey, ItemCategory category) {
    if (classificationCache.is_null()) classificationCache = json::object();
    std::string key = wstringToUtf8(itemNameOrKey);
    if (key.empty() && !itemNameOrKey.empty()) { // Handle wstringToUtf8 failure
         std::wcerr << L"WebClassifier::updateCache: Failed to convert item name to UTF-8 for cache key. Item: " << itemNameOrKey << std::endl;
        return;
    }
    std::string categoryStr = categoryToString(category);
    classificationCache[key] = categoryStr;
    saveCache();
}

/**
 * @brief Converts an ItemCategory enum to its string representation for JSON storage.
 * @param category The ItemCategory enum value.
 * @return std::string The string representation of the category.
 */
std::string WebClassifier::categoryToString(ItemCategory category) {
    // (Implementation remains the same)
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

/**
 * @brief Converts a string from JSON back to an ItemCategory enum.
 * @param categoryStr The string representation of the category.
 * @return ItemCategory The corresponding enum value, or UNCLASSIFIED if the string is not recognized.
 */
ItemCategory WebClassifier::stringToCategory(const std::string& categoryStr) {
    // (Implementation remains the same)
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

/**
 * @brief Extracts the parent folder path from a given item path.
 * @param itemPath The full path to the item.
 * @return std::wstring The path of the parent folder, or an empty string on error/if no parent.
 */
std::wstring WebClassifier::getParentFolderPath(const std::wstring& itemPath) {
    if (itemPath.empty()) return L"";
    try {
        std::filesystem::path p(itemPath);
        if (p.has_parent_path()) {
            return p.parent_path().wstring();
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::wcerr << L"WebClassifier::getParentFolderPath: Filesystem error for path '" << itemPath
                   << L"'. Error: " << e.what() << std::endl;
    } catch (const std::exception& e) { // Catch other potential errors
        std::wcerr << L"WebClassifier::getParentFolderPath: Generic error for path '" << itemPath
                   << L"'. Error: " << e.what() << std::endl;
    }
    return L"";
}

/**
 * @brief Extracts the folder name from a given folder path.
 * @param folderPath The full path to the folder.
 * @return std::wstring The name of the folder, or an empty string on error.
 */
std::wstring WebClassifier::getFolderName(const std::wstring& folderPath) {
    if (folderPath.empty()) return L"";
     try {
        std::filesystem::path p(folderPath);
        if (p.has_filename()) {
            return p.filename().wstring();
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::wcerr << L"WebClassifier::getFolderName: Filesystem error for path '" << folderPath
                   << L"'. Error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::wcerr << L"WebClassifier::getFolderName: Generic error for path '" << folderPath
                   << L"'. Error: " << e.what() << std::endl;
    }
    return L"";
}

#endif // _WIN32 for cache and path logic

/**
 * @brief URL-encodes a wide string. Converts to UTF-8 then percent-encodes.
 * @param wide_data The wide string to encode.
 * @return std::string The URL-encoded string. Returns empty on UTF-8 conversion failure.
 */
std::string WebClassifier::urlEncode(const std::wstring& wide_data) {
#ifdef _WIN32
    std::string data_utf8 = wstringToUtf8(wide_data);
    if (data_utf8.empty() && !wide_data.empty()) { // Check if conversion failed
        // Error already printed by wstringToUtf8
        return "";
    }
    if (data_utf8.empty()) return ""; // If input was empty, data_utf8 is empty.

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
    // Non-Windows stub for urlEncode
    // std::wcerr << L"Warning: urlEncode is a basic stub on non-Windows." << std::endl;
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
            s += "%3F"; // Placeholder for non-ASCII wide chars
        }
    }
    return s;
#endif
}

/**
 * @brief Executes an HTTP GET request using WinINet.
 * @param url The URL to fetch.
 * @param response Output string to store the HTTP response body.
 * @return true if the request was made and response reading attempted, false on critical errors (e.g., cannot open session/URL).
 */
bool WebClassifier::executeHttpGet(const std::string& url, std::string& response) {
#ifdef _WIN32
    response.clear();
    HINTERNET hInternetSession = NULL;
    HINTERNET hHttpOpen = NULL;
    const DWORD BUFFER_SIZE = 8192;
    std::vector<char> buffer_vec(BUFFER_SIZE);
    DWORD bytesRead = 0;

    // Step 1: Initialize WinINet session
    hInternetSession = InternetOpenA("SmartDesktopOrganizer/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternetSession) {
        std::wcerr << L"WebClassifier::executeHttpGet: InternetOpenA failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Step 2: Open the URL
    hHttpOpen = InternetOpenUrlA(hInternetSession, url.c_str(), NULL, 0,
                               INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE | INTERNET_FLAG_KEEP_CONNECTION, 0);
    if (!hHttpOpen) {
        std::wcerr << L"WebClassifier::executeHttpGet: InternetOpenUrlA failed for URL '" << url.c_str()
                   << L"'. Error: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternetSession);
        return false;
    }

    // Step 3: Read the response data
    while (InternetReadFile(hHttpOpen, buffer_vec.data(), (DWORD)buffer_vec.size() - 1, &bytesRead) && bytesRead > 0) {
        buffer_vec[bytesRead] = '\0';
        response.append(buffer_vec.data());
    }
    // After loop, bytesRead == 0. Check for errors if response is empty or last read was partial.
    // GetLastError() can provide more info if InternetReadFile returned FALSE.

    // Step 4: Close handles
    InternetCloseHandle(hHttpOpen);
    InternetCloseHandle(hInternetSession);
    return true;
#else
    // Non-Windows stub for executeHttpGet
    // std::wcerr << L"Warning: executeHttpGet is a stub on non-Windows." << std::endl;
    response = "<html><body>Mock HTTP GET response for " + url + ". Keywords: game, software, application. Stub.</body></html>";
    return true;
#endif
}

/**
 * @brief Classifies an item based on keywords found in HTML content.
 * This is a basic implementation using keyword scoring.
 * @param htmlResponse The HTML content (UTF-8 string) from a web search.
 * @param itemName The original name of the item being classified (for context).
 * @return ItemCategory The deduced category.
 */
ItemCategory WebClassifier::classifyItemFromHtml(const std::string& htmlResponse, const std::wstring& itemName) {
#ifdef _WIN32
    // Logic: Convert HTML to lowercase. Define keyword lists for GAME and PROGRAM.
    // Score based on keyword presence. Check item name for hints.
    // More sophisticated logic could involve NLP, regex, or DOM parsing.
    std::string lowerHtml = toLower_internal(htmlResponse);
    std::wstring lowerItemNameW = itemName;
    std::transform(lowerItemNameW.begin(), lowerItemNameW.end(), lowerItemNameW.begin(), ::towlower);

    // Keywords are illustrative and can be expanded.
    std::vector<std::string> gameKeywords = {"video game", "computer game", "pc game", "official game site", "download game", "buy game", "play game", "игра", "компьютерная игра", "скачать игру", "купить игру", "играть в игру", "официальный сайт игры", "steam store", "epic games store", "gog.com", "origin store", "ubisoft connect", "game review", "gameplay", "walkthrough", "прохождение игры", "обзор игры"};
    std::vector<std::string> programKeywords = {"software", "application", "program", "utility", "tool", "driver", "sdk", "ide", "программа", "приложение", "утилита", "инструмент", "драйвер", "download software", "official website", "скачать программу", "официальный сайт"};

    int gameScore = 0;
    int programScore = 0;
    for (const auto& keyword : gameKeywords) if (lowerHtml.find(keyword) != std::string::npos) gameScore++;
    for (const auto& keyword : programKeywords) if (lowerHtml.find(keyword) != std::string::npos) programScore++;

    std::string lowerItemNameS_utf8 = wstringToUtf8(lowerItemNameW);
    if (toLower_internal(lowerItemNameS_utf8).find("game") != std::string::npos || toLower_internal(lowerItemNameS_utf8).find("игра") != std::string::npos) gameScore += 2; // Boost score from item name
    if (toLower_internal(lowerItemNameS_utf8).find("software") != std::string::npos || toLower_internal(lowerItemNameS_utf8).find("app") != std::string::npos || toLower_internal(lowerItemNameS_utf8).find("program") != std::string::npos) programScore += 2;

    // Decision logic based on scores
    if (gameScore > 0 && programScore == 0) return ItemCategory::GAME;
    if (programScore > 0 && gameScore == 0) return ItemCategory::PROGRAM;
    if (gameScore > programScore) { // Game score is higher
        // Refine if item name suggests development tool
        if (lowerItemNameW.find(L"sdk") != std::wstring::npos || lowerItemNameW.find(L"engine") != std::wstring::npos || lowerItemNameW.find(L"editor") != std::wstring::npos || lowerItemNameW.find(L"development kit") != std::wstring::npos || lowerItemNameW.find(L"dev kit") != std::wstring::npos) {
            if (lowerHtml.find("game development") != std::string::npos || lowerHtml.find("разработка игр") != std::string::npos || lowerHtml.find("game engine") != std::string::npos) {
                return ItemCategory::PROGRAM; // It's a game development tool
            }
        }
        return ItemCategory::GAME;
    }
    if (programScore > gameScore) return ItemCategory::PROGRAM; // Program score is higher
    if (gameScore > 0 && gameScore == programScore) { // Scores are tied and positive
        // Ambiguous case, could check more specific details or default.
        // If item name has strong program hints, prefer PROGRAM.
        if (lowerItemNameW.find(L"studio") != std::wstring::npos || lowerItemNameW.find(L"maker") != std::wstring::npos ||
            lowerItemNameW.find(L"develop") != std::wstring::npos || lowerItemNameW.find(L"sdk") != std::wstring::npos) {
            return ItemCategory::PROGRAM;
        }
        return ItemCategory::UNCLASSIFIED; // Default for ties if not clearly a program tool
    }
    return ItemCategory::UNCLASSIFIED; // Default if no keywords hit or scores are zero
#else
    // Non-Windows stub for classifyItemFromHtml
    // std::wcerr << L"Warning: classifyItemFromHtml is a stub on non-Windows." << std::endl;
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

/**
 * @brief Performs a web search for the given desktop item to determine its category.
 * Uses caching to avoid repeated searches for the same item or parent folder.
 * Implements parent folder search fallback if item classification is uncertain.
 * @param item The DesktopItem to classify.
 * @param outHtmlResponse Output string to store the HTML response (for debugging).
 * @return ItemCategory The deduced category.
 */
ItemCategory WebClassifier::performSearch(const DesktopItem& item, std::string& outHtmlResponse) {
    outHtmlResponse.clear();
#ifdef _WIN32
    // Cache stores item name (UTF-8 string) to category (string)
    // Step 1: Try classifying item name directly from cache or web
    ItemCategory category = ItemCategory::UNCLASSIFIED; // Default to UNCLASSIFIED
    std::string keyItemName = wstringToUtf8(item.name);

    if (keyItemName.empty() && !item.name.empty()) { // UTF-8 conversion failed for item name
        std::wcerr << L"WebClassifier::performSearch: Failed to convert item name '" << item.name << L"' to UTF-8 for cache key." << std::endl;
        // Cannot proceed without a valid key for cache or search.
        return ItemCategory::UNCLASSIFIED;
    }

    if (classificationCache.contains(keyItemName)) {
        category = getCachedCategory(item.name); // Use original wstring item.name
        std::wcout << L"  [Cache HIT for item: '" << item.name << L"'] -> " << categoryToString(category) << std::endl;
    } else {
        std::wcout << L"  [Web SEARCH for item: '" << item.name << L"']" << std::endl;
        std::wstring searchQueryItem = item.name + L" software application game file type category";
        std::string encodedQueryItem = urlEncode(searchQueryItem);

        if (!encodedQueryItem.empty() || searchQueryItem.empty()) {
            std::string urlItem = "https://www.google.com/search?q=" + encodedQueryItem + "&hl=en";
            // Throttling logic
            static auto lastRequestTime = std::chrono::steady_clock::now() - std::chrono::seconds(5);
            auto now = std::chrono::steady_clock::now();
            auto timeSinceLastRequest = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRequestTime);
            const long long minIntervalMs = 2000;
            if (timeSinceLastRequest.count() < minIntervalMs) {
                std::this_thread::sleep_for(std::chrono::milliseconds(minIntervalMs - timeSinceLastRequest.count()));
            }
            lastRequestTime = std::chrono::steady_clock::now();

            std::string itemHtmlResponse;
            bool success = executeHttpGet(urlItem, itemHtmlResponse);
            if (success && !itemHtmlResponse.empty()) {
                category = classifyItemFromHtml(itemHtmlResponse, item.name);
                if (!outHtmlResponse.empty()) outHtmlResponse += "\n==== Item Search Response ("+wstringToUtf8(item.name)+") ====\n";
                outHtmlResponse += itemHtmlResponse;
            } else {
                category = ItemCategory::UNCLASSIFIED;
            }
            updateCache(item.name, category);
        } else {
             category = ItemCategory::UNCLASSIFIED;
             updateCache(item.name, category); // Cache as UNCLASSIFIED if encoding fails for item name
        }
    }

    // Step 2: If item is still UNCLASSIFIED and is a file/shortcut, try parent folder
    if (category == ItemCategory::UNCLASSIFIED &&
        (item.type == DesktopItem::ItemType::FILE || item.type == DesktopItem::ItemType::SHORTCUT)) {

        std::wstring parentPath = getParentFolderPath(item.path);
        if (!parentPath.empty()) {
            std::wstring folderName = getFolderName(parentPath);
            if (!folderName.empty() && folderName != L"." && folderName != L"..") { // Ensure folder name is valid
                ItemCategory folderCategory = ItemCategory::UNCLASSIFIED;
                std::string keyFolderName = wstringToUtf8(folderName);

                if (keyFolderName.empty() && !folderName.empty()){ // UTF-8 conversion failed for folder name
                     std::wcerr << L"WebClassifier::performSearch: Failed to convert folder name '" << folderName << L"' to UTF-8 for cache key." << std::endl;
                } else if (classificationCache.contains(keyFolderName)) {
                    folderCategory = getCachedCategory(folderName);
                    std::wcout << L"  Item '" << item.name << L"' unclassified. [Cache HIT for parent folder: '" << folderName << L"'] -> " << categoryToString(folderCategory) << std::endl;
                } else {
                    std::wcout << L"  Item '" << item.name << L"' unclassified. [Web SEARCH for parent folder: '" << folderName << L"']" << std::endl;
                    std::wstring searchQueryFolder = folderName + L" game software application program category"; // Generic query for folder
                    std::string encodedQueryFolder = urlEncode(searchQueryFolder);

                    if (!encodedQueryFolder.empty() || searchQueryFolder.empty()) {
                        std::string urlFolder = "https://www.google.com/search?q=" + encodedQueryFolder + "&hl=en";
                        // Throttling logic (separate from item search throttling to be safe)
                        static auto lastRequestTimeFolder = std::chrono::steady_clock::now() - std::chrono::seconds(5);
                        auto nowFolder = std::chrono::steady_clock::now();
                        auto timeSinceLastRequestFolder = std::chrono::duration_cast<std::chrono::milliseconds>(nowFolder - lastRequestTimeFolder);
                        const long long minIntervalMs = 2000;
                        if (timeSinceLastRequestFolder.count() < minIntervalMs) {
                             std::this_thread::sleep_for(std::chrono::milliseconds(minIntervalMs - timeSinceLastRequestFolder.count()));
                        }
                        lastRequestTimeFolder = std::chrono::steady_clock::now();

                        std::string folderHtmlResponse;
                        bool folderSuccess = executeHttpGet(urlFolder, folderHtmlResponse);
                        if (folderSuccess && !folderHtmlResponse.empty()) {
                            folderCategory = classifyItemFromHtml(folderHtmlResponse, folderName); // Classify based on folder's content
                            if (!outHtmlResponse.empty()) outHtmlResponse += "\n==== Folder Search Response (" + wstringToUtf8(folderName) + ") ====\n";
                            outHtmlResponse += folderHtmlResponse;
                        } else {
                            folderCategory = ItemCategory::UNCLASSIFIED;
                        }
                        updateCache(folderName, folderCategory); // Cache folder's classification
                    } else {
                        folderCategory = ItemCategory::UNCLASSIFIED;
                        updateCache(folderName, folderCategory); // Cache as UNCLASSIFIED if encoding fails for folder name
                    }
                }

                // If folder search yielded GAME or PROGRAM, use it for the item.
                if (folderCategory == ItemCategory::GAME || folderCategory == ItemCategory::PROGRAM) {
                    // std::wcout << L"  Using category '" << categoryToString(folderCategory) << L"' from parent folder '" << folderName << L"' for item '" << item.name << L"'." << std::endl;
                    return folderCategory; // Return folder's category for the item
                }
            }
        }
    }
    return category; // Return original item's category (could be UNCLASSIFIED)
#else
    // Non-Windows stub for performSearch
    // std::wcerr << L"Warning: performSearch is a stub on non-Windows." << std::endl;
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
    try { std::filesystem::path p(itemPath); if (p.has_parent_path()) return p.parent_path().wstring(); } catch(...) {}
    size_t last_slash_idx = itemPath.rfind(L'/');
    if (std::wstring::npos == last_slash_idx) last_slash_idx = itemPath.rfind(L'\\');
    if (std::wstring::npos != last_slash_idx) return itemPath.substr(0, last_slash_idx);
    return L"";
}
std::wstring WebClassifier::getFolderName(const std::wstring& folderPath) {
    if (folderPath.empty()) return L"";
    try { std::filesystem::path p(folderPath); if (p.has_filename()) return p.filename().wstring(); } catch(...) {}
    size_t last_slash_idx = folderPath.rfind(L'/');
    if (std::wstring::npos == last_slash_idx) last_slash_idx = folderPath.rfind(L'\\'); // Corrected from itemPath
    if (std::wstring::npos != last_slash_idx) return folderPath.substr(last_slash_idx + 1);
    return folderPath;
}
#endif
