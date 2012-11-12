/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:21
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader\FileLogger.cpp
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\LiveBoot Downloader
	file base:	FileLogger
	file ext:	cpp
	author:		tanyc
	
	purpose:	
*********************************************************************/
#include "FileLogger.h"
#include "stdafx.h"

std::map<std::wstring, Logger*> FileLogger::m_appender;
void FileLogger::addAppender(std::wstring name, Logger *pLoger)
{
	static CThreadLock	locker;
	MyAutolock<CThreadLock> _lk(locker);

	if(pLoger)
	{
		if(pLoger->IsOK())
		{
			typedef std::pair<std::wstring, Logger*>Log_Pair;
			m_appender.insert(Log_Pair(name, pLoger));
		}
	}
}

void FileLogger::removeAppenders(std::wstring name)
{
	static CThreadLock	locker;
	MyAutolock<CThreadLock> _lk(locker);

	if(!name.empty())
	{
		std::map<std::wstring, Logger*>::iterator iter;
		iter = m_appender.find(name);
		if(iter != m_appender.end())
		{
			Logger *plog = iter->second;
			if(plog)
			{
				delete plog;
				plog = NULL;
			}
			m_appender.erase(iter);
		}
	}
}
void FileLogger::removeAllAppenders()
{
	static CThreadLock	locker;
	MyAutolock<CThreadLock> _lk(locker);

	std::map<std::wstring, Logger*>::iterator iter;
	for(iter = m_appender.begin(); iter != m_appender.end(); iter++)
	{
		Logger *log  = iter->second;
		if(log)
		{
			delete log;
			log = NULL;
		}
	}
	m_appender.clear();
}
Logger* FileLogger::getLoggerInstance(const std::wstring& name)
{
	static CThreadLock	locker;
	MyAutolock<CThreadLock> _lk(locker);

	std::map<std::wstring, Logger*>::const_iterator iter;
	iter = m_appender.find(name);
	if(iter != m_appender.end())
	{
		return iter->second;
	}
	else
	{
		return NULL;
	}
}

void FileLogger::Log(const std::wstring appendername, const std::wstring szfilename, int nfileline, const std::wstring szlog, LogSections eSection, LogLevels eLevel)
{
	static CThreadLock	locker;
	MyAutolock<CThreadLock> _lk(locker);

	if(!appendername.empty())
	{
		Logger *log = FileLogger::getLoggerInstance(appendername);
		if(log)
		{
			log->Log(szfilename, nfileline, szlog, eSection, eLevel);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
Logger::Logger(const std::wstring& szFile)
{
	//std::locale::global(std::locale(""));
	std::locale &loc=std::locale::global(std::locale(std::locale(),"",LC_CTYPE));
	_ofStream.open(szFile.c_str());
	std::locale::global(loc); 
	if(!_ofStream.good())
	{
		m_ok = false;
	}
	else
	{
		m_ok = true;
	}
}

Logger::~Logger()
{
	if (_ofStream.is_open())
	{
		_ofStream.close();
	}
}

void Logger::Log(const std::wstring szfilename, int nfileline, const std::wstring szlog, LogSections eSection, LogLevels eLevel)
{
	static CThreadLock	locker;
	MyAutolock<CThreadLock> _lk(locker);

	if(!IsOK() && _ofStream.good())
	{
		return;
	}

	wchar_t line[256] = {0};
	_itow(nfileline, line, 10);

	// get time
	__time64_t time;
	_time64(&time );
	struct tm gmt;
	errno_t err = _localtime64_s( &gmt, &time );
	if(!err)
	{
		wchar_t timebuf[256] = {0};
		wcsftime(timebuf, sizeof(timebuf), _T("%Y-%m-%d %H:%M:%S"), &gmt);

		std::wstring filename = szfilename;
		std::wstring::size_type pos = szfilename.rfind('\\');
		if(pos != std::wstring::npos)
		{
			filename = szfilename.substr(szfilename.rfind('\\')+1);
		}

		std::wostringstream ostr;
		ostr << timebuf << _T(" ");
		ostr << _T("[") << Convert::ToString(eSection) << _T("] [") << Convert::ToString(eLevel) << _T("] ");
		ostr << _T("(File :") << filename << _T(" Line :") << line << _T(") : ");
		ostr << szlog;
		std::wstring str = ostr.str();

		_ofStream << str << std::endl;
		//_ofStream.write(str.c_str(), static_cast<std::streamsize>(str.length()));
	}
}

bool Logger::IsOK()
{
	return m_ok;
}