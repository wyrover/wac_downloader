/********************************************************************
	created:	2011/06/13
	created:	13:6:2011   10:25
	filename: 	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager\DownloadBase.cpp
	file path:	d:\Work\SVN\LiveBoot\Src\Code\LiveBoot Downloader\DownLoaderManager
	file base:	DownloadBase
	file ext:	cpp
	author:		tanyc
	
	purpose:	
*********************************************************************/
#include "stdafx.h"
#include "DownloadBase.h"
#include "DownloadManager.h"
#include "Shlwapi.h"
#include "Base64.hpp"
#pragma comment ( lib, "shlwapi.lib" )


// 下载数据保存的临时缓冲大小
#define TEMP_SAVE_BUFFER_SIZE		(10*NET_BUFFER_SIZE)

// 当下载的数据达到这个数的时候才保存到文件中
#define WRITE_TEMP_SAVE_MIN_BYTES	(TEMP_SAVE_BUFFER_SIZE - 2*NET_BUFFER_SIZE)

// 重试多少次， 每100毫秒左右检测一次是否被停止
int g_nRetryTimes = 1;

CDownloadBase::CDownloadBase()
{
	m_hThread					= NULL;
	m_TimeLastModified			= -1;
	m_csDownloadUrl				= _T("");
	m_csSaveFileName			= _T("");

	m_Proc_SaveDownloadInfo		= NULL;
	m_wSaveDownloadInfo_Param	= NULL;

	m_bSupportResume			= FALSE;
	m_nFileTotalSize			= -1;
	m_csReferer					= _T("");
	m_csUserAgent				= _T("Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)");
	m_csUsername				= _T("");
	m_csPassword				= _T("");
	m_csProtocolType			= "http";
	m_csServer					= _T("");
	m_csObject					= _T("");
	m_csFileName				= _T("");
	m_nPort						= DEFAULT_HTTP_PORT ;
	m_nIndex					= -1;
	m_csCookieFlag				= _T("");
	m_hfile						= NULL;
	ResetVar ();
	m_hEvtEndModule				= ::CreateEvent ( NULL, TRUE, FALSE, NULL );
	m_pDownloadMTR				= NULL;

	m_nSerialNum				= -1;
	m_csMD5_Web					= _T("");

}

void CDownloadBase::ResetVar()
{
	Clear_Thread_Handle ();
	m_nWillDownloadStartPos		= 0;
	m_nWillDownloadSize			= 0;
	m_nDownloadedSize			= 0;
	m_nTempSaveBytes			= 0;

	m_bDownloadSuccess			= FALSE;

	if ( HANDLE_IS_VALID ( m_hfile ) )
	{
		CLOSE_HANDLE(m_hfile);
	}
}

CDownloadBase::~CDownloadBase()
{
	StopDownload ();
	Clear_Thread_Handle ();
	CLOSE_HANDLE ( m_hEvtEndModule );
	m_hEvtEndModule = NULL;
	if ( HANDLE_IS_VALID ( m_hfile ) )
	{
		CLOSE_HANDLE(m_hfile);
	}
}

void CDownloadBase::StopDownload (DWORD dwTimeout)
{
	m_SocketClient.Disconnect ();
	if ( HANDLE_IS_VALID(m_hEvtEndModule) )
	{
		::SetEvent ( m_hEvtEndModule );
		WaitForThreadEnd ( m_hThread, dwTimeout );
		Clear_Thread_Handle();
	}
}

//
// 设置认证信息
//
void CDownloadBase::SetAuthorization ( LPCTSTR lpszUsername, LPCTSTR lpszPassword )
{
	if( lpszUsername != NULL )
	{
		m_csUsername	 = GET_SAFE_STRING(lpszUsername);
		m_csPassword	 = GET_SAFE_STRING(lpszPassword);
	}
	else
	{
		m_csUsername	 = _T("");
		m_csPassword	 = _T("");
	}
}

// 设置Referer
void CDownloadBase::SetReferer(LPCTSTR lpszReferer)
{
	if( lpszReferer != NULL )
		m_csReferer = lpszReferer;
	else
		m_csReferer = _T("");
}

// 设置UserAgent
void CDownloadBase::SetUserAgent(LPCTSTR lpszUserAgent)
{
	m_csUserAgent = lpszUserAgent;	
	if( m_csUserAgent.IsEmpty())
		m_csUserAgent = _T("Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)");
}

