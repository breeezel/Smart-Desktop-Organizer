#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <limits>
#include <codecvt>

#include "DesktopManager/DesktopChecker.h"
#include "DesktopManager/DesktopInfo.h"
#include "DesktopManager/DesktopSorter.h"
#include "WebClassifier/WebClassifier.h"

#ifdef _WIN32
// No specific includes needed here for main
#endif

// --- Global Instances for CLI Testing ---
DesktopInfo globalDesktopInfoInstance;    // For option 6 and potentially others
WebClassifier globalWebClassifierInstance;  // For options 4, 5, and implicitly by DesktopSorter
DesktopSorter globalDesktopSorterInstance;  // For options 3 and 6

// --- Helper Functions for Printing (unchanged) ---
std::wstring file_time_to_wstring(const std::filesystem::file_time_type& ftime) {
    if (ftime == std::filesystem::file_time_type::min()) {
        return L"N/A";
    }
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm t_local;
#ifdef _WIN32
    errno_t err = localtime_s(&t_local, &tt);
    if (err != 0) return L"Error converting time (localtime_s)";
#else
    if (localtime_r(&tt, &t_local) == nullptr) return L"Error converting time (localtime_r)";
#endif
    std::wstringstream wss;
    wss << std::put_time(&t_local, L"%Y-%m-%d %H:%M:%S");
    if (wss.fail()) return L"Error formatting time";
    return wss.str();
}

std::wstring item_type_to_wstring(DesktopItem::ItemType type) {
    switch (type) {
        case DesktopItem::ItemType::FILE: return L"File";
        case DesktopItem::ItemType::FOLDER: return L"Folder";
        case DesktopItem::ItemType::SHORTCUT: return L"Shortcut";
        case DesktopItem::ItemType::OTHER: return L"Other";
        default: return L"Unknown";
    }
}

