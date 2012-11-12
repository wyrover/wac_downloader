// PublicFunction.cpp: implementation of the CPublicFunction class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PublicFunction.h"
#include  <io.h>
#include <direct.h>
#include <shellapi.h>
#include "Shlwapi.h"
#pragma comment ( lib, "Shlwapi.lib" )
const char f_seps[]   = ";";


void MytoLower(wchar_t *src)   
{   
	//_ASSERT(src);   
	wchar_t *p = src;   
	while(*p != _T('\0'))   
	{   
		if(64 < *p && *p < 91) //A是65，Z是90   
		{   
			*p = (*p+32);   
		}   
		p++;   
	} 
}   

CString GetOneLine(CString &str)
{
	int nPos = str.Find (_T("\r\n"), 0 );
	if ( nPos < 0 ) return _T("");
	CString csOneLine = str.Left ( nPos );
	str = str.Mid ( nPos + 2 );
	return csOneLine;
}

int CalcCharCount ( LPCTSTR lpszText, wchar_t chCalc )
{
	int nLen = STRLEN_SZ(lpszText);
	int nCount = 0;
	for ( int i=0; i<nLen; i++ )
	{
		if ( lpszText[i] == chCalc )
			nCount ++;
	}
	return nCount;
}

BOOL CopyBuffer_Date ( int *data, SYSTEMTIME &SysTime )
{
	const int nMinDateDigitCount = 3;
	ASSERT_ADDRESS ( data, nMinDateDigitCount * sizeof(int) );

	/********年(1000 ~ 9999)********/
	if ( data[0] < 1000 || data[0] >= 9999 )
		return FALSE;
	SysTime.wYear = data[0];

	/********月(1--12)********/
	if ( data[1] < 1 || data[1] > 12 )
		return FALSE;
	SysTime.wMonth = data[1];

	/********日(1--31)********/
	if ( data[2] < 1 && data[2] > 31 )
		return FALSE;
	SysTime.wDay = data[2];

	return TRUE;
}

BOOL CopyBuffer_Time ( int *data, SYSTEMTIME &SysTime )
{
	const int nMinDateDigitCount = 3;
	ASSERT_ADDRESS ( data, nMinDateDigitCount * sizeof(int) );

	/********时(0--23)********/
	if ( data[0] <0 || data[0] > 23 )
		return FALSE;
	SysTime.wHour = data[0];

	/********分(0--59)********/
	if ( data[1] < 0 || data[1] > 59 )
		return FALSE;
	SysTime.wMinute = data[1];

	/********秒********/
	if ( data[2] < 0 || data[2] > 59 )
		return FALSE;
	SysTime.wSecond = data[2];

	return TRUE;
}

//
// ConvertStrToCTime() 将一个表示日期的字符串（按年、月、日、时、分、秒的顺序，
// 如"2001-08-09 18:03:30"）转成 CTime 格式.
// return : ------------------------------------------------------------
//		0	-	错误
//		1	-	是时期时间
//		2	-	是日期
//		3	-	是时间
//
int ConvertStrToCTime(wchar_t *chtime, CTime &cTime )
{
	int  i, j, k;
	wchar_t tmpbuf[8] = {0};
	int  value[6] = {0};
	SYSTEMTIME SysTime = {0};

	if ((!chtime) ) return FALSE; /* invalid parameter */

	memset((void *)value, 0, sizeof(value));
	for (i=0, j=0, k=0;  ; i++)
	{
		if (chtime[i]<'0' || chtime[i]>'9') /* 非数字字符 */
		{
			tmpbuf[j] = '\0';
			if ( j > 0 )
			{
				value[k++] = _wtoi(tmpbuf);
				j = 0;
				if ( k >= 6 ) break;
			}
			if ( chtime[i] == '\0' ) break;
		}
		else if (j < 7) tmpbuf[j++] = chtime[i];
	}
	if ( k < 3 ) return 0;
	if	(
		CalcCharCount ( chtime, '-' ) < 2
		&&
		CalcCharCount ( chtime, '/' ) < 2
		&&
		CalcCharCount ( chtime, ':' ) < 2
		)
		return 0;

	int nRet = 0;
	// 是日期时间
	if	(
		( k>=6 )
		&&
		CopyBuffer_Date ( value, SysTime )
		&&
		CopyBuffer_Time ( &value[3], SysTime )
		)
	{
		nRet = 1;
	}
	// 是日期
	else if	(
		(k>=3)
		&&
		CopyBuffer_Date ( value, SysTime )
		)
	{
		nRet = 2;
	}

	// 是时间
	if	(
		(k>=3)
		&&
		CopyBuffer_Time ( value, SysTime )
		)
	{
		nRet = 3;
	}

	if ( SysTime.wYear < 1971 )
		SysTime.wYear = 1971;
	if ( SysTime.wMonth < 1 )
		SysTime.wMonth = 1;
	if ( SysTime.wDay < 1 )
		SysTime.wDay = 1;

	CTime tTemp ( SysTime );
	cTime = tTemp;

	return nRet;
}


