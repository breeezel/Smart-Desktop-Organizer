#include <iostream>
#include <vector>
#include <string>
#include <locale>    // Required for std::locale
#include <iomanip>   // For std::put_time, std::fixed, std::setprecision
#include <sstream>   // For string stream conversion of time
#include <chrono>    // For time conversions
#include <limits>    // For std::numeric_limits

#include "DesktopManager/DesktopChecker.h"
#include "DesktopManager/DesktopInfo.h"
#include "DesktopManager/DesktopSorter.h"

#ifdef _WIN32
#include <windows.h> // For CoInitialize, CoUninitialize
#include <Objbase.h> // For CoInitialize, CoUninitialize (alternative)
#endif

// --- Helper Functions for Printing ---
std::wstring file_time_to_wstring(const std::filesystem::file_time_type& ftime) {
    if (ftime == std::filesystem::file_time_type::min()) {
        return L"N/A";
    }
    // Convert file_time_type to system_clock::time_point
    // This conversion logic can be tricky due to epoch differences.
    // A common approach:
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);

    std::tm t_local;
#ifdef _WIN32
    errno_t err = localtime_s(&t_local, &tt);
    if (err != 0) {
        return L"Error converting time (localtime_s)";
    }
#else
    if (localtime_r(&tt, &t_local) == nullptr) { // POSIX
         return L"Error converting time (localtime_r)";
    }
