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

#ifdef _WIN32
// No specific includes needed here for main
#endif

// --- Global Instances for CLI Testing ---
// Logger and ConfigManager are singletons, accessed via getInstance().
DesktopInfo globalDesktopInfoInstance;
WebClassifier globalWebClassifierInstance;  // For option 4 (direct test) and 5 (clear cache)
DesktopSorter globalDesktopSorterInstance;  // For options 3 and 6
DesktopFileOperations globalFileOpsInstance;


std::wstring file_time_to_wstring(const std::filesystem::file_time_type& ftime) {
    if (ftime == std::filesystem::file_time_type::min()) return L"N/A";
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm t_local;
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
std::wstring item_category_to_wstring(ItemCategory category) { /* ... */
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

void runDesktopCheck() {
    LOG_INFO(L"CLI: User initiated Desktop Check operation.");
    std::wcout << L"\n--- Running Desktop Check ---" << std::endl;
    DesktopChecker checker;
    std::wstring desktopPath = checker.getDesktopPath();
    if (desktopPath.empty()) { /* ... */ return; }
    std::wcout << L"Desktop Path: " << desktopPath << std::endl;

    std::wcout << L"\nSearching for invalid shortcuts..." << std::endl;
    std::vector<std::wstring> invalidShortcuts = checker.findInvalidShortcuts(desktopPath);
    /* ... print invalidShortcuts ... */

    std::wcout << L"\nSearching for empty folders..." << std::endl;
    std::vector<std::wstring> emptyFolders = checker.findEmptyFolders(desktopPath);
    /* ... print emptyFolders ... */

    std::vector<std::wstring> itemsToDelete;
    itemsToDelete.insert(itemsToDelete.end(), invalidShortcuts.begin(), invalidShortcuts.end());
    itemsToDelete.insert(itemsToDelete.end(), emptyFolders.begin(), emptyFolders.end());

    if (!itemsToDelete.empty()) {
        // ... (print items proposed for deletion) ...
        std::wstring confirm;
        std::wcout << L"Are you sure you want to delete these items? (yes/no): ";
        std::getline(std::wcin, confirm);
        if (confirm.empty()) std::getline(std::wcin, confirm);

        if (confirm == L"yes" || confirm == L"YES") {
            LOG_INFO(L"CLI: User confirmed deletion of " + std::to_wstring(itemsToDelete.size()) + L" items.");
            bool useRecycleBin = ConfigManager::getInstance().shouldUseRecycleBin(); // Use Singleton
            std::wcout << L"Using Recycle Bin: " << (useRecycleBin ? L"Yes" : L"No") << std::endl;

            if (globalFileOpsInstance.deleteItems(itemsToDelete, useRecycleBin)) { /* ... */ }
            else { /* ... */ }
        } else { /* ... */ }
    } else { /* ... */ }
    std::wcout << L"-----------------------------" << std::endl;
}

void displayDesktopInfo() { /* uses globalDesktopInfoInstance, no ConfigManager here */
    LOG_INFO(L"CLI: User initiated Display Desktop Info operation.");
    std::wcout << L"\n--- Displaying Desktop Info ---" << std::endl;
    DesktopChecker checker;
    std::wstring desktopPath = checker.getDesktopPath();
    std::wcout << L"Screen Width: " << globalDesktopInfoInstance.getScreenWidth() << L"px" << std::endl;
    std::wcout << L"Screen Height: " << globalDesktopInfoInstance.getScreenHeight() << L"px" << std::endl;
    std::wcout << L"Primary Monitor DPI Scale: " << std::fixed << std::setprecision(2) << globalDesktopInfoInstance.getPrimaryMonitorDpiScale() * 100 << L"%" << std::endl;
    std::wcout << L"\nGathering all desktop items from: " << (desktopPath.empty() ? L"[Path Not Found/Applicable]" : desktopPath) << std::endl;
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);
    if (allItems.empty()) { std::wcout << L"No desktop items found or failed to retrieve items." << std::endl;
    } else {
        std::wcout << L"Desktop Items (" << allItems.size() << L"):" << std::endl;
        for (const auto& item : allItems) {
            std::wcout << L"  Name: " << item.name << std::endl;
            std::wcout << L"    Path: " << item.path << std::endl;
            std::wcout << L"    Type: " << item_type_to_wstring(item.type) << std::endl;
            std::wcout << L"    Coords: (" << item.x << L", " << item.y << L")" << std::endl;
            std::wcout << L"    Last Modified: " << file_time_to_wstring(item.lastModified) << std::endl;
        }
    }
    std::wcout << L"-----------------------------" << std::endl;
}
void testBasicSort() { /* uses globalDesktopSorterInstance, no direct ConfigManager here */
    LOG_INFO(L"CLI: User initiated Test Basic Sort operation.");
    std::wcout << L"\n--- Testing Basic Sort (includes Web Classification via DesktopSorter) ---" << std::endl;
    DesktopChecker checker;
    std::wstring desktopPath = checker.getDesktopPath();
    if (desktopPath.empty() && std::wstring(L"").length() > 0) { /* ... */ return; }
    std::wcout << L"Gathering items from: " << (desktopPath.empty() ? L"[Path Not Found/Applicable]" : desktopPath) << std::endl;
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);
    if (allItems.empty()) { /* ... */ }
    else { /* ... */
        const int itemsToTest = std::min((int)allItems.size(), 2);
        for (int i = 0; i < itemsToTest; ++i) {
            const auto& item = allItems[i];
            std::wcout << L"\nTesting item: " << item.name << std::endl;
            ItemCategory category = globalDesktopSorterInstance.classifyItem(item);
            std::wcout << L"  Classified (DesktopSorter): " << item_category_to_wstring(category) << std::endl;
            if (item.x != -1 && item.y != -1) { /* ... */ globalDesktopSorterInstance.setItemPosition(item.name, item.x + 60, item.y + 60); }
            else { /* ... */ }
        }
        // ... user guidance
    }
    std::wcout << L"--------------------------" << std::endl;
}
void testWebSearchAndClassifyDirectly() { /* uses globalWebClassifierInstance, no direct ConfigManager here */
    LOG_INFO(L"CLI: User initiated Direct WebClassifier Test operation.");
    std::wcout << L"\n--- Testing WebClassifier Directly (Search & HTML Classification with Cache) ---" << std::endl;
    std::wstring itemName;
    std::wcout << L"Enter an item name to search and classify (e.g., Notepad++, Firefox, MyGame.exe): ";
    std::getline(std::wcin, itemName);
    if (itemName.empty()) std::getline(std::wcin, itemName);
    if (itemName.empty()) { /* ... */ return; }
    std::string htmlResponseOutput;
    DesktopItem testItem; testItem.name = itemName; testItem.path = L"C:\\DummyPath\\" + itemName; testItem.type = DesktopItem::ItemType::FILE;
    ItemCategory category = globalWebClassifierInstance.performSearch(testItem, htmlResponseOutput);
    std::wcout << L"Web search and HTML classification complete." << std::endl;
    std::wcout << L"  Item: " << itemName << L"  Deduced Category: " << item_category_to_wstring(category) << std::endl;
    /* ... print snippet ... */
    std::wcout << L"----------------------------------------------------------" << std::endl;
}
void clearWebClassifierCacheCli() { /* uses globalWebClassifierInstance */
#ifdef _WIN32
    LOG_INFO(L"CLI: User initiated Clear WebClassifier Cache operation.");
    std::wcout << L"\n--- Clearing Web Classifier Cache ---" << std::endl;
    globalWebClassifierInstance.clearCache();
#else
    std::wcout << L"\n--- Clearing Web Classifier Cache (Not applicable on non-Windows) ---" << std::endl;
#endif
    std::wcout << L"-----------------------------------" << std::endl;
}
void classifySpecificItem() { /* uses global instances, no direct ConfigManager here */
    LOG_INFO(L"CLI: User initiated Classify Specific Item operation.");
    std::wcout << L"\n--- Classify Specific Item (via DesktopSorter) ---" << std::endl;
    DesktopChecker checker;
    std::wstring desktopPath = checker.getDesktopPath();
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);
    if (allItems.empty()) { /* ... */ return; }
    // ... list items ...
    for (size_t i = 0; i < allItems.size(); ++i) {
        std::wcout << L"  " << i + 1 << L": " << allItems[i].name << L" (Path: " << allItems[i].path << L")" << std::endl;
    }
    std::wcout << L"Enter the number of the item to classify: ";
    std::wstring itemNumberInput;
    std::getline(std::wcin, itemNumberInput);
    if (itemNumberInput.empty()) std::getline(std::wcin, itemNumberInput);
    // ... (parse input) ...
    int itemIndex = -1;
    try {
        if (!itemNumberInput.empty()) {
            size_t tempNum = std::stoul(itemNumberInput);
            if (tempNum > 0 && tempNum <= allItems.size()) itemIndex = static_cast<int>(tempNum - 1);
        }
    } catch (const std::exception&) { itemIndex = -1; }
    if (itemIndex == -1) { /* ... */ return; }
    const DesktopItem& selectedItem = allItems[itemIndex];
    ItemCategory category = globalDesktopSorterInstance.classifyItem(selectedItem);
    std::wcout << L"  Deduced Category (DesktopSorter): " << item_category_to_wstring(category) << std::endl;
    std::wcout << L"----------------------------------------------------" << std::endl;
}