int GetMouthByShortStr ( LPCTSTR lpszShortMonth )
{
	const wchar_t* szConstMonth[] =
	{
		_T("jan"), _T("feb"), _T("mar"), _T("apr"), _T("may"), _T("jun"), _T("jul"), _T("aug"), _T("sep"), _T("oct"), _T("nov"), _T("dec"), _T("")
	};

	CString csShortMonth = GET_SAFE_STRING ( lpszShortMonth );
	for ( int i=0; i<sizeof(szConstMonth)/sizeof(szConstMonth[0]); i++ )
	{
		if ( csShortMonth.CompareNoCase ( szConstMonth[i] ) == 0 )
		{
			return ( i+1 );
		}
	}

	return -1;
};

CString hwFormatMessage ( DWORD dwErrorCode )
{
	CString csError;
	LPVOID pv;
	FormatMessage (
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwErrorCode,
		MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
		(LPTSTR)&pv,
		0,
		NULL);
	if(pv)
	{
		csError = (char*)pv;
		LocalFree ( pv );
	}

	return csError;
}



//
// 为了方便 strchr() 或 strrchr() 函数正确查找字符，将字符串中的中文字符串用指定的字符替换掉
//
void ReplaceChineseStrToEnglish ( wchar_t *szBuf, wchar_t cReplace )
{
	if ( !szBuf ) return;
	for ( int i=0; szBuf[i] != '\0'; i++ )
	{
		if ( szBuf[i] < 0 && szBuf[i+1] != '\0' )
		{
			szBuf[i] = cReplace;
			szBuf[i+1] = cReplace;
			i ++;
		}
	}
}

/********************************************************************************
* Function Type	:	Global
* Parameter		:	lpszDirName		-	[in] 目录名
* Return Value	:	没有路径的文件名
* Description	:	确保路径存在，如果目录不存在就创建目录,可以创建多层次的目录
*********************************************************************************/
const wchar_t* MakeSureDirectory(LPCTSTR lpszDirName)
{
	wchar_t tempbuf[255];
	const wchar_t* p1 = NULL;
	const wchar_t* p2 = lpszDirName;
	int len;
	while(1)
	{
		p1 = hwStrChr(p2,'\\');
		if( p1 )
		{
			ZeroMemory(tempbuf,sizeof(tempbuf));
			len = (int)((p1 - lpszDirName > sizeof(tempbuf)) ? sizeof(tempbuf) : (p1 - lpszDirName));
			if(len < 1)	//如：“\123\456\”目录，第一个就是“\”
			{
				p2 = p1 + 1;
				continue;
			}
			wcsncpy((wchar_t*)tempbuf,(const wchar_t*)lpszDirName, len);
			if(_waccess((const wchar_t*)tempbuf,0) == -1)	//Not exist
			{
				if(_wmkdir((const wchar_t*)tempbuf) != 0)
					return FALSE;
			}
			p2 = p1 + 1;
		}
		else break;
	}

	return p2;
}


