#include "DesktopFileOperations.h"

#ifdef _WIN32 // This whole class implementation is Windows-specific

#include "../Logging/Logging.h" // Included via .h, but good for explicitness if needed
#include <shlobj.h>  // For SHFileOperationW specifically
#include <iostream>  // For std::wcerr in case logger isn't working (should not happen)
#include <vector>    // For std::vector<wchar_t> in moveItemToRecycleBin

DesktopFileOperations::DesktopFileOperations() {
    LOG_DEBUG(L"DesktopFileOperations: Windows implementation constructor called.");
}

bool DesktopFileOperations::deleteFileOrEmptyFolderPermanently(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        LOG_ERROR(L"Permanent Delete: Failed to get attributes for '" + path + L"'. Error: " + std::to_wstring(GetLastError()));
        return false;
    }

    if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
        // It's a directory, attempt to remove if empty
        if (RemoveDirectoryW(path.c_str())) {
            LOG_INFO(L"Permanently deleted empty folder: " + path);
            return true;
        } else {
            // RemoveDirectoryW fails if the directory is not empty.
            LOG_ERROR(L"Failed to permanently delete folder (it might not be empty or access denied): '" + path + L"'. Error: " + std::to_wstring(GetLastError()));
            return false;
        }
    } else {
        // It's a file
        if (DeleteFileW(path.c_str())) {
            LOG_INFO(L"Permanently deleted file: " + path);
            return true;
        } else {
            LOG_ERROR(L"Failed to permanently delete file: '" + path + L"'. Error: " + std::to_wstring(GetLastError()));
            return false;
        }
    }
}

bool DesktopFileOperations::moveItemToRecycleBin(const std::wstring& path) {
    // SHFileOperation requires a double-null-terminated string for pFrom.
    // The path itself should not end with a null, the API expects a list of strings,
    // each terminated by one null, and the entire list terminated by two nulls.
    // For a single path, it's path + '\0' + '\0'.
    std::vector<wchar_t> doubleNullPath;
    doubleNullPath.reserve(path.length() + 2); // Reserve space
    for(wchar_t ch : path) { // Copy the path characters
        doubleNullPath.push_back(ch);
    }
    doubleNullPath.push_back(L'\0'); // First null terminator
    doubleNullPath.push_back(L'\0'); // Second null terminator

    SHFILEOPSTRUCTW sfos = {0};
    sfos.hwnd = NULL; // No owner window for console app or background task
    sfos.wFunc = FO_DELETE;
    sfos.pFrom = doubleNullPath.data();
    // FOF_ALLOWUNDO: Moves to Recycle Bin.
    // FOF_NOCONFIRMATION: Suppresses "Are you sure?" dialog.
    // FOF_NOERRORUI: Suppresses error dialogs (errors are handled programmatically).
    // FOF_SILENT: Does not display progress (though FO_DELETE is usually quick).
    sfos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    LOG_DEBUG(L"Attempting to move to Recycle Bin: " + path);
    int result = SHFileOperationW(&sfos);

    if (result == 0 && !sfos.fAnyOperationsAborted) { // Success as per MSDN documentation
        LOG_INFO(L"Moved to Recycle Bin: " + path);
        return true;
    } else {
        // Log detailed error. SHFileOperationW returns non-zero on error.
        // sfos.fAnyOperationsAborted is true if user cancels (not possible with FOF_NOCONFIRMATION)
        // or if an error prevents operation on any file.
        LOG_ERROR(L"Failed to move to Recycle Bin: '" + path + L"'. SHFileOperationW result: " + std::to_wstring(result)
                  + L" (Aborted: " + (sfos.fAnyOperationsAborted ? L"Yes" : L"No")
                  + L"). GetLastError(): " + std::to_wstring(GetLastError())); // GetLastError might provide additional context
        return false;
    }
}