//
// 设置保存下载信息回调函数
//
void CDownloadBase::Set_SaveDownloadInfo_Callback ( FUNC_SaveDownloadInfo Proc_SaveDownloadInfo, WPARAM wParam )
{
	m_Proc_SaveDownloadInfo = Proc_SaveDownloadInfo;
	m_wSaveDownloadInfo_Param = wParam;
}

//
// 下载任务的线程函数
//
DWORD WINAPI ThreadProc_Download(LPVOID lpParameter)
{
	CDownloadBase *pDownloadPub = (CDownloadBase*)lpParameter;
	_ASSERT ( pDownloadPub );
	return pDownloadPub->ThreadProc_Download ();
}

BOOL CDownloadBase::ThreadProc_Download()
{
	BOOL bRet = FALSE;
	for ( int i=0; i<g_nRetryTimes; i++ )
	{
		CString szLog;
		szLog.Format(_T("DownloadOnce, try time: %d, thread seq: %d."), i, m_nIndex);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);
		if ( DownloadOnce () )
		{
			bRet = TRUE;
			break;
		}
		else
		{
			// fail
			if ( m_Proc_SaveDownloadInfo )
			{
				m_Proc_SaveDownloadInfo ( m_nIndex, 0, 0, m_wSaveDownloadInfo_Param );
			}
		}
		SLEEP_RETURN_Down ( 100 );
	}

	return DownloadEnd ( bRet );
}

BOOL CDownloadBase::DownloadOnce()
{
	// 打开文件
	if ( !OpenFileForSave () )
		return FALSE;

	// 连接到服务器
	if ( !m_SocketClient.Is_Connected () )
	{
		if ( !Connect () )
			return FALSE;
	}
	return TRUE;
}

//
// 创建线程下载文件
//
BOOL CDownloadBase::Download (
							 __int64 nWillDownloadStartPos,		// 要下载文件的开始位置
							 __int64 nWillDownloadSize,			// 本次需要下载的大小，-1表示一直下载到文件尾
							 __int64 nDownloadedSize			// 已下载的字节数，指完全写到文件中的字节数
							 )
{
	if ( nWillDownloadSize == 0 )
	{
		m_bDownloadSuccess = TRUE;
		return TRUE;
	}

	// 设置下载参数
	m_nWillDownloadStartPos	= nWillDownloadStartPos;
	Set_WillDownloadSize ( nWillDownloadSize );
	if ( m_nFileTotalSize > 0 && Get_WillDownloadSize() > m_nFileTotalSize )
	{
		Set_WillDownloadSize ( m_nFileTotalSize );
	}
	Set_DownloadedSize ( nDownloadedSize );


	// 创建一个下载线程
	DWORD dwThreadId = 0;
	m_hThread = CreateThread ( NULL, 0, ::ThreadProc_Download, LPVOID(this), 0, &dwThreadId );
	if ( !HANDLE_IS_VALID(m_hThread) )
	{
		CString szLog;
		szLog.Format(_T("Create sub download thread(%d) failed."), m_nIndex);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);

		return FALSE;
	}
	return TRUE;
}

//
// 下载结束
//
BOOL CDownloadBase::DownloadEnd(BOOL bRes)
{
	m_bDownloadSuccess = bRes;
	m_SocketClient.Disconnect ();
	if ( HANDLE_IS_VALID ( m_hfile ) )
	{
		CLOSE_HANDLE(m_hfile);
	}

	CString szLog;
	szLog.Format(_T("Sub download thread(%d) has finished, url: %s, result is: %s."), 
		m_nIndex,
		m_csDownloadUrl,
		bRes ? _T("successful") : _T("failed"));
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	return bRes;
}

BOOL CDownloadBase::Connect()
{
	CString szLog;
	if ( !HANDLE_IS_VALID(m_hEvtEndModule) )
		return FALSE;

	if ( m_csServer.IsEmpty() )
	{
		szLog.Format(_T("Sub Download thread(%d) connecting, server is empty."), m_nIndex);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);

		return FALSE;
	}
	m_SocketClient.SetEventOfEndModule ( m_hEvtEndModule );
	m_SocketClient.m_nIndex = m_nIndex;
	// 连接到服务器
	if ( !m_SocketClient.Connect ( m_csServer, m_nPort ) )
		return FALSE;

	szLog.Format(_T("Sub Download thread(%d) connect to server [%s:%d] successful."), m_nIndex, m_csServer, m_nPort);
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	return TRUE;
}

