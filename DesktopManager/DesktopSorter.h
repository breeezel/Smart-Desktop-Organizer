#ifndef DESKTOPSORTER_H
#define DESKTOPSORTER_H

#include "DesktopInfo.h"
#include "../WebClassifier/WebClassifier.h"
#include "../Logging/Logging.h"
#include "ItemTypes.h" // Include the new common ItemCategory definition

#include <string>
#include <vector>
#include <memory>

// ItemCategory enum is now in ItemTypes.h
// enum class ItemCategory {
//     FOLDER,
//     GAME,
//     PROGRAM,
//     TEXT_FILE,
//     IMAGE_FILE,
//     VIDEO_FILE,
//     ARCHIVE_FILE,
//     UNCLASSIFIED,
//     SPECIAL_SYSTEM
// };

class DesktopSorter {
public:
    DesktopSorter();
    ~DesktopSorter();

    void setItemPosition(const std::wstring& itemName, int x, int y);
    ItemCategory classifyItem(const DesktopItem& item); // ItemCategory now comes from ItemTypes.h

private:
    std::unique_ptr<WebClassifier> m_webClassifier;

#ifdef _WIN32
    HWND getDesktopListViewHandle();
    std::wstring getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess);
#endif
};

#endif // DESKTOPSORTER_H
