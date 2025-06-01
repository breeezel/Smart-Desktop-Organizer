#ifndef DESKTOPINFO_H
#define DESKTOPINFO_H

#include <vector>
#include <string>
#include <filesystem> // For std::filesystem::file_time_type

#ifdef _WIN32
#include <windows.h> // For HWND, POINT, FILETIME (though FILETIME usage in struct is removed)
#endif

// Forward declaration for DesktopChecker to get desktop path
// class DesktopChecker;

struct DesktopItem {
    std::wstring path;
    std::wstring name;
    enum class ItemType { FILE, FOLDER, SHORTCUT, OTHER };
    ItemType type;
    int x, y;
    std::filesystem::file_time_type lastModified;

    // Default constructor
    DesktopItem() : x(0), y(0), type(ItemType::OTHER), lastModified(std::filesystem::file_time_type::min()) {}
};

class DesktopInfo {
public:
    DesktopInfo(); // Constructor
    ~DesktopInfo(); // Destructor

    int getScreenWidth();
    int getScreenHeight();
    float getPrimaryMonitorDpiScale();
    std::vector<DesktopItem> getAllDesktopItems(const std::wstring& desktopPath);

private:
    // Private helper methods
#ifdef _WIN32
    DesktopItem::ItemType determineItemType(const std::wstring& itemPath, DWORD fileAttributes);
#else
    DesktopItem::ItemType determineItemType(const std::wstring& itemPath, unsigned int fileAttributes); // Placeholder for non-Windows
#endif
    bool getFileLastModified(const std::wstring& itemPath, std::filesystem::file_time_type& lastModifiedTime);

#ifdef _WIN32
    // For advanced ListView interaction (handle with care) - Windows specific
    HWND getDesktopListViewHandle();
    std::wstring getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess);
    POINT getItemPositionFromListView(HWND listViewHwnd, int index);
#endif
    // Add more helpers as needed, e.g. for COM initialization if used broadly
    bool comInitialized;
};

#endif // DESKTOPINFO_H