BOOL CDownloadBase::SetDownloadUrl(LPCTSTR lpszDownloadUrl)
{
	if ( !lpszDownloadUrl ) return FALSE;
	m_csDownloadUrl = lpszDownloadUrl;

	// 检验要下载的URL是否为空
	m_csDownloadUrl.TrimLeft();
	m_csDownloadUrl.TrimRight();
	if( m_csDownloadUrl.IsEmpty() )
		return FALSE;

	// 检验要下载的URL是否有效
	if ( !ParseURLDIY(m_csDownloadUrl, m_csServer, m_csObject, m_nPort, m_csProtocolType))
	{
		CString szLog;
		szLog.Format(_T("Sub Download thread(%d), failed to parse the url [%s]."), m_nIndex, m_csDownloadUrl);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);

		return FALSE;
	}
	return TRUE;
}

//
// 从服务器接收数据并保存到文件中
//
BOOL CDownloadBase::RecvDataAndSaveToFile(CCustomizeSocket &SocketClient,char *szTailData/*=NULL*/, int nTailSize/*=0*/)
{
	__int64 nDownloadedSize = Get_DownloadedSize();
	if ( szTailData && nTailSize > 0 )
	{
		nDownloadedSize = SaveDataToFile ( szTailData, nTailSize );
		if ( nDownloadedSize < 0 )
		{
			return FALSE;
		}
	}

	char szRecvBuf[NET_BUFFER_SIZE] = {0};
	char* szTempSaveBuf = new char[TEMP_SAVE_BUFFER_SIZE];
	if ( !szTempSaveBuf )
	{
		CString szLog;
		szLog.Format(_T("Sub Download thread(%d), Allocate temp memory failed."), m_nIndex);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
		return FALSE;
	}
	m_nTempSaveBytes = 0;

	while ( TRUE )
	{
		BOOL bDownloadFinished = FALSE;
		int nReadSize = 0;
		int nTempSaveBytes = Get_TempSaveBytes ();
		__int64 nRecvTotalBytes = nDownloadedSize+nTempSaveBytes;	// 保存在文件中的字节数加上临时缓冲中的字节数就是总共接收到字节数
		__int64 nWillDownloadSize = Get_WillDownloadSize ();
		if	(
			// 从字节数判断，本次下载已经完成了。
			( nWillDownloadSize > 0 && nRecvTotalBytes >= nWillDownloadSize )
			||
			// 模块结束事件有信号，要结束了。
			( ::WaitForSingleObject ( m_hEvtEndModule, 1 ) == WAIT_OBJECT_0 )
			)
		{
			bDownloadFinished = TRUE;
		}
		else
		{
			// 这次将要下载的字节数
			__int64 nRecvBytesThisTimes = sizeof(szRecvBuf);
			if ( nWillDownloadSize > 0 )
				nRecvBytesThisTimes = nWillDownloadSize - nRecvTotalBytes;
			_ASSERT ( nRecvBytesThisTimes >= 0 );
			nRecvBytesThisTimes = MIN ( nRecvBytesThisTimes, sizeof(szRecvBuf) );
			nReadSize = SocketClient.Receive ( szRecvBuf, (int)nRecvBytesThisTimes );
			// 没有读到数据
			if ( nReadSize <= 0 )
			{
				if ( nWillDownloadSize <= 0 )
				{
					// 认为下载已经完成
					bDownloadFinished = TRUE;
				}
				else
				{
					// 用户停止了下载，socket事件被关闭
					bDownloadFinished = TRUE;
				}
			}
			else
			{
			}
		}

		// 先将数据保存到临时缓冲中
		if ( nReadSize > 0 )
		{
			// 需要放入缓存的字节数
			nReadSize = MIN ( nReadSize, TEMP_SAVE_BUFFER_SIZE - nTempSaveBytes );

			memcpy ( szTempSaveBuf+nTempSaveBytes, szRecvBuf, nReadSize );
			nTempSaveBytes += nReadSize;
			_ASSERT ( nTempSaveBytes < TEMP_SAVE_BUFFER_SIZE );
		}
		// 当下载已完成或者收到的数据超过一定数量时才保存到文件中
		if ( bDownloadFinished || nTempSaveBytes >= WRITE_TEMP_SAVE_MIN_BYTES )
		{
			// 保存文件失败，下载也应该终止
			nDownloadedSize = SaveDataToFile ( szTempSaveBuf, nTempSaveBytes );
			if ( nDownloadedSize < 0 )
			{
				break;
			}
			nTempSaveBytes = 0;
		}

		Set_TempSaveBytes ( nTempSaveBytes );
		if(IsCallBackEnable())
		{
			Notify ( m_nIndex, GetSerialNum(), NOTIFY_TYPE_HAS_GOT_THREAD_CACHE_SIZE, (LPVOID)nTempSaveBytes );
		}

		if ( bDownloadFinished )
		{
			break;
		}
	}

	if ( szTempSaveBuf ) delete[] szTempSaveBuf;
	szTempSaveBuf = NULL;

	BOOL bRes = FALSE;
	__int64 nWillDownloadSize = Get_WillDownloadSize ();

	//如果下载数据够了，返回成功
	if ( nWillDownloadSize != -1 )
	{
		if ( nDownloadedSize >= nWillDownloadSize )
		{
			bRes = TRUE;
		}
	}
	//nWillDownloadSize == -1, 不知大小
	else if ( nDownloadedSize > 0 )
	{
		bRes = TRUE;
	}
	//if(!bRes)
	//{
	//	int nTest = 0;
	//}
	return bRes;
}

