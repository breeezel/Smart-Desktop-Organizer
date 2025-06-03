// stdafx.h : ULTRA MINIMAL include file for MFC applications
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef WINVER
#define WINVER 0x0A00 // Windows 10
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions (CToolBar, CStatusBar)
#include <afxdlgs.h>        // MFC dialog classes (CDialogEx)
#include <afxcmn.h>         // MFC support for Windows Common Controls (CListCtrl)