// --- New CLI options for ConfigManager ---
void viewSettings() {
    LOG_INFO(L"CLI: User initiated View Settings operation.");
    std::wcout << L"\n--- Current Application Settings ---" << std::endl;
    std::wcout << L"Web Classifier Enabled: " << (ConfigManager::getInstance().isWebClassifierEnabled() ? L"Yes" : L"No") << std::endl;
    std::wcout << L"Delete to Recycle Bin: " << (ConfigManager::getInstance().shouldUseRecycleBin() ? L"Yes" : L"No") << std::endl;
    std::wcout << L"Excluded Paths:" << std::endl;
    const auto& excluded = ConfigManager::getInstance().getExcludedPaths();
    if (excluded.empty()) {
        std::wcout << L"  (No excluded paths)" << std::endl;
    } else {
        for (const auto& path : excluded) {
            std::wcout << L"  - " << path << std::endl;
        }
    }
    std::wcout << L"------------------------------------" << std::endl;
}

void toggleWebClassifier() {
    LOG_INFO(L"CLI: User initiated Toggle Web Classifier operation.");
    bool currentStatus = ConfigManager::getInstance().isWebClassifierEnabled();
    ConfigManager::getInstance().setWebClassifierEnabled(!currentStatus); // This will also save
    bool newStatus = ConfigManager::getInstance().isWebClassifierEnabled();
    std::wcout << L"Web Classifier status changed from " << (currentStatus ? L"Enabled" : L"Disabled")
               << L" to " << (newStatus ? L"Enabled" : L"Disabled") << L"." << std::endl;
    LOG_INFO(L"Web Classifier status set to: " + std::wstring(newStatus ? L"Enabled" : L"Disabled"));
}

