// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>


#include <atlstr.h>
#include <atltime.h>
#include <ATLComTime.h>
#include <shlobj.h>

#include "../dll_downloader/FileLogger.h"

#include "gdiplus.h"
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

//for webbrowser
#include <atlbase.h>
#include <atlwin.h>
#include <windows.h>
#pragma comment(lib,"atl")
#pragma comment(lib,"User32.lib")

#define LOG_UI_FILE_NAME				_T("LiveBootDownloaderLog.log")
#define LOG_UI_APPENDER_NAME			_T("ui_liveboot_downloader")


#ifdef _DEBUG
#ifndef __LOG_ENABLE__
	#define __LOG_ENABLE__
	#define __LOG_LOG__(a, b, c)		FileLogger::Log(LOG_UI_APPENDER_NAME, _T(__FILE__), __LINE__, a, b, c)
#endif
#else
	#define __LOG_LOG__(a, b, c)
#endif