//返回该线程下载已下载的数据量，失败返回-1
__int64 CDownloadBase::SaveDataToFile(char *data, int size)
{
	_ASSERT ( HANDLE_IS_VALID ( m_hfile ) );
	if ( !data || size < 0 ) return -1;
	__int64 nDownloadedSize = -1;

	// 保存下载的数据
	BOOL bRet = TRUE;
	DWORD dwNumberOfBytesWritten = 0;
	if(::WriteFile(m_hfile, data, size, &dwNumberOfBytesWritten, NULL))
	{
		//success
	}
	else
	{
		bRet = FALSE;
	}

	if ( !bRet )
	{
		CString szLog;
		szLog.Format(_T("Sub Download thread(%d), write data to local file failed."), m_nIndex);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);
	}
	else
	{
		nDownloadedSize = Get_DownloadedSize();
		nDownloadedSize += size;
		Set_DownloadedSize ( nDownloadedSize );
		if ( m_Proc_SaveDownloadInfo )
		{
			m_Proc_SaveDownloadInfo ( m_nIndex, nDownloadedSize, size, m_wSaveDownloadInfo_Param );
		}
	}
	if ( !bRet ) return -1;
	return nDownloadedSize;
}

BOOL CDownloadBase::GetRemoteSiteInfo_Pro()
{
	return TRUE;
}

BOOL CDownloadBase::GetRemoteSiteInfo ()
{
	for ( int i = 0; i < 3; i++ )
	{
		if ( GetRemoteSiteInfo_Pro () )
		{
			m_SocketClient.Disconnect ();
			return TRUE;
		}
	}

	m_SocketClient.Disconnect ();
	return FALSE;
}

CString CDownloadBase::GetRemoteFileName()
{
	int nPos = m_csObject.ReverseFind ( '/' );
	if ( nPos <= 0 ) return m_csObject;
	return m_csObject.Mid ( nPos+1 );
}

