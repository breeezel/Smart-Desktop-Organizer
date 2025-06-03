// stdafx.h : MINIMAL include file for MFC applications
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

// Target Windows 10. This is important for API availability.
#ifndef WINVER
#define WINVER 0x0A00           // Windows 10
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00     // Windows 10
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#define _AFX_ALL_WARNINGS // turns off MFC's hiding of some common and often safely ignored warning messages

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions (CToolBar, CStatusBar)
#include <afxdlgs.h>        // MFC dialog classes (CDialogEx)
#include <afxcmn.h>         // MFC support for Windows Common Controls (CListCtrl)
// #include <afxcontrolbars.h> // MFC support for ribbons and control bars - Temporarily commented
// #include <afxdisp.h>        // MFC Automation classes - Temporarily commented

// Commonly used Windows headers - Keep these minimal for now
// #include <shlobj.h>      // For SHBrowseForFolder, shell functions - Temporarily commented
// #include <shlwapi.h>     // For PathFileExists, other shell light-weight utilities - Temporarily commented
// #include <wininet.h>     // For Internet functions used by WebClassifier - Temporarily commented

// Standard C++ library headers - Temporarily comment out to isolate MFC/PCH issues
// #include <vector>
// #include <string>
// #include <map>
// #include <algorithm>
// #include <memory>
