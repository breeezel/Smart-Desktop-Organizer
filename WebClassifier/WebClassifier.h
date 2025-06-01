#pragma once
#include <string>
#include <vector>
#include "DesktopManager/DesktopSorter.h"
#include "DesktopManager/DesktopInfo.h"

#ifdef _WIN32
#include <windows.h>
#include "nlohmann_json/json.hpp"
using json = nlohmann::json;
#endif

/**
 * @class WebClassifier
 * @brief Provides functionality to classify desktop items by performing web searches
 *        and analyzing HTML content. Uses caching for efficiency.
 *
 * On Windows, this class uses WinINet for HTTP requests and nlohmann/json for caching.
 * On other platforms, it provides non-functional stubs.
 */
class WebClassifier {
public:
    /**
     * @brief Constructor. Initializes the cache if applicable on the platform.
     */
    WebClassifier();

    /**
     * @brief Destructor. Handles any necessary cleanup.
     */
    ~WebClassifier();

    /**
     * @brief Executes an HTTP GET request to the specified URL.
     * @param url The URL to fetch (UTF-8 encoded std::string).
     * @param response Output parameter to store the HTTP response body (UTF-8 string).
     * @return true if the request was successfully made and a response (even if empty) was processed, false on critical errors (e.g., cannot connect).
     * @note On non-Windows, this is a stub and returns a mock HTML response.
     */
    bool executeHttpGet(const std::string& url, std::string& response);

    /**
     * @brief Performs a web search for the given desktop item to determine its category.
     * This method uses caching. If the item or its parent folder (for files/shortcuts)
     * is found in the cache, the cached category is returned. Otherwise, it performs
     * a web search (throttled to avoid excessive requests), classifies the HTML,
     * updates the cache, and then returns the category.
     * @param item The DesktopItem to classify.
     * @param outHtmlResponse Output parameter to store the raw HTML response from the web search (for debugging purposes).
     * @return ItemCategory The deduced category (GAME, PROGRAM, or UNCLASSIFIED if uncertain).
     * @note On non-Windows, this is a stub using mock data.
     */
    ItemCategory performSearch(const DesktopItem& item, std::string& outHtmlResponse);

    /**
     * @brief Classifies an item based on keywords found in HTML content.
     * This method is typically called by `performSearch` after fetching HTML.
     * @param htmlResponse The HTML content (UTF-8 string) from a web search.
     * @param itemName The original name of the item being classified (used for context in classification logic).
     * @return ItemCategory The deduced category.
     * @note On non-Windows, this is a stub.
     */
    ItemCategory classifyItemFromHtml(const std::string& htmlResponse, const std::wstring& itemName);

#ifdef _WIN32
    /**
     * @brief Clears the classification cache both in memory and on disk.
     * @note This operation is only functional on Windows.
     */
    void clearCache();
#endif

private:
#ifdef _WIN32
    json classificationCache;        ///< In-memory JSON object for caching classifications.
    std::wstring cacheFilePath;      ///< Full path to the cache file (e.g., classification_cache.json).

    // Cache operations
    std::wstring getCacheFilePath();
    void loadCache();
    void saveCache();
    ItemCategory getCachedCategory(const std::wstring& itemNameOrKey);
    void updateCache(const std::wstring& itemNameOrKey, ItemCategory category);

    // Utility for cache
    std::string categoryToString(ItemCategory category);
    ItemCategory stringToCategory(const std::string& categoryStr);
#endif // End of _WIN32 specific private members

    // Common private methods (stubs for non-windows exist for these)
    /**
     * @brief URL-encodes a wide string.
     * On Windows, converts to UTF-8 then percent-encodes.
     * On non-Windows, this is a basic stub and not fully URL-safe for all characters.
     * @param data The wide string to encode.
     * @return std::string The URL-encoded string.
     */
    std::string urlEncode(const std::wstring& data);

    /**
     * @brief Extracts the parent folder path from a given item path.
     * @param itemPath The full path to the item.
     * @return std::wstring The path of the parent folder, or an empty string on error/if no parent.
     * @note Provides a basic stub for non-Windows.
     */
    std::wstring getParentFolderPath(const std::wstring& itemPath);

    /**
     * @brief Extracts the folder name from a given folder path.
     * @param folderPath The full path to the folder.
     * @return std::wstring The name of the folder, or an empty string on error.
     * @note Provides a basic stub for non-Windows.
     */
    std::wstring getFolderName(const std::wstring& folderPath);
    // Note: getParentFolderPath and getFolderName were here before, but should be with other path helpers if also for non-windows
    // Forcing them out of the #ifdef _WIN32 block for private members if they have stubs.
    // Actually, they are already outside in my current version of WebClassifier.h that I am looking at.
    // The error is likely because the previous edit to WebClassifier.h was not complete or I missed it.
    // Let me ensure the structure is:
    // private:
    // #ifdef _WIN32
    //   // win-specific cache members
    //   // win-specific cache methods
    //   // win-specific category to/from string
    // #endif
    //   // Common private methods (like urlEncode, and NOW path helpers if they have stubs)
    //   std::string urlEncode(const std::wstring& data);
    //   std::wstring getParentFolderPath(const std::wstring& itemPath); // MOVED/ENSURED OUTSIDE
    //   std::wstring getFolderName(const std::wstring& folderPath);   // MOVED/ENSURED OUTSIDE
    // This structure was what I intended. The error is surprising if the file was correctly updated.
    // Let's re-check the previous version of WebClassifier.h I submitted.
    // The previous version was:
    // #ifdef _WIN32
    //    json classificationCache;
    //    std::wstring cacheFilePath;
    //    std::wstring getCacheFilePath();
    //    void loadCache();
    //    void saveCache();
    //    ItemCategory getCachedCategory(const std::wstring& itemNameOrKey);
    //    void updateCache(const std::wstring& itemNameOrKey, ItemCategory category);
    //    std::string categoryToString(ItemCategory category);
    //    ItemCategory stringToCategory(const std::string& categoryStr);
    //    std::wstring getParentFolderPath(const std::wstring& itemPath); // <<<< HERE, INSIDE #ifdef _WIN32
    //    std::wstring getFolderName(const std::wstring& folderPath);   // <<<< HERE, INSIDE #ifdef _WIN32
    // #endif
    //    std::string urlEncode(const std::wstring& data);
    // This confirms my mistake. Path helpers were declared inside #ifdef _WIN32.

    // Corrected structure should be:
    // private:
    // #ifdef _WIN32
    //   json classificationCache;
    //   std::wstring cacheFilePath;
    //   std::wstring getCacheFilePath();
    //   void loadCache();
    //   void saveCache();
    //   ItemCategory getCachedCategory(const std::wstring& itemNameOrKey);
    //   void updateCache(const std::wstring& itemNameOrKey, ItemCategory category);
    //   std::string categoryToString(ItemCategory category);
    //   ItemCategory stringToCategory(const std::string& categoryStr);
    // #endif
    //   // Common private methods (stubs for non-windows exist for these)
    //   std::string urlEncode(const std::wstring& data);
    //   std::wstring getParentFolderPath(const std::wstring& itemPath);
    //   std::wstring getFolderName(const std::wstring& folderPath);
    // I will apply this corrected structure.
};
