#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <limits>
#include <codecvt>
#include <algorithm>

#include "DesktopManager/DesktopChecker.h"
#include "DesktopManager/DesktopInfo.h"
#include "DesktopManager/DesktopSorter.h"
#include "DesktopManager/DesktopFileOperations.h"
#include "WebClassifier/WebClassifier.h"
#include "ConfigManager/ConfigManager.h"
#include "Logging/Logging.h"
#include "DesktopArrangement/DesktopLayoutManager.h" // Added

#ifdef _WIN32
// No specific includes needed here for main
#endif

// --- Global Instances for CLI Testing ---
ConfigManager& globalConfigManager = ConfigManager::getInstance(); // Use reference for singleton
DesktopInfo globalDesktopInfoInstance;
WebClassifier globalWebClassifierInstance;
DesktopSorter globalDesktopSorterInstance;
DesktopFileOperations globalFileOpsInstance;
DesktopLayoutManager globalLayoutManagerInstance; // Added


std::wstring file_time_to_wstring(const std::filesystem::file_time_type& ftime) {
    if (ftime == std::filesystem::file_time_type::min()) return L"N/A";
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm t_local{}; // Initialize to zero
#ifdef _WIN32
    if (localtime_s(&t_local, &tt) != 0) return L"Error converting time";
#else
    if (localtime_r(&tt, &t_local) == nullptr) return L"Error converting time";
#endif
    std::wstringstream wss;
    wss << std::put_time(&t_local, L"%Y-%m-%d %H:%M:%S");
    if (wss.fail()) return L"Error formatting time";
    return wss.str();
}
std::wstring item_type_to_wstring(DesktopItem::ItemType type) { /* ... */
    switch (type) {
        case DesktopItem::ItemType::FILE: return L"File";
        case DesktopItem::ItemType::FOLDER: return L"Folder";
        case DesktopItem::ItemType::SHORTCUT: return L"Shortcut";
        case DesktopItem::ItemType::OTHER: return L"Other";
        default: return L"Unknown";
    }
}
// Uses ItemCategory from DesktopManager/ItemTypes.h (included via DesktopSorter.h or others)
std::wstring item_category_to_wstring_main(ItemCategory category) {
    switch (category) {
        case ItemCategory::FOLDER: return L"Folder";
        case ItemCategory::GAME: return L"Game";
        case ItemCategory::PROGRAM: return L"Program";
        case ItemCategory::TEXT_FILE: return L"Text File";
        case ItemCategory::IMAGE_FILE: return L"Image File";
        case ItemCategory::VIDEO_FILE: return L"Video File";
        case ItemCategory::ARCHIVE_FILE: return L"Archive File";
        case ItemCategory::UNCLASSIFIED: return L"Unclassified";
        case ItemCategory::SPECIAL_SYSTEM: return L"Special System";
        default: return L"Unknown Category";
    }
}