#endif

    std::wstringstream wss;
    wss << std::put_time(&t_local, L"%Y-%m-%d %H:%M:%S");
    if (wss.fail()) {
        return L"Error formatting time";
    }
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
    DesktopChecker checker; // COM init/uninit handled by its constructor/destructor if needed
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
    DesktopInfo dInfo; // Handles its own COM
    DesktopChecker checker; // To get desktop path easily
    std::wstring desktopPath = checker.getDesktopPath();


    std::wcout << L"Screen Width: " << dInfo.getScreenWidth() << L"px" << std::endl;
    std::wcout << L"Screen Height: " << dInfo.getScreenHeight() << L"px" << std::endl;
    std::wcout << L"Primary Monitor DPI Scale: " << std::fixed << std::setprecision(2) << dInfo.getPrimaryMonitorDpiScale() * 100 << L"%" << std::endl;

    if (desktopPath.empty()) {
        #ifdef _WIN32
        std::wcerr << L"\nCould not retrieve desktop path. Cannot list items." << std::endl;
        #else
        std::wcout << L"\nDesktop path functionality is limited on non-Windows for listing items." << std::endl;
        // Attempt to use a default or stubbed path if meaningful for non-Windows tests
        // desktopPath = L"."; // Example: current directory for stub testing
        #endif
    }

    // Proceed to list items if path is available or if stub can work with a default
    // On non-Windows, desktopPath might be from getenv("HOME") + "/Desktop" via checker.
    std::wcout << L"\nGathering all desktop items from: " << (desktopPath.empty() ? L"[Default/Stub Path]" : desktopPath) << std::endl;
    std::vector<DesktopItem> allItems = dInfo.getAllDesktopItems(desktopPath);

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
    std::wcout << L"\n--- Testing Basic Sort ---" << std::endl;
    DesktopInfo dInfo;    // Handles its own COM
    DesktopSorter dSorter;  // Handles its own COM
    DesktopChecker checker; // For desktop path
    std::wstring desktopPath = checker.getDesktopPath();

    if (desktopPath.empty() && std::wstring(L"").length() > 0) { // Check ifdef _WIN32 effectively for path requirement
        std::wcerr << L"Desktop path is unknown. Cannot perform sort test." << std::endl;
        return;
    }

    std::wcout << L"Gathering items from: " << desktopPath << std::endl;
    std::vector<DesktopItem> allItems = dInfo.getAllDesktopItems(desktopPath);

    if (allItems.empty()) {
        std::wcout << L"No items found on the desktop to test sorting/moving." << std::endl;
    } else {
        std::wcout << L"Found " << allItems.size() << L" items. Testing first few..." << std::endl;
        const int itemsToTest = std::min((int)allItems.size(), 2); // Test up to 2 items

        for (int i = 0; i < itemsToTest; ++i) {
            const auto& item = allItems[i];
            std::wcout << L"\nTesting item: " << item.name << std::endl;
            std::wcout << L"  Current Coords: (" << item.x << L", " << item.y << L")" << std::endl;

            ItemCategory category = dSorter.classifyItem(item);
            std::wcout << L"  Classified as: " << item_category_to_wstring(category) << std::endl;

            if (item.x != -1 && item.y != -1) { // Check if coordinates are valid
                int newX = item.x + 60; // Try to move it slightly
                int newY = item.y + 60;
                std::wcout << L"  Attempting to move to: (" << newX << L", " << newY << L")" << std::endl;
                dSorter.setItemPosition(item.name, newX, newY);
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


int main() {
#ifdef _WIN32
    // COM should be initialized by each class (DesktopInfo, DesktopSorter) that needs it.
    // However, for operations directly in main or for overall session, one-time init is also an option.
    // HRESULT com_hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    // if (FAILED(com_hr)) {
    //     std::wcerr << L"Main: CoInitializeEx failed. Some features might not work. Error: " << com_hr << std::endl;
    // }
#endif

    try {
        std::locale::global(std::locale(""));
    } catch (const std::runtime_error& e) {
        std::wcerr << L"Critical: Failed to set global locale: " << e.what() << std::endl;
        // Depending on severity, might choose to exit or continue with default locale
    }
    std::wcout.imbue(std::locale("")); // Ensure wcout also uses the global locale
    std::wcerr.imbue(std::locale(""));
    std::wcin.imbue(std::locale(""));


    std::wcout << L"Welcome to Smart Desktop Organizer - CLI Tester" << std::endl;

    int choice = -1;
    while (choice != 0) {
        std::wcout << L"\nPlease select an option:" << std::endl;
        std::wcout << L"  1: Run Desktop Check (Invalid shortcuts, empty folders)" << std::endl;
        std::wcout << L"  2: Display Desktop Info (Screen details, list items)" << std::endl;
        std::wcout << L"  3: Test Basic Sort (Classify and move first few items)" << std::endl;
        std::wcout << L"  0: Exit" << std::endl;
        std::wcout << L"Enter your choice: ";

        std::wstring inputLine;
        if (!std::getline(std::wcin, inputLine)) { // Use wcin for wide string input
             if (std::wcin.eof()){
                std::wcout << L"EOF reached. Exiting." << std::endl;
                break;
             }
            std::wcerr << L"Error reading input." << std::endl;
            std::wcin.clear(); // Clear error flags
            std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Discard bad input
            continue;
        }

        // Attempt to convert wstring to int for choice
        try {
            if (inputLine.length() == 1 && inputLine[0] >= L'0' && inputLine[0] <= L'3') { // Basic validation
                 choice = std::stoi(inputLine);
            } else {
                choice = -1; // Invalid input format
            }
        } catch (const std::invalid_argument& ia) {
            choice = -1; // Not an integer
        } catch (const std::out_of_range& oor) {
            choice = -1; // Integer out of range
        }


        switch (choice) {
            case 1:
                runDesktopCheck();
                break;
            case 2:
                displayDesktopInfo();
                break;
            case 3:
                testBasicSort();
                break;
            case 0:
                std::wcout << L"Exiting program." << std::endl;
                break;
            default:
                std::wcerr << L"Invalid option selected. Please try again." << std::endl;
                break;
        }
    }

#ifdef _WIN32
    // if (SUCCEEDED(com_hr)) { // Only uninitialize if main's CoInitialize succeeded
    //     CoUninitialize();
    // }
    // Note: COM is uninitialized by DesktopInfo/DesktopSorter destructors if they initialized it.
    // If main itself calls CoInitialize, it should call CoUninitialize.
    // For this design, relying on class-level COM management is cleaner.
#endif

    return 0;
}
