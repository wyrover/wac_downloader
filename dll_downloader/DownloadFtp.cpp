// DownloadFtp.cpp: implementation of the CDownloadFtp class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DownloadFtp.h"

CDownloadFtp::CDownloadFtp()
{
}

CDownloadFtp::~CDownloadFtp()
{
}

//
// 创建一个 FTP 的数据通道连接
//
BOOL CDownloadFtp::CreateFTPDataConnect(CCustomizeSocket &SocketClient)
{
	_ASSERT ( SocketClient.m_socket == INVALID_SOCKET );
	// 设被动模式
	if ( !m_SocketClient.SendString ( _T("PASV\r\n") ) ) return FALSE;
	CString csResponseStr;
	if ( !m_SocketClient.GetResponse ( 227, &csResponseStr ) )
		return FALSE;
	CString csPasvIP;
	USHORT uPasvPort = 0;
	if ( !m_SocketClient.GetIPAndPortByPasvString ( csResponseStr, csPasvIP, uPasvPort ) )
		return FALSE;

	// 创建数据通道连接
	if ( !SocketClient.Connect ( csPasvIP, uPasvPort ) )
		return FALSE;

	return TRUE;
}

//
// 连接需要下载的服务器
//
BOOL CDownloadFtp::Connect ()
{
	if ( !CDownloadBase::Connect () )
		return FALSE;

	SLEEP_RETURN_Down ( 0 );
	int nResponseCode = m_SocketClient.GetResponse ();
	if ( nResponseCode != 220 )
	{
		CString szLog;
		szLog.Format(_T("(%d) Connect to [%s:%u] failed."), m_nIndex, m_csServer, m_nPort);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);
	}

	// 登录
	SLEEP_RETURN_Down ( 0 );
	if ( m_csUsername.IsEmpty () )
		m_csUsername = "anonymous";
	if ( !m_SocketClient.SendString ( _T("USER %s\r\n"), m_csUsername ) ) return FALSE;
	nResponseCode = m_SocketClient.GetResponse ();

	// 需要密码
	SLEEP_RETURN_Down ( 0 );
	if ( nResponseCode == 331 )
	{
		if ( !m_SocketClient.SendString ( _T("PASS %s\r\n"), m_csPassword ) ) return FALSE;
		nResponseCode = m_SocketClient.GetResponse ();
	}

	// 登录失败
	if ( nResponseCode != 230 )
	{
		CString szLog;
		szLog.Format(_T("(%d) Login failed."), m_nIndex);
		__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_ERROR);

		return FALSE;
	}

	CString szLog;
	szLog.Format(_T("(%d) Welcome message：--------------------------\r\n%s\r\n."), m_nIndex, m_SocketClient.GetResponseHistoryString());
	__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_TRACE);

	m_SocketClient.ClearResponseHistoryString ();
	return TRUE;
}

BOOL CDownloadFtp::DownloadOnce()
{
	// 不需要下载了
	__int64 nWillDownloadSize = Get_WillDownloadSize();				// 本次应该下载的字节数
	__int64 nDownloadedSize = Get_DownloadedSize ();				// 已下载字节数
	if ( nWillDownloadSize > 0 && nDownloadedSize >= nWillDownloadSize )
		return DownloadEnd(TRUE);

	if ( !CDownloadBase::DownloadOnce () )
		return DownloadEnd(FALSE);

	// 设置接收数据类型为 二进制模式
	if ( !m_SocketClient.SendString ( _T("TYPE I\r\n") ) )
		return DownloadEnd(FALSE);
	if ( !m_SocketClient.GetResponse ( 200 ) )
		return DownloadEnd(FALSE);
	SLEEP_RETURN_Down ( 0 );

	// 创建数据通道连接
	CCustomizeSocket SocketClient;
	SocketClient.SetEventOfEndModule ( m_hEvtEndModule );
	SocketClient.m_nIndex = m_nIndex;
	if ( !CreateFTPDataConnect ( SocketClient ) )
		return DownloadEnd(FALSE);
	SLEEP_RETURN_Down ( 0 );

	// 设定下载文件的起始位置
	__int64 nWillDownloadStartPos = Get_WillDownloadStartPos ();	// 开始位置
	if ( !m_SocketClient.SendString ( _T("REST %I64d\r\n"), nWillDownloadStartPos+nDownloadedSize ) )
		return DownloadEnd(FALSE);
	if ( !m_SocketClient.GetResponse ( 350 ) )
		return DownloadEnd(FALSE);

	// 提交下载文件的请求
	if ( !m_SocketClient.SendString ( _T("RETR %s\r\n"), m_csObject ) )
		return DownloadEnd(FALSE);
	if ( !m_SocketClient.GetResponse ( 150 ) )
		return DownloadEnd(FALSE);
	SLEEP_RETURN_Down ( 0 );

	// 从数据通道读取数据，并保存到文件中
	BOOL bRet = RecvDataAndSaveToFile(SocketClient);
	SocketClient.Disconnect ();
	return DownloadEnd ( bRet );

}

