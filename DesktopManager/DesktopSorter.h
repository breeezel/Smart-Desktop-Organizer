#ifndef DESKTOPSORTER_H
#define DESKTOPSORTER_H

#include "DesktopInfo.h" // For DesktopItem
#include <string>
#include <vector>

// Forward declare HWND if windows.h is not included directly here
#if defined(_WIN32)
typedef struct HWND__* HWND; // Or include <windows.h>
#else
typedef void* HWND; // Placeholder for HWND on non-Windows
#endif

enum class ItemCategory {
    FOLDER,
    GAME,
    PROGRAM,
    TEXT_FILE,
    IMAGE_FILE, // Added for more variety
    VIDEO_FILE, // Added for more variety
    ARCHIVE_FILE, // Added for more variety
    UNCLASSIFIED,
    SPECIAL_SYSTEM // e.g. Recycle Bin, This PC (not handled by basic classification yet)
};

class DesktopSorter {
public:
    DesktopSorter();
    ~DesktopSorter();

    // Attempts to set the position of a named item on the desktop.
    // Note: Desktop's "Auto Arrange Icons" or "Align Icons to Grid" can override this.
    void setItemPosition(const std::wstring& itemName, int x, int y);

    // Classifies an item based on basic rules (placeholder).
    ItemCategory classifyItem(const DesktopItem& item);

private:
#ifdef _WIN32
    HWND getDesktopListViewHandle();
    // Helper to get item name from ListView (similar to DesktopInfo)
    std::wstring getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess);
#endif
    bool comInitialized; // Always declare, usage is conditional in .cpp
    // Add any other private helper methods if needed
};

#endif // DESKTOPSORTER_H