// --- CLI Option Handlers ---
void runDesktopCheck() { /* ... uses globalConfigManager.shouldUseRecycleBin() ... */
    LOG_INFO(L"CLI: User initiated Desktop Check operation.");
    std::wcout << L"\n--- Running Desktop Check ---" << std::endl;
    DesktopChecker checker;
    std::wstring desktopPath = checker.getDesktopPath();
    if (desktopPath.empty()) { LOG_ERROR(L"CLI: Desktop Check - Failed to get desktop path."); std::wcerr << L"Failed to get desktop path." << std::endl; return; }
    std::wcout << L"Desktop Path: " << desktopPath << std::endl;
    std::wcout << L"\nSearching for invalid shortcuts..." << std::endl;
    std::vector<std::wstring> invalidShortcuts = checker.findInvalidShortcuts(desktopPath);
    if (invalidShortcuts.empty()) { std::wcout << L"No invalid shortcuts found." << std::endl; }
    else { std::wcout << L"Invalid shortcuts found (" << invalidShortcuts.size() << L"):" << std::endl; for (const auto& s : invalidShortcuts) std::wcout << L"  " << s << std::endl; }
    std::wcout << L"\nSearching for empty folders..." << std::endl;
    std::vector<std::wstring> emptyFolders = checker.findEmptyFolders(desktopPath);
    if (emptyFolders.empty()) { std::wcout << L"No empty folders found." << std::endl; }
    else { std::wcout << L"Empty folders found (" << emptyFolders.size() << L"):" << std::endl; for (const auto& f : emptyFolders) std::wcout << L"  " << f << std::endl; }
    std::vector<std::wstring> itemsToDelete; itemsToDelete.insert(itemsToDelete.end(), invalidShortcuts.begin(), invalidShortcuts.end()); itemsToDelete.insert(itemsToDelete.end(), emptyFolders.begin(), emptyFolders.end());
    if (!itemsToDelete.empty()) {
        std::wcout << L"\n--- Proposed Deletions ---" << std::endl;
        for (size_t i = 0; i < itemsToDelete.size(); ++i) std::wcout << L"  " << i+1 << L": " << itemsToDelete[i] << std::endl;
        std::wstring confirm; std::wcout << L"Are you sure you want to delete these items? (yes/no): ";
        std::getline(std::wcin, confirm); if (confirm.empty()) std::getline(std::wcin, confirm);
        if (confirm == L"yes" || confirm == L"YES") {
            bool useRecycleBin = ConfigManager::getInstance().shouldUseRecycleBin(); // Use singleton
            if (globalFileOpsInstance.deleteItems(itemsToDelete, useRecycleBin)) { /* success logs */ } else { /* error logs */ }
        } else { LOG_INFO(L"CLI: User cancelled deletion."); std::wcout << L"Deletion cancelled." << std::endl; }
    } else { std::wcout << L"\nNo items for deletion." << std::endl; }
    std::wcout << L"-----------------------------" << std::endl;
}
void displayDesktopInfo() { /* uses globalDesktopInfoInstance */
    LOG_INFO(L"CLI: User initiated Display Desktop Info operation.");
    std::wcout << L"\n--- Displaying Desktop Info ---" << std::endl;
    DesktopChecker checker; std::wstring desktopPath = checker.getDesktopPath();
    ScreenMetrics sm(globalDesktopInfoInstance.getScreenWidth(), globalDesktopInfoInstance.getScreenHeight(), globalDesktopInfoInstance.getPrimaryMonitorDpiScale()); // Use ScreenMetrics from LayoutManager.h
    std::wcout << L"Screen Width: " << sm.screenWidth << L"px, Height: " << sm.screenHeight << L"px, DPI Scale: " << std::fixed << std::setprecision(2) << sm.dpiScale * 100 << L"%" << std::endl;
    std::wcout << L"  Effective Icon WxH: " << sm.effectiveIconWidth << L"x" << sm.effectiveIconHeight << L", Cell WxH: " << sm.cellWidth << L"x" << sm.cellHeight << std::endl;
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);
    if (allItems.empty()) { std::wcout << L"No desktop items found." << std::endl; }
    else {
        std::wcout << L"Desktop Items (" << allItems.size() << L"):" << std::endl;
        for (const auto& item : allItems) {
            std::wcout << L"  Name: " << item.name.substr(0, 50) << (item.name.length() > 50 ? L"..." : L"") << std::endl;
            std::wcout << L"    Path: " << item.path.substr(0, 70) << (item.path.length() > 70 ? L"..." : L"") << std::endl;
            std::wcout << L"    Type: " << item_type_to_wstring(item.type) << L", Category: " << item_category_to_wstring_main(item.category) << std::endl; // Display category
            std::wcout << L"    Original Pos: (" << item.original_x << L"," << item.original_y << L")"
                       << L", Target Pos: (" << item.x << L"," << item.y << L")" << std::endl; // Display original and target
            std::wcout << L"    Last Modified: " << file_time_to_wstring(item.lastModified) << std::endl;
        }
    }
    std::wcout << L"-----------------------------" << std::endl;
}
void testBasicSort() { /* uses globalDesktopSorterInstance, globalDesktopInfoInstance */
    LOG_INFO(L"CLI: User initiated Test Basic Sort operation.");
    std::wcout << L"\n--- Testing Basic Sort (assigns category, then DesktopLayoutManager will sort) ---" << std::endl;
    DesktopChecker checker; std::wstring desktopPath = checker.getDesktopPath();
    if (desktopPath.empty()) { LOG_ERROR(L"CLI: Test Basic Sort - Desktop path is unknown."); std::wcerr << L"Desktop path is unknown." << std::endl; return; }
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);
    if (allItems.empty()) { std::wcout << L"No items to test sorting." << std::endl; return; }

    std::wcout << L"Classifying items before potential sort/layout test..." << std::endl;
    for (DesktopItem& item : allItems) { // Needs to be non-const to set category
        globalDesktopSorterInstance.classifyItem(item); // Sets item.category
        std::wcout << L"  Item: " << item.name.substr(0,30) << L"... Category: " << item_category_to_wstring_main(item.category) << std::endl;
    }
    // Note: Actual sorting and moving is now part of option 11 (Calculate Layout)
    std::wcout << L"Classification step complete. To see layout, use option 11." << std::endl;
    std::wcout << L"--------------------------" << std::endl;
}
void testWebSearchAndClassifyDirectly() { /* uses globalWebClassifierInstance */
    LOG_INFO(L"CLI: User initiated Direct WebClassifier Test operation.");
    std::wcout << L"\n--- Testing WebClassifier Directly ---" << std::endl;
    std::wstring itemName; std::wcout << L"Enter item name to search: ";
    std::getline(std::wcin, itemName); if (itemName.empty()) std::getline(std::wcin, itemName);
    if (itemName.empty()) { std::wcerr << L"Empty item name." << std::endl; return; }
    std::string htmlResponseOutput;
    DesktopItem testItem; testItem.name = itemName; testItem.path = L"C:\\DummyPath\\" + itemName; testItem.type = DesktopItem::ItemType::FILE; // Type might influence search
    ItemCategory category = globalWebClassifierInstance.performSearch(testItem, htmlResponseOutput);
    std::wcout << L"  Item: " << itemName << L", Deduced Category: " << item_category_to_wstring_main(category) << std::endl;
    if (ConfigManager::getInstance().isWebClassifierEnabled() && !htmlResponseOutput.empty()) { /* print snippet */ }
    std::wcout << L"----------------------------------------------------------" << std::endl;
}
void clearWebClassifierCacheCli() { /* uses globalWebClassifierInstance */
#ifdef _WIN32
    LOG_INFO(L"CLI: User initiated Clear WebClassifier Cache operation.");
    globalWebClassifierInstance.clearCache();
#else
    LOG_INFO(L"CLI: Clear WebClassifier Cache (No-op on non-Windows).");
    std::wcout << L"Clear WebClassifier Cache (Not applicable on non-Windows)" << std::endl;
#endif
}
void classifySpecificItem() { /* uses global instances */
    LOG_INFO(L"CLI: User initiated Classify Specific Item operation.");
    std::wcout << L"\n--- Classify Specific Item (via DesktopSorter) ---" << std::endl;
    DesktopChecker checker; std::wstring desktopPath = checker.getDesktopPath();
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);
    if (allItems.empty()) { /* ... */ return; }
    for (size_t i = 0; i < allItems.size(); ++i) std::wcout << L"  " << i + 1 << L": " << allItems[i].name << std::endl;
    std::wcout << L"Enter item number: "; std::wstring input; std::getline(std::wcin, input); if (input.empty()) std::getline(std::wcin, input);
    int itemIdx = -1; try { if(!input.empty()) {size_t n = std::stoul(input); if (n > 0 && n <= allItems.size()) itemIdx = n-1;} } catch(...) {}
    if (itemIdx == -1) { std::wcerr << L"Invalid." << std::endl; return; }
    DesktopItem& selectedItem = allItems[itemIdx]; // Needs to be non-const ref
    ItemCategory category = globalDesktopSorterInstance.classifyItem(selectedItem); // Sets selectedItem.category
    std::wcout << L"Item: " << selectedItem.name << L", Category: " << item_category_to_wstring_main(category) << std::endl;
    std::wcout << L"----------------------------------------------------" << std::endl;
}
void viewSettings() { /* uses ConfigManager::getInstance() */
    LOG_INFO(L"CLI: User initiated View Settings operation.");
    std::wcout << L"\n--- Current Application Settings ---" << std::endl;
    std::wcout << L"Web Classifier Enabled: " << (ConfigManager::getInstance().isWebClassifierEnabled() ? L"Yes" : L"No") << std::endl;
    std::wcout << L"Delete to Recycle Bin: " << (ConfigManager::getInstance().shouldUseRecycleBin() ? L"Yes" : L"No") << std::endl;
    std::wcout << L"Excluded Paths:" << std::endl;
    const auto& excluded = ConfigManager::getInstance().getExcludedPaths();
    if (excluded.empty()) { std::wcout << L"  (No excluded paths)" << std::endl; }
    else { for (const auto& path : excluded) std::wcout << L"  - " << path << std::endl; }
    std::wcout << L"------------------------------------" << std::endl;
}
void toggleWebClassifier() { /* uses ConfigManager::getInstance() */
    LOG_INFO(L"CLI: User initiated Toggle Web Classifier operation.");
    bool currentStatus = ConfigManager::getInstance().isWebClassifierEnabled();
    ConfigManager::getInstance().setWebClassifierEnabled(!currentStatus);
    std::wcout << L"Web Classifier status set to " << (ConfigManager::getInstance().isWebClassifierEnabled() ? L"Enabled" : L"Disabled") << L"." << std::endl;
}
void toggleRecycleBin() { /* uses ConfigManager::getInstance() */
    LOG_INFO(L"CLI: User initiated Toggle Recycle Bin operation.");
    bool currentStatus = ConfigManager::getInstance().shouldUseRecycleBin();
    ConfigManager::getInstance().setShouldUseRecycleBin(!currentStatus);
    std::wcout << L"Delete to Recycle Bin status set to " << (ConfigManager::getInstance().shouldUseRecycleBin() ? L"Yes" : L"No") << L"." << std::endl;
}
void manageExclusions() { /* uses ConfigManager::getInstance() */
    LOG_INFO(L"CLI: User initiated Manage Exclusions operation.");
    std::wstring subChoiceStr; int subChoice = -1;
    while(subChoice != 0) { /* ... sub-menu as before using ConfigManager::getInstance() ... */ }
}