void toggleRecycleBin() {
    LOG_INFO(L"CLI: User initiated Toggle Recycle Bin operation.");
    bool currentStatus = ConfigManager::getInstance().shouldUseRecycleBin();
    ConfigManager::getInstance().setShouldUseRecycleBin(!currentStatus); // This will also save
    bool newStatus = ConfigManager::getInstance().shouldUseRecycleBin();
    std::wcout << L"Delete to Recycle Bin status changed from " << (currentStatus ? L"Yes" : L"No")
               << L" to " << (newStatus ? L"Yes" : L"No") << L"." << std::endl;
    LOG_INFO(L"Delete to Recycle Bin status set to: " + std::wstring(newStatus ? L"Yes" : L"No"));
}

void manageExclusions() {
    LOG_INFO(L"CLI: User initiated Manage Exclusions operation.");
    std::wstring subChoiceStr; int subChoice = -1;
    while(subChoice != 0) {
        // ... (menu as before) ...
        std::wcout << L"\n--- Manage Exclusions ---" << std::endl;
        std::wcout << L"  1: List Excluded Paths" << std::endl;
        std::wcout << L"  2: Add Exclusion Path" << std::endl;
        std::wcout << L"  3: Remove Exclusion Path" << std::endl;
        std::wcout << L"  0: Back to Main Menu" << std::endl;
        std::wcout << L"Enter your choice: ";
        std::getline(std::wcin, subChoiceStr);
        if (subChoiceStr.empty() && subChoice != -1) std::getline(std::wcin, subChoiceStr);
        try {
            if (!subChoiceStr.empty() && subChoiceStr.find_first_not_of(L"0123") == std::wstring::npos) {
                 subChoice = std::stoi(subChoiceStr);
            } else { subChoice = -1; }
        } catch (const std::exception&) { subChoice = -1; }

        switch (subChoice) {
            case 1: {
                const auto& paths = ConfigManager::getInstance().getExcludedPaths(); // Use Singleton
                /* ... print paths ... */
                break;
            }
            case 2: {
                std::wcout << L"Enter path to exclude: ";
                std::wstring pathToAdd;
                std::getline(std::wcin, pathToAdd);
                if (pathToAdd.empty()) std::getline(std::wcin, pathToAdd);
                if (!pathToAdd.empty()) {
                    ConfigManager::getInstance().addExcludedPath(pathToAdd); // Use Singleton
                    LOG_INFO(L"CLI: Added exclusion: " + pathToAdd);
                    std::wcout << L"Path '" << pathToAdd << L"' added." << std::endl;
                } else { std::wcout << L"Empty path not added." << std::endl;}
                break;
            }
            case 3: {
                const auto& paths = ConfigManager::getInstance().getExcludedPaths(); // Use Singleton
                if (paths.empty()) { /* ... */ break; }
                // ... (list paths for removal) ...
                std::wstring numStr;
                std::getline(std::wcin, numStr);
                if (numStr.empty()) std::getline(std::wcin, numStr);
                int pathIdx = -1;
                try { /* ... parse numStr ... */
                     if (!numStr.empty()) {
                        size_t tempNum = std::stoul(numStr);
                        if (tempNum > 0 && tempNum <= paths.size()) pathIdx = static_cast<int>(tempNum - 1);
                    }
                } catch (const std::exception&) {}
                if (pathIdx != -1) {
                    std::wstring removedPath = paths[pathIdx]; // Careful with concurrent modification if list changes
                    ConfigManager::getInstance().removeExcludedPath(removedPath); // Use Singleton
                    LOG_INFO(L"CLI: Removed exclusion: " + removedPath);
                    std::wcout << L"Path '" << removedPath << L"' removed." << std::endl;
                } else if (numStr != L"0") {std::wcout << L"Invalid selection." << std::endl;}
                break;
            }
            // ...
        }
    }
}