//
//
// 从一个完整的全路径名(包含文件名)中分离出路径（没有文件名）和
// 文件名，如：从“E:\001\002.exe”中分得“E:\001\”，结果存入到
// lsOnlyPath中，“002.exe”存入szOnlyFileName中
//
BOOL PartFileAndPathByFullPath (
								IN LPCTSTR lpszFilePath,			// 全路径名(包含文件名)
								OUT wchar_t *szOnlyFileName,			// 光文件名（没有路径）
								int nFileNameSize,
								OUT wchar_t *szOnlyPath /*=NULL*/,		// 光路径（没有文件名）
								int nPathSize/*=0*/
								)
{
	_ASSERT ( lpszFilePath );
	wchar_t chDirPart = _T('\\');
	if ( hwStrChr ( lpszFilePath, '/' ) )
		chDirPart = '/';

	if ( szOnlyFileName )
	{
		memset ( szOnlyFileName, 0, nFileNameSize );
	}
	if ( szOnlyPath )
	{
		memset ( szOnlyPath, 0, nPathSize );
	}

	WIN32_FILE_ATTRIBUTE_DATA FileAttrData;
	if ( GetFileAttributesEx ( lpszFilePath, GetFileExInfoStandard, (LPVOID)&FileAttrData ) &&
		( FileAttrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY
		&& FileAttrData.dwFileAttributes != 0xffffffff ) //	本身就是目录
	{
		if ( szOnlyPath )
		{
			STRNCPY ( szOnlyPath, lpszFilePath, nPathSize );
			StandardizationPathBuffer ( szOnlyPath, nPathSize, chDirPart );
		}
		return TRUE;
	}

	wchar_t *p = hwStrrChr ( lpszFilePath, chDirPart );
	if ( !p )
	{
		STRNCPY ( szOnlyFileName, lpszFilePath, nFileNameSize );
		return TRUE;
	}

	if ( szOnlyFileName )
		STRNCPY ( szOnlyFileName, p+1, nFileNameSize );
	if ( szOnlyPath )
	{
		STRNCPY ( szOnlyPath, lpszFilePath, nPathSize );
		int nLen = p-lpszFilePath+1;
		if ( nPathSize-1 < nLen ) return FALSE;
		szOnlyPath [ nLen ] = '\0';
	}
	return TRUE;
}

BOOL PartPathAndFileAndExtensionName (
									  IN LPCTSTR lpszFilePath,			// 全路径名(包含文件名)
									  OUT CString *pcsOnlyPath,			// 输出光路径（没有文件名）
									  OUT CString *pcsOnlyFileName,		// 输出光文件名（没有路径）
									  OUT CString *pcsExtensionName		// 输出扩展名
									  )
{
	wchar_t szOnlyPath[MAX_PATH] = {0};
	wchar_t szOnlyFileName[MAX_PATH] = {0};
	wchar_t szExtensionName[MAX_PATH] = {0};
	if ( !PartFileAndPathByFullPath ( lpszFilePath, szOnlyFileName, MAX_PATH, szOnlyPath, MAX_PATH ) )
		return FALSE;
	if ( !PartFileAndExtensionName ( szOnlyFileName, szOnlyFileName, MAX_PATH, szExtensionName, MAX_PATH ) )
		return FALSE;

	if ( pcsOnlyPath ) *pcsOnlyPath = szOnlyPath;
	if ( pcsOnlyFileName ) *pcsOnlyFileName = szOnlyFileName;
	if ( pcsExtensionName ) *pcsExtensionName = szExtensionName;

	return TRUE;
}

wchar_t *hwStrrChr ( const wchar_t *string, int c )
{
	if ( !string ) return NULL;
	CString csString = string;
	ReplaceChineseStrToEnglish ( csString.GetBuffer(csString.GetLength()), (c>=127) ? (c-1) : (c+1) );
	csString.ReleaseBuffer ();
	int nPos = csString.ReverseFind ( c );
	if ( nPos < 0 ) return NULL;
	return ( (wchar_t*)string + nPos );
}

wchar_t *hwStrChr ( const wchar_t *string, int c )
{
	if ( !string ) return NULL;
	CString csString = string;
	ReplaceChineseStrToEnglish ( csString.GetBuffer(csString.GetLength()), (c>=127) ? (c-1) : (c+1) );
	csString.ReleaseBuffer ();
	int nPos = csString.Find ( c );
	if ( nPos < 0 ) return NULL;
	return ( (wchar_t*)string + nPos );
}

//
// 根据文件名来找它的文件名和扩展名，如：完整路径为“E:\123\456.bmp”，那么 szFileName 等于“E:\123\456”
// szExtensionName 等于“bmp”
//
BOOL PartFileAndExtensionName (
							   IN LPCTSTR lpszFileName,
							   OUT wchar_t *szFileName,
							   IN int nFileNameSize,
							   OUT wchar_t *szExtensionName/*=NULL*/,
							   IN int nExtensionNameSize/*=0*/ )
{
	_ASSERT ( lpszFileName );
	wchar_t chDirPart = '\\';
	if ( hwStrChr ( lpszFileName, '/' ) )
		chDirPart = '/';

	if ( szFileName )
	{
		STRNCPY ( szFileName, lpszFileName, nFileNameSize );
	}
	if ( szExtensionName )
	{
		memset ( szExtensionName, 0, nExtensionNameSize );
	}

	wchar_t *p_Dot = hwStrrChr ( lpszFileName, '.' );
	if ( !p_Dot )
	{
		return TRUE;
	}
	wchar_t *p_Slash = hwStrrChr ( lpszFileName, chDirPart );
	if ( szFileName )
	{
		if ( p_Dot-lpszFileName >= nFileNameSize )
			return FALSE;

		if ( int(p_Dot-lpszFileName) < (int)wcslen(lpszFileName)-1 )
			szFileName [p_Dot-lpszFileName] = '\0';
	}
	if ( p_Slash > p_Dot )
	{
		return TRUE;
	}

	if ( szExtensionName )
	{
		// "mp3?xcode=d426ca38c702b192165aae570e00d8bd"
		//STRNCPY ( szExtensionName, p_Dot + 1, nExtensionNameSize );

		wchar_t c_ExtensionName[256] = {0};
		STRNCPY ( c_ExtensionName, p_Dot + 1, 256 );

		CString sz(c_ExtensionName);
		int nPos = sz.GetLength();
		for(int i = 0; i < sz.GetLength(); i++)
		{
			wchar_t c = sz.GetAt(i);
			int  n = (int)c;
			if( ((n >= 48) && n <= 57) || ((n >= 65) && n <= 90) || ((n >= 97) && n <= 122) )
			{
				// 0 - 9 a - z A - Z
			}
			else
			{
				nPos = i;
				break;
			}
		}
		CString name = sz.Left(nPos);
		wcsncpy(szExtensionName, name.GetBuffer(name.GetLength()), name.GetLength());
	}
	return TRUE;
}


//
// 创建一个空文件
//
// 返回值：0  -> successful
//         1  -> create file failed
//         2  -> not enough disk space
//         3  -> invalid parameter
unsigned int CreateNullFile ( LPCTSTR lpszFileName, __int64 nFileSize )
{
	if ( !lpszFileName )
	{
		return 3;
	}

	unsigned int nRet = 1;
	if(::DeleteFile ( lpszFileName ) == FALSE)
	{
		CString szLog;
		szLog.Format(_T("DeleteFile failed: %s, errno: %d"), lpszFileName, GetLastError());
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
	}
	
	// 创建
	HANDLE hFile = ::CreateFile(lpszFileName,						// file to open
								GENERIC_WRITE,						// open for read
								0,									// no share
								NULL,								// default security
								CREATE_NEW,							// create new file
								FILE_ATTRIBUTE_NORMAL,				// normal file
								NULL);								// no attr. template	
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CString szLog;
		szLog.Format(_T("CreateFile failed: %s"), lpszFileName);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		nRet = 1;
	}
	else
	{
		if ( nFileSize > 0 )
		{
			LARGE_INTEGER li;
			li.QuadPart = nFileSize;
			li.LowPart = ::SetFilePointer (hFile, li.LowPart, &li.HighPart, FILE_BEGIN);
			if (li.LowPart != INVALID_SET_FILE_POINTER && li.QuadPart == nFileSize)
			{
				if(::SetEndOfFile(hFile))
				{
					// success
					nRet = 0;
				}
				else
				{
					// not enough disk space
					nRet = 2;
				}
			}
		}
		else
		{
			nRet = 0;
		}
	}

	CLOSE_HANDLE(hFile);
	if(nRet != 0)
	{
		::DeleteFile ( lpszFileName );
	}
	return nRet;
}