bool DesktopFileOperations::deleteItems(const std::vector<std::wstring>& itemPaths, bool useRecycleBin) {
    if (itemPaths.empty()) {
        LOG_INFO(L"DesktopFileOperations::deleteItems: No items provided for deletion.");
        return true;
    }

    LOG_INFO(L"DesktopFileOperations::deleteItems: Starting deletion process for " + std::to_wstring(itemPaths.size())
             + L" items. To Recycle Bin: " + (useRecycleBin ? L"Yes" : L"No"));
    bool allSucceeded = true;

    // For SHFileOperation with multiple files, it's more efficient to pass them all at once.
    // However, for clarity of individual logging and mixed permanent/recycle bin (though not mixed here),
    // iterating is fine. If all are going to recycle bin, one SHFileOperation call is better.
    if (useRecycleBin) {
        // Prepare a single double-null-terminated string list for all paths
        std::vector<wchar_t> pFromPaths;
        for (const auto& path : itemPaths) {
            if (path.empty()) continue; // Skip empty paths
            pFromPaths.insert(pFromPaths.end(), path.begin(), path.end());
            pFromPaths.push_back(L'\0'); // Null terminate this path
        }
        if (pFromPaths.empty()) { // All paths were empty
             LOG_WARNING(L"DesktopFileOperations::deleteItems: All provided paths were empty for Recycle Bin operation.");
             return true; // Or false, depending on desired strictness
        }
        pFromPaths.push_back(L'\0'); // Final double null terminator for the list

        SHFILEOPSTRUCTW sfos = {0};
        sfos.hwnd = NULL;
        sfos.wFunc = FO_DELETE;
        sfos.pFrom = pFromPaths.data();
        sfos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

        LOG_DEBUG(L"Attempting to move " + std::to_wstring(itemPaths.size()) + L" items to Recycle Bin in a single operation.");
        int result = SHFileOperationW(&sfos);

        if (result == 0 && !sfos.fAnyOperationsAborted) {
            LOG_INFO(L"All " + std::to_wstring(itemPaths.size()) + L" items successfully processed for Recycle Bin by SHFileOperationW.");
            // SHFileOperation doesn't easily tell *which* files failed if some did but others succeeded
            // unless fAnyOperationsAborted is true or result is non-zero.
            // For this simplified case, assume all success if result is 0.
            for(const auto& path : itemPaths) LOG_DEBUG(L"  - Processed for Recycle Bin: " + path);
        } else {
            allSucceeded = false;
            LOG_ERROR(L"SHFileOperationW failed to move one or more items to Recycle Bin. Result: " + std::to_wstring(result)
                      + L" (Aborted: " + (sfos.fAnyOperationsAborted ? L"Yes" : L"No")
                      + L"). GetLastError(): " + std::to_wstring(GetLastError()));
            // Individual logging per file is harder with single SHFileOperation call
        }

    } else { // Permanent deletion, one by one
        for (const auto& path : itemPaths) {
            if (path.empty()) continue;
            LOG_DEBUG(L"Attempting to permanently delete: " + path);
            bool success = deleteFileOrEmptyFolderPermanently(path);
            if (!success) {
                allSucceeded = false;
                // Specific error already logged by deleteFileOrEmptyFolderPermanently
            }
        }
    }

    if (allSucceeded) {
        LOG_INFO(L"DesktopFileOperations::deleteItems: All specified items processed for deletion successfully.");
    } else {
        LOG_WARNING(L"DesktopFileOperations::deleteItems: One or more items could not be deleted. Check logs for details.");
    }
    return allSucceeded;
}

#else // Non-Windows stub implementations (already defined in .h for simplicity, but can be here)

// DesktopFileOperations::DesktopFileOperations() {
//     LOG_DEBUG(L"DesktopFileOperations: Non-Windows stub constructor called.");
// }
//
// bool DesktopFileOperations::deleteItems(const std::vector<std::wstring>& itemPaths, bool useRecycleBin) {
//     LOG_INFO(L"DesktopFileOperations (Stub): Attempting to delete " + std::to_wstring(itemPaths.size()) + L" items. Use Recycle Bin: " + (useRecycleBin ? L"Yes" : L"No"));
//     if (itemPaths.empty()) {
//         return true;
//     }
//     for(const auto& path : itemPaths) {
//         LOG_DEBUG(L"DesktopFileOperations (Stub): 'Deleting' " + path);
//     }
//     // Simulate success for stub
//     // std::wcout << L"[Stub] DesktopFileOperations: '" << std::to_wstring(itemPaths.size()) << L"' items processed for deletion." << std::endl;
//     return true;
// }

#endif // _WIN32