// New function for CLI option 11
static void calculateAndDisplayLayout() {
    LOG_INFO(L"CLI: User initiated 'Calculate Proposed Desktop Layout'.");
    std::wcout << L"\n--- Calculating Proposed Desktop Layout ---" << std::endl;

    ScreenMetrics screenMetrics;
    screenMetrics.screenWidth = globalDesktopInfoInstance.getScreenWidth();
    screenMetrics.screenHeight = globalDesktopInfoInstance.getScreenHeight();
    screenMetrics.dpiScale = globalDesktopInfoInstance.getPrimaryMonitorDpiScale();

    // These could come from ConfigManager in the future
    screenMetrics.baseIconWidth = 72;
    screenMetrics.baseIconHeight = 72;
    screenMetrics.baseIconGapX = 20;
    screenMetrics.baseIconGapY = 20;
    screenMetrics.marginTop = 20;
    screenMetrics.marginBottom = 20;
    screenMetrics.marginLeft = 20;
    screenMetrics.marginRight = 20;
    screenMetrics.calculateEffectiveDimensions();

    LOG_INFO(L"Using Screen Metrics: Width=" + std::to_wstring(screenMetrics.screenWidth) +
             L", Height=" + std::to_wstring(screenMetrics.screenHeight) +
             L", DPI Scale=" + std::to_wstring(screenMetrics.dpiScale) +
             L", Cell W=" + std::to_wstring(screenMetrics.cellWidth) +
             L", Cell H=" + std::to_wstring(screenMetrics.cellHeight));

    DesktopChecker checker; // For path
    std::wstring desktopPath = checker.getDesktopPath();
    if (desktopPath.empty()) {
        LOG_ERROR(L"CLI: Calculate Layout - Failed to get desktop path.");
        std::wcout << L"Error: Could not retrieve desktop path." << std::endl;
        return;
    }
    std::vector<DesktopItem> items = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);
    LOG_INFO(L"CLI: Calculate Layout - Found " + std::to_wstring(items.size()) + L" desktop items.");
    if (items.empty()) {
        std::wcout << L"No desktop items found to arrange." << std::endl;
        return;
    }
    std::wcout << L"Found " << items.size() << L" items on the desktop." << std::endl;

    std::wcout << L"Classifying items..." << std::endl;
    for (DesktopItem& item : items) {
        globalDesktopSorterInstance.classifyItem(item);
    }
    LOG_INFO(L"CLI: Calculate Layout - Item classification complete.");
    std::wcout << L"Item classification complete." << std::endl;

    std::wcout << L"Calculating new layout..." << std::endl;
    // calculateLayout sorts items by date globally, then calculates new x,y for each item
    std::vector<DesktopItem> newLayout = globalLayoutManagerInstance.calculateLayout(items, screenMetrics);
    LOG_INFO(L"CLI: Calculate Layout - Layout calculation complete.");
    std::wcout << L"Layout calculation complete." << std::endl;

    std::wcout << L"\n--- Calculated Desktop Layout ---" << std::endl;
    std::wcout << std::left
               << std::setw(40) << L"Name"
               << std::setw(18) << L"Category"
               << std::setw(18) << L"Original Pos"
               << std::setw(18) << L"Target Pos" << std::endl;
    std::wcout << std::wstring(94, L'-') << std::endl;

    for (const auto& item : newLayout) { // Use newLayout which is a copy of items with updated coords
        std::wstring originalPos = L"(" + std::to_wstring(item.original_x) + L"," + std::to_wstring(item.original_y) + L")";
        std::wstring targetPos;
        if (item.x == -1 && item.y == -1) {
            targetPos = L"UNPLACED";
        } else {
            targetPos = L"(" + std::to_wstring(item.x) + L"," + std::to_wstring(item.y) + L")";
        }
        std::wcout << std::left
                   << std::setw(40) << item.name.substr(0, 38)
                   << std::setw(18) << item_category_to_wstring_main(item.category)
                   << std::setw(18) << originalPos
                   << std::setw(18) << targetPos << std::endl;
    }
    std::wcout << std::wstring(94, L'-') << std::endl;
    LOG_INFO(L"CLI: Calculate Layout - Proposed layout displayed to user.");
}


