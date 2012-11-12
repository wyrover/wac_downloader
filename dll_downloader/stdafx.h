// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// 某些 CString 构造函数将是显式的


#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include <atlstr.h>
#include <atltime.h>
#include <ATLComTime.h>

#include "FileLogger.h"

#define LOG_FUNCTION_FILE_NAME				_T("DownloaderFunctionLog.log")
#define LOG_FUNCTION_APPENDER_NAME			_T("WAC_downloader")


#ifdef _DEBUG
#ifndef __LOG_ENABLE__
#define __LOG_ENABLE__
#define __LOG_LOG__(a, b, c)			FileLogger::Log(LOG_FUNCTION_APPENDER_NAME, _T(__FILE__), __LINE__, a, b, c)
#endif
#else
#define __LOG_LOG__(a, b, c)

#endif