//
// 获取远程站点信息，如：是否支持断点续传、要下载的文件大小和创建时间等
//
BOOL CDownloadFtp::GetRemoteSiteInfo_Pro()
{
	BOOL bRet = FALSE;
	CCustomizeSocket SocketClient;
	CString csReadData;
	char szRecvBuf[NET_BUFFER_SIZE] = {0};
	int nReadSize = 0;
	CString csOneLine;
	int i;

	SocketClient.m_nIndex = m_nIndex;
	if ( !HANDLE_IS_VALID(m_hEvtEndModule) )
		goto Finished;
	SocketClient.SetEventOfEndModule ( m_hEvtEndModule );

	if ( !CDownloadBase::GetRemoteSiteInfo_Pro() )
		goto Finished;

	if ( !m_SocketClient.Is_Connected () && !Connect () )
		goto Finished;
	SLEEP_RETURN ( 0 );

	// 设置接收数据类型为 ASCII
	if ( !m_SocketClient.SendString ( _T("TYPE A\r\n") ) ) goto Finished;
	if ( !m_SocketClient.GetResponse ( 200 ) )
		goto Finished;
	SLEEP_RETURN ( 0 );

	// 判断是否支持断点续传
	if ( !m_SocketClient.SendString ( _T("REST 1\r\n") ) ) goto Finished;
	m_bSupportResume = ( m_SocketClient.GetResponse () == 350 );
	ATLTRACE ( "是否支持断点续传：%s\n", m_bSupportResume ? "支持":"不支持" );
	if ( m_bSupportResume )
	{
		if ( !m_SocketClient.SendString ( _T("REST 0\r\n") ) ) goto Finished;
		//VERIFY ( m_SocketClient.GetResponse ( 350 ) );
	}
	SLEEP_RETURN ( 0 );

	// 创建数据通道连接
	if ( !CreateFTPDataConnect ( SocketClient ) )
		goto Finished;
	// 发送列举文件命令
	if ( !m_SocketClient.SendString ( _T("LIST %s\r\n"), m_csObject ) )
		goto Finished;
	if ( !m_SocketClient.GetResponse ( 150 ) )
		goto Finished;
	SLEEP_RETURN ( 0 );

	// 从数据通道读取文件列表信息，直到命令通道返回 "226" 回应码，注意：这里接收数据用非阻塞型的。
	for ( i=0; ;i++ )
	{
		memset ( szRecvBuf, 0, sizeof(szRecvBuf) );
		nReadSize = SocketClient.Receive ( szRecvBuf, sizeof(szRecvBuf), FALSE );
		if ( nReadSize < 0 )
		{
			CString szLog;
			szLog.Format(_T("(%d) Receive file list info failed."), m_nIndex);
			__LOG_LOG__( szLog.GetBuffer(szLog.GetLength()), L_LOGSECTION_MODULE, L_LOGLEVEL_WARNING);

			break;
		}
		csReadData += szRecvBuf;
		int nResponseCode = m_SocketClient.GetResponse ( (CString*)NULL, FALSE );
		if ( nResponseCode == -1 ) goto Finished;
		else if ( nResponseCode == 0 )
		{
			SLEEP_RETURN ( 100 );
		}
		else if ( nResponseCode == 226 )
		{
			break;
		}
		else goto Finished;
		SLEEP_RETURN ( 0 );
	}

	for ( i=0; !csReadData.IsEmpty() ; i++ )
	{
		csOneLine = GetOneLine ( csReadData );
		csReadData.ReleaseBuffer();
		if ( !csOneLine.IsEmpty() )
		{
			ParseFileInfoStr ( csOneLine );
		}
	}
	bRet = TRUE;

Finished:
	return bRet;
}

