/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:19
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader\FileLogger.h
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader
	file base:	FileLogger
	file ext:	h
	author:		tanyc
	
	purpose:	
*********************************************************************/
#pragma once
#include <sstream>
#include <fstream>
#include <Windows.h>
#include <time.h>
#include <map>
#include <tchar.h>

class CThreadLock 
{
public:
	CThreadLock() { InitializeCriticalSection(&m_cs); }
	~CThreadLock() { DeleteCriticalSection(&m_cs); }
	void Lock() { EnterCriticalSection(&m_cs); }
	void Unlock() { LeaveCriticalSection(&m_cs); }
private:
	CRITICAL_SECTION m_cs;
};

template <typename T>
class MyAutolock 
{
public:
	MyAutolock(T& lockable) : m_lockable(lockable) { m_lockable.Lock(); }
	~MyAutolock() { m_lockable.Unlock(); }
private:
	T& m_lockable;
};


enum LogSections
{
	L_LOGSECTION_APP_INIT = 0,
	L_LOGSECTION_APP_GENERAL,
	L_LOGSECTION_APP_UNINIT,
	L_LOGSECTION_MODULE
};

enum LogLevels
{
	L_LOGLEVEL_TRACE = 0,
	L_LOGLEVEL_WARNING,
	L_LOGLEVEL_ERROR,
	L_LOGLEVEL_DEBUG
};

class Convert
{
public: static std::wstring ToString(LogSections gSection)
		{
			switch(gSection)
			{
			case L_LOGSECTION_APP_INIT: return _T("Init");
			case L_LOGSECTION_APP_GENERAL: return _T("General");
			case L_LOGSECTION_APP_UNINIT: return _T("Uninit");
			case L_LOGSECTION_MODULE: return _T("Module");
			}

			_ASSERTE(FALSE);
			return _T("***UNKNOWN***");
		}

public: static std::wstring ToString(LogLevels gLevel)
		{
			switch(gLevel)
			{
			case L_LOGLEVEL_TRACE: return _T("Trace");
			case L_LOGLEVEL_WARNING: return _T("Warning");
			case L_LOGLEVEL_ERROR: return _T("Error");
			case L_LOGLEVEL_DEBUG: return _T("Debug");
			}

			_ASSERTE(FALSE);
			return _T("***UNKNOWN***");
		}
};


class Logger
{
public:
	Logger(const std::wstring& szFile);
	~Logger();

	bool IsOK();
	void Log(const std::wstring szfilename, int nfileline, const std::wstring szlog, LogSections eSection, LogLevels eLevel);

private:
	std::wofstream _ofStream;
	bool m_ok;
};

class FileLogger
{
public:
	static void addAppender(std::wstring name, Logger *pLoger);
	static void removeAppenders(std::wstring name);
	static void removeAllAppenders();
	static void Log(const std::wstring appendername, const std::wstring szfilename, int nfileline, const std::wstring szlog, LogSections eSection, LogLevels eLevel);

private:
	static Logger* getLoggerInstance(const std::wstring& name);
	static std::map<std::wstring, Logger*> m_appender;
};