//const char* g_szBase64TAB = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// BASE64编码
int Base64Encode(LPCTSTR lpszEncoding, CString &strEncoded)
{
	//widechartomultibyte
	int astrLen     = WideCharToMultiByte(CP_ACP, 0, lpszEncoding, -1, NULL, 0, NULL, NULL);
	char* converted = new char[astrLen];
	WideCharToMultiByte(CP_ACP, 0, lpszEncoding, -1, converted, astrLen, NULL, NULL);
	delete converted;
	converted = NULL;
	std::string encodeStr = converted;
	std::string outputStr = Base64::encode(encodeStr);

	int rv_size = MultiByteToWideChar(CP_UTF8, 0, outputStr.c_str(),outputStr.size(), NULL, 0);
	wchar_t* woutput = new wchar_t[rv_size];
	memset(woutput, 0, sizeof(woutput));
	MultiByteToWideChar(CP_UTF8, 0, outputStr.c_str(),outputStr.size(), woutput, rv_size);
	strEncoded = woutput;
	delete woutput;
	woutput = NULL;

	return strEncoded.GetLength();

	//UINT	m_nBase64Mask[]= { 0, 1, 3, 7, 15, 31, 63, 127, 255 };
	//int nDigit;
	//int nNumBits = 6;
	//int nIndex = 0;
	//int nInputSize;

	//strEncoded = _T( "" );
	//if( lpszEncoding == NULL )
	//	return 0;

	//if( ( nInputSize = lstrlen(lpszEncoding) ) == 0 )
	//	return 0;

	////widechartomultibyte
	//int astrLen     = WideCharToMultiByte(CP_ACP, 0, lpszEncoding, -1, NULL, 0, NULL, NULL);
	//char* converted = new char[astrLen];
	//WideCharToMultiByte(CP_ACP, 0, lpszEncoding, -1, converted, astrLen, NULL, NULL);
	//std::string outputStr;

	//int nBitsRemaining = 0;
	//long lBitsStorage	=0;
	//long lScratch		=0;
	//int nBits;
	//UCHAR c;

	//while( nNumBits > 0 )
	//{
	//	while( ( nBitsRemaining < nNumBits ) &&  ( nIndex < nInputSize ) ) 
	//	{
	//		c = converted[ nIndex++ ];
	//		lBitsStorage <<= 8;
	//		lBitsStorage |= (c & 0xff);
	//		nBitsRemaining += 8;
	//	}
	//	if( nBitsRemaining < nNumBits ) 
	//	{
	//		lScratch = lBitsStorage << ( nNumBits - nBitsRemaining );
	//		nBits    = nBitsRemaining;
	//		nBitsRemaining = 0;
	//	}	 
	//	else 
	//	{
	//		lScratch = lBitsStorage >> ( nBitsRemaining - nNumBits );
	//		nBits	 = nNumBits;
	//		nBitsRemaining -= nNumBits;
	//	}
	//	nDigit = (int)(lScratch & m_nBase64Mask[nNumBits]);
	//	nNumBits = nBits;
	//	if( nNumBits <=0 )
	//		break;

	//	outputStr += g_szBase64TAB[ nDigit ];
	//}

	//int rv_size = MultiByteToWideChar(CP_UTF8, 0, outputStr.c_str(),outputStr.size(), NULL, 0);
	//wchar_t* woutput = new wchar_t[rv_size];
	//memset(woutput, 0, sizeof(woutput));
	//MultiByteToWideChar(CP_UTF8, 0, outputStr.c_str(),outputStr.size(), woutput, rv_size);
	//strEncoded = woutput;

	//// Pad with '=' as per RFC 1521
	//while( strEncoded.GetLength() % 4 != 0 )
	//	strEncoded += '=';

	//delete woutput;
	//delete converted;

	//return strEncoded.GetLength();

}

