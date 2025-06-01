#pragma once

#ifdef _WIN32 // This whole class is Windows-specific due to WinAPI calls

#include <string>
#include <vector>
#include <windows.h> // For SHFileOperationW, DeleteFileW, RemoveDirectoryW, GetFileAttributesW

// Forward declaration or include Logging.h for logging macros
// It's better to include it to ensure LOG_ macros are available.
// Adjust path as necessary if Logging.h is not found via include directories.
#include "../Logging/Logging.h"

class DesktopFileOperations {
public:
    DesktopFileOperations();

    /**
     * @brief Deletes a list of specified files or empty folders.
     * @param itemPaths A vector of wstrings, each being a full path to an item to delete.
     * @param useRecycleBin If true, items are moved to the Recycle Bin. If false, items are deleted permanently.
     *                      Note: RemoveDirectoryW for permanent deletion only works on empty directories.
     * @return true if all operations attempted were successful (or if no items were provided),
     *         false if any single operation failed. Detailed success/failure of individual items is logged.
     */
    bool deleteItems(const std::vector<std::wstring>& itemPaths, bool useRecycleBin);

private:
    /**
     * @brief Permanently deletes a single file or an empty directory.
     * @param path Full path to the file or empty directory.
     * @return true on success, false on failure.
     */
    bool deleteFileOrEmptyFolderPermanently(const std::wstring& path);

    /**
     * @brief Moves a single item (file or folder) to the Recycle Bin.
     * @param path Full path to the item.
     * @return true on success, false on failure.
     */
    bool moveItemToRecycleBin(const std::wstring& path);
};

#else // Non-Windows placeholder/stub

#include <string>
#include <vector>
#include <iostream> // For stub messages
#include "../Logging/Logging.h" // Include for LOG_ macros in stub

class DesktopFileOperations {
public:
    DesktopFileOperations() {
        LOG_DEBUG(L"DesktopFileOperations: Non-Windows stub constructor called.");
    }

    bool deleteItems(const std::vector<std::wstring>& itemPaths, bool useRecycleBin) {
        LOG_INFO(L"DesktopFileOperations (Stub): Attempting to delete " + std::to_wstring(itemPaths.size()) + L" items. Use Recycle Bin: " + (useRecycleBin ? L"Yes" : L"No"));
        if (itemPaths.empty()) {
            return true;
        }
        for(const auto& path : itemPaths) {
            LOG_DEBUG(L"DesktopFileOperations (Stub): 'Deleting' " + path);
        }
        // Simulate success for stub, actual file operations are not performed.
        std::wcout << L"[Stub] DesktopFileOperations: '" << std::to_wstring(itemPaths.size()) << L"' items processed for deletion." << std::endl;
        return true;
    }
};

#endif // _WIN32