int main() {
    Logger::getInstance().logInfo(L"SmartDesktopOrganizer CLI session started.");
    ConfigManager::getInstance(); // Ensure singleton is initialized, loads settings
    if (ConfigManager::getInstance().saveSettings()) { // Save to ensure file exists with defaults if new
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
        // ... (Menu print as before, with option 10 for Manage Exclusions) ...
        std::wcout << L"\nPlease select an option:" << std::endl;
        std::wcout << L"  1: Run Desktop Check" << std::endl;
        std::wcout << L"  2: Display Desktop Info" << std::endl;
        std::wcout << L"  3: Test Basic Sort (via DesktopSorter)" << std::endl;
        std::wcout << L"  4: Test WebClassifier Directly" << std::endl;
        std::wcout << L"  5: Clear WebClassifier Cache" << std::endl;
        std::wcout << L"  6: Classify Specific Item (via DesktopSorter)" << std::endl;
        std::wcout << L"  --- Settings ---" << std::endl;
        std::wcout << L"  7: View Settings" << std::endl;
        std::wcout << L"  8: Toggle Web Classifier Enabled" << std::endl;
        std::wcout << L"  9: Toggle Delete to Recycle Bin" << std::endl;
        std::wcout << L" 10: Manage Excluded Paths" << std::endl;
        std::wcout << L"  0: Exit" << std::endl;
        std::wcout << L"Enter your choice: ";
        std::wstring inputLine;
        if (!std::getline(std::wcin, inputLine)) { /* ... EOF or error ... */ break; }
        try { /* ... parse choice ... */
            if (!inputLine.empty() && inputLine.find_first_not_of(L"0123456789") == std::wstring::npos) {
                 choice = std::stoi(inputLine);
            } else if (!inputLine.empty()) { choice = -1; }
            else { choice = -1; }
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
            case 0: LOG_INFO(L"CLI: User selected Exit."); std::wcout << L"Exiting program." << std::endl; break;
            default: /* ... invalid option ... */
                if (!inputLine.empty()){
                   LOG_WARNING(L"CLI: Invalid menu option selected by user: " + inputLine);
                   std::wcerr << L"Invalid option selected. Please try again." << std::endl;
                } else {
                    std::wcout << L"Please enter a choice." << std::endl;
                }
                break;
        }
    }
    Logger::getInstance().logInfo(L"SmartDesktopOrganizer CLI session ended.");
    return 0;
}