// STATIC BASE64解码
int Base64Decode(LPCTSTR lpszDecoding, CString &strDecoded)
{

	//widechartomultibyte
	int astrLen     = WideCharToMultiByte(CP_ACP, 0, lpszDecoding, -1, NULL, 0, NULL, NULL);
	char* converted = new char[astrLen];
	WideCharToMultiByte(CP_ACP, 0, lpszDecoding, -1, converted, astrLen, NULL, NULL);
	std::string decStr = converted;
	delete []converted;
	converted = NULL;

	std::string dec = Base64::decode(decStr);

	std::string strRetVal = "";
	int size = dec.size();
	for(int i = 0 ; i < size; i++)
	{
		char buf[3] = {0};
		sprintf(buf, "%02X", (unsigned char)dec.at(i));
		strRetVal += buf;
	}

	int rv_size = MultiByteToWideChar(CP_UTF8, 0, strRetVal.c_str(),strRetVal.size(), NULL, 0);
	wchar_t* woutput = new wchar_t[rv_size+1];
	memset(woutput, 0, sizeof(wchar_t)*(rv_size+1));
	MultiByteToWideChar(CP_UTF8, 0, strRetVal.c_str(),strRetVal.size(), woutput, rv_size);

	strDecoded = woutput;

	delete []woutput;
	woutput = NULL;
	return strDecoded.GetLength();


	//int nIndex =0;
	//int nDigit;
	//int nDecode[ 256 ];
	//int nSize;
	//int nNumBits = 6;
	//int i;

	//if( lpszDecoding == NULL )
	//	return 0;

	//if( ( nSize = lstrlen(lpszDecoding) ) == 0 )
	//	return 0;

	//// Build Decode Table
	//for( i = 0; i < 256; i++ ) 
	//	nDecode[i] = -2; // Illegal digit
	//for( i=0; i < 64; i++ )
	//{
	//	nDecode[ g_szBase64TAB[ i ] ] = i;
	//	nDecode[ '=' ] = -1; 
	//}

	////widechartomultibyte
	//int astrLen     = WideCharToMultiByte(CP_ACP, 0, lpszDecoding, -1, NULL, 0, NULL, NULL);
	//char* converted = new char[astrLen];
	//WideCharToMultiByte(CP_ACP, 0, lpszDecoding, -1, converted, astrLen, NULL, NULL);
	//std::string outputStr;

	//// Clear the output buffer
	//strDecoded = _T("");
	//long lBitsStorage  =0;
	//int nBitsRemaining = 0;
	//int nScratch = 0;
	//UCHAR c;

	//// Decode the Input
	//for( nIndex = 0, i = 0; nIndex < nSize; nIndex++ )
	//{
	//	c = converted[ nIndex ];

	//	// 忽略所有不合法的字符
	//	if( c> 0x7F)
	//		continue;

	//	nDigit = nDecode[c];
	//	if( nDigit >= 0 ) 
	//	{
	//		lBitsStorage = (lBitsStorage << nNumBits) | (nDigit & 0x3F);
	//		nBitsRemaining += nNumBits;
	//		while( nBitsRemaining > 7 ) 
	//		{
	//			nScratch = lBitsStorage >> (nBitsRemaining - 8);
	//			outputStr += (char)(nScratch & 0xFF);
	//			i++;
	//			nBitsRemaining -= 8;
	//		}
	//	}
	//}	
	//std::string strRetVal = "";
	//int size = outputStr.size();
	//for(int i = 0 ; i < size; i++)
	//{
	//	char buf[3] = {0};
	//	sprintf(buf, "%02X", (unsigned char)outputStr.at(i));
	//	strRetVal += buf;
	//}

	//int rv_size = MultiByteToWideChar(CP_UTF8, 0, strRetVal.c_str(),strRetVal.size(), NULL, 0);
	//wchar_t* woutput = new wchar_t[rv_size];
	//memset(woutput, 0, sizeof(woutput));
	//MultiByteToWideChar(CP_UTF8, 0, strRetVal.c_str(),strRetVal.size(), woutput, rv_size);

	//strDecoded = woutput;

	//delete woutput;
	//delete converted;
	//return strDecoded.GetLength();
}

void url_Encode(std::wstring& str)
{
	std::wstring output;
	int size = str.length();
	for(int i = 0; i < size; ++i)
	{
		if(str[i] == _T(' '))
		{
			output += _T("%20");
		}
		else if(str[i] == _T('+'))
		{
			output += _T("%2B");
		}
		else
		{
			output += str[i];
		}
	}

	str = output;
}

void url_Decode(CString& str)
{
	str.Replace(_T("%20"), _T(" "));
	str.Replace(_T("%2B"), _T("+"));
}
//
// 从URL里面拆分出Server、Object、协议类型等信息，其中 Object 里的值是区分大小写的，否则有些网站可能会下载不了
//
BOOL ParseURLDIY(LPCTSTR lpszURL, CString &strServer, CString &strObject,USHORT& nPort, CString &csProtocolType)
{
	if ( !lpszURL || wcslen(lpszURL) < 1 ) return FALSE;
	CString csURL_Lower(lpszURL);
	csURL_Lower.TrimLeft();
	csURL_Lower.TrimRight();
	csURL_Lower.Replace ( _T("\\"), _T("/") );
	CString csURL = csURL_Lower;
	csURL_Lower.MakeLower ();

	// 清除数据
	strServer = _T("");
	strObject = _T("");
	nPort	  = 0;

	int nPos = csURL_Lower.Find(_T("://"));
	if( nPos == -1 )
	{
		csURL_Lower.Insert ( 0, _T("http://") );
		csURL.Insert ( 0, _T("http://") );
		nPos = 4;
	}
	csProtocolType = csURL_Lower.Left ( nPos );

	csURL_Lower = csURL_Lower.Mid( csProtocolType.GetLength()+3 );
	csURL = csURL.Mid( csProtocolType.GetLength()+3 );
	nPos = csURL_Lower.Find('/');
	if ( nPos == -1 )
		return FALSE;

	strObject = csURL.Mid(nPos);
	//转义+和空格
	std::wstring object = strObject.GetBuffer(strObject.GetLength());
	url_Encode(object);
	strObject = object.c_str();

	CString csServerAndPort = csURL_Lower.Left(nPos);

	// 查找是否有端口号，站点服务器域名一般用小写
	nPos = csServerAndPort.Find(_T(":"));
	if( nPos == -1 )
	{
		strServer	= csServerAndPort;
		nPort		= DEFAULT_HTTP_PORT;
		if ( csProtocolType == _T("ftp") )
			nPort	= DEFAULT_FTP_PORT;
	}
	else
	{
		strServer = csServerAndPort.Left( nPos );
		CString str = csServerAndPort.Mid( nPos+1 );
		nPort	  = (USHORT)_ttoi( str.GetBuffer(str.GetLength()) );
		str.ReleaseBuffer();
	}
	return TRUE;
}

