#include "DesktopLayoutManager.h"
#include <algorithm>
#include <vector>
#include <map>
#include <windows.h> // For POINT

// Logging.h is included via DesktopLayoutManager.h for LOG_ macros

static std::wstring categoryToWString_dlm(ItemCategory category) { // Renamed to avoid potential conflicts
    switch (category) {
        case ItemCategory::FOLDER: return L"Folder";
        case ItemCategory::GAME: return L"Game";
        case ItemCategory::PROGRAM: return L"Program";
        case ItemCategory::TEXT_FILE: return L"TextFile";
        case ItemCategory::IMAGE_FILE: return L"ImageFile";
        case ItemCategory::VIDEO_FILE: return L"VideoFile";
        case ItemCategory::ARCHIVE_FILE: return L"ArchiveFile";
        case ItemCategory::UNCLASSIFIED: return L"Unclassified";
        case ItemCategory::SPECIAL_SYSTEM: return L"SpecialSystem";
        default: return L"Unknown(" + std::to_wstring(static_cast<int>(category)) + L")";
    }
}

DesktopLayoutManager::DesktopLayoutManager() {
    LOG_DEBUG(L"DesktopLayoutManager: Constructor called.");
}

/**
 * @brief Calculates how many items should be placed in a single row or column within a zone.
 * Attempts to meet "не менее 5-7 элементов, но не более 10-12, с учетом свободного места."
 * (not less than 5-7 elements, but not more than 10-12, considering free space).
 *
 * @param zoneDimension The width of the zone if placing in rows, or height if placing in columns.
 * @param itemCellDimension The total width (cellWidth) or height (cellHeight) one item occupies.
 * @param totalItemCountInCategory Total number of items in this specific category.
 * @param categoryContext For logging purposes.
 * @return The calculated number of items per line (row/column).
 */
int DesktopLayoutManager::calculateItemsPerLine(int zoneDimension, int itemCellDimension, int totalItemCountInCategory, ItemCategory categoryContext) {
    if (itemCellDimension <= 0) {
        LOG_WARNING(L"DesktopLayoutManager::calculateItemsPerLine: itemCellDimension is zero or negative for category "
                    + categoryToWString_dlm(categoryContext) + L". Defaulting to 1 item per line.");
        return 1;
    }

    // Max items that can possibly fit based on available space
    int maxPossibleInSpace = zoneDimension / itemCellDimension;
    if (maxPossibleInSpace == 0) maxPossibleInSpace = 1; // Must be able to fit at least one item if zoneDimension > 0

    // Default constraints from ТЗ
    const int minItemsPerLine = 5; // Use 5 as the lower end of "5-7"
    const int maxItemsPerLine = 12;

    int calculated = maxPossibleInSpace; // Start with what physically fits

    // If we have few items in total for this category, try to fit them all in one line,
    // but not more than what space allows or the absolute max (12).
    if (totalItemCountInCategory <= maxItemsPerLine) {
        calculated = std::min(maxPossibleInSpace, totalItemCountInCategory);
    } else { // We have more items than the max cap
        calculated = std::min(maxPossibleInSpace, maxItemsPerLine);
    }

    // If the calculated number is below the desired minimum, but we have enough items
    // AND enough space for that minimum, then use the minimum.
    if (calculated < minItemsPerLine && totalItemCountInCategory >= minItemsPerLine) {
        calculated = std::min(maxPossibleInSpace, minItemsPerLine);
    }

    // Ensure at least 1 item per line if there are items.
    if (calculated == 0 && totalItemCountInCategory > 0) {
        calculated = 1;
    }
    if (totalItemCountInCategory == 0) { // Should not happen if catItems.empty() is checked by caller
        calculated = 0;
    }


    LOG_DEBUG(L"DesktopLayoutManager::calculateItemsPerLine for Category " + categoryToWString_dlm(categoryContext) +
              L": ZoneDim=" + std::to_wstring(zoneDimension) +
              L", ItemCellDim=" + std::to_wstring(itemCellDimension) +
              L", TotalItems=" + std::to_wstring(totalItemCountInCategory) +
              L", MaxPossibleSpace=" + std::to_wstring(maxPossibleInSpace) +
              L", CalculatedItemsPerLine=" + std::to_wstring(calculated));
    return calculated;
}


