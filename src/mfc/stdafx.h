// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

// Target Windows XP SP1 or later.
// This also defines _WIN32_WINNT to be 0x0501 for things like GetDpiForMonitor
// If targeting newer Windows versions primarily, these could be updated.
#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#define _AFX_ALL_WARNINGS // turns off MFC's hiding of some common and often safely ignored warning messages

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdlgs.h>        // MFC dialog classes (CDialogEx)
#include <afxcmn.h>         // MFC support for Windows Common Controls (like CListCtrl)
#include <afxcontrolbars.h> // MFC support for ribbons and control bars
#include <afxdisp.h>        // MFC Automation classes

// Commonly used Windows headers with MFC
#include <shlobj.h>         // For SHBrowseForFolder, shell functions
#include <shlwapi.h>        // For PathFileExists, other shell light-weight utilities
#include <wininet.h>        // For Internet functions used by WebClassifier

// Standard C++ library headers that might be used frequently in MFC files
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <memory> // For std::unique_ptr etc.