//
// 等待线程退出
//
BOOL WaitForThreadEnd ( HANDLE hThread, DWORD dwWaitTime /*=5000*/ )
{
	BOOL bRet = TRUE;
	if ( !HANDLE_IS_VALID(hThread) ) return TRUE;
	if ( ::WaitForSingleObject ( hThread, dwWaitTime ) == WAIT_TIMEOUT )
	{
		bRet = FALSE;
		CString szLog;
		szLog = _T("WaitForThreadEnd timeout");
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		::TerminateThread ( hThread, 0 );
	}
	return bRet;
}


//
// 标准化路径缓冲，如果不是以“\”结尾，将自动加上
//
void StandardizationPathBuffer ( wchar_t *szPath, int nSize, wchar_t cFlagChar/*='\\'*/ )
{
	int nLen = wcslen(szPath);
	if ( nLen < 1 ) return;
	ASSERT_ADDRESS ( szPath, nLen+1 );
	wchar_t szTemp[4] = {0};
	szTemp[0] = cFlagChar;
	if ( szPath[nLen-1] != cFlagChar )
		wcsncat ( szPath, szTemp, nSize );

	CString csPath = StandardizationFileForPathName ( szPath, FALSE );
	wcsncpy ( szPath, csPath, nSize );
}

//
// 标准化路径或文件名，把不符合文件名命名规则的字符替换成指定的字符。//u 在 StandardizationPathBuffer() 中
// 加上该函数
//
CString StandardizationFileForPathName ( LPCTSTR lpszFileOrPathName, BOOL bIsFileName, wchar_t cReplaceChar/*='_'*/ )
{
	CString csFileOrPathName = GET_SAFE_STRING(lpszFileOrPathName);
	CString csHead, csTail;
	// 路径名中最后一个'\\'是正常的。另外类似“c:\\”的字符也是正常的。所以先提取出来不参与后面的替换，等替换完以后再补回来
	if ( !bIsFileName )
	{
		if ( csFileOrPathName.GetLength() >= 1 && (csFileOrPathName[csFileOrPathName.GetLength()-1] == '\\' || csFileOrPathName[csFileOrPathName.GetLength()-1] == '/') )
		{
			csTail += csFileOrPathName[csFileOrPathName.GetLength()-1];
			csFileOrPathName = csFileOrPathName.Left ( csFileOrPathName.GetLength()-1 );
		}

		if ( csFileOrPathName.GetLength() >= 2 && isalpha(csFileOrPathName[0]) && csFileOrPathName[1]==':' )
		{
			csHead = csFileOrPathName.Left(2);
			csFileOrPathName = csFileOrPathName.Mid(2);
		}
		if ( csFileOrPathName.GetLength() >= 1 && (csFileOrPathName[0]=='\\' || csFileOrPathName[0]=='/') )
		{
			csHead += csFileOrPathName[0];
			csFileOrPathName = csFileOrPathName.Mid(1);
		}
	}
	else
	{
		csFileOrPathName.Replace ( _T("\\"), _T("_") );
		csFileOrPathName.Replace ( _T("/"), _T("_") );
	}
	csFileOrPathName.Replace ( _T(":"), _T("_") );
	csFileOrPathName.Replace ( _T("*"), _T("_") );
	csFileOrPathName.Replace ( _T("?"), _T("_") );
	csFileOrPathName.Replace ( _T("\""), _T("_") );
	csFileOrPathName.Replace ( _T("<"), _T("_") );
	csFileOrPathName.Replace ( _T(">"), _T("_") );
	csFileOrPathName.Replace ( _T("|"), _T("_") );
	csFileOrPathName.Insert ( 0, csHead );
	csFileOrPathName += csTail;

	return csFileOrPathName;
}