void CDownloadBase::Set_DownloadedSize(__int64 nDownloadedSize)
{
	m_CSFor_DownloadedSize.Lock ();
	m_nDownloadedSize = nDownloadedSize;
	m_CSFor_DownloadedSize.Unlock ();
}

__int64 CDownloadBase::Get_DownloadedSize()
{
	__int64 nDownloadedSize = 0;

	m_CSFor_DownloadedSize.Lock ();
	nDownloadedSize = m_nDownloadedSize;
	m_CSFor_DownloadedSize.Unlock ();

	return nDownloadedSize;
}

void CDownloadBase::Set_TempSaveBytes(int nTempSaveBytes)
{
	m_CSFor_TempSaveBytes.Lock ();
	m_nTempSaveBytes = nTempSaveBytes;
	m_CSFor_TempSaveBytes.Unlock ();
}

int CDownloadBase::Get_TempSaveBytes()
{
	int nTempSaveBytes = 0;
	m_CSFor_TempSaveBytes.Lock ();
	nTempSaveBytes = m_nTempSaveBytes;
	m_CSFor_TempSaveBytes.Unlock ();

	return nTempSaveBytes;
}

void CDownloadBase::Set_WillDownloadSize(__int64 nWillDownloadSize)
{
	m_CSFor_WillDownloadSize.Lock ();
	m_nWillDownloadSize = nWillDownloadSize;
	m_CSFor_WillDownloadSize.Unlock ();

	Notify ( m_nIndex, GetSerialNum(), NOTIFY_TYPE_HAS_GOT_THREAD_WILL_DOWNLOAD_SIZE, (LPVOID)m_nWillDownloadSize );
}

__int64 CDownloadBase::Get_WillDownloadSize()
{
	__int64 nWillDownloadSize = 0;
	m_CSFor_WillDownloadSize.Lock ();
	nWillDownloadSize = m_nWillDownloadSize;
	m_CSFor_WillDownloadSize.Unlock ();

	return nWillDownloadSize;
}

BOOL CDownloadBase::SetSaveFileName(LPCTSTR lpszSaveFileName)
{
	if ( !lpszSaveFileName ) return FALSE;
	m_csSaveFileName = lpszSaveFileName;
	if ( m_csSaveFileName.IsEmpty() )
		return FALSE;
	return TRUE;
}

//
// 获取尚未下载的字节数，写到文件中的和临时缓冲里的都算是已经下载的
//
__int64 CDownloadBase::GetUndownloadBytes()
{
	// 总共需要下载的字节数减去已经下载的字节数
	return Get_WillDownloadSize () - ( Get_DownloadedSize () + Get_TempSaveBytes () );
}

