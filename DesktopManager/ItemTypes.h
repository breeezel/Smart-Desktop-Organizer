#pragma once

// Defines the categories an item can belong to.
// Used by DesktopSorter, WebClassifier, DesktopInfo (for DesktopItem), and DesktopLayoutManager.
enum class ItemCategory {
    FOLDER,
    GAME,
    PROGRAM,
    TEXT_FILE,
    IMAGE_FILE,
    VIDEO_FILE,
    ARCHIVE_FILE,
    // Add other specific file types as needed, e.g., DOCUMENT, SPREADSHEET, PRESENTATION, AUDIO_FILE
    UNCLASSIFIED,
    SPECIAL_SYSTEM // e.g. Recycle Bin, My Computer (though usually filtered out from sorting)
};