int main() {
    Logger::getInstance().logInfo(L"SmartDesktopOrganizer CLI session started.");
    ConfigManager::getInstance();
    if (ConfigManager::getInstance().saveSettings()) {
         LOG_INFO(L"Initial settings (defaults or loaded) checked/saved to disk.");
    } else {
         LOG_ERROR(L"Failed to save initial settings to disk (path issue or permissions?).");
    }
    try { std::locale::global(std::locale("")); }
    catch (const std::runtime_error& e) { LOG_ERROR(L"Critical: Failed to set global locale: " + std::wstring(e.what(), e.what() + strlen(e.what()))); }
    std::wcout.imbue(std::locale("")); std::wcerr.imbue(std::locale("")); std::wcin.imbue(std::locale(""));

    std::wcout << L"Welcome to Smart Desktop Organizer - CLI Tester" << std::endl;
    int choice = -1;
    while (choice != 0) {
        std::wcout << L"\nPlease select an option:" << std::endl;
        std::wcout << L"  1: Run Desktop Check" << std::endl;
        std::wcout << L"  2: Display Desktop Info" << std::endl;
        std::wcout << L"  3: Test Basic Sort (Classify items via DesktopSorter)" << std::endl;
        std::wcout << L"  4: Test WebClassifier Directly" << std::endl;
        std::wcout << L"  5: Clear WebClassifier Cache" << std::endl;
        std::wcout << L"  6: Classify Specific Item (via DesktopSorter)" << std::endl;
        std::wcout << L"  7: View Settings" << std::endl;
        std::wcout << L"  8: Toggle Web Classifier Enabled" << std::endl;
        std::wcout << L"  9: Toggle Delete to Recycle Bin" << std::endl;
        std::wcout << L" 10: Manage Excluded Paths" << std::endl;
        std::wcout << L" 11: Calculate Proposed Desktop Layout" << std::endl; // New option
        std::wcout << L"  0: Exit" << std::endl;
        std::wcout << L"Enter your choice: ";

        std::wstring inputLine;
        if (!std::getline(std::wcin, inputLine)) { if (std::wcin.eof()){ LOG_INFO(L"EOF reached. Exiting."); break; } continue; }
        try {
            if (!inputLine.empty() && inputLine.find_first_not_of(L"0123456789") == std::wstring::npos) {
                 choice = std::stoi(inputLine);
            } else if (!inputLine.empty()) { choice = -1; } else { choice = -1;}
        } catch (const std::exception&) { choice = -1; }

        switch (choice) {
            case 1: runDesktopCheck(); break;
            case 2: displayDesktopInfo(); break;
            case 3: testBasicSort(); break;
            case 4: testWebSearchAndClassifyDirectly(); break;
            case 5: clearWebClassifierCacheCli(); break;
            case 6: classifySpecificItem(); break;
            case 7: viewSettings(); break;
            case 8: toggleWebClassifier(); break;
            case 9: toggleRecycleBin(); break;
            case 10: manageExclusions(); break;
            case 11: calculateAndDisplayLayout(); break; // New case
            case 0: LOG_INFO(L"CLI: User selected Exit."); std::wcout << L"Exiting program." << std::endl; break;
            default:
                if (!inputLine.empty()){ LOG_WARNING(L"CLI: Invalid menu option: " + inputLine); std::wcerr << L"Invalid option." << std::endl; }
                else { std::wcout << L"Please enter a choice." << std::endl; }
                break;
        }
    }
    Logger::getInstance().logInfo(L"SmartDesktopOrganizer CLI session ended.");
    return 0;
}
