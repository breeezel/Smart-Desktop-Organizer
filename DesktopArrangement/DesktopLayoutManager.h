#pragma once

#include <vector>
#include <string>
#include <map>
#include <algorithm> // For std::max
#include "../DesktopManager/DesktopInfo.h"
#include "../DesktopManager/ItemTypes.h"
#include "../Logging/Logging.h"

struct Rect {
    int x = 0, y = 0, width = 0, height = 0;
};

struct ScreenMetrics {
    int screenWidth = 1920;
    int screenHeight = 1080;
    float dpiScale = 1.0f;

    // Base dimensions at 100% DPI scaling
    int baseIconWidth = 72;
    int baseIconHeight = 72;
    int baseIconGapX = 15; // Horizontal gap between icons
    int baseIconGapY = 15; // Vertical gap between icons

    // Effective values after DPI scaling
    int effectiveIconWidth = 72;
    int effectiveIconHeight = 72;
    int effectiveIconGapX = 15;
    int effectiveIconGapY = 15;

    // Total space one icon cell will occupy (icon + gap)
    // This is effectively the grid cell size.
    int cellWidth = 87;
    int cellHeight = 87;

    // Usable area on screen after margins
    int effectiveScreenWidth = 1920;
    int effectiveScreenHeight = 1080;
    int marginTop = 20;
    int marginBottom = 20;
    int marginLeft = 20;
    int marginRight = 20;

    ScreenMetrics() { calculateEffectiveDimensions(); }

    ScreenMetrics(int width, int height, float scale)
        : screenWidth(width), screenHeight(height), dpiScale(scale) {
        calculateEffectiveDimensions();
    }

    void calculateEffectiveDimensions() {
        effectiveIconWidth = static_cast<int>(baseIconWidth * dpiScale);
        effectiveIconHeight = static_cast<int>(baseIconHeight * dpiScale);
        effectiveIconGapX = static_cast<int>(baseIconGapX * dpiScale);
        effectiveIconGapY = static_cast<int>(baseIconGapY * dpiScale);

        // cellWidth/Height is the total space an icon takes up, including its own size and the gap *after* it.
        // This is what's used for grid step calculations.
        cellWidth = effectiveIconWidth + effectiveIconGapX;
        cellHeight = effectiveIconHeight + effectiveIconGapY;

        effectiveScreenWidth = screenWidth - marginLeft - marginRight;
        effectiveScreenHeight = screenHeight - marginTop - marginBottom;

        LOG_DEBUG(L"ScreenMetrics Updated: Screen WxH: " + std::to_wstring(screenWidth) + L"x" + std::to_wstring(screenHeight) + L", Scale: " + std::to_wstring(dpiScale));
        LOG_DEBUG(L"  Effective Area WxH: " + std::to_wstring(effectiveScreenWidth) + L"x" + std::to_wstring(effectiveScreenHeight));
        LOG_DEBUG(L"  Icon WxH (eff): " + std::to_wstring(effectiveIconWidth) + L"x" + std::to_wstring(effectiveIconHeight));
        LOG_DEBUG(L"  Gap X,Y (eff): " + std::to_wstring(effectiveIconGapX) + L"x" + std::to_wstring(effectiveIconGapY));
        LOG_DEBUG(L"  Cell WxH (grid step): " + std::to_wstring(cellWidth) + L"x" + std::to_wstring(cellHeight));
    }
};

class DesktopLayoutManager {
public:
    DesktopLayoutManager();

    std::vector<DesktopItem> calculateLayout(
        std::vector<DesktopItem>& items,
        const ScreenMetrics& screenMetrics);

private:
    std::map<ItemCategory, std::vector<DesktopItem>> groupItemsByCategory(
        const std::vector<DesktopItem>& items);

    int calculateItemsPerLine(int zoneDimension, int itemCellDimension, int totalItemCountInCategory, ItemCategory categoryContext);
};