std::vector<DesktopItem> DesktopLayoutManager::calculateLayout(
    std::vector<DesktopItem>& items,
    const ScreenMetrics& sm) {

    LOG_INFO(L"DesktopLayoutManager::calculateLayout - Starting layout for " + std::to_wstring(items.size()) + L" items.");

    // Global sort by lastModified date (newest first)
    std::sort(items.begin(), items.end(), [](const DesktopItem& a, const DesktopItem& b) {
        return a.lastModified > b.lastModified;
    });
    LOG_INFO(L"DesktopLayoutManager: All " + std::to_wstring(items.size()) + L" items sorted globally by modification date (newest first).");

    auto groupedItems = groupItemsByCategory(items);
    LOG_INFO(L"DesktopLayoutManager: Items grouped by category.");
    for(const auto& pair : groupedItems) {
        LOG_DEBUG(L"  Category '" + categoryToWString_dlm(pair.first) + L"' has " + std::to_wstring(pair.second.size()) + L" items.");
    }

    std::map<ItemCategory, Rect> zones;
    // Zones are defined within the effective screen area (sm.effectiveScreenWidth, sm.effectiveScreenHeight)
    // Coordinates here are relative to (0,0) of this effective area.
    // Margins (sm.marginLeft, sm.marginTop) will be added at the very end.

    // Define heights for Folder and Text zones based on percentages
    int folderZoneH = sm.effectiveScreenHeight * 20 / 100;
    int textFileZoneH = sm.effectiveScreenHeight * 15 / 100;

    // Calculate Y start positions
    int folderZoneYStart = sm.effectiveScreenHeight - folderZoneH;
    int textFileZoneYStart = folderZoneYStart - textFileZoneH;

    if (textFileZoneYStart < 0) {
        LOG_WARNING(L"Screen too small for dedicated TextFile zone above Folders. Adjusting TextFile zone.");
        textFileZoneH = folderZoneYStart; // Max possible height without going below 0
        textFileZoneYStart = 0;
    }

    zones[ItemCategory::FOLDER] = {0, folderZoneYStart, sm.effectiveScreenWidth, folderZoneH};
    zones[ItemCategory::TEXT_FILE] = {0, textFileZoneYStart, sm.effectiveScreenWidth, textFileZoneH};

    // Define the middle zone for other categories
    int middleZoneYStart = 0;
    int middleZoneHeight = textFileZoneYStart; // Height from top of screen to start of TextFile zone
    if (middleZoneHeight < 0) middleZoneHeight = 0; // Safety check

    // Subdivide the middle zone
    int leftStripW = sm.effectiveScreenWidth * 25 / 100;
    int rightStripW = sm.effectiveScreenWidth * 25 / 100;
    int centerStripW = sm.effectiveScreenWidth - leftStripW - rightStripW;

    zones[ItemCategory::PROGRAM]      = {0, middleZoneYStart, leftStripW, middleZoneHeight};
    zones[ItemCategory::GAME]         = {leftStripW + centerStripW, middleZoneYStart, rightStripW, middleZoneHeight};
    zones[ItemCategory::UNCLASSIFIED] = {leftStripW, middleZoneYStart, centerStripW, middleZoneHeight};
    zones[ItemCategory::IMAGE_FILE]   = zones[ItemCategory::UNCLASSIFIED];
    zones[ItemCategory::VIDEO_FILE]   = zones[ItemCategory::UNCLASSIFIED];
    zones[ItemCategory::ARCHIVE_FILE] = zones[ItemCategory::UNCLASSIFIED];
    zones[ItemCategory::SPECIAL_SYSTEM] = {0,0,0,0}; // Not placed

    std::map<ItemCategory, POINT> currentCategoryCursors;
    std::map<ItemCategory, int> currentCategoryItemsInCurrentLine;
    std::map<ItemCategory, int> itemsPerLineForCategory;

    for (auto const& [category, catItems] : groupedItems) {
        if (catItems.empty() || zones.find(category) == zones.end() || category == ItemCategory::SPECIAL_SYSTEM) {
            continue;
        }
        const Rect& zone = zones.at(category);
        currentCategoryItemsInCurrentLine[category] = 0;

        switch (category) {
            case ItemCategory::FOLDER:      // Bottom-up, Right-to-Left
                itemsPerLineForCategory[category] = calculateItemsPerLine(zone.width, sm.cellWidth, catItems.size(), category);
                currentCategoryCursors[category] = {zone.x + zone.width - sm.effectiveIconWidth, zone.y + zone.height - sm.effectiveIconHeight};
                break;
            case ItemCategory::TEXT_FILE:   // Bottom-up, Right-to-Left (like Folders)
                itemsPerLineForCategory[category] = calculateItemsPerLine(zone.width, sm.cellWidth, catItems.size(), category);
                currentCategoryCursors[category] = {zone.x + zone.width - sm.effectiveIconWidth, zone.y + zone.height - sm.effectiveIconHeight};
                break;
            case ItemCategory::GAME: // Top-down, Right-to-Left
                itemsPerLineForCategory[category] = calculateItemsPerLine(zone.width, sm.cellWidth, catItems.size(), category);
                currentCategoryCursors[category] = {zone.x + zone.width - sm.effectiveIconWidth, zone.y };
                break;
            case ItemCategory::PROGRAM:     // Top-to-Bottom, Left-to-Right (column-wise)
                itemsPerLineForCategory[category] = calculateItemsPerLine(zone.height, sm.cellHeight, catItems.size(), category); // Items per column
                currentCategoryCursors[category] = {zone.x, zone.y};
                break;
            case ItemCategory::UNCLASSIFIED:
            case ItemCategory::IMAGE_FILE:
            case ItemCategory::VIDEO_FILE:
            case ItemCategory::ARCHIVE_FILE:
            default:                        // Top-down, Left-to-Right
                itemsPerLineForCategory[category] = calculateItemsPerLine(zone.width, sm.cellWidth, catItems.size(), category);
                currentCategoryCursors[category] = {zone.x, zone.y};
                break;
        }
        LOG_DEBUG(L"Init cursor for " + categoryToWString_dlm(category) + L": ZoneX=" + std::to_wstring(zone.x) + L", ZoneY=" + std::to_wstring(zone.y) + L", CursorX=" + std::to_wstring(currentCategoryCursors[category].x) + L", CursorY=" + std::to_wstring(currentCategoryCursors[category].y) + L", ItemsPerLine=" + std::to_wstring(itemsPerLineForCategory[category]));
    }

    for (DesktopItem& item : items) {
        ItemCategory category = item.category;
        if (zones.find(category) == zones.end() || category == ItemCategory::SPECIAL_SYSTEM || itemsPerLineForCategory.count(category) == 0) {
            LOG_DEBUG(L"Skipping placement for item '" + item.name + L"' (category: " + categoryToWString_dlm(category) + L") - no zone or items per line defined.");
            item.x = -1; item.y = -1; // Mark as unplaced
            continue;
        }

        const Rect& zone = zones.at(category);
        POINT& cursor = currentCategoryCursors.at(category);
        int& itemsInThisLine = currentCategoryItemsInCurrentLine.at(category);
        int maxItemsPerThisLine = itemsPerLineForCategory.at(category);

        if (maxItemsPerThisLine == 0) { // Should be at least 1 if catItems not empty
            LOG_WARNING(L"Max items per line is 0 for category " + categoryToWString_dlm(category) + L". Skipping item " + item.name);
            item.x = -1; item.y = -1; continue;
        }

        // Check if cursor needs to wrap BEFORE placing current item
        // This depends on the direction of placement for the category

        bool outOfBounds = false;
        switch (category) {
            case ItemCategory::FOLDER: // Bottom-up, RTL
                if (itemsInThisLine >= maxItemsPerThisLine) {
                    cursor.x = zone.x + zone.width - sm.effectiveIconWidth;
                    cursor.y -= sm.cellHeight;
                    itemsInThisLine = 0;
                }
                if (cursor.y < zone.y) outOfBounds = true;
                break;
            case ItemCategory::TEXT_FILE: // Bottom-up, RTL (like Folders)
                if (itemsInThisLine >= maxItemsPerThisLine) {
                    cursor.x = zone.x + zone.width - sm.effectiveIconWidth; // Reset to right edge
                    cursor.y -= sm.cellHeight;                             // Move to row above
                    itemsInThisLine = 0;
                }
                if (cursor.y < zone.y) outOfBounds = true; // Check if cursor moved above zone top
                break;
            case ItemCategory::GAME: // Top-down, RTL
                 if (itemsInThisLine >= maxItemsPerThisLine) {
                    cursor.x = zone.x + zone.width - sm.effectiveIconWidth;
                    cursor.y += sm.cellHeight;
                    itemsInThisLine = 0;
                }
                if (cursor.y + sm.effectiveIconHeight > zone.y + zone.height) outOfBounds = true;
                break;
            case ItemCategory::PROGRAM: // TTB, LTR (column-wise)
                if (itemsInThisLine >= maxItemsPerThisLine) {
                    cursor.y = zone.y;
                    cursor.x += sm.cellWidth;
                    itemsInThisLine = 0;
                }
                if (cursor.x + sm.effectiveIconWidth > zone.x + zone.width) outOfBounds = true;
                break;
            default: // UNCLASSIFIED & others: TTD, LTR
                if (itemsInThisLine >= maxItemsPerThisLine) {
                    cursor.x = zone.x;
                    cursor.y += sm.cellHeight;
                    itemsInThisLine = 0;
                }
                if (cursor.y + sm.effectiveIconHeight > zone.y + zone.height) outOfBounds = true;
                break;
        }

        if(outOfBounds){
            LOG_WARNING(L"Category " + categoryToWString_dlm(category) + L" ran out of space in its zone for item '" + item.name + L"'. Item unplaced.");
            item.x = -1; item.y = -1; // Mark as unplaced
            // Potentially could stop placing for this category or try a fallback zone
            continue;
        }

        item.x = sm.marginLeft + cursor.x;
        item.y = sm.marginTop + cursor.y;
        LOG_DEBUG(L"Placed item '" + item.name + L"' (Cat: " + categoryToWString_dlm(category) + L") at screen (" + std::to_wstring(item.x) + L"," + std::to_wstring(item.y) + L") zone-rel ("+ std::to_wstring(cursor.x) +L","+std::to_wstring(cursor.y)+L")");
        itemsInThisLine++;

        // Advance cursor for the *next* item (after current item is placed)
        switch (category) {
            case ItemCategory::FOLDER: cursor.x -= sm.cellWidth; break; // RTL
            case ItemCategory::TEXT_FILE: cursor.x -= sm.cellWidth; break; // RTL (like Folders)
            case ItemCategory::GAME: cursor.x -= sm.cellWidth; break; // RTL
            case ItemCategory::PROGRAM: cursor.y += sm.cellHeight; break; // TTB
            default: cursor.x += sm.cellWidth; break; // LTR
        }
    }

    LOG_INFO(L"DesktopLayoutManager::calculateLayout - Placement calculation complete.");
    return items;
}

std::map<ItemCategory, std::vector<DesktopItem>> DesktopLayoutManager::groupItemsByCategory(
    const std::vector<DesktopItem>& items) {
    std::map<ItemCategory, std::vector<DesktopItem>> categorizedItems;
    // LOG_DEBUG(L"DesktopLayoutManager::groupItemsByCategory - Grouping " + std::to_wstring(items.size()) + L" items.");
    for (const auto& item : items) {
        categorizedItems[item.category].push_back(item);
    }
    return categorizedItems;
}
