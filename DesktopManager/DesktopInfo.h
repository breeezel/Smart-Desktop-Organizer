#ifndef DESKTOPINFO_H
#define DESKTOPINFO_H

#include <vector>
#include <string>
#include <filesystem>
#include "ItemTypes.h"

#ifdef _WIN32
#include <windows.h>
#endif

struct DesktopItem {
    std::wstring path;
    std::wstring name;
    enum class ItemType { FILE, FOLDER, SHORTCUT, OTHER };
    ItemType type;

    // Original coordinates read from the desktop
    int original_x = 0;
    int original_y = 0;

    // Target coordinates after layout calculation
    int x = 0;
    int y = 0;

    std::filesystem::file_time_type lastModified;
    ItemCategory category;

    DesktopItem() : type(ItemType::OTHER),
                    original_x(0), original_y(0),
                    x(0), y(0), // Initialize target coords too
                    lastModified(std::filesystem::file_time_type::min()),
                    category(ItemCategory::UNCLASSIFIED) {}
};

class DesktopInfo {
public:
    DesktopInfo();
    ~DesktopInfo();

    int getScreenWidth();
    int getScreenHeight();
    float getPrimaryMonitorDpiScale();
    std::vector<DesktopItem> getAllDesktopItems(const std::wstring& desktopPath);

private:
#ifdef _WIN32
    DesktopItem::ItemType determineItemType(const std::wstring& itemPath, DWORD fileAttributes);
#else
    DesktopItem::ItemType determineItemType(const std::wstring& itemPath, unsigned int fileAttributes);
#endif
    bool getFileLastModified(const std::wstring& itemPath, std::filesystem::file_time_type& lastModifiedTime);

#ifdef _WIN32
    HWND getDesktopListViewHandle();
    std::wstring getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess);
    POINT getItemPositionFromListView(HWND listViewHwnd, int index);
#endif
};

#endif // DESKTOPINFO_H