BOOL CDownloadBase::OpenFileForSave()
{
	_ASSERT ( !m_csSaveFileName.IsEmpty() );

	// 先关闭文件
	if ( HANDLE_IS_VALID ( m_hfile ) )
	{
		CLOSE_HANDLE(m_hfile);
	}
	BOOL bRet = FALSE;

	// 测试文件是否存在
	m_hfile = ::CreateFile(m_csSaveFileName,				// file to open
		GENERIC_READ | GENERIC_WRITE,		// open for read write
		FILE_SHARE_READ | FILE_SHARE_WRITE,	// share for reading
		NULL,								// default security
		OPEN_EXISTING,						// existing file only
		FILE_ATTRIBUTE_NORMAL,				// normal file
		NULL);								// no attr. template
	if (m_hfile == INVALID_HANDLE_VALUE) 
	{ 
		// 文件不存在，创建
		m_hfile = ::CreateFile(m_csSaveFileName,			// file to open
			GENERIC_READ | GENERIC_WRITE,		// open for read write
			FILE_SHARE_READ | FILE_SHARE_WRITE,	// share for reading
			NULL,								// default security
			CREATE_NEW,							// create new file
			FILE_ATTRIBUTE_NORMAL,				// normal file
			NULL);								// no attr. template	
	}

	if (m_hfile == INVALID_HANDLE_VALUE)
	{
		bRet = FALSE;
	}
	else
	{
		__int64 nWillDownloadStartPos = Get_WillDownloadStartPos ();
		nWillDownloadStartPos += Get_DownloadedSize ();

		LARGE_INTEGER li;
		li.QuadPart = nWillDownloadStartPos;
		li.LowPart = ::SetFilePointer (m_hfile, li.LowPart, &li.HighPart, FILE_BEGIN);
		if (li.LowPart != INVALID_SET_FILE_POINTER && li.QuadPart == nWillDownloadStartPos)
		{
			bRet = TRUE;
		}
		else
		{
			bRet = FALSE;
		}
	}
	return bRet;
}

void CDownloadBase::Clear_Thread_Handle()
{
	CLOSE_HANDLE ( m_hThread );
}


//
// 获取下载对象的文件名（带扩展名的）
//
CString CDownloadBase::GetDownloadObjectFileName(CString *pcsExtensionName)
{
	_ASSERT ( !m_csObject.IsEmpty() );
	CString csOnlyPath, csOnlyFileName, csExtensionName;
	if ( !PartPathAndFileAndExtensionName ( m_csObject, &csOnlyPath, &csOnlyFileName, &csExtensionName ) )
		return _T("");
	if ( pcsExtensionName ) *pcsExtensionName = csExtensionName;
	if ( !csExtensionName.IsEmpty() )
	{
		csOnlyFileName += ".";
		csOnlyFileName += csExtensionName;
	}
	url_Decode(csOnlyFileName);
	return csOnlyFileName;
}

CString CDownloadBase::GetRefererFromURL()
{
	CString temp = m_csDownloadUrl;
	int nPos = temp.Find('?');
	if(nPos > 0)
	{
		temp = temp.Left(nPos);
	}
	nPos = temp.ReverseFind ( '/' );
	if ( nPos < 0 ) return _T("");
	return temp.Left ( nPos );
}

BOOL CDownloadBase::ThreadIsRunning()
{
	if ( !HANDLE_IS_VALID(m_hThread) )
		return FALSE;
	return ( WaitForSingleObject ( m_hThread, 0 ) != WAIT_OBJECT_0 );
}

int CDownloadBase::SetTaskID(int nPos)
{
	m_nSerialNum = nPos;

	return 1;
}

int CDownloadBase::GetSerialNum()
{
	return m_nSerialNum;
}

BOOL CDownloadBase::IsCallBackEnable()
{
	BOOL bEnable = TRUE;
	if(m_pDownloadMTR)
	{
		CDownloadManager *pMtr = (CDownloadManager*)m_pDownloadMTR;
		if(pMtr)
		{
			bEnable = pMtr->IsCallBackEnable();
		}
	}
	return bEnable;
}


bool CDownloadBase::SetObserver( IObserver *pObserver )
{
	m_pObserver = pObserver;
	return true;
}
void CDownloadBase::Notify(t_DownloadNotifyPara* pdlntfData, WPARAM wParam)
{
	if(m_pObserver)
	{
		m_pObserver->OnNotify(pdlntfData, wParam);
	}
}
void CDownloadBase::Notify(int nIndex, int nSerialNum, UINT nNotityType, LPVOID lpNotifyData)
{
	t_DownloadNotifyPara DownloadNotifyPara = {0};
	DownloadNotifyPara.nIndex = nIndex;
	DownloadNotifyPara.nNotityType = nNotityType;
	DownloadNotifyPara.lpNotifyData = lpNotifyData;
	DownloadNotifyPara.nSerialNum = nSerialNum;
	Notify(&DownloadNotifyPara, NULL);
}