/********************************************************************************
* Function Type	:	public
* Parameter		:	buf		-	输出缓冲
* Return Value	:	字符个数
* Description	:	获取当前时间的字符串,如：2003-10-01 12:00:00
*********************************************************************************/
DWORD GetCurTimeString ( wchar_t *buf, time_t tNow/*=0*/ )
{
	ASSERT_ADDRESS ( buf, DATETIME_TYPE_LENGTH );
	if ( tNow == 0 ) tNow = time(NULL);
	CTime cTime ( tNow );
	CString csNow = cTime.Format ( _T("%Y-%m-%d %H:%M:%S") );
	return (DWORD)_snwprintf ( buf, DATETIME_TYPE_LENGTH, _T("%s"), csNow );
}

CString GetCurTimeString ( time_t tNow/*=0*/ )
{
	wchar_t szTime[DATETIME_TYPE_LENGTH+1] = {0};
	GetCurTimeString ( szTime, tNow );
	return szTime;
}


//////////////////////////////////////////////////////////////////////////
bool URL_GBK_Decode(const wchar_t *encd, wchar_t* decd)
{
	if( encd == 0 )
		return false;

	int i, j=0;
	const wchar_t *cd = encd;
	wchar_t p[2];
	unsigned int num;
	for( i = 0; i < (int)wcslen( cd ); i++ )
	{
		memset( p, 0, sizeof(p) );
		if( cd[i] != '%' )
		{
			decd[j++] = cd[i];
			continue;
		}else if(cd[i] == '%'
			&& ( (cd[i+1]>='0'&&cd[i+1]<='9') ||
			(cd[i+1]>='A'&&cd[i+1]<='F') ||
			(cd[i+1]>='a'&&cd[i+1]<='f') ) )
		{
			p[0] = cd[++i];
			p[1] = cd[++i];

			swscanf( p, _T("%x"), &num );
			swprintf(p, _T("%c"), num );
			decd[j++] = p[0];
		}else
		{
			decd[j++] = cd[i];
		}
	}
	decd[j] = '\0';

	return true;
}


