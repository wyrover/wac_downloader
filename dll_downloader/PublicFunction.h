// PublicFunction.h: interface for the CPublicFunction class.
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "string.h"
#include "stdio.h"
#include "stdafx.h"
using namespace ATL;


#define SLEEP_RETURN(x)\
{\
	if ( ::WaitForSingleObject ( m_hEvtEndModule, x ) == WAIT_OBJECT_0 )\
	return FALSE;\
}
#define SLEEP_BREAK(x)\
{\
	if ( ::WaitForSingleObject ( m_hEvtEndModule, x ) == WAIT_OBJECT_0 )\
	break;\
}


//==========================================================================
// 常用操作宏
//==========================================================================
#define GET_SAFE_STRING(str) ( (str)?(str):_T("") )
#define STRNCPY(sz1,sz2,size) \
{\
	wcsncpy(((wchar_t*)(sz1)),(sz2)?((wchar_t*)(sz2)):_T(""),(size));\
	((wchar_t*)(sz1))[(size)-1] = _T('\0');\
}

// 时间类型的数据长度
#define DATETIME_TYPE_LENGTH				20
#define STRCPY(sz1,sz2) strcpy ( (wchar_t*)(sz1), (wchar_t*)((sz2)?(sz2):_T("")) )
#define STRLEN_SZ(sz) ((sz)?wcslen((wchar_t*)(sz)):0)
#define ASSERT_ADDRESS(p,size) _ASSERT((p)!=NULL && (p!=NULL && !IsBadReadPtr(p,size) && (!TRUE|| !IsBadWritePtr((LPVOID)p,size))))
#define LENGTH(x) sizeof(x)/sizeof(x[0])
#define MIN(x,y) (((DWORD)(x)<(DWORD)(y))?(x):(y))
#define MAX(x,y) (((DWORD)(x)>(DWORD)(y))?(x):(y))

// 有效的句柄
#define HANDLE_IS_VALID(h) ((HANDLE)(h) && ((HANDLE)(h)!=INVALID_HANDLE_VALUE))

// 关闭句柄
#define CLOSE_HANDLE(h)\
{\
	if ( HANDLE_IS_VALID(h) )\
	{\
	::CloseHandle ( h );\
	(h) = NULL;\
	}\
}

int ConvertStrToCTime(wchar_t *chtime, CTime &cTime );
void MytoLower(wchar_t *src);
CString GetOneLine ( CString &str );
int GetMouthByShortStr ( LPCTSTR lpszShortMonth );
CString hwFormatMessage ( DWORD dwErrorCode );
const wchar_t* MakeSureDirectory(LPCTSTR lpszDirName);
BOOL PartPathAndFileAndExtensionName (
									  IN LPCTSTR lpszFilePath,			// 全路径名(包含文件名)
									  OUT CString *pcsOnlyPath,			// 输出光路径（没有文件名）
									  OUT CString *pcsOnlyFileName,		// 输出光文件名（没有路径）
									  OUT CString *pcsExtensionName		// 输出扩展名
									  );
BOOL PartFileAndPathByFullPath (
								LPCTSTR lpszFilePath,
								wchar_t *szOnlyFileName,
								int nFileNameSize,
								wchar_t *szOnlyPath =NULL,
								int nPathSize=0
								);
DWORD GetCurTimeString(wchar_t *buf, time_t tNow=0);
CString GetCurTimeString ( time_t tNow=0 );
void StandardizationPathBuffer ( wchar_t *szPath, int nSize, wchar_t cFlagChar='\\' );
CString StandardizationFileForPathName ( LPCTSTR lpszFileOrPathName, BOOL bIsFileName, wchar_t cReplaceChar='_' );
wchar_t *hwStrrChr ( const wchar_t *string, int c );
wchar_t *hwStrChr ( const wchar_t *string, int c );
BOOL PartFileAndExtensionName (
							   IN LPCTSTR lpszFileName,
							   OUT wchar_t *szFileName,
							   IN int nFileNameSize,
							   OUT wchar_t *szExtensionName=NULL,
							   IN int nExtensionNameSize=0 );
unsigned int CreateNullFile ( LPCTSTR lpszFileName, __int64 nFileSize );
BOOL WaitForThreadEnd ( HANDLE hThread, DWORD dwWaitTime=5000 );



class CThreadGuard 
{
public:
	CThreadGuard() { InitializeCriticalSection(&m_cs); }
	~CThreadGuard() { DeleteCriticalSection(&m_cs); }
	void Lock() { EnterCriticalSection(&m_cs); }
	void Unlock() { LeaveCriticalSection(&m_cs); }
private:
	CRITICAL_SECTION m_cs;
};

template <typename T>
class autolock 
{
public:
	autolock(T& lockable) : m_lockable(lockable) { m_lockable.Lock(); }
	~autolock() { m_lockable.Unlock(); }
private:
	T& m_lockable;
};



bool URL_GBK_Decode(const wchar_t* url,wchar_t* decd);
bool URL_IS_UTF8(const wchar_t* url);
bool URL_UTF8_Decode(const wchar_t* url,wchar_t* decd);
bool URL_Decode(const wchar_t* url,wchar_t* pStr);