//
// 从 "-rw-rw-rw-   1 user     group    37979686 Mar  9 13:39 FTP-Callgle 1.4.0_20060309.cab" 字符串中分析
// 出文件信息
//
void CDownloadFtp::ParseFileInfoStr(CString &csFileInfoStr)
{
	csFileInfoStr.TrimLeft (); csFileInfoStr.TrimRight ();
	if ( csFileInfoStr.IsEmpty() ) return;
	BOOL bLastCharIsSpace = ( csFileInfoStr[0]==' ' );
	int nSpaceCount = 0;
	CString csFileTime1, csFileTime2, csFileTime3;
	CString csNodeStr;
	CString csFileName;
	for ( int i=0; i<csFileInfoStr.GetLength(); i++ )
	{
		if ( csFileInfoStr[i]==' ' )
		{
			if ( !bLastCharIsSpace )
			{
				csNodeStr = csFileInfoStr.Mid ( i );
				csNodeStr.TrimLeft (); csNodeStr.TrimRight ();
				nSpaceCount ++;
				int nFindPos = csNodeStr.Find ( _T(" "), 0 );
				if ( nFindPos < 0 ) nFindPos = csNodeStr.GetLength() - 1;
				csNodeStr = csNodeStr.Left ( nFindPos );
				// 文件大小
				if ( nSpaceCount == 4 )
				{
					if ( m_nIndex == -1 )	// 主线程才需要获取文件大小的信息
					{
						m_nFileTotalSize = (__int64)_wtoi64(csNodeStr);
						//////////////////////////////////////////////////////////////////////////
						if(IsCallBackEnable())
						{
							int nSerialNum = GetSerialNum();
							Notify ( m_nIndex, nSerialNum, NOTIFY_TYPE_HAS_GOT_REMOTE_FILESIZE, (LPVOID)m_nFileTotalSize );
						}
						//////////////////////////////////////////////////////////////////////////
						__int64 nWillDownloadStartPos = Get_WillDownloadStartPos ();	// 开始位置
						__int64 nWillDownloadSize = Get_WillDownloadSize();				// 本次应该下载的字节数
						__int64 nDownloadedSize = Get_DownloadedSize ();				// 已下载字节数
						if ( m_nFileTotalSize > 0 && nWillDownloadSize-nDownloadedSize > m_nFileTotalSize )
							Set_WillDownloadSize ( m_nFileTotalSize-nDownloadedSize );
					}
				}
				// 文件时间第一节
				else if ( nSpaceCount == 5 )
				{
					csFileTime1 = csNodeStr;
				}
				// 文件时间第二节
				else if ( nSpaceCount == 6 )
				{
					csFileTime2 = csNodeStr;
				}
				// 文件时间第三节
				else if ( nSpaceCount == 7 )
				{
					csFileTime3 = csNodeStr;
				}
				else if ( nSpaceCount > 7 )
				{
					csFileName = csFileInfoStr.Mid ( i );
					csFileName.TrimLeft(); csFileName.TrimRight();
					break;
				}
			}
			bLastCharIsSpace = TRUE;
		}
		else
		{
			bLastCharIsSpace = FALSE;
		}
	}

	GetFileTimeInfo ( csFileTime1, csFileTime2, csFileTime3 );
}

void CDownloadFtp::GetFileTimeInfo(CString csFileTime1, CString csFileTime2, CString csFileTime3)
{
	if ( csFileTime3.IsEmpty() ) return;
	CString csYear, csMonth, csDay, csHour, csMinute, csSecond;
	int nMonth = GetMouthByShortStr ( csFileTime1 );
	_ASSERT ( nMonth >= 1 && nMonth <= 12 );
	csMonth.Format ( _T("%02d"), nMonth );

	COleDateTime tOleNow = COleDateTime::GetCurrentTime ();
	csDay.Format ( _T("%02d"), _wtoi(csFileTime2) );
	csSecond = "00";
	// 带年的日期，如：Aug 21 2006
	if ( csFileTime3.Find ( _T(":"), 0 ) < 0 )
	{
		csYear = csFileTime3;
		csHour = "00";
		csMinute = "00";
	}
	// 不带年的日期，如：Feb 21 01:11
	else
	{
		csYear.Format ( _T("%04d"), tOleNow.GetYear() );
		int nPos = csFileTime3.Find ( _T(":"), 0 );
		if ( nPos < 0 ) nPos = csFileTime3.GetLength() - 1;
		csHour = csFileTime3.Left ( nPos );
		csMinute = csFileTime3.Mid ( nPos+1 );
	}

	CString csFileTimeInfo;
	csFileTimeInfo.Format ( _T("%s-%s-%s %s:%s:%s"), csYear, csMonth, csDay, csHour, csMinute, csSecond );
	ConvertStrToCTime ( csFileTimeInfo.GetBuffer(csFileTimeInfo.GetLength()), m_TimeLastModified );
	csFileTimeInfo.ReleaseBuffer();
}
