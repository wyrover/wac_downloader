// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// ĳЩ CString ���캯��������ʽ��


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