bool URL_IS_UTF8(const wchar_t* url)
{
	return false;

	//wchar_t buffer[4096];
	//
	//ZeroMemory(buffer,sizeof(buffer));

	//URL_GBK_Decode(url,buffer);

	//wchar_t* pGBKStr = buffer;
	//int strLen = wcslen(pGBKStr);

	//int nWideByte = 0;
	//for( int i=0; i<strLen; i++)
	//{
	//	if( pGBKStr[i] < 0 )
	//	{
	//		nWideByte++;
	//	}
	//}

	//if( nWideByte == 0 )
	//{
	//	return false;
	//}

	//int nUtfLen =
	//	MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,pGBKStr,-1,NULL, 0);
	//if( nUtfLen >0 && (nWideByte%3==0) &&
	//	(strLen - (2*nWideByte/3)) == (nUtfLen-1) )
	//{
	//	return true;
	//}

	//return false;
}


bool URL_UTF8_Decode(const wchar_t* url,wchar_t* decd)
{
	
	wchar_t buffer[4096];
	memset(buffer,0,sizeof(buffer));

	URL_GBK_Decode(url,buffer);
	wcscpy(buffer, url);
	return true;

	//char* pGBKStr = buffer;

	//int   nGbkLen = MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,pGBKStr,-1,NULL, 0);

	//if( nGbkLen >0 ) // UTF8 STR
	//{
	//	LPWSTR wStr = new WCHAR[nGbkLen];
	//	MultiByteToWideChar(CP_UTF8,0,pGBKStr, -1, wStr, nGbkLen);

	//	int astrLen     = WideCharToMultiByte(CP_ACP, 0, wStr, -1, NULL, 0, NULL, NULL);
	//	char* converted = new char[astrLen];
	//	WideCharToMultiByte(CP_ACP, 0, wStr, -1, converted, astrLen, NULL, NULL);
	//	strcpy(decd,converted);

	//	delete wStr;
	//	delete converted;

	//	return true;
	//}

	//return false;
}


bool URL_Decode(const wchar_t* url,wchar_t* pStr)
{
	int nwidebyte = 0;
	wchar_t* pgbkstr = NULL;

	wchar_t buffer[4096];
	memset(buffer,0,sizeof(buffer));

	URL_GBK_Decode(url,buffer);
	//pgbkstr    = buffer;
	//int strlen = wcslen(pgbkstr);

	//for( int i=0; i<strlen; i++)
	//{
	//	if( pgbkstr[i] < 0 )
	//	{
	//		nwidebyte++;
	//	}
	//}

	//int ngbklen =
	//	multibytetowidechar(cp_acp,mb_err_invalid_chars,pgbkstr,-1,null, 0);

	//if( ngbklen >0 && (strlen - (nwidebyte/2)) == (ngbklen-1) ) // gbk str
	//{
	//	strcpy(pstr,pgbkstr);
	//}

	//ngbklen = multibytetowidechar(cp_utf8,mb_err_invalid_chars,pgbkstr,-1,null, 0);
	//if( ngbklen >0 &&
	//	(nwidebyte%3==0) &&
	//	(strlen - (2*nwidebyte/3)) == (ngbklen-1))              // utf8 str
	//{
	//	lpwstr wstr = new wchar[ngbklen];
	//	multibytetowidechar(cp_utf8,0,pgbkstr, -1, wstr, ngbklen);

	//	int astrlen     = widechartomultibyte(cp_acp, 0, wstr, -1, null, 0, null, null);
	//	char* converted = new char[astrlen];
	//	widechartomultibyte(cp_acp, 0, wstr, -1, converted, astrlen, null, null);
	//	strcpy(pstr,converted);

	//	delete wstr;
	//	delete converted;

	//	return true;
	//}

	wcscpy(pStr,buffer);

	return true;
}