std::wstring item_category_to_wstring(ItemCategory category) {
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
void runDesktopCheck() {
    std::wcout << L"\n--- Running Desktop Check ---" << std::endl;
    DesktopChecker checker; // Local instance is fine
    std::wstring desktopPath = checker.getDesktopPath();
    if (desktopPath.empty()) {
        std::wcerr << L"Failed to get desktop path." << std::endl;
        return;
    }
    std::wcout << L"Desktop Path: " << desktopPath << std::endl;
    std::wcout << L"\nSearching for invalid shortcuts..." << std::endl;
    std::vector<std::wstring> invalidShortcuts = checker.findInvalidShortcuts(desktopPath);
    if (invalidShortcuts.empty()) {
        std::wcout << L"No invalid shortcuts found." << std::endl;
    } else {
        std::wcout << L"Invalid shortcuts found (" << invalidShortcuts.size() << L"):" << std::endl;
        for (const auto& shortcut : invalidShortcuts) {
            std::wcout << L"  " << shortcut << std::endl;
        }
    }
    std::wcout << L"\nSearching for empty folders..." << std::endl;
    std::vector<std::wstring> emptyFolders = checker.findEmptyFolders(desktopPath);
    if (emptyFolders.empty()) {
        std::wcout << L"No empty folders found." << std::endl;
    } else {
        std::wcout << L"Empty folders found (" << emptyFolders.size() << L"):" << std::endl;
        for (const auto& folder : emptyFolders) {
            std::wcout << L"  " << folder << std::endl;
        }
    }
    std::wcout << L"-----------------------------" << std::endl;
}

void displayDesktopInfo() {
    std::wcout << L"\n--- Displaying Desktop Info ---" << std::endl;
    // Use global instance or local, for now global is fine for consistency
    DesktopChecker checker; // Path is needed
    std::wstring desktopPath = checker.getDesktopPath();
    std::wcout << L"Screen Width: " << globalDesktopInfoInstance.getScreenWidth() << L"px" << std::endl;
    std::wcout << L"Screen Height: " << globalDesktopInfoInstance.getScreenHeight() << L"px" << std::endl;
    std::wcout << L"Primary Monitor DPI Scale: " << std::fixed << std::setprecision(2) << globalDesktopInfoInstance.getPrimaryMonitorDpiScale() * 100 << L"%" << std::endl;

    std::wcout << L"\nGathering all desktop items from: " << (desktopPath.empty() ? L"[Path Not Found/Applicable]" : desktopPath) << std::endl;
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);
    if (allItems.empty()) {
        std::wcout << L"No desktop items found or failed to retrieve items." << std::endl;
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

void testBasicSort() {
    std::wcout << L"\n--- Testing Basic Sort (includes Web Classification via DesktopSorter) ---" << std::endl;
    DesktopChecker checker;
    std::wstring desktopPath = checker.getDesktopPath();
    if (desktopPath.empty() && std::wstring(L"").length() > 0) {
        std::wcerr << L"Desktop path is unknown. Cannot perform sort test." << std::endl;
        return;
    }
    std::wcout << L"Gathering items from: " << (desktopPath.empty() ? L"[Path Not Found/Applicable]" : desktopPath) << std::endl;
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath); // Use global
    if (allItems.empty()) {
        std::wcout << L"No items found on the desktop to test sorting/moving." << std::endl;
    } else {
        std::wcout << L"Found " << allItems.size() << L" items. Testing classification and move for first few..." << std::endl;
        const int itemsToTest = std::min((int)allItems.size(), 2);
        for (int i = 0; i < itemsToTest; ++i) {
            const auto& item = allItems[i];
            std::wcout << L"\nTesting item: " << item.name << std::endl;
            std::wcout << L"  Current Coords: (" << item.x << L", " << item.y << L")" << std::endl;
            ItemCategory category = globalDesktopSorterInstance.classifyItem(item); // Use global
            std::wcout << L"  Classified (DesktopSorter): " << item_category_to_wstring(category) << std::endl;
            if (item.x != -1 && item.y != -1) {
                int newX = item.x + 60;
                int newY = item.y + 60;
                std::wcout << L"  Attempting to move to: (" << newX << L", " << newY << L")" << std::endl;
                globalDesktopSorterInstance.setItemPosition(item.name, newX, newY); // Use global
            } else {
                std::wcout << L"  Skipping move for this item as current coordinates are not available." << std::endl;
            }
        }
        std::wcout << L"\nUSER ACTION (for Windows):" << std::endl;
        std::wcout << L"  Please observe your desktop. Did the tested items move?" << std::endl;
        std::wcout << L"  Note: 'Auto Arrange Icons' or 'Align Icons to Grid' settings might prevent or alter the move." << std::endl;
    }
    std::wcout << L"--------------------------" << std::endl;
}

void testWebSearchAndClassifyDirectly() {
    std::wcout << L"\n--- Testing WebClassifier Directly (Search & HTML Classification with Cache) ---" << std::endl;
    std::wstring itemName;
    std::wcout << L"Enter an item name to search and classify (e.g., Notepad++, Firefox, MyGame.exe): ";
    std::getline(std::wcin, itemName); // Read full line after menu choice
    if (itemName.empty()) {
        // Try reading again if first getline was consumed by menu newline
        std::getline(std::wcin, itemName);
    }
    if (itemName.empty()) {
        std::wcerr << L"Invalid input or empty item name." << std::endl;
        return;
    }

    std::string htmlResponseOutput;
    std::wcout << L"Performing web search for: " << itemName << std::endl;

    // Create a dummy DesktopItem for the test
    DesktopItem testItem;
    testItem.name = itemName;
    testItem.path = L"C:\\Users\\Default\\Desktop\\" + itemName; // Example path
    testItem.type = DesktopItem::ItemType::FILE; // Assume file for direct test
    // Other DesktopItem fields (x,y,lastModified) are not strictly needed for this specific WebClassifier test call

    ItemCategory category = globalWebClassifierInstance.performSearch(testItem, htmlResponseOutput);

    std::wcout << L"Web search and HTML classification complete." << std::endl;
    std::wcout << L"  Item: " << itemName << std::endl;
    std::wcout << L"  Deduced Category: " << item_category_to_wstring(category) << std::endl;

    if (!htmlResponseOutput.empty()) {
        std::wcout << L"HTML Response Snippet (first 100 chars for debugging):" << std::endl;
        std::wstring wideResponseSnippet;
        try {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter_utf8;
            wideResponseSnippet = converter_utf8.from_bytes(htmlResponseOutput.substr(0, 100));
        } catch (const std::exception&) {
            wideResponseSnippet.reserve(100);
            for(size_t i=0; i < htmlResponseOutput.length() && i < 100; ++i) {
                wideResponseSnippet += static_cast<wchar_t>(static_cast<unsigned char>(htmlResponseOutput[i]));
            }
        }
        std::wcout << wideResponseSnippet << (htmlResponseOutput.length() > 100 ? L"..." : L"") << std::endl;
    } else {
        std::wcout << L"  (HTML response was empty or search failed/was cached and empty)" << std::endl;
    }
    std::wcout << L"----------------------------------------------------------" << std::endl;
}

void clearWebClassifierCache() {
#ifdef _WIN32
    std::wcout << L"\n--- Clearing Web Classifier Cache ---" << std::endl;
    globalWebClassifierInstance.clearCache();
    std::wcout << L"Web classifier cache has been cleared." << std::endl;
#else
    std::wcout << L"\n--- Clearing Web Classifier Cache (Not applicable on non-Windows) ---" << std::endl;
#endif
    std::wcout << L"-----------------------------------" << std::endl;
}

void classifySpecificItem() {
    std::wcout << L"\n--- Classify Specific Item (via DesktopSorter) ---" << std::endl;
    DesktopChecker checker; // Path is needed
    std::wstring desktopPath = checker.getDesktopPath();
    std::vector<DesktopItem> allItems = globalDesktopInfoInstance.getAllDesktopItems(desktopPath);

    if (allItems.empty()) {
        std::wcout << L"No desktop items found to classify." << std::endl;
        return;
    }

    std::wcout << L"Available desktop items:" << std::endl;
    for (size_t i = 0; i < allItems.size(); ++i) {
        std::wcout << L"  " << i + 1 << L": " << allItems[i].name
                   << L" (Path: " << allItems[i].path << L")" << std::endl;
    }

    std::wcout << L"Enter the number of the item to classify: ";
    std::wstring itemNumberInput;
    std::getline(std::wcin, itemNumberInput); // Read full line after menu choice
    if (itemNumberInput.empty()) {
        std::getline(std::wcin, itemNumberInput); // Try again
    }

    int itemIndex = -1;
    size_t itemNumber = 0;
    try {
        if (!itemNumberInput.empty()) {
            itemNumber = std::stoul(itemNumberInput); // Use stoul for size_t
            if (itemNumber > 0 && itemNumber <= allItems.size()) {
                itemIndex = static_cast<int>(itemNumber - 1);
            }
        }
    } catch (const std::exception& e) {
        // Invalid input, itemIndex remains -1
    }

    if (itemIndex == -1) {
        std::wcerr << L"Invalid item number selected." << std::endl;
        return;
    }

    const DesktopItem& selectedItem = allItems[itemIndex];
    std::wcout << L"\nClassifying selected item: " << selectedItem.name << std::endl;

    ItemCategory category = globalDesktopSorterInstance.classifyItem(selectedItem);

    std::wcout << L"  Original Name: " << selectedItem.name << std::endl;
    std::wcout << L"  Original Path: " << selectedItem.path << std::endl;
    std::wcout << L"  Deduced Category (DesktopSorter): " << item_category_to_wstring(category) << std::endl;
    std::wcout << L"----------------------------------------------------" << std::endl;
}


int main() {
    try {
        std::locale::global(std::locale(""));
    } catch (const std::runtime_error& e) {
        std::wcerr << L"Critical: Failed to set global locale: " << e.what() << std::endl;
    }
    std::wcout.imbue(std::locale(""));
    std::wcerr.imbue(std::locale(""));
    std::wcin.imbue(std::locale(""));

    std::wcout << L"Welcome to Smart Desktop Organizer - CLI Tester" << std::endl;

    int choice = -1;
    while (choice != 0) {
        std::wcout << L"\nPlease select an option:" << std::endl;
        std::wcout << L"  1: Run Desktop Check (Invalid shortcuts, empty folders)" << std::endl;
        std::wcout << L"  2: Display Desktop Info (Screen details, list items)" << std::endl;
        std::wcout << L"  3: Test Basic Sort (Classify and move first few items via DesktopSorter)" << std::endl;
        std::wcout << L"  4: Test WebClassifier Directly (Search & Classify item type)" << std::endl;
        std::wcout << L"  5: Clear Web Classifier Cache" << std::endl;
        std::wcout << L"  6: Classify Specific Item (via DesktopSorter)" << std::endl; // New option
        std::wcout << L"  0: Exit" << std::endl;
        std::wcout << L"Enter your choice: ";

        std::wstring inputLine;
        // std::wcin.clear(); // Not always needed here if getline is robust
        // std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer before getline

        if (!std::getline(std::wcin, inputLine)) {
             if (std::wcin.eof()){
                std::wcout << L"EOF reached. Exiting." << std::endl;
                break;
             }
            continue;
        }

        try {
            if (!inputLine.empty() && inputLine.find_first_not_of(L"0123456789") == std::wstring::npos) {
                 choice = std::stoi(inputLine);
            } else if (!inputLine.empty()) {
                choice = -1;
            } else {
                 choice = -1;
            }
        } catch (const std::invalid_argument& ia) {
            choice = -1;
        } catch (const std::out_of_range& oor) {
            choice = -1;
        }

        switch (choice) {
            case 1: runDesktopCheck(); break;
            case 2: displayDesktopInfo(); break;
            case 3: testBasicSort(); break;
            case 4: testWebSearchAndClassifyDirectly(); break; // Renamed to avoid conflict
            case 5: clearWebClassifierCache(); break;
            case 6: classifySpecificItem(); break; // New case
            case 0: std::wcout << L"Exiting program." << std::endl; break;
            default:
                if (!inputLine.empty()){
                   std::wcerr << L"Invalid option selected. Please try again." << std::endl;
                } else {
                    std::wcout << L"Please enter a choice." << std::endl;
                }
                break;
        }
    }
    return 0